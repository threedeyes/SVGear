/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef ICON_CACHE_H
#define ICON_CACHE_H

#include <Path.h>
#include <DataIO.h>
#include <Locker.h>

class IconCache {
public:
							IconCache();
	virtual					~IconCache();

			status_t		GetIcon(int32 id, BMallocIO* data);
			status_t		SaveIcon(int32 id, const void* data, size_t size);

private:
			void			_Init();
			void			_Cleanup();

			BPath			fCacheDir;
			off_t			fMaxCacheSize;
			int32			fWriteCounter;
			BLocker			fLock;
			bool			fInitialized;
};

#endif
