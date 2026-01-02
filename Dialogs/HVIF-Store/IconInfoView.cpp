/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <ControlLook.h>
#include <Font.h>
#include <Catalog.h>
#include <File.h>
#include <Directory.h>
#include <Entry.h>
#include <NodeInfo.h>
#include <FindDirectory.h>
#include <MessageRunner.h>
#include <IconUtils.h>
#include <Window.h>

#include <cstring>
#include <cstdio>

#include "IconInfoView.h"
#include "IconGridView.h"
#include "HvifStoreDefs.h"

#undef  B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT          "HVIFStoreDialog"

const float IconInfoView::kBasePreviewSize = 64.0f;
const float IconInfoView::kBasePadding = 10.0f;
const float IconInfoView::kBasePanelWidth = 200.0f;
const float IconInfoView::kBaseFontSize = 12.0f;


IconInfoView::IconInfoView()
	:
	BView("IconInfo", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fCurrentItem(NULL),
	fPreviewSize(kBasePreviewSize),
	fPadding(kBasePadding),
	fPanelWidth(kBasePanelWidth),
	fCursorOverLink(false),
	fDragButton(0),
	fClickPoint(0, 0),
	fDragStarted(false),
	fTagRects(20)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	_CalculateSizes();
	SetExplicitMinSize(BSize(fPanelWidth, 250));
	SetExplicitMaxSize(BSize(fPanelWidth, B_SIZE_UNLIMITED));
	SetExplicitPreferredSize(BSize(fPanelWidth, B_SIZE_UNSET));

	for (int32 i = 0; i < kFormatCount; i++)
		fFormatRects[i] = BRect();
}


IconInfoView::~IconInfoView()
{
}


void
IconInfoView::_CalculateSizes()
{
	float fontSize = be_plain_font->Size();
	float scale = fontSize / kBaseFontSize;

	if (scale < 1.0f)
		scale = 1.0f;

	fPreviewSize = kBasePreviewSize * scale;
	fPadding = kBasePadding * scale;
	fPanelWidth = kBasePanelWidth * scale;
}


void
IconInfoView::AttachedToWindow()
{
	BView::AttachedToWindow();
	_CalculateSizes();
	SetExplicitMinSize(BSize(fPanelWidth, 250));
	SetExplicitMaxSize(BSize(fPanelWidth, B_SIZE_UNLIMITED));
	SetExplicitPreferredSize(BSize(fPanelWidth, B_SIZE_UNSET));

	SetEventMask(B_POINTER_EVENTS, 0);
}


void
IconInfoView::SetTarget(BMessenger target)
{
	fTarget = target;
}


void
IconInfoView::SetFilterTags(const char* tags)
{
	if (fCurrentFilterTags != tags) {
		fCurrentFilterTags = tags;
		Invalidate();
	}
}


BSize
IconInfoView::MinSize()
{
	return BSize(fPanelWidth, 250);
}


BSize
IconInfoView::MaxSize()
{
	return BSize(fPanelWidth, B_SIZE_UNLIMITED);
}


BSize
IconInfoView::PreferredSize()
{
	return BSize(fPanelWidth, 350);
}


void
IconInfoView::SetIcon(IconItem* item)
{
	fCurrentItem = item;

	for (int32 i = 0; i < kFormatCount; i++)
		fFormatRects[i] = BRect();

	fTagRects.MakeEmpty();

	fDragButton = 0;
	fDragStarted = false;

	Invalidate();
}


void
IconInfoView::Clear()
{
	fCurrentItem = NULL;

	for (int32 i = 0; i < kFormatCount; i++)
		fFormatRects[i] = BRect();

	fTagRects.MakeEmpty();

	fDragButton = 0;
	fDragStarted = false;

	Invalidate();
}


