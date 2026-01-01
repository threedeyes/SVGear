/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <UrlProtocolRoster.h>
#include <HttpRequest.h>
#include <HttpResult.h>
#include <Json.h>
#include <Bitmap.h>
#include <IconUtils.h>
#include <File.h>
#include <DataIO.h>
#include <OS.h>
#include <Autolock.h>
#include <Catalog.h>

#include "HvifStoreClient.h"
#include "HvifStoreDefs.h"

#undef  B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT          "HVIFStoreDialog"

using namespace BPrivate::Network;

struct RequestContext {
	BUrl url;
	uint32 successWhat;
	BMessenger target;
	BMessage extraData;
	int32 generation;
	volatile bool cancelled;
	volatile bool finished;
	thread_id threadId;
	HvifStoreClient* client;
	BString baseUrl;

	RequestContext()
		: generation(0), cancelled(false), finished(false), 
		  threadId(-1), client(NULL) {}
};


HvifStoreClient::HvifStoreClient(BMessenger target)
	:
	BLooper("HvifClient"),
	fTarget(target),
	fBaseUrl(SERVER_URL),
	fCurrentGeneration(0),
	fRequestLock("RequestLock")
{
	Run();
}


HvifStoreClient::~HvifStoreClient()
{
	CancelAllRequests();

	while (true) {
		bool allFinished = true;

		{
			BAutolock lock(&fRequestLock);
			for (int32 i = 0; i < fActiveRequests.CountItems(); i++) {
				RequestContext* ctx = (RequestContext*)fActiveRequests.ItemAt(i);
				if (ctx != NULL && !ctx->finished) {
					allFinished = false;
					break;
				}
			}
		}

		if (allFinished)
			break;

		snooze(10000);
	}

	BAutolock lock(&fRequestLock);
	for (int32 i = 0; i < fActiveRequests.CountItems(); i++) {
		RequestContext* ctx = (RequestContext*)fActiveRequests.ItemAt(i);
		delete ctx;
	}
	fActiveRequests.MakeEmpty();
}


void
HvifStoreClient::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSearch: {
			BString q, t;
			int32 p, l;
			if (message->FindString("query", &q) == B_OK &&
				message->FindString("tags", &t) == B_OK &&
				message->FindInt32("page", &p) == B_OK &&
				message->FindInt32("limit", &l) == B_OK) {

				BString url = fBaseUrl;
				url << "/api.php?page=" << p << "&limit=" << l;
				if (!q.IsEmpty()) url << "&search=" << BUrl::UrlEncode(q);
				if (!t.IsEmpty()) url << "&tags=" << BUrl::UrlEncode(t);

				_PerformRequest(BUrl(url.String(), true), kMsgIconsLoaded);
			}
			break;
		}

		case kMsgFetchCategories: {
			BString url = fBaseUrl;
			url << "/api.php?action=get_meta_categories";
			_PerformRequest(BUrl(url.String(), true), kMsgCategoriesLoaded);
			break;
		}

		case kMsgIconPreviewReady: {
			int32 id, generation, size;
			BString path;
			if (message->FindInt32("id", &id) == B_OK &&
				message->FindString("path", &path) == B_OK &&
				message->FindInt32("generation", &generation) == B_OK &&
				message->FindInt32("size", &size) == B_OK) {
				BString url = fBaseUrl;
				url << "/uploads/" << path;
				BMessage data;
				data.AddInt32("id", id);
				data.AddInt32("generation", generation);
				data.AddInt32("size", size);
				_PerformRequest(BUrl(url.String(), true), kMsgIconPreviewReady, &data);
			}
			break;
		}

		case kMsgDownloadIcon: {
			_CleanupFinishedRequests();

			RequestContext* ctx = new RequestContext;
			ctx->successWhat = kMsgIconDataReady;
			ctx->target = fTarget;
			ctx->generation = fCurrentGeneration;
			ctx->cancelled = false;
			ctx->finished = false;
			ctx->client = this;
			ctx->baseUrl = fBaseUrl;
			ctx->extraData = *message;

			thread_id thread = spawn_thread(_IconDownloadThreadEntry, 
				"IconDownload", B_NORMAL_PRIORITY, ctx);

			if (thread < B_OK) {
				BMessage error(kMsgNetworkError);
				error.AddString("error", B_TRANSLATE("Failed to create download thread"));
				fTarget.SendMessage(&error);
				delete ctx;
				return;
			}

			ctx->threadId = thread;

			{
				BAutolock lock(&fRequestLock);
				fActiveRequests.AddItem(ctx);
			}

			resume_thread(thread);
			break;
		}

		default:
			BLooper::MessageReceived(message);
	}
}


void
HvifStoreClient::FetchCategories()
{
	PostMessage(kMsgFetchCategories);
}


