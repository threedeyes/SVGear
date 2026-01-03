/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <FindDirectory.h>
#include <Directory.h>
#include <File.h>
#include <Entry.h>
#include <List.h>
#include <String.h>
#include <Autolock.h>

#include <algorithm>
#include <cstdio>

#include "IconCache.h"

static const off_t kDefaultMaxCacheSize = 32 * 1024 * 1024;
static const int32 kCleanupInterval = 50;

struct CacheEntry {
	entry_ref ref;
	time_t modTime;
	off_t size;
};

static int
CompareCacheEntries(const void* a, const void* b)
{
	const CacheEntry* entryA = *(const CacheEntry**)a;
	const CacheEntry* entryB = *(const CacheEntry**)b;

	if (entryA->modTime < entryB->modTime)
		return -1;
	if (entryA->modTime > entryB->modTime)
		return 1;
	return 0;
}

IconCache::IconCache()
	:
	fMaxCacheSize(kDefaultMaxCacheSize),
	fWriteCounter(0),
	fLock("IconCacheLock"),
	fInitialized(false)
{
}

IconCache::~IconCache()
{
}

void
IconCache::_Init()
{
	if (fInitialized)
		return;

	if (find_directory(B_USER_CACHE_DIRECTORY, &fCacheDir) != B_OK)
		return;

	fCacheDir.Append("HvifStore");

	BDirectory dir(fCacheDir.Path());
	if (dir.InitCheck() != B_OK) {
		create_directory(fCacheDir.Path(), 0777);
	}

	fInitialized = true;
}

status_t
IconCache::GetIcon(int32 id, BMallocIO* data)
{
	BAutolock lock(fLock);
	_Init();

	if (!fInitialized)
		return B_ERROR;

	BString name;
	name << id;

	BPath path(fCacheDir);
	path.Append(name.String());

	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return B_ENTRY_NOT_FOUND;

	off_t size;
	file.GetSize(&size);

	if (size == 0)
		return B_ERROR;

	data->SetSize(size);
	if (file.Read(const_cast<void*>(data->Buffer()), size) != (ssize_t)size)
		return B_ERROR;

	file.SetModificationTime(time(NULL));

	return B_OK;
}

status_t
IconCache::SaveIcon(int32 id, const void* data, size_t size)
{
	BAutolock lock(fLock);
	_Init();

	if (!fInitialized || size == 0)
		return B_ERROR;

	BString name;
	name << id;

	BPath path(fCacheDir);
	path.Append(name.String());

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() != B_OK)
		return B_ERROR;

	if (file.Write(data, size) != (ssize_t)size)
		return B_ERROR;

	fWriteCounter++;
	if (fWriteCounter > kCleanupInterval) {
		fWriteCounter = 0;
		_Cleanup();
	}

	return B_OK;
}

void
IconCache::_Cleanup()
{
	BDirectory dir(fCacheDir.Path());
	if (dir.InitCheck() != B_OK)
		return;

	BList entries;
	off_t totalSize = 0;

	entry_ref ref;
	while (dir.GetNextRef(&ref) == B_OK) {
		BEntry entry(&ref);
		if (entry.InitCheck() != B_OK)
			continue;

		off_t size;
		time_t modTime;

		if (entry.GetSize(&size) != B_OK)
			continue;
		if (entry.GetModificationTime(&modTime) != B_OK)
			continue;

		CacheEntry* cacheEntry = new CacheEntry;
		cacheEntry->ref = ref;
		cacheEntry->size = size;
		cacheEntry->modTime = modTime;

		entries.AddItem(cacheEntry);
		totalSize += size;
	}

	if (totalSize > fMaxCacheSize) {
		entries.SortItems(CompareCacheEntries);

		while (totalSize > fMaxCacheSize && !entries.IsEmpty()) {
			CacheEntry* item = (CacheEntry*)entries.RemoveItem((int32)0);
			if (item) {
				BEntry entry(&item->ref);
				entry.Remove();
				totalSize -= item->size;
				delete item;
			}
		}
	}

	for (int32 i = 0; i < entries.CountItems(); i++) {
		delete (CacheEntry*)entries.ItemAt(i);
	}
}
