/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <TranslationUtils.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <File.h>
#include <Message.h>

#include "ImageTracer.h"
#include "SVGConstants.h"
#include "SVGVectorizationWorker.h"

SVGVectorizationWorker::SVGVectorizationWorker(BHandler* target)
	: fTarget(target),
	fWorkerThread(-1),
	fShouldStop(false)
{
}

SVGVectorizationWorker::~SVGVectorizationWorker()
{
	StopVectorization();
}

void
SVGVectorizationWorker::StartVectorization(const BString& imagePath, const TracingOptions& options)
{
	StopVectorization();

	fImagePath = imagePath;
	fOptions = options;
	fShouldStop = false;

	fWorkerThread = spawn_thread(_WorkerThread, "vectorization_worker", B_NORMAL_PRIORITY, this);
	if (fWorkerThread >= 0)
		resume_thread(fWorkerThread);
}

void
SVGVectorizationWorker::StopVectorization()
{
	if (fWorkerThread > 0) {
		fShouldStop = true;
		status_t exitValue;
		wait_for_thread(fWorkerThread, &exitValue);
		fWorkerThread = -1;
	}
}

int32
SVGVectorizationWorker::_WorkerThread(void* data)
{
	SVGVectorizationWorker* worker = static_cast<SVGVectorizationWorker*>(data);
	worker->_DoVectorization();
	return B_OK;
}

void
SVGVectorizationWorker::_DoVectorization()
{
	if (fShouldStop || !fTarget)
		return;

	try {
		BitmapData bitmapData = _LoadBitmap(fImagePath);

		if (fShouldStop)
			return;

		if (!bitmapData.IsValid()) {
			BMessage errorMsg(MSG_VECTORIZATION_ERROR);
			errorMsg.AddString("error", "Failed to load image");
			fTarget->Looper()->PostMessage(&errorMsg, fTarget);
			return;
		}

		ImageTracer tracer;
		BString svgResult = tracer.BitmapToSvg(bitmapData, fOptions).c_str();

		if (fShouldStop)
			return;

		BMessage resultMsg(MSG_VECTORIZATION_COMPLETED);
		resultMsg.AddString("svg_data", svgResult);
		resultMsg.AddString("image_path", fImagePath);
		fTarget->Looper()->PostMessage(&resultMsg, fTarget);
	} catch (const std::exception& e) {
		if (!fShouldStop && fTarget) {
			BMessage errorMsg(MSG_VECTORIZATION_ERROR);
			errorMsg.AddString("error", e.what());
			fTarget->Looper()->PostMessage(&errorMsg, fTarget);
		}
	} catch (...) {
		if (!fShouldStop && fTarget) {
			BMessage errorMsg(MSG_VECTORIZATION_ERROR);
			errorMsg.AddString("error", "Unknown error during vectorization");
			fTarget->Looper()->PostMessage(&errorMsg, fTarget);
		}
	}
}

BitmapData
SVGVectorizationWorker::_LoadBitmap(const BString& path)
{
	BBitmap* bitmap = BTranslationUtils::GetBitmap(path.String());
	if (!bitmap)
		return BitmapData();

	BRect bounds = bitmap->Bounds();
	int32 width = static_cast<int32>(bounds.Width()) + 1;
	int32 height = static_cast<int32>(bounds.Height()) + 1;

	BBitmap* rgbaBitmap = new BBitmap(bounds, B_RGBA32);
	if (rgbaBitmap->ImportBits(bitmap) != B_OK) {
		delete bitmap;
		delete rgbaBitmap;
		return BitmapData();
	}

	uint8* bits = static_cast<uint8*>(rgbaBitmap->Bits());
	int32 bytesPerRow = rgbaBitmap->BytesPerRow();

	std::vector<unsigned char> data;
	data.reserve(width * height * 4);

	for (int32 y = 0; y < height; y++) {
		uint8* row = bits + y * bytesPerRow;
		for (int32 x = 0; x < width; x++) {
			uint8* pixel = row + x * 4;
			data.push_back(pixel[2]); // R
			data.push_back(pixel[1]); // G  
			data.push_back(pixel[0]); // B
			data.push_back(pixel[3]); // A
		}
	}

	delete bitmap;
	delete rgbaBitmap;

	return BitmapData(width, height, data);
}
