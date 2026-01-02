/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef HVIF_STORE_DEFS_H
#define HVIF_STORE_DEFS_H

#include <SupportDefs.h>

#define SERVER_URL "https://hvif-store.art"
#define APP_USER_AGENT "HvifStoreClient/1.0 (Haiku)"

static const int32 kDefaultPageLimit = 30;
static const bigtime_t kSearchDebounceDelay = 400000;
static const float kBaseFontSize = 12.0f;
static const float kBaseWindowWidth = 860.0f;
static const float kBaseWindowHeight = 540.0f;

static const int32 kMaxConcurrentRequests = 15;
static const int32 kMaxRetries = 2;

enum {
	kMsgSearch           = 'srch',
	kMsgSearchDelayed    = 'sdly',
	kMsgLoadMore         = 'ldmr',
	kMsgTagToggled       = 'tggl',
	kMsgSelectIcon       = 'slic',
	kMsgOpenIcon         = 'open',
	kMsgCancel           = 'cncl',

	kMsgFetchCategories  = 'ftct',
	kMsgDownloadIcon     = 'dnic',

	kMsgCategoriesLoaded = 'ctld',
	kMsgIconsLoaded      = 'icld',
	kMsgIconPreviewReady = 'iprd',
	kMsgIconDataReady    = 'idrd',
	kMsgNetworkError     = 'nerr',
	
	kMsgRequestFinished  = 'rqfn',
	kMsgRequeueRequest   = 'rqrq',
	kMsgAbortQueue       = 'abrq',

	kMsgLoadingStarted   = 'ldst',
	kMsgLoadingFinished  = 'ldfn',
	kMsgDialogClosed     = 'dlcl'
};

#endif
