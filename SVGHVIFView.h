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
#include <Roster.h>
#include <Entry.h>
#include <Path.h>
#include <FindDirectory.h>
#include <NodeInfo.h>
#include <MessageRunner.h>

class HVIFView : public BView {
public:
    HVIFView(const char* name);
    virtual ~HVIFView();

    void SetIcon(const uint8* data, size_t size);
    void RemoveIcon();

    void OpenInIconOMatic();
    bool HasValidIcon() const;

    virtual void Draw(BRect updateRect);
    virtual void MouseDown(BPoint point);
    virtual void MouseMoved(BPoint point, uint32 transit, const BMessage* message);
    virtual void MouseUp(BPoint point);
    virtual void MessageReceived(BMessage* message);

private:
    BBitmap* fIcon;
    uint8* fData;
    size_t fDataSize;

    uint32 fDragButton;
    BPoint fClickPoint;
    bool fDragStarted;

    static const int32 kDragThreshold;

    void _StartDrag(BPoint point);
    void _OpenInIconOMatic();
    status_t _CreateTempFile(BPath& tempPath);
    void _SetupTempFile(const BPath& tempPath);
    void _CleanupOldFiles();
    void _DeleteFileDelayed(const BPath& filePath);
};

#endif
