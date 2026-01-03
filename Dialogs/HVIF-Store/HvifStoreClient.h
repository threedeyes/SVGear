/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef HVIF_STORE_CLIENT_H
#define HVIF_STORE_CLIENT_H

#include <Looper.h>
#include <String.h>
#include <Url.h>
#include <UrlRequest.h>
#include <Messenger.h>
#include <Locker.h>
#include <List.h>

#include "IconCache.h"

using namespace BPrivate::Network;

struct RequestContext;

class HvifStoreClient : public BLooper {
public:
							HvifStoreClient(BMessenger target);
	virtual                 ~HvifStoreClient();

	virtual void            MessageReceived(BMessage* message);

			void            FetchCategories();
			void            Search(const char* query, const char* tags,
								int32 page, int32 limit);
			void            FetchPreview(int32 id, const char* relativePath,
								const char* hash, int32 generation, int32 size);
			void            DownloadIconData(int32 id, const char* title,
								const char* author, const char* license,
								const char* mimeType, const char* tags,
								const char* hvifPath, const char* svgPath,
								const char* iomPath);
			void            CancelAllRequests();
			int32           CurrentGeneration() const { return fCurrentGeneration; }

private:
			void            _QueueRequest(BUrl url, uint32 what,
								BMessage* extraData = NULL);
			void            _ProcessQueue();
			void            _ClearPendingQueue();

			static int32    _ThreadEntry(void* data);
			static int32    _IconDownloadThreadEntry(void* data);
			static status_t _DownloadToBuffer(const BUrl& url, BMallocIO& buffer,
								volatile bool* cancelled = NULL);
			static bool     _TryDownloadFormat(RequestContext* ctx, BMessage& reply,
								const char* pathField, const char* dataField);

			BMessenger      fTarget;
			BString         fBaseUrl;
			int32           fCurrentGeneration;
			volatile bool   fShuttingDown;
			bigtime_t       fLastErrorTime;

			BList           fActiveRequests;
			BList           fPendingRequests;

			BLocker         fRequestLock;
			IconCache*      fIconCache;
};

#endif