void
IconInfoView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgDeleteTempFile: {
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


BRect
IconInfoView::_GetPreviewRect() const
{
	if (fCurrentItem == NULL || fCurrentItem->bitmap == NULL)
		return BRect();

	BRect bounds = Bounds();
	float y = floorf(fPadding * 1.5f);

	BRect bmpBounds = fCurrentItem->bitmap->Bounds();
	float bmpWidth = bmpBounds.Width() + 1.0f;
	float bmpHeight = bmpBounds.Height() + 1.0f;

	float previewX = floorf((bounds.Width() - bmpWidth) / 2.0f);
	return BRect(previewX, y, previewX + bmpWidth - 1, y + bmpHeight - 1);
}


bool
IconInfoView::_IsOverPreview(BPoint point) const
{
	if (fCurrentItem == NULL || fCurrentItem->bitmap == NULL)
		return false;

	if (fCurrentItem->hvifData == NULL || fCurrentItem->hvifSize <= 0)
		return false;

	return _GetPreviewRect().Contains(point);
}


bool
IconInfoView::_IsOverClickable(BPoint point) const
{
	if (_IsOverPreview(point))
		return true;

	if (!_GetTagAt(point).IsEmpty())
		return true;

	if (_GetFormatAt(point) != kFormatNone)
		return true;

	return false;
}


void
IconInfoView::_UpdateCursor(BPoint where)
{
	bool overClickable = _IsOverClickable(where);

	if (overClickable != fCursorOverLink) {
		fCursorOverLink = overClickable;

		if (fCursorOverLink) {
			if (_IsOverPreview(where)) {
				BCursor grabCursor(B_CURSOR_ID_GRAB);
				SetViewCursor(&grabCursor);
			} else {
				BCursor linkCursor(B_CURSOR_ID_FOLLOW_LINK);
				SetViewCursor(&linkCursor);
			}
		} else {
			SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
		}
	} else if (fCursorOverLink) {
		if (_IsOverPreview(where)) {
			BCursor grabCursor(B_CURSOR_ID_GRAB);
			SetViewCursor(&grabCursor);
		} else {
			BCursor linkCursor(B_CURSOR_ID_FOLLOW_LINK);
			SetViewCursor(&linkCursor);
		}
	}
}


void
IconInfoView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if (fDragButton != 0 && !fDragStarted) {
		if (abs((int32)(where.x - fClickPoint.x)) > kDragThreshold ||
			abs((int32)(where.y - fClickPoint.y)) > kDragThreshold) {

			fDragStarted = true;
			_StartDrag(fClickPoint);
		}
	}

	if (transit == B_EXITED_VIEW) {
		if (fCursorOverLink) {
			fCursorOverLink = false;
			SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
		}
	} else {
		_UpdateCursor(where);
	}

	BView::MouseMoved(where, transit, dragMessage);
}


void
IconInfoView::MouseDown(BPoint where)
{
	if (_IsOverPreview(where)) {
		BMessage* currentMessage = Window()->CurrentMessage();
		uint32 buttons = 0;
		int32 clicks = 1;

		if (currentMessage != NULL) {
			currentMessage->FindInt32("buttons", (int32*)&buttons);
			currentMessage->FindInt32("clicks", &clicks);
		}

		if (clicks == 1) {
			fDragButton = buttons;
			fClickPoint = where;
			fDragStarted = false;
			SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
			return;
		}
	}

	IconFormat format = _GetFormatAt(where);
	if (format != kFormatNone) {
		BMessage msg(kMsgSaveFormat);
		msg.AddInt32("format", format);
		fTarget.SendMessage(&msg);
		return;
	}

	BString clickedTag = _GetTagAt(where);
	if (!clickedTag.IsEmpty()) {
		BMessage msg(kMsgMetaTagClicked);
		msg.AddString("tag", clickedTag);
		fTarget.SendMessage(&msg);
	}
}


void
IconInfoView::MouseUp(BPoint where)
{
	fDragButton = 0;
	fDragStarted = false;
	BView::MouseUp(where);
}


