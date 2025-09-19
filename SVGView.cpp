/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <File.h>
#include <String.h>
#include <Message.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Window.h>
#include <Screen.h>
#include <InterfaceKit.h>
#include <Catalog.h>

#include <fs_attr.h>
#include <stdio.h>
#include <string.h>

#include "SVGView.h"
#include "SVGConstants.h"
#include "SVGApplication.h"

const float SVGView::kMinScale = 0.01f;
const float SVGView::kMaxScale = 500.0f;
const float SVGView::kScaleStep = 1.2f;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SVGView"

SVGView::SVGView(const char* name)
	: BSVGView(name),
	fIsDragging(false),
	fIsRightDragging(false),
	fTarget(NULL),
	fPlaceholderIcon(NULL),
	fVectorizationBitmap(NULL),
	fShowVectorizationBitmap(false)
{
	SetExplicitMinSize(BSize(256, 192));
	SetFlags(Flags() | B_FULL_UPDATE_ON_RESIZE);
	fPlaceholderIcon = SVGApplication::GetIcon(NULL, 128);
}

SVGView::~SVGView()
{
	delete fVectorizationBitmap;
}

void
SVGView::Draw(BRect updateRect)
{
	if (fShowVectorizationBitmap && fVectorizationBitmap) {
		_DrawVectorizationBitmap();
	} else if (IsLoaded()) {
		BSVGView::Draw(updateRect);
	} else {
		_DrawPlaceholder();
	}
}

void
SVGView::_DrawPlaceholder()
{
	BRect bounds = Bounds();

	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	FillRect(bounds);

	if (fPlaceholderIcon) {
		BRect iconBounds = fPlaceholderIcon->Bounds();
		float iconX = (bounds.Width() - iconBounds.Width()) / 2.0f;
		float iconY = (bounds.Height() - iconBounds.Height()) / 2.0f - 12.0f;

		SetDrawingMode(B_OP_ALPHA);
		DrawBitmap(fPlaceholderIcon, BPoint(iconX, iconY));
		SetDrawingMode(B_OP_COPY);
	}

	const char* text = B_TRANSLATE("Drop a file to view or vectorize");

	font_height height;
	GetFontHeight(&height);

	float textWidth = StringWidth(text);
	float textX = (bounds.Width() - textWidth) / 2.0f;
	float textY = bounds.Height() / 2.0f + 80.0f;

	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
	DrawString(text, BPoint(textX, textY));
}

void
SVGView::_DrawVectorizationBitmap()
{
	if (!fVectorizationBitmap)
		return;

	BRect bounds = Bounds();

	if (fShowTransparency) {
		_DrawTransparencyGrid();
	} else {
		SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		FillRect(bounds);
	}

	BRect bitmapRect = _GetVectorizationBitmapRect();

	SetDrawingMode(B_OP_ALPHA);
	DrawBitmap(fVectorizationBitmap, bitmapRect);
	SetDrawingMode(B_OP_COPY);
}

BRect
SVGView::_GetVectorizationBitmapRect() const
{
	if (!fVectorizationBitmap)
		return BRect();

	BRect bitmapBounds = fVectorizationBitmap->Bounds();
	float bitmapWidth = bitmapBounds.Width() + 1;
	float bitmapHeight = bitmapBounds.Height() + 1;

	// Use the same scaling and offset as the SVG
	float scaledWidth = bitmapWidth * fScale;
	float scaledHeight = bitmapHeight * fScale;

	float x = fOffsetX;
	float y = fOffsetY;

	return BRect(x, y, x + scaledWidth - 1, y + scaledHeight - 1);
}

void
SVGView::MouseDown(BPoint where)
{
	if (!fSVGImage && !fVectorizationBitmap)
		return;

	BMessage* currentMessage = Window()->CurrentMessage();
	uint32 buttons = 0;
	currentMessage->FindInt32("buttons", (int32*)&buttons);

	if (buttons & B_PRIMARY_MOUSE_BUTTON) {
		fIsDragging = true;
		fIsRightDragging = false;
		fLastMousePosition = where;
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	} else if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		if (fVectorizationBitmap) {
			fShowVectorizationBitmap = true;
			Invalidate();
		}

		fIsRightDragging = true;
		fIsDragging = false;
		fLastMousePosition = where;
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	}
}

void
SVGView::MouseUp(BPoint where)
{
	if (fIsDragging)
		fIsDragging = false;
	if (fIsRightDragging)
		fIsRightDragging = false;

	fShowVectorizationBitmap = false;
	Invalidate();
}

void
SVGView::MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
{
	if ((fIsDragging || fIsRightDragging) && (fSVGImage || fVectorizationBitmap)) {
		BPoint delta = where - fLastMousePosition;
		fOffsetX += delta.x;
		fOffsetY += delta.y;

		fLastMousePosition = where;
		_UpdateStatus();
		Invalidate();
	}
}

