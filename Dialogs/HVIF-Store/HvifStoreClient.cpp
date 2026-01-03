/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <UrlProtocolRoster.h>
#include <HttpRequest.h>
#include <HttpHeaders.h>
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

static const bigtime_t kShutdownTimeout = 2000000;
static const bigtime_t kRequestTimeout = 20000000;

struct RequestContext {
	BUrl url;
	uint32 successWhat;
	BMessenger target;
	BMessage extraData;
	int32 generation;
	int32 retriesLeft;
	volatile bool cancelled;
	thread_id threadId;
	HvifStoreClient* client;
	BString baseUrl;
	BUrlRequest* requestObject;

	RequestContext()
		: generation(0), retriesLeft(kMaxRetries),
		  cancelled(false), threadId(-1), client(NULL), requestObject(NULL) {}
};


HvifStoreClient::HvifStoreClient(BMessenger target)
	:
	BLooper("HvifClient"),
	fTarget(target),
	fBaseUrl(SERVER_URL),
	fCurrentGeneration(0),
	fShuttingDown(false),
	fLastErrorTime(0),
	fRequestLock("RequestLock")
{
	fIconCache = new IconCache();
	Run();
}


HvifStoreClient::~HvifStoreClient()
{
	fShuttingDown = true;

	BAutolock lock(&fRequestLock);
	_ClearPendingQueue();

	for (int32 i = 0; i < fActiveRequests.CountItems(); i++) {
		RequestContext* ctx = (RequestContext*)fActiveRequests.ItemAt(i);
		if (ctx != NULL) {
			ctx->cancelled = true;
			if (ctx->requestObject) ctx->requestObject->Stop();
		}
	}
	lock.Unlock();

	bigtime_t startTime = system_time();

	while (true) {
		int32 activeCount = 0;
		{
			BAutolock lock2(&fRequestLock);
			activeCount = fActiveRequests.CountItems();
		}

		if (activeCount == 0)
			break;

		if (system_time() - startTime > kShutdownTimeout) {
			break;
		}

		snooze(50000);
	}

	lock.Lock();
	for (int32 i = 0; i < fActiveRequests.CountItems(); i++)
		delete (RequestContext*)fActiveRequests.ItemAt(i);
	for (int32 i = 0; i < fPendingRequests.CountItems(); i++)
		delete (RequestContext*)fPendingRequests.ItemAt(i);

	fActiveRequests.MakeEmpty();
	fPendingRequests.MakeEmpty();
	
	delete fIconCache;
}


