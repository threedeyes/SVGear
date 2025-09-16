/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef HVIF_VIEW_H
#define HVIF_VIEW_H

#include <View.h>
#include <Bitmap.h>
#include <IconUtils.h>
#include <ControlLook.h>
#include <Message.h>
#include <MimeType.h>

class HVIFView : public BView {
public:
    HVIFView(const char* name);
    virtual ~HVIFView();

    void SetIcon(const uint8* data, size_t size);
    void RemoveIcon();
    
    virtual void Draw(BRect updateRect);
    virtual void MouseDown(BPoint point);
    virtual void MouseMoved(BPoint point, uint32 transit, const BMessage* message);
    virtual void MouseUp(BPoint point);

private:
    BBitmap* fIcon;
    uint8* fData;
    size_t fDataSize;

    bool fMouseDown;
    BPoint fMouseDownPoint;
    bool fDragStarted;
    static const float kDragThreshold;

    void _StartDrag(BPoint point);
};

#endif
