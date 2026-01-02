/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <ControlLook.h>
#include <Font.h>
#include <Catalog.h>

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
	fPanelWidth(kBasePanelWidth)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	_CalculateSizes();
	SetExplicitMinSize(BSize(fPanelWidth, 250));
	SetExplicitMaxSize(BSize(fPanelWidth, B_SIZE_UNLIMITED));
	SetExplicitPreferredSize(BSize(fPanelWidth, B_SIZE_UNSET));
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
	Invalidate();
}


void
IconInfoView::Clear()
{
	fCurrentItem = NULL;
	Invalidate();
}


void
IconInfoView::MouseDown(BPoint where)
{
	BString clickedTag = _GetTagAt(where);
	if (!clickedTag.IsEmpty()) {
		BMessage msg(kMsgMetaTagClicked);
		msg.AddString("tag", clickedTag);
		fTarget.SendMessage(&msg);
	}
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

	SetHighColor(tint_color(ui_color(B_PANEL_TEXT_COLOR), B_DARKEN_1_TINT));
	DrawString(B_TRANSLATE("File sizes:"), BPoint(contentX, y + fh.ascent));
	y += lineHeight + 4;

	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));

	if (fCurrentItem->hvifSize > 0) {
		BString sizeStr = "HVIF: ";
		sizeStr << _FormatSize(fCurrentItem->hvifSize);
		DrawString(sizeStr, BPoint(contentX + 8, y + fh.ascent));
		y += lineHeight;
	}

	if (fCurrentItem->svgSize > 0) {
		BString sizeStr = "SVG: ";
		sizeStr << _FormatSize(fCurrentItem->svgSize);
		DrawString(sizeStr, BPoint(contentX + 8, y + fh.ascent));
		y += lineHeight;
	}

	if (fCurrentItem->iomSize > 0) {
		BString sizeStr = "IOM: ";
		sizeStr << _FormatSize(fCurrentItem->iomSize);
		DrawString(sizeStr, BPoint(contentX + 8, y + fh.ascent));
		y += lineHeight;
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
IconInfoView::_GetTagAt(BPoint point)
{
	if (fCurrentItem == NULL || fCurrentItem->tags.IsEmpty())
		return "";

	font_height fh;
	GetFontHeight(&fh);
	float fontH = fh.ascent + fh.descent;
	float lineHeight = fh.ascent + fh.descent + fh.leading;

	float y = floorf(fPadding * 1.5f);
	if (fCurrentItem->bitmap != NULL) {
		BRect bmpBounds = fCurrentItem->bitmap->Bounds();
		y += bmpBounds.Height() + 1.0f + fPadding / 2;
	}
	y += lineHeight + fPadding; // Title
	if (!fCurrentItem->author.IsEmpty())
		y += lineHeight + 2;

	if (!fCurrentItem->license.IsEmpty())
		y += lineHeight + 2;

	if (!fCurrentItem->mimeType.IsEmpty())
		y += lineHeight + 2;

	y += 4;
	y += lineHeight + 2;

	float startX = fPadding + 2;
	float maxWidth = Bounds().Width() - fPadding * 2;

	float tagVPadding = 3.0f;
	float tagHPadding = 6.0f;
	float spacingX = 4.0f;
	float spacingY = 4.0f;
	float tagHeight = fontH + tagVPadding * 2;

	float currentX = startX;
	float currentY = y;

	BString string(fCurrentItem->tags);
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
		if (rect.Contains(point)) {
			if (isMeta) return tag;
			return "";
		}

		currentX += tagWidth + spacingX;

		if (nextPos < 0) break;
	}

	return "";
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