void
IconInfoView::_StartDrag(BPoint point)
{
	if (fCurrentItem == NULL || fCurrentItem->bitmap == NULL ||
		fCurrentItem->hvifData == NULL || fCurrentItem->hvifSize <= 0)
		return;

	BPath tempPath;
	status_t status = _CreateTempFile(tempPath);
	if (status != B_OK)
		return;

	BFile tempFile(tempPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (tempFile.InitCheck() != B_OK)
		return;

	ssize_t written = tempFile.Write(fCurrentItem->hvifData, fCurrentItem->hvifSize);
	if (written != (ssize_t)fCurrentItem->hvifSize) {
		tempFile.Unset();
		return;
	}

	_SetupTempFile(tempPath);
	tempFile.Unset();

	BMessage msg(B_SIMPLE_DATA);
	msg.AddData("icon", B_VECTOR_ICON_TYPE, fCurrentItem->hvifData, fCurrentItem->hvifSize);

	BRect previewRect = _GetPreviewRect();
	BPoint clickOffset(point.x - previewRect.left, point.y - previewRect.top);
	msg.AddPoint("click_pt", clickOffset);

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

	BBitmap* dragBitmap = new BBitmap(fCurrentItem->bitmap);
	DragMessage(&msg, dragBitmap, B_OP_ALPHA, clickOffset, this);

	fDragButton = 0;
	_DeleteFileDelayed(tempPath);
}


status_t
IconInfoView::_CreateTempFile(BPath& tempPath)
{
	BPath tempDir;
	status_t status = find_directory(B_SYSTEM_TEMP_DIRECTORY, &tempDir);
	if (status != B_OK)
		return status;

	BString safeName;
	if (fCurrentItem != NULL) {
		safeName = fCurrentItem->title;
		safeName.ReplaceAll("/", "_");
		safeName.ReplaceAll(":", "_");
		safeName.ReplaceAll(" ", "_");
	} else {
		safeName = "icon";
	}

	char tempName[B_FILE_NAME_LENGTH];
	snprintf(tempName, sizeof(tempName), "hvif_%s_%lld.hvif", safeName.String(), system_time());

	status = tempPath.SetTo(tempDir.Path(), tempName);
	return status;
}


void
IconInfoView::_SetupTempFile(const BPath& tempPath)
{
	if (fCurrentItem == NULL || fCurrentItem->hvifData == NULL)
		return;

	BFile file(tempPath.Path(), B_READ_WRITE);
	if (file.InitCheck() != B_OK)
		return;

	BNodeInfo nodeInfo(&file);
	if (nodeInfo.InitCheck() == B_OK) {
		nodeInfo.SetType(MIME_HVIF_SIGNATURE);
		nodeInfo.SetIcon(fCurrentItem->hvifData, fCurrentItem->hvifSize);
	}
}


void
IconInfoView::_DeleteFileDelayed(const BPath& filePath)
{
	BMessage* deleteMsg = new BMessage(kMsgDeleteTempFile);
	deleteMsg->AddString("path", filePath.Path());

	BMessageRunner* runner = new BMessageRunner(this, deleteMsg, kTempFileDeleteDelay, 1);
	(void)runner;
}


IconFormat
IconInfoView::_GetFormatAt(BPoint point) const
{
	for (int32 i = 0; i < kFormatCount; i++) {
		if (fFormatRects[i].IsValid() && fFormatRects[i].Contains(point))
			return (IconFormat)i;
	}
	return kFormatNone;
}


BString
IconInfoView::_GetTagAt(BPoint point) const
{
	for (int32 i = 0; i < fTagRects.CountItems(); i++) {
		TagRect* tagRect = fTagRects.ItemAt(i);
		if (tagRect->rect.Contains(point) && tagRect->isMeta)
			return tagRect->name;
	}
	return "";
}


float
IconInfoView::_DrawFormats(float x, float y, float maxWidth)
{
	font_height fh;
	GetFontHeight(&fh);
	float lineHeight = fh.ascent + fh.descent + fh.leading;

	rgb_color linkColor = ui_color(B_LINK_TEXT_COLOR);
	rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);

	struct FormatInfo {
		const char* name;
		int32 size;
		IconFormat format;
	};

	FormatInfo formats[kFormatCount] = {
		{ "HVIF", fCurrentItem ? fCurrentItem->hvifSize : 0, kFormatHVIF },
		{ "SVG", fCurrentItem ? fCurrentItem->svgSize : 0, kFormatSVG },
		{ "IOM", fCurrentItem ? fCurrentItem->iomSize : 0, kFormatIOM }
	};

	for (int32 i = 0; i < kFormatCount; i++) {
		fFormatRects[i] = BRect();

		if (formats[i].size <= 0)
			continue;

		float textX = x + 8;
		float textY = y + fh.ascent;

		float nameWidth = StringWidth(formats[i].name);
		fFormatRects[i] = BRect(textX, y, textX + nameWidth, y + lineHeight);

		SetHighColor(linkColor);
		DrawString(formats[i].name, BPoint(textX, textY));

		float colonX = textX + nameWidth;
		SetHighColor(textColor);
		BString sizeStr = ": ";
		sizeStr << _FormatSize(formats[i].size);
		DrawString(sizeStr, BPoint(colonX, textY));

		y += lineHeight;
	}

	return y;
}