void
HvifStoreClient::MessageReceived(BMessage* message)
{
	if (fShuttingDown) {
		if (message->what == kMsgRequestFinished) {
			RequestContext* ctx = NULL;
			if (message->FindPointer("context", (void**)&ctx) == B_OK) {
				BAutolock lock(&fRequestLock);
				fActiveRequests.RemoveItem(ctx);
				delete ctx;
			}
		}
		return;
	}

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
#if B_HAIKU_VERSION > B_HAIKU_VERSION_1_BETA_5
				_QueueRequest(BUrl(url.String(), true), kMsgIconsLoaded);
#else
				_QueueRequest(BUrl(url.String()), kMsgIconsLoaded);
#endif
			}
			break;
		}

		case kMsgFetchCategories: {
			BString url = fBaseUrl;
			url << "/api.php?action=get_meta_categories";
#if B_HAIKU_VERSION > B_HAIKU_VERSION_1_BETA_5
			_QueueRequest(BUrl(url.String(), true), kMsgCategoriesLoaded);
#else
			_QueueRequest(BUrl(url.String()), kMsgCategoriesLoaded);
#endif
			break;
		}

		case kMsgIconPreviewReady: {
			int32 id, generation, size;
			BString path, hash;
			if (message->FindInt32("id", &id) == B_OK &&
				message->FindString("path", &path) == B_OK &&
				message->FindInt32("generation", &generation) == B_OK &&
				message->FindInt32("size", &size) == B_OK) {

				hash = message->GetString("hash", "");

				BString url = fBaseUrl;
				url << "/uploads/" << path;
				BMessage data;
				data.AddInt32("id", id);
				data.AddString("hash", hash);
				data.AddInt32("generation", generation);
				data.AddInt32("size", size);
#if B_HAIKU_VERSION > B_HAIKU_VERSION_1_BETA_5
				_QueueRequest(BUrl(url.String(), true), kMsgIconPreviewReady, &data);
#else
				_QueueRequest(BUrl(url.String()), kMsgIconPreviewReady, &data);
#endif
			}
			break;
		}

		case kMsgDownloadIcon: {
			RequestContext* ctx = new RequestContext;
			ctx->successWhat = kMsgIconDataReady;
			ctx->target = fTarget;
			ctx->generation = fCurrentGeneration;
			ctx->client = this;
			ctx->baseUrl = fBaseUrl;
			ctx->extraData = *message;

			thread_id thread = spawn_thread(_IconDownloadThreadEntry, 
				"IconDownload", B_NORMAL_PRIORITY, ctx);

			if (thread < B_OK) {
				delete ctx;
				BMessage error(kMsgNetworkError);
				error.AddString("error", B_TRANSLATE("Failed to create download thread"));
				fTarget.SendMessage(&error);
				return;
			}

			{
				BAutolock lock(&fRequestLock);
				fActiveRequests.AddItem(ctx);
			}

			ctx->threadId = thread;
			resume_thread(thread);
			break;
		}

		case kMsgRequestFinished: {
			RequestContext* ctx = NULL;
			if (message->FindPointer("context", (void**)&ctx) == B_OK) {
				BAutolock lock(&fRequestLock);
				fActiveRequests.RemoveItem(ctx);
				delete ctx;
			}
			_ProcessQueue();
			break;
		}

		case kMsgRequeueRequest: {
			RequestContext* ctx = NULL;
			if (message->FindPointer("context", (void**)&ctx) == B_OK) {
				BAutolock lock(&fRequestLock);
				if (ctx->cancelled) {
					fActiveRequests.RemoveItem(ctx);
					delete ctx;
				} else {
					fActiveRequests.RemoveItem(ctx);
					fPendingRequests.AddItem(ctx, 0);
				}
			}
			_ProcessQueue();
			break;
		}

		case kMsgAbortQueue: {
			_ClearPendingQueue();
			break;
		}

		case kMsgNetworkError: {
			bigtime_t now = system_time();
			if (now - fLastErrorTime > 1000000) {
				fLastErrorTime = now;
				fTarget.SendMessage(message);
			}
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
HvifStoreClient::FetchPreview(int32 id, const char* relativePath,
	const char* hash, int32 generation, int32 size)
{
	BMessage msg(kMsgIconPreviewReady);
	msg.AddInt32("id", id);
	msg.AddString("path", relativePath);
	msg.AddString("hash", hash);
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
	_ClearPendingQueue();
}


void
HvifStoreClient::_ClearPendingQueue()
{
	for (int32 i = 0; i < fPendingRequests.CountItems(); i++) {
		delete (RequestContext*)fPendingRequests.ItemAt(i);
	}
	fPendingRequests.MakeEmpty();
}


void
HvifStoreClient::_QueueRequest(BUrl url, uint32 what, BMessage* extraData)
{
	if (fShuttingDown)
		return;

	RequestContext* ctx = new RequestContext;
	ctx->url = url;
	ctx->successWhat = what;
	ctx->target = fTarget;
	ctx->generation = fCurrentGeneration;
	ctx->client = this;
	if (extraData)
		ctx->extraData = *extraData;

	BAutolock lock(&fRequestLock);
	fPendingRequests.AddItem(ctx);

	if (Lock()) {
		_ProcessQueue();
		Unlock();
	}
}


void
HvifStoreClient::_ProcessQueue()
{
	if (fShuttingDown)
		return;

	BAutolock lock(&fRequestLock);

	while (fActiveRequests.CountItems() < kMaxConcurrentRequests
			&& !fPendingRequests.IsEmpty()) {

		RequestContext* ctx = (RequestContext*)fPendingRequests.RemoveItem((int32)0);

		if (ctx->generation != fCurrentGeneration) {
			delete ctx;
			continue;
		}

		thread_id thread = spawn_thread(_ThreadEntry, "NetworkRequest",
			B_NORMAL_PRIORITY, ctx);

		if (thread >= B_OK) {
			ctx->threadId = thread;
			fActiveRequests.AddItem(ctx);
			resume_thread(thread);
		} else {
			delete ctx;
		}
	}
}


status_t
HvifStoreClient::_DownloadToBuffer(const BUrl& url, BMallocIO& buffer, volatile bool* cancelled)
{
	BUrlRequest* request = BUrlProtocolRoster::MakeRequest(url, &buffer);
	if (request == NULL)
		return B_ERROR;

	BHttpRequest* httpReq = dynamic_cast<BHttpRequest*>(request);
	if (httpReq)
		httpReq->SetUserAgent(APP_USER_AGENT);

	thread_id thread = request->Run();

	status_t threadResult;
	wait_for_thread(thread, &threadResult);

	if (cancelled != NULL && *cancelled) {
		delete request;
		return B_CANCELED;
	}

	const BHttpResult* result = dynamic_cast<const BHttpResult*>(&request->Result());
	status_t status = B_ERROR;

	if (request->Status() == B_OK && result != NULL && result->StatusCode() == 200)
		status = B_OK;

	delete request;
	return status;
}


bool
HvifStoreClient::_TryDownloadFormat(RequestContext* ctx, BMessage& reply,
	const char* pathField, const char* dataField)
{
	BString path = ctx->extraData.GetString(pathField, "");
	if (path.IsEmpty() || ctx->cancelled)
		return false;

	BString url = ctx->baseUrl;
	url << "/uploads/" << path;
	BMallocIO buffer;

#if B_HAIKU_VERSION > B_HAIKU_VERSION_1_BETA_5
	if (_DownloadToBuffer(BUrl(url.String(), true), buffer, &ctx->cancelled) == B_OK) {
#else
	if (_DownloadToBuffer(BUrl(url.String()), buffer, &ctx->cancelled) == B_OK) {
#endif
		reply.AddData(dataField, B_RAW_TYPE, buffer.Buffer(), buffer.BufferLength());
		return true;
	}

	return false;
}


int32
HvifStoreClient::_IconDownloadThreadEntry(void* data)
{
	RequestContext* ctx = (RequestContext*)data;
	BMessenger clientMessenger(ctx->client);

	if (ctx->cancelled) {
		BMessage finished(kMsgRequestFinished);
		finished.AddPointer("context", ctx);
		clientMessenger.SendMessage(&finished);
		return 0;
	}

	BMessage reply(kMsgIconDataReady);

	reply.AddInt32("id", ctx->extraData.GetInt32("id", 0));
	reply.AddString("title", ctx->extraData.GetString("title", ""));

	if (ctx->extraData.HasInt32("clipboard_action"))
		reply.AddInt32("clipboard_action", ctx->extraData.GetInt32("clipboard_action", 0));

	BString savePath;
	if (ctx->extraData.FindString("save_path", &savePath) == B_OK) {
		reply.AddString("save_path", savePath);
		reply.AddInt32("save_format", ctx->extraData.GetInt32("save_format", -1));
	}

	reply.AddString("author", ctx->extraData.GetString("author", ""));
	reply.AddString("license", ctx->extraData.GetString("license", ""));
	reply.AddString("mime_type", ctx->extraData.GetString("mime_type", ""));
	reply.AddString("tags", ctx->extraData.GetString("tags", ""));

	bool hasAnyData = false;
	if (_TryDownloadFormat(ctx, reply, "hvif_path", "hvif_data"))
		hasAnyData = true;
	if (_TryDownloadFormat(ctx, reply, "svg_path", "svg_data"))
		hasAnyData = true;
	if (_TryDownloadFormat(ctx, reply, "iom_path", "iom_data"))
		hasAnyData = true;

	if (!ctx->cancelled) {
		if (hasAnyData) {
			if (ctx->generation == ctx->client->CurrentGeneration()) {
				ctx->target.SendMessage(&reply);
			}
		} else {
			BMessage error(kMsgNetworkError);
			error.AddString("error", B_TRANSLATE("Failed to download icon data"));
			clientMessenger.SendMessage(&error);
		}
	}

	BMessage finished(kMsgRequestFinished);
	finished.AddPointer("context", ctx);
	clientMessenger.SendMessage(&finished);

	return 0;
}


int32
HvifStoreClient::_ThreadEntry(void* data)
{
	RequestContext* ctx = (RequestContext*)data;
	BMessenger clientMessenger(ctx->client);

	if (ctx->cancelled) {
		BMessage finished(kMsgRequestFinished);
		finished.AddPointer("context", ctx);
		clientMessenger.SendMessage(&finished);
		return 0;
	}

	BMallocIO buffer;
	BUrlRequest* request = NULL;
	bool fromCache = false;
	bool success = false;
	int32 statusCode = 0;
	
	if (ctx->successWhat == kMsgIconPreviewReady) {
		int32 id = ctx->extraData.GetInt32("id", 0);
		BString hash = ctx->extraData.GetString("hash", "");
		if (id > 0 && !hash.IsEmpty() && ctx->client->fIconCache->GetIcon(id, hash.String(), &buffer) == B_OK) {
			fromCache = true;
			success = true;
			statusCode = 200;
		}
	}

	if (!fromCache) {
		request = BUrlProtocolRoster::MakeRequest(ctx->url, &buffer);
		ctx->requestObject = request;

		if (request != NULL) {
			BHttpRequest* httpReq = dynamic_cast<BHttpRequest*>(request);
			if (httpReq)
				httpReq->SetUserAgent(APP_USER_AGENT);

			thread_id thread = request->Run();

			status_t threadResult;
			wait_for_thread(thread, &threadResult);

			if (!ctx->cancelled) {
				const BHttpResult* result = dynamic_cast<const BHttpResult*>(&request->Result());
				statusCode = (result != NULL) ? result->StatusCode() : 0;

				if (request->Status() == B_OK && result != NULL && statusCode == 200) {
					success = true;
					
					if (ctx->successWhat == kMsgIconPreviewReady) {
						int32 id = ctx->extraData.GetInt32("id", 0);
						BString hash = ctx->extraData.GetString("hash", "");
						if (id > 0 && !hash.IsEmpty()) {
							ctx->client->fIconCache->SaveIcon(id, hash.String(),
								buffer.Buffer(), buffer.BufferLength());
						}
					}
				}
			}
		}
	}

	if (!ctx->cancelled) {
		if (ctx->generation == ctx->client->CurrentGeneration()) {
			if (success) {
				BMessage reply(ctx->successWhat);
				reply.AddMessage("extra", &ctx->extraData);

				if (ctx->successWhat == kMsgIconPreviewReady) {
				    int32 generation = ctx->extraData.GetInt32("generation", 0);
				    int32 size = ctx->extraData.GetInt32("size", 64);

				    BBitmap* bmp = new BBitmap(
				        BRect(0, 0, size - 1, size - 1), B_RGBA32);
				    if (BIconUtils::GetVectorIcon((const uint8*)buffer.Buffer(),
				        buffer.BufferLength(), bmp) == B_OK) {
				        reply.AddPointer("bitmap", bmp);
				        reply.AddInt32("id", ctx->extraData.GetInt32("id", 0));
				        reply.AddInt32("generation", generation);
				        reply.AddData("hvif_data", B_RAW_TYPE, buffer.Buffer(), buffer.BufferLength());
				        ctx->target.SendMessage(&reply);
				    } else {
				        delete bmp;
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
						clientMessenger.SendMessage(&error);
					}
				}
			} else {
				if (ctx->retriesLeft > 0) {
					ctx->retriesLeft--;
					ctx->requestObject = NULL;
					delete request;

					BMessage requeue(kMsgRequeueRequest);
					requeue.AddPointer("context", ctx);
					clientMessenger.SendMessage(&requeue);
					return 0;
				} else {
					if (statusCode == 0) {
						BMessage abortMsg(kMsgAbortQueue);
						clientMessenger.SendMessage(&abortMsg);
					}

					if (ctx->successWhat != kMsgIconPreviewReady) {
						BMessage error(kMsgNetworkError);
						error.AddString("url", ctx->url.UrlString());
						error.AddInt32("status", statusCode);
						clientMessenger.SendMessage(&error);
					}
				}
			}
		}
	}

	ctx->requestObject = NULL;
	delete request;

	BMessage finished(kMsgRequestFinished);
	finished.AddPointer("context", ctx);
	clientMessenger.SendMessage(&finished);

	return 0;
}
