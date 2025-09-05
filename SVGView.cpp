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

#include <fs_attr.h>
#include <stdio.h>
#include <string.h>

#include "SVGView.h"
#include "SVGConstants.h"

const float SVGView::kMinScale = 0.01f;
const float SVGView::kMaxScale = 500.0f;
const float SVGView::kScaleStep = 1.2f;

SVGView::SVGView(const char* name)
	: BSVGView(name),
	  fIsDragging(false),
	  fTarget(NULL)
{
	SetExplicitMinSize(BSize(32, 32));
}

void
SVGView::MouseDown(BPoint where)
{
	if (!fSVGImage)
		return;

	BMessage* currentMessage = Window()->CurrentMessage();
	uint32 buttons = 0;
	currentMessage->FindInt32("buttons", (int32*)&buttons);

	if (buttons & B_PRIMARY_MOUSE_BUTTON) {
		fIsDragging = true;
		fLastMousePosition = where;
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	}
}

void
SVGView::MouseUp(BPoint where)
{
	if (fIsDragging)
		fIsDragging = false;
}

void
SVGView::MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
{
	if (fIsDragging && fSVGImage) {
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
		case B_MOUSE_WHEEL_CHANGED: {
			if (!fSVGImage)
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
            if (strcmp(mimeType, "image/svg+xml") == 0)
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

	return BSVGView::LoadFromFile(filename);
}

void
SVGView::ZoomIn(BPoint center)
{
	if (!fSVGImage)
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
	if (!fSVGImage)
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
	if (fSVGImage) {
		fAutoScale = true;
		_CalculateAutoScale();
		_UpdateStatus();
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
	if (!fSVGImage)
		return;

	fAutoScale = false;

	float scaleFactor = newScale / fScale;
	fOffsetX = zoomCenter.x - (zoomCenter.x - fOffsetX) * scaleFactor;
	fOffsetY = zoomCenter.y - (zoomCenter.y - fOffsetY) * scaleFactor;

	fScale = newScale;
	_UpdateStatus();
	Invalidate();
}