void
HvifStoreClient::Search(const char* query, const char* tags, int32 page, int32 limit)
{
	BMessage msg(kMsgSearch);
	msg.AddString("query", query);
	msg.AddString("tags", tags);
	msg.AddInt32("page", page);
	msg.AddInt32("limit", limit);
	PostMessage(&msg);
}


void
HvifStoreClient::FetchPreview(int32 id, const char* relativePath, int32 generation, int32 size)
{
	BMessage msg(kMsgIconPreviewReady);
	msg.AddInt32("id", id);
	msg.AddString("path", relativePath);
	msg.AddInt32("generation", generation);
	msg.AddInt32("size", size);
	PostMessage(&msg);
}


void
HvifStoreClient::DownloadIconData(int32 id, const char* title,
	const char* author, const char* license, const char* mimeType,
	const char* tags, const char* hvifPath, const char* svgPath,
	const char* iomPath)
{
	BMessage msg(kMsgDownloadIcon);
	msg.AddInt32("id", id);
	msg.AddString("title", title);
	msg.AddString("author", author);
	msg.AddString("license", license);
	msg.AddString("mime_type", mimeType);
	msg.AddString("tags", tags);
	msg.AddString("hvif_path", hvifPath);
	msg.AddString("svg_path", svgPath);
	msg.AddString("iom_path", iomPath);
	PostMessage(&msg);
}


void
HvifStoreClient::CancelAllRequests()
{
	fCurrentGeneration++;

	BAutolock lock(&fRequestLock);

	for (int32 i = 0; i < fActiveRequests.CountItems(); i++) {
		RequestContext* ctx = (RequestContext*)fActiveRequests.ItemAt(i);
		if (ctx != NULL)
			ctx->cancelled = true;
	}
}


void
HvifStoreClient::_CleanupFinishedRequests()
{
	BAutolock lock(&fRequestLock);

	for (int32 i = fActiveRequests.CountItems() - 1; i >= 0; i--) {
		RequestContext* ctx = (RequestContext*)fActiveRequests.ItemAt(i);
		if (ctx != NULL && ctx->finished) {
			fActiveRequests.RemoveItem(i);
			delete ctx;
		}
	}
}


void
HvifStoreClient::_PerformRequest(BUrl url, uint32 what, BMessage* extraData)
{
	_CleanupFinishedRequests();

	RequestContext* ctx = new RequestContext;
	ctx->url = url;
	ctx->successWhat = what;
	ctx->target = fTarget;
	ctx->generation = fCurrentGeneration;
	ctx->cancelled = false;
	ctx->finished = false;
	ctx->client = this;
	if (extraData)
		ctx->extraData = *extraData;

	thread_id thread = spawn_thread(_ThreadEntry, "NetworkRequest",
		B_NORMAL_PRIORITY, ctx);

	if (thread < B_OK) {
		BMessage error(kMsgNetworkError);
		error.AddString("error", B_TRANSLATE("Failed to create network thread"));
		error.AddString("url", url.UrlString());
		fTarget.SendMessage(&error);
		delete ctx;
		return;
	}

	ctx->threadId = thread;

	{
		BAutolock lock(&fRequestLock);
		fActiveRequests.AddItem(ctx);
	}

	resume_thread(thread);
}


status_t
HvifStoreClient::_DownloadToBuffer(const BUrl& url, BMallocIO& buffer)
{
	BUrlRequest* request = BUrlProtocolRoster::MakeRequest(url, &buffer);
	if (request == NULL)
		return B_ERROR;

	thread_id thread = request->Run();
	wait_for_thread(thread, NULL);

	const BHttpResult* result = dynamic_cast<const BHttpResult*>(&request->Result());
	status_t status = B_ERROR;

	if (request->Status() == B_OK && result != NULL && result->StatusCode() == 200)
		status = B_OK;

	delete request;
	return status;
}


