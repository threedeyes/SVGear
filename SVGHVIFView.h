/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef HVIF_VIEW_H
#define HVIF_VIEW_H

#include <View.h>
#include <Bitmap.h>
#include <IconUtils.h>
#include <Message.h>
#include <MimeType.h>

#include <View.h>
#include <Bitmap.h>
#include <IconUtils.h>
#include <ControlLook.h>
#include <Message.h>
#include <MimeType.h>
#include <stdio.h>

class HVIFView : public BView {
public:
    HVIFView(const char* name) 
        : BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
          fIcon(NULL), fData(NULL), fDataSize(0) {
    }
    
    ~HVIFView() {
        delete fIcon;
        delete[] fData;
    }
    
    void SetIcon(const uint8* data, size_t size) {
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
    
    void RemoveIcon() {
		delete fIcon;
        delete[] fData;
        fIcon = NULL;
        fData = NULL;
        fDataSize = 0;
        Invalidate();
    }
    
    void Draw(BRect updateRect) {
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
    
    void MouseDown(BPoint point) {
        if (!fIcon) return;
        
        BMessage msg(B_SIMPLE_DATA);
        msg.AddData("icon", B_VECTOR_ICON_TYPE, fData, fDataSize);
        
        BBitmap* drag = new BBitmap(fIcon);
        DragMessage(&msg, drag, B_OP_ALPHA, BPoint(11, 11));
    }

private:
    BBitmap* fIcon;
    uint8* fData;
    size_t fDataSize;
};

#endif