void
IconInfoView::Draw(BRect updateRect)
{
	BRect bounds = Bounds();

	SetLowColor(ViewColor());
	FillRect(updateRect, B_SOLID_LOW);

	SetHighColor(tint_color(ViewColor(), B_DARKEN_2_TINT));
	StrokeLine(BPoint(0, bounds.top), BPoint(0, bounds.bottom));

	font_height fh;
	GetFontHeight(&fh);
	float lineHeight = fh.ascent + fh.descent + fh.leading;

	for (int32 i = 0; i < kFormatCount; i++)
		fFormatRects[i] = BRect();

	fTagRects.MakeEmpty();

	if (fCurrentItem == NULL) {
		SetHighColor(tint_color(ViewColor(), B_DARKEN_2_TINT));

		const char* text = B_TRANSLATE("Select an icon");
		float textWidth = StringWidth(text);
		float x = (bounds.Width() - textWidth) / 2;
		float y = bounds.Height() / 2;

		DrawString(text, BPoint(x, y));
		return;
	}

	float y = floorf(fPadding * 1.5f);
	float maxWidth = bounds.Width() - fPadding * 2;
	float contentX = fPadding + 2;

	if (fCurrentItem->bitmap != NULL) {
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

		BRect bmpBounds = fCurrentItem->bitmap->Bounds();
		float bmpWidth = bmpBounds.Width() + 1.0f;
		float bmpHeight = bmpBounds.Height() + 1.0f;

		float previewX = floorf((bounds.Width() - bmpWidth) / 2.0f);
		BRect destRect(previewX, y, previewX + bmpWidth - 1, y + bmpHeight - 1);

		DrawBitmap(fCurrentItem->bitmap, destRect);

		SetDrawingMode(B_OP_COPY);
		y += bmpHeight + fPadding / 2;
	}

	BFont boldFont(be_bold_font);
	boldFont.SetSize(be_plain_font->Size() + 1);
	SetFont(&boldFont);
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));

	BString title = fCurrentItem->title;
	TruncateString(&title, B_TRUNCATE_END, maxWidth);

	float titleWidth = StringWidth(title);
	float titleX = (bounds.Width() - titleWidth) / 2;
	DrawString(title, BPoint(titleX, y + fh.ascent));
	y += lineHeight + fPadding;

	SetFont(be_plain_font);
	GetFontHeight(&fh);
	lineHeight = fh.ascent + fh.descent + fh.leading;

	if (!fCurrentItem->author.IsEmpty()) {
		y = _DrawField(B_TRANSLATE("Author:"), fCurrentItem->author, y, maxWidth);
	}

	if (!fCurrentItem->license.IsEmpty()) {
		y = _DrawField(B_TRANSLATE("License:"), fCurrentItem->license, y, maxWidth);
	}

	if (!fCurrentItem->mimeType.IsEmpty()) {
		y = _DrawField(B_TRANSLATE("MIME:"), fCurrentItem->mimeType, y, maxWidth);
	}

	if (!fCurrentItem->tags.IsEmpty()) {
		y += 4;
		SetHighColor(tint_color(ui_color(B_PANEL_TEXT_COLOR), B_DARKEN_1_TINT));
		DrawString(B_TRANSLATE("Tags:"), BPoint(contentX, y + fh.ascent));
		y += lineHeight + 2;

		y = _DrawTags(fCurrentItem->tags, contentX, y, maxWidth);
	}

	y += 6;

	SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
	StrokeLine(BPoint(fPadding, y), BPoint(bounds.Width() - fPadding, y));
	y += 8;

	bool hasAnyFormat = (fCurrentItem->hvifSize > 0 ||
		fCurrentItem->svgSize > 0 || fCurrentItem->iomSize > 0);

	if (hasAnyFormat) {
		SetHighColor(tint_color(ui_color(B_PANEL_TEXT_COLOR), B_DARKEN_1_TINT));
		DrawString(B_TRANSLATE("Save as:"), BPoint(contentX, y + fh.ascent));
		y += lineHeight + 4;

		y = _DrawFormats(contentX, y, maxWidth);
	}
}


