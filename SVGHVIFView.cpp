/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <string.h>

#include "SVGHVIFView.h"

const float HVIFView::kDragThreshold = 5.0f;

HVIFView::HVIFView(const char* name)
    : BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
      fIcon(NULL),
      fData(NULL),
      fDataSize(0),
      fMouseDown(false),
      fMouseDownPoint(0, 0),
      fDragStarted(false)
{
}

HVIFView::~HVIFView()
{
    delete fIcon;
    delete[] fData;
}

void 
HVIFView::SetIcon(const uint8* data, size_t size)
{
    delete fIcon;
    delete[] fData;
    
    fData = new uint8[size];
    memcpy(fData, data, size);
    fDataSize = size;
    
    BRect rect(Bounds());
    rect.InsetBy(1, 1);
    fIcon = new BBitmap(rect, B_RGBA32);
    if (BIconUtils::GetVectorIcon(fData, fDataSize, fIcon) != B_OK) {
        delete fIcon;
        fIcon = NULL;
    }
    Invalidate();
}

void 
HVIFView::RemoveIcon()
{
    delete fIcon;
    delete[] fData;
    fIcon = NULL;
    fData = NULL;
    fDataSize = 0;
    Invalidate();
}

void 
HVIFView::Draw(BRect updateRect)
{
    BRect rect(Bounds());
    rgb_color base = ui_color(B_MENU_BACKGROUND_COLOR);
    be_control_look->DrawBorder(this, rect, updateRect, base, B_PLAIN_BORDER,
        0, BControlLook::B_BOTTOM_BORDER);
    be_control_look->DrawMenuBarBackground(this, rect, updateRect, base, 0,
        BControlLook::B_ALL_BORDERS & ~BControlLook::B_LEFT_BORDER);

    if (fIcon) {
        SetDrawingMode(B_OP_ALPHA);
        SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
        DrawBitmap(fIcon);
    }
}

void 
HVIFView::MouseDown(BPoint point)
{
    if (!fIcon) 
        return;
    
    fMouseDown = true;
    fMouseDownPoint = point;
    fDragStarted = false;
    SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}

void 
HVIFView::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
    if (!fMouseDown || fDragStarted || !fIcon)
        return;

    BPoint delta = point - fMouseDownPoint;
    float distance = sqrt(delta.x * delta.x + delta.y * delta.y);

    if (distance >= kDragThreshold) {
        fDragStarted = true;
        _StartDrag(fMouseDownPoint);
    }
}

void 
HVIFView::MouseUp(BPoint point)
{
    fMouseDown = false;
    fDragStarted = false;
}

void 
HVIFView::_StartDrag(BPoint point)
{
    if (!fIcon || !fData)
        return;

    BMessage msg(B_SIMPLE_DATA);
    msg.AddData("icon", B_VECTOR_ICON_TYPE, fData, fDataSize);

    BBitmap* drag = new BBitmap(fIcon);
    DragMessage(&msg, drag, B_OP_ALPHA, BPoint(11, 11));
}
