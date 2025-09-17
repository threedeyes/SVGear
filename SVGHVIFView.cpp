/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <string.h>
#include <File.h>
#include <Alert.h>
#include <Directory.h>
#include <MessageRunner.h>

#include "SVGHVIFView.h"
#include "SVGConstants.h"

const int32 HVIFView::kDragThreshold = 3;

HVIFView::HVIFView(const char* name)
	: BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fIcon(NULL),
	fData(NULL),
	fDataSize(0),
	fDragButton(0),
	fClickPoint(0, 0),
	fDragStarted(false)
{
    _CleanupOldFiles();
}

HVIFView::~HVIFView()
{
	_CleanupOldFiles();

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

    BMessage* currentMessage = Window()->CurrentMessage();
    int32 clicks = 1;
    if (currentMessage)
        currentMessage->FindInt32("clicks", &clicks);

    if (clicks > 1) {
        _OpenInIconOMatic();
        return;
    }

    uint32 buttons = 0;
    if (currentMessage)
        currentMessage->FindInt32("buttons", (int32*)&buttons);

    fDragButton = buttons;
    fClickPoint = point;
    fDragStarted = false;
    SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}

void 
HVIFView::MouseMoved(BPoint where, uint32 transit, const BMessage* message)
{
    if (fDragButton == 0 || fDragStarted || !fIcon
        || (abs((int32)(where.x - fClickPoint.x)) <= kDragThreshold
            && abs((int32)(where.y - fClickPoint.y)) <= kDragThreshold))
        return;

    fDragStarted = true;
    _StartDrag(where);
}

void 
HVIFView::MouseUp(BPoint point)
{
    fDragButton = 0;
    fDragStarted = false;
}

void
HVIFView::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case MSG_DELETE_FILE: {
            BString filePath;
            if (message->FindString("path", &filePath) == B_OK) {
                BEntry entry(filePath.String());
                if (entry.Exists())
                    entry.Remove();
            }
            break;
        }
        default:
            BView::MessageReceived(message);
            break;
    }
}

void 
HVIFView::_StartDrag(BPoint point)
{
    if (!fIcon || !fData)
        return;

    BPath tempPath;
    status_t status = _CreateTempFile(tempPath);
    if (status != B_OK)
        return;

    BFile tempFile(tempPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
    if (tempFile.InitCheck() != B_OK)
        return;

    ssize_t written = tempFile.Write(fData, fDataSize);
    if (written != (ssize_t)fDataSize) {
        tempFile.Unset();
        return;
    }

    _SetupTempFile(tempPath);
    tempFile.Unset();

    BMessage msg(B_SIMPLE_DATA);
    msg.AddData("icon", B_VECTOR_ICON_TYPE, fData, fDataSize);
    msg.AddPoint("click_pt", fClickPoint);

    entry_ref ref;
    BEntry entry(tempPath.Path());
    if (entry.GetRef(&ref) == B_OK) {
        msg.AddRef("refs", &ref);
    }

    BPoint tmpLoc;
    uint32 button;
    GetMouse(&tmpLoc, &button);
    msg.AddInt32("buttons", (int32)button);
    msg.AddInt32("be:actions", B_COPY_TARGET);

    BBitmap* dragBitmap = new BBitmap(fIcon);
    DragMessage(&msg, dragBitmap, B_OP_ALPHA, fClickPoint, this);

    fDragButton = 0;
    _DeleteFileDelayed(tempPath);
}

void
HVIFView::_OpenInIconOMatic()
{
    if (!fData || fDataSize == 0)
        return;

    BPath tempPath;
    status_t status = _CreateTempFile(tempPath);
    if (status != B_OK)
        return;

    BFile tempFile(tempPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
    if (tempFile.InitCheck() != B_OK)
        return;

    ssize_t written = tempFile.Write(fData, fDataSize);
    if (written != (ssize_t)fDataSize)
        return;

    _SetupTempFile(tempPath);
    tempFile.Unset();

    const char* argv[1];
    argv[0] = tempPath.Path();
    be_roster->Launch("application/x-vnd.haiku-icon_o_matic", 1, argv);
}

status_t
HVIFView::_CreateTempFile(BPath& tempPath)
{
    BPath tempDir;
    status_t status = find_directory(B_SYSTEM_TEMP_DIRECTORY, &tempDir);
    if (status != B_OK)
        return status;

    char tempName[B_FILE_NAME_LENGTH];
    snprintf(tempName, sizeof(tempName), "hvif_icon_%ld.hvif", system_time());

    status = tempPath.SetTo(tempDir.Path(), tempName);
    return status;
}

void
HVIFView::_SetupTempFile(const BPath& tempPath)
{
    BFile file(tempPath.Path(), B_READ_WRITE);
    if (file.InitCheck() != B_OK)
        return;

    BNodeInfo nodeInfo(&file);
    if (nodeInfo.InitCheck() == B_OK) {
        nodeInfo.SetType(MIME_HVIF_SIGNATURE);
        nodeInfo.SetIcon(fData, fDataSize);
    }
}

void
HVIFView::_CleanupOldFiles()
{
    BPath tempDir;
    if (find_directory(B_SYSTEM_TEMP_DIRECTORY, &tempDir) != B_OK)
        return;

    BDirectory dir(tempDir.Path());
    if (dir.InitCheck() != B_OK)
        return;

    BEntry entry;
    while (dir.GetNextEntry(&entry) == B_OK) {
        char name[B_FILE_NAME_LENGTH];
        if (entry.GetName(name) == B_OK) {
            if (strncmp(name, "hvif_icon_", 10) == 0) {
                time_t modTime;
                entry.GetModificationTime(&modTime);
                time_t now = time(NULL);
                if (now - modTime > 3600) {
                    entry.Remove();
                }
            }
        }
    }
}

void
HVIFView::_DeleteFileDelayed(const BPath& filePath)
{
    BMessage* deleteMsg = new BMessage(MSG_DELETE_FILE);
    deleteMsg->AddString("path", filePath.Path());

    BMessageRunner* runner = new BMessageRunner(this, deleteMsg, 10000000);
    runner->SetCount(1);
}
