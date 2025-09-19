/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SVG_VIEW_H
#define SVG_VIEW_H

#include <Invoker.h>
#include <Bitmap.h>

#include "BSVGView.h"

class SVGView : public BSVGView {
public:
	SVGView(const char* name = "main_svg_view");
	virtual ~SVGView();

	virtual void Draw(BRect updateRect);
	virtual void MouseDown(BPoint where);
	virtual void MouseUp(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage);
	virtual void MessageReceived(BMessage* message);
	
	bool IsSVGFile(const char* filePath);
	status_t LoadFromFile(const char* filename, const char* units = "px", float dpi = 96.0f);

	void ZoomIn(BPoint center = BPoint(-1, -1));
	void ZoomOut(BPoint center = BPoint(-1, -1));
	void ZoomToFit();
	void ZoomToOriginal();

	void ResetView();

	void SetTarget(BHandler* target) { fTarget = target; }

	void SetVectorizationBitmap(BBitmap* bitmap);
	void ClearVectorizationBitmap();
	bool HasVectorizationBitmap() const { return fVectorizationBitmap != NULL; }
	void SetShowVectorizationBitmap(bool show);
	bool IsShowingVectorizationBitmap() const { return fShowVectorizationBitmap; }

private:
	void _UpdateStatus();
	void _ZoomAtPoint(float newScale, BPoint zoomCenter);
	void _DrawPlaceholder();
	void _DrawVectorizationBitmap();
	void _DrawOverlayText(const char* text, alignment horizontal = B_ALIGN_RIGHT,
						vertical_alignment vertical = B_ALIGN_TOP, float margin = 10.0,
						float padding = 8.0, float cornerRadius = 6.0);
	BRect _GetVectorizationBitmapRect() const;

private:
	bool		fIsDragging;
	bool		fIsRightDragging;
	BPoint		fLastMousePosition;
	BHandler*	fTarget;
	BBitmap*	fPlaceholderIcon;

	BBitmap*	fVectorizationBitmap;
	bool		fShowVectorizationBitmap;

	static const float kMinScale;
	static const float kMaxScale;
	static const float kScaleStep;
};

#endif