int32
HvifStoreClient::_IconDownloadThreadEntry(void* data)
{
	RequestContext* ctx = (RequestContext*)data;

	if (ctx->cancelled) {
		ctx->finished = true;
		return 0;
	}

	BMessage reply(kMsgIconDataReady);

	reply.AddInt32("id", ctx->extraData.GetInt32("id", 0));
	reply.AddString("title", ctx->extraData.GetString("title", ""));
	reply.AddString("author", ctx->extraData.GetString("author", ""));
	reply.AddString("license", ctx->extraData.GetString("license", ""));
	reply.AddString("mime_type", ctx->extraData.GetString("mime_type", ""));
	reply.AddString("tags", ctx->extraData.GetString("tags", ""));

	BString hvifPath = ctx->extraData.GetString("hvif_path", "");
	BString svgPath = ctx->extraData.GetString("svg_path", "");
	BString iomPath = ctx->extraData.GetString("iom_path", "");

	bool hasAnyData = false;

	if (!hvifPath.IsEmpty() && !ctx->cancelled) {
		BString url = ctx->baseUrl;
		url << "/uploads/" << hvifPath;
		BMallocIO buffer;
		if (_DownloadToBuffer(BUrl(url.String(), true), buffer) == B_OK) {
			reply.AddData("hvif_data", B_RAW_TYPE, buffer.Buffer(), 
				buffer.BufferLength());
			hasAnyData = true;
		}
	}

	if (!svgPath.IsEmpty() && !ctx->cancelled) {
		BString url = ctx->baseUrl;
		url << "/uploads/" << svgPath;
		BMallocIO buffer;
		if (_DownloadToBuffer(BUrl(url.String(), true), buffer) == B_OK) {
			reply.AddData("svg_data", B_RAW_TYPE, buffer.Buffer(), 
				buffer.BufferLength());
			hasAnyData = true;
		}
	}

	if (!iomPath.IsEmpty() && !ctx->cancelled) {
		BString url = ctx->baseUrl;
		url << "/uploads/" << iomPath;
		BMallocIO buffer;
		if (_DownloadToBuffer(BUrl(url.String(), true), buffer) == B_OK) {
			reply.AddData("iom_data", B_RAW_TYPE, buffer.Buffer(), 
				buffer.BufferLength());
			hasAnyData = true;
		}
	}

	if (!ctx->cancelled) {
		if (hasAnyData) {
			ctx->target.SendMessage(&reply);
		} else {
			BMessage error(kMsgNetworkError);
			error.AddString("error", B_TRANSLATE("Failed to download icon data"));
			ctx->target.SendMessage(&error);
		}
	}

	ctx->finished = true;
	return 0;
}


int32
HvifStoreClient::_ThreadEntry(void* data)
{
	RequestContext* ctx = (RequestContext*)data;

	if (ctx->cancelled) {
		ctx->finished = true;
		return 0;
	}

	BMallocIO buffer;

	BUrlRequest* request = BUrlProtocolRoster::MakeRequest(ctx->url, &buffer);
	if (request == NULL) {
		if (!ctx->cancelled) {
			BMessage error(kMsgNetworkError);
			error.AddString("error", B_TRANSLATE("Failed to create request"));
			error.AddString("url", ctx->url.UrlString());
			ctx->target.SendMessage(&error);
		}
		ctx->finished = true;
		return 0;
	}

	thread_id thread = request->Run();
	wait_for_thread(thread, NULL);

	if (ctx->cancelled) {
		delete request;
		ctx->finished = true;
		return 0;
	}

	const BHttpResult* result = dynamic_cast<const BHttpResult*>(&request->Result());
	int32 statusCode = (result != NULL) ? result->StatusCode() : 0;

	if (request->Status() == B_OK && result != NULL && statusCode == 200) {
		BMessage reply(ctx->successWhat);
		reply.AddMessage("extra", &ctx->extraData);

		if (ctx->successWhat == kMsgIconPreviewReady) {
			int32 generation = ctx->extraData.GetInt32("generation", 0);
			int32 size = ctx->extraData.GetInt32("size", 64);

			if (!ctx->cancelled) {
				BBitmap* bmp = new BBitmap(
					BRect(0, 0, size - 1, size - 1), B_RGBA32);
				if (BIconUtils::GetVectorIcon((const uint8*)buffer.Buffer(),
					buffer.BufferLength(), bmp) == B_OK) {
					reply.AddPointer("bitmap", bmp);
					reply.AddInt32("id", ctx->extraData.GetInt32("id", 0));
					reply.AddInt32("generation", generation);
					ctx->target.SendMessage(&reply);
				} else {
					delete bmp;
					BMessage error(kMsgNetworkError);
					error.AddString("error", B_TRANSLATE("Failed to decode icon"));
					error.AddInt32("id", ctx->extraData.GetInt32("id", 0));
					ctx->target.SendMessage(&error);
				}
			}
		} else {
			BString jsonString((const char*)buffer.Buffer(), buffer.BufferLength());
			BMessage jsonMsg;
			if (BJson::Parse(jsonString, jsonMsg) == B_OK) {
				reply.AddMessage("json", &jsonMsg);
				ctx->target.SendMessage(&reply);
			} else {
				BMessage error(kMsgNetworkError);
				error.AddString("error", B_TRANSLATE("JSON parse failed"));
				ctx->target.SendMessage(&error);
			}
		}
	} else if (!ctx->cancelled) {
		BMessage error(kMsgNetworkError);
		error.AddString("url", ctx->url.UrlString());
		error.AddInt32("status", statusCode);
		ctx->target.SendMessage(&error);
	}

	delete request;
	ctx->finished = true;
	return 0;
}