void
SVGView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SHAPE_SELECTED: {
			int32 shapeIndex;
			if (message->FindInt32("shape_index", &shapeIndex) == B_OK) {
				SetHighlightedShape(shapeIndex);
			}
			break;
		}
		case MSG_PATH_SELECTED: {
			int32 shapeIndex, pathIndex;
			if (message->FindInt32("shape_index", &shapeIndex) == B_OK &&
				message->FindInt32("path_index", &pathIndex) == B_OK) {
				SetHighlightedPath(shapeIndex, pathIndex);
			}
			break;
		}
		case MSG_CONTROL_POINTS_SELECTED: {
			int32 shapeIndex, pathIndex;
			bool showBezierHandles = false;
			if (message->FindInt32("shape_index", &shapeIndex) == B_OK &&
				message->FindInt32("path_index", &pathIndex) == B_OK) {
				message->FindBool("show_bezier_handles", &showBezierHandles);
				SetHighlightControlPoints(shapeIndex, pathIndex, showBezierHandles);
			}
			break;
		}
		case MSG_CLEAR_SELECTION: {
			ClearHighlight();
			break;
		}
		case B_MOUSE_WHEEL_CHANGED: {
			if (!fSVGImage && !fVectorizationBitmap)
				break;

			float deltaY = 0;
			message->FindFloat("be:wheel_delta_y", &deltaY);

			uint32 buttons;
			BPoint where;
			GetMouse(&where, &buttons);

			if (deltaY != 0) {
				float newScale = fScale;

				if (deltaY > 0) {
					newScale = fScale / kScaleStep;
				} else {
					newScale = fScale * kScaleStep;
				}

				if (newScale < kMinScale)
					newScale = kMinScale;
				if (newScale > kMaxScale)
					newScale = kMaxScale;

				if (newScale != fScale) {
					_ZoomAtPoint(newScale, where);
				}
			}
			break;
		}
		default:
			BSVGView::MessageReceived(message);
			break;
	}
}

bool
SVGView::IsSVGFile(const char* filePath)
{
	if (!filePath)
		return false;

	BFile file(filePath, B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return false;

	BNodeInfo nodeInfo(&file);
	if (nodeInfo.InitCheck() == B_OK) {
		char mimeType[B_MIME_TYPE_LENGTH];
		if (nodeInfo.GetType(mimeType) == B_OK) {
			if (strcmp(mimeType, MIME_SVG_SIGNATURE) == 0)
				return true;
		}
	}

	char buffer[512];
	ssize_t bytesRead = file.Read(buffer, sizeof(buffer) - 1);
	if (bytesRead <= 0)
		return false;

	buffer[bytesRead] = '\0';

	BString content(buffer);
	content.ToLower();

	if (content.FindFirst("<?xml") != B_ERROR &&
		content.FindFirst("<svg") != B_ERROR) {
		return true;
	}

	if (content.FindFirst("<svg") != B_ERROR) {
		return true;
	}

	return false;
}

status_t
SVGView::LoadFromFile(const char* filename, const char* units, float dpi)
{
	if (!IsSVGFile(filename))
		return B_ERROR;

	return BSVGView::LoadFromFile(filename, units, dpi);
}

void
SVGView::ZoomIn(BPoint center)
{
	if (!fSVGImage && !fVectorizationBitmap)
		return;

	fAutoScale = false;

	if (center.x < 0 || center.y < 0) {
		BRect bounds = Bounds();
		center.Set(bounds.Width() / 2, bounds.Height() / 2);
	}

	float newScale = fScale * kScaleStep;
	if (newScale > kMaxScale)
		newScale = kMaxScale;

	if (newScale != fScale)
		_ZoomAtPoint(newScale, center);
}

void
SVGView::ZoomOut(BPoint center)
{
	if (!fSVGImage && !fVectorizationBitmap)
		return;

	fAutoScale = false;

	if (center.x < 0 || center.y < 0) {
		BRect bounds = Bounds();
		center.Set(bounds.Width() / 2, bounds.Height() / 2);
	}

	float newScale = fScale / kScaleStep;
	if (newScale < kMinScale)
		newScale = kMinScale;

	if (newScale != fScale)
		_ZoomAtPoint(newScale, center);
}

void
SVGView::ZoomToFit()
{
	FitToWindow();
	_UpdateStatus();
}

void
SVGView::ZoomToOriginal()
{
	ActualSize();
	_UpdateStatus();
}

void
SVGView::ResetView()
{
	if (fSVGImage || fVectorizationBitmap) {
		fAutoScale = true;
		_CalculateAutoScale();
		_UpdateStatus();
		Invalidate();
	}
}

void
SVGView::SetVectorizationBitmap(BBitmap* bitmap)
{
	delete fVectorizationBitmap;
	fVectorizationBitmap = bitmap;

	if (fVectorizationBitmap && !fSVGImage) {
		fScale = 1.0f;
		fOffsetX = 0.0f;
		fOffsetY = 0.0f;
	}

	Invalidate();
}

void
SVGView::ClearVectorizationBitmap()
{
	delete fVectorizationBitmap;
	fVectorizationBitmap = NULL;
	fShowVectorizationBitmap = false;
	Invalidate();
}

void
SVGView::SetShowVectorizationBitmap(bool show)
{
	if (fShowVectorizationBitmap != show) {
		fShowVectorizationBitmap = show;
		Invalidate();
	}
}

void
SVGView::_UpdateStatus()
{
	if (fTarget) {
		BMessage message(MSG_SVG_STATUS_UPDATE);
		message.AddFloat("scale", fScale);
		message.AddPoint("offset", BPoint(fOffsetX, fOffsetY));
		message.AddBool("auto_scale", fAutoScale);
		if (fSVGImage) {
			message.AddFloat("width", fSVGImage->width);
			message.AddFloat("height", fSVGImage->height);
		}
		BMessenger(fTarget).SendMessage(&message);
	}
}

void
SVGView::_ZoomAtPoint(float newScale, BPoint zoomCenter)
{
	if (!fSVGImage && !fVectorizationBitmap)
		return;

	fAutoScale = false;

	float scaleFactor = newScale / fScale;
	fOffsetX = zoomCenter.x - (zoomCenter.x - fOffsetX) * scaleFactor;
	fOffsetY = zoomCenter.y - (zoomCenter.y - fOffsetY) * scaleFactor;

	fScale = newScale;
	_UpdateStatus();
	Invalidate();
}
