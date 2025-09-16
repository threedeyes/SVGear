/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SVG_VECTORIZATION_WORKER_H
#define SVG_VECTORIZATION_WORKER_H

#include <String.h>
#include <Handler.h>
#include <Looper.h>
#include <OS.h>

#include "TracingOptions.h"
#include "BitmapData.h"

class SVGVectorizationWorker {
public:
	SVGVectorizationWorker(BHandler* target);
	~SVGVectorizationWorker();

	void StartVectorization(const BString& imagePath, const TracingOptions& options);
	void StopVectorization();
	bool IsRunning() const { return fWorkerThread > 0; }

private:
	static int32 _WorkerThread(void* data);
	void _DoVectorization();
	BitmapData _LoadBitmap(const BString& path);

private:
	BHandler*       fTarget;
	BString         fImagePath;
	TracingOptions  fOptions;
	thread_id       fWorkerThread;
	volatile bool   fShouldStop;
};

#endif