float
IconInfoView::_DrawField(const char* label, const char* value, float y, float maxWidth)
{
	font_height fh;
	GetFontHeight(&fh);
	float lineHeight = fh.ascent + fh.descent + fh.leading;
	float contentX = fPadding + 2;

	SetHighColor(tint_color(ui_color(B_PANEL_TEXT_COLOR), B_DARKEN_1_TINT));
	DrawString(label, BPoint(contentX, y + fh.ascent));

	float labelWidth = StringWidth(label) + 6;
	float valueMaxWidth = maxWidth - labelWidth;

	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
	BString val(value);

	if (StringWidth(val) > valueMaxWidth) {
		y += lineHeight;
		TruncateString(&val, B_TRUNCATE_MIDDLE, maxWidth - 8);
		DrawString(val, BPoint(contentX + 8, y + fh.ascent));
	} else {
		DrawString(val, BPoint(contentX + labelWidth, y + fh.ascent));
	}

	return y + lineHeight + 2;
}


float
IconInfoView::_DrawTags(const char* tags, float x, float y, float maxWidth)
{
	font_height fh;
	GetFontHeight(&fh);
	float fontH = fh.ascent + fh.descent;

	float tagVPadding = 3.0f;
	float tagHPadding = 6.0f;
	float spacingX = 4.0f;
	float spacingY = 4.0f;

	float tagHeight = fontH + tagVPadding * 2;
	float startX = x;
	float currentX = x;
	float currentY = y;

	BString string(tags);
	int32 pos = 0;

	while (true) {
		int32 nextPos = string.FindFirst(',', pos);
		BString tag;
		if (nextPos >= 0) {
			string.CopyInto(tag, pos, nextPos - pos);
			pos = nextPos + 1;
		} else {
			string.CopyInto(tag, pos, string.Length() - pos);
		}

		tag.Trim();
		if (tag.IsEmpty()) {
			if (nextPos < 0) break;
			continue;
		}

		bool isMeta = false;
		if (tag.StartsWith("[") && tag.EndsWith("]")) {
			isMeta = true;
			tag.Remove(0, 1);
			tag.Truncate(tag.Length() - 1);
		}

		float tagWidth = StringWidth(tag) + tagHPadding * 2;

		if (currentX + tagWidth > startX + maxWidth && currentX > startX) {
			currentX = startX;
			currentY += tagHeight + spacingY;
		}

		BRect rect(currentX, currentY, currentX + tagWidth, currentY + tagHeight);

		fTagRects.AddItem(new TagRect(rect, tag.String(), isMeta));

		if (isMeta) {
			BString bracketTag = "[";
			bracketTag << tag << "]";
			bool isActive = fCurrentFilterTags.FindFirst(bracketTag) >= 0;

			if (isActive) {
				SetHighColor(ui_color(B_CONTROL_HIGHLIGHT_COLOR));
				FillRoundRect(rect, 4.0f, 4.0f);
				SetHighColor(ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR));
			} else {
				SetHighColor(ui_color(B_MENU_SELECTION_BACKGROUND_COLOR));
				FillRoundRect(rect, 4.0f, 4.0f);
				SetHighColor(ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR));
			}
		} else {
			SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
			float radius = tagHeight / 2.0f;
			FillRoundRect(rect, radius, radius);
			SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
		}

		float textY = currentY + tagVPadding + fh.ascent;
		DrawString(tag, BPoint(currentX + tagHPadding, textY));

		currentX += tagWidth + spacingX;

		if (nextPos < 0) break;
	}

	return currentY + tagHeight + spacingY;
}


BString
IconInfoView::_FormatSize(int32 bytes) const
{
	BString result;
	
	if (bytes < 1024) {
		result << bytes << " B";
	} else if (bytes < 1024 * 1024) {
		result.SetToFormat("%.1f KB", bytes / 1024.0);
	} else {
		result.SetToFormat("%.1f MB", bytes / (1024.0 * 1024.0));
	}

	return result;
}
