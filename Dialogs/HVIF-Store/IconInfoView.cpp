/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <ControlLook.h>
#include <Font.h>
#include <Catalog.h>

#include "IconInfoView.h"
#include "IconGridView.h"

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

		SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
		y = _DrawWrappedText(fCurrentItem->tags, contentX + 4, y, maxWidth - 4);
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
IconInfoView::_DrawWrappedText(const char* text, float x, float y, float maxWidth)
{
	font_height fh;
	GetFontHeight(&fh);
	float lineHeight = fh.ascent + fh.descent + 2;

	BString remaining(text);

	while (!remaining.IsEmpty()) {
		BString line = remaining;

		if (StringWidth(line) > maxWidth) {
			int32 len = line.Length();
			while (len > 0 && StringWidth(line.String(), len) > maxWidth) {
				len--;
			}

			int32 breakPos = len;
			for (int32 i = len - 1; i > 0; i--) {
				char c = line[i];
				if (c == ',' || c == ' ') {
					breakPos = i + 1;
					break;
				}
			}

			line.Truncate(breakPos);
			remaining.Remove(0, breakPos);
			remaining.Trim();
		} else {
			remaining = "";
		}

		DrawString(line, BPoint(x, y + fh.ascent));
		y += lineHeight;
	}

	return y;
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
