/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "SVGListItem.h"
#include "SVGApplication.h"
#include "nanosvg.h"

#include <Catalog.h>
#include <math.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SVGListItem"

SVGListItem::SVGListItem(NSVGshape* shape, int32 index)
	: BListItem(),
	  fType(SVG_ITEM_SHAPE),
	  fShape(shape),
	  fPath(NULL),
	  fPaint(NULL),
	  fIndex(index),
	  fShapeIndex(index),
	  fPathIndex(-1),
	  fIsStroke(false),
	  fHeight(0),
	  fIcon(NULL)
{
}

SVGListItem::SVGListItem(NSVGpath* path, int32 shapeIndex, int32 pathIndex)
	: BListItem(),
	  fType(SVG_ITEM_PATH),
	  fShape(NULL),
	  fPath(path),
	  fPaint(NULL),
	  fIndex(pathIndex),
	  fShapeIndex(shapeIndex),
	  fPathIndex(pathIndex),
	  fIsStroke(false),
	  fHeight(0),
	  fIcon(NULL)
{
}

SVGListItem::SVGListItem(NSVGpaint* paint, const char* name, int32 shapeIndex, bool isStroke)
	: BListItem(),
	  fType(SVG_ITEM_PAINT),
	  fShape(NULL),
	  fPath(NULL),
	  fPaint(paint),
	  fName(name),
	  fIndex(-1),
	  fShapeIndex(shapeIndex),
	  fPathIndex(-1),
	  fIsStroke(isStroke),
	  fHeight(0),
	  fIcon(NULL)
{
}

SVGListItem::~SVGListItem()
{
}

void
SVGListItem::SetIcon(BBitmap* bitmap)
{
	fIcon = bitmap;
}

void
SVGListItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	_DrawBackground(owner, frame);

	rgb_color textColor;
	if (IsSelected()) {
		textColor = ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR);
	} else {
		textColor = ui_color(B_LIST_ITEM_TEXT_COLOR);
	}

	owner->SetHighColor(textColor);

	BString text;
	float leftMargin = 4;

	font_height fh;
	owner->GetFontHeight(&fh);
	float iconSize = fh.ascent + fh.descent + fh.leading;
	if (iconSize < 12) iconSize = 12;
	if (iconSize > 32) iconSize = 32;

	BRect iconRect(frame.left + 2, frame.top + 2, 
				   frame.left + 2 + iconSize, frame.top + 2 + iconSize);
	_DrawIcon(owner, iconRect);

	leftMargin = iconRect.right + 4;

	switch (fType) {
		case SVG_ITEM_SHAPE:
			if (fShape) {
				text.SetToFormat("Shape %ld", fIndex);
				if (fShape->id && strlen(fShape->id) > 0)
					text << " (" << fShape->id << ")";
			}
			break;

		case SVG_ITEM_PATH:
			if (fPath) {
				text.SetToFormat("Path %ld.%ld (%d pts)", fShapeIndex, fPathIndex, fPath->npts);
				if (fPath->closed)
					text << " [closed]";
			}
			leftMargin += 4;
			break;

		case SVG_ITEM_PAINT:
			text = fName;
			break;
	}

	float textY = frame.top + (frame.Height() + fh.ascent - fh.descent) / 2;
	owner->DrawString(text.String(), BPoint(leftMargin, textY));

	float swatchSize = iconSize;

	if (fType == SVG_ITEM_SHAPE && fShape) {
		BRect colorRect(frame.right - (swatchSize * 2 + 2),
						frame.top + (frame.Height() - swatchSize) / 2,
						frame.right - 2,
						frame.top + (frame.Height() - swatchSize) / 2 + swatchSize);
		_DrawColorSwatches(owner, colorRect, fShape, swatchSize);
	} else if (fType == SVG_ITEM_PAINT && fPaint) {
		BRect colorRect(frame.right - swatchSize - 2,
						frame.top + (frame.Height() - swatchSize) / 2,
						frame.right - 2,
						frame.top + (frame.Height() - swatchSize) / 2 + swatchSize);
		_DrawSingleColorSwatch(owner, colorRect, fPaint);
	}
}

void
SVGListItem::Update(BView* owner, const BFont* font)
{
	font_height fh;
	font->GetHeight(&fh);
	fHeight = fh.ascent + fh.descent + fh.leading + 4;
	SetHeight(fHeight);
}

void
SVGListItem::_DrawBackground(BView* owner, BRect frame)
{
	rgb_color backgroundColor;
	
	if (IsSelected()) {
		backgroundColor = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
	} else {
		backgroundColor = ui_color(B_LIST_BACKGROUND_COLOR);
	}

	owner->SetHighColor(backgroundColor);
	owner->FillRect(frame);
}

void
SVGListItem::_DrawColorSwatches(BView* owner, BRect rect, NSVGshape* shape, float swatchSize)
{
	if (!shape)
		return;

	BRect fillRect(rect.left, rect.top,
				   rect.left + swatchSize - 1, rect.top + swatchSize);
	BRect strokeRect(rect.left + swatchSize + 1, rect.top,
					 rect.left + (swatchSize * 2), rect.top + swatchSize);

	if (shape->fill.type != NSVG_PAINT_NONE) {
		_DrawSingleColorSwatch(owner, fillRect, &shape->fill);
	} else {
		owner->SetHighColor(200, 200, 200);
		owner->FillRect(fillRect);
		owner->SetHighColor(255, 0, 0);
		owner->StrokeLine(fillRect.LeftTop(), fillRect.RightBottom());
		owner->SetHighColor(0, 0, 0);
		owner->StrokeRect(fillRect);
	}

	if (shape->stroke.type != NSVG_PAINT_NONE) {
		_DrawSingleColorSwatch(owner, strokeRect, &shape->stroke);
	} else {
		owner->SetHighColor(200, 200, 200);
		owner->FillRect(strokeRect);
		owner->SetHighColor(255, 0, 0);
		owner->StrokeLine(strokeRect.LeftTop(), strokeRect.RightBottom());
		owner->SetHighColor(0, 0, 0);
		owner->StrokeRect(strokeRect);
	}
}

void
SVGListItem::_DrawSingleColorSwatch(BView* owner, BRect rect, NSVGpaint* paint)
{
	if (!paint || paint->type == NSVG_PAINT_NONE)
		return;

	_DrawTransparencyBackground(owner, rect);

	if (paint->type == NSVG_PAINT_COLOR) {
		unsigned int color = paint->color;
		rgb_color fillColor = _NSVGColorToRGB(color);
		owner->SetHighColor(fillColor);
		owner->SetDrawingMode(B_OP_ALPHA);
		owner->FillRect(rect);
		owner->SetDrawingMode(B_OP_COPY);
	} else if (paint->type == NSVG_PAINT_LINEAR_GRADIENT) {
		_DrawLinearGradient(owner, rect, paint);
	} else if (paint->type == NSVG_PAINT_RADIAL_GRADIENT) {
		_DrawRadialGradient(owner, rect, paint);
	}

	owner->SetHighColor(0, 0, 0);
	owner->StrokeRect(rect);
}

void
SVGListItem::_DrawIcon(BView* owner, BRect rect)
{
	if (!fIcon)
		return;

	owner->PushState();
	owner->SetDrawingMode(B_OP_ALPHA);

	BRect iconRect = fIcon->Bounds();
	BPoint offset;
	offset.x = rect.left + (rect.Width() - iconRect.Width()) / 2;
	offset.y = rect.top + (rect.Height() - iconRect.Height()) / 2;

	owner->DrawBitmap(fIcon, offset);
	owner->PopState();
}

void
SVGListItem::_DrawLinearGradient(BView* owner, BRect rect, NSVGpaint* paint)
{
	if (!paint->gradient || paint->gradient->nstops < 2) {
		owner->SetHighColor(128, 128, 128);
		owner->FillRect(rect);
		return;
	}

	NSVGgradient* grad = paint->gradient;
	
	BGradientLinear gradient(rect.left, rect.top + rect.Height() / 2, 
							rect.right, rect.top + rect.Height() / 2);

	for (int i = 0; i < grad->nstops; i++) {
		rgb_color color = _NSVGColorToRGB(grad->stops[i].color);
		float offset = grad->stops[i].offset;
		if (offset < 0.0f) offset = 0.0f;
		if (offset > 1.0f) offset = 1.0f;
		int32 haiku_offset = (int32)(offset * 255.0f);
		gradient.AddColor(color, haiku_offset);
	}

	owner->SetDrawingMode(B_OP_ALPHA);
	owner->FillRect(rect, gradient);
	owner->SetDrawingMode(B_OP_COPY);
}

void
SVGListItem::_DrawRadialGradient(BView* owner, BRect rect, NSVGpaint* paint)
{
	if (!paint->gradient || paint->gradient->nstops < 2) {
		owner->SetHighColor(128, 128, 128);
		owner->FillRect(rect);
		return;
	}

	NSVGgradient* grad = paint->gradient;
	
	BPoint center(rect.left + rect.Width() / 2, rect.top + rect.Height() / 2);
	float radius = min_c(rect.Width(), rect.Height()) / 2;

	BGradientRadial gradient(center.x, center.y, radius);

	for (int i = 0; i < grad->nstops; i++) {
		rgb_color color = _NSVGColorToRGB(grad->stops[i].color);
		float offset = grad->stops[i].offset;
		if (offset < 0.0f) offset = 0.0f;
		if (offset > 1.0f) offset = 1.0f;
		int32 haiku_offset = (int32)(offset * 255.0f);
		gradient.AddColor(color, haiku_offset);
	}

	owner->SetDrawingMode(B_OP_ALPHA);
	owner->FillRect(rect, gradient);
	owner->SetDrawingMode(B_OP_COPY);
}

void
SVGListItem::_DrawTransparencyBackground(BView* owner, BRect rect)
{
	owner->SetHighColor(255, 255, 255);
	owner->FillRect(rect);
	owner->SetHighColor(220, 220, 220);

	const float checkerSize = 3.0f;
	for (float y = rect.top; y < rect.bottom; y += checkerSize) {
		for (float x = rect.left; x < rect.right; x += checkerSize) {
			int checkerX = (int)((x - rect.left) / checkerSize);
			int checkerY = (int)((y - rect.top) / checkerSize);

			if ((checkerX + checkerY) % 2 == 1) {
				BRect checkerRect(x, y, 
								min_c(x + checkerSize, rect.right),
								min_c(y + checkerSize, rect.bottom));
				owner->FillRect(checkerRect);
			}
		}
	}
}

rgb_color
SVGListItem::_NSVGColorToRGB(unsigned int color)
{
	rgb_color result;
	result.red = (color >> 0) & 0xFF;
	result.green = (color >> 8) & 0xFF;
	result.blue = (color >> 16) & 0xFF;
	result.alpha = (color >> 24) & 0xFF;

	if (result.alpha == 0 && (color & 0x00FFFFFF) != 0) {
		result.alpha = 255;
	}

	return result;
}

rgb_color
SVGListItem::_InterpolateColor(const rgb_color& color1, const rgb_color& color2, float t)
{
	rgb_color result;
	result.red = (uint8)(color1.red + t * (color2.red - color1.red));
	result.green = (uint8)(color1.green + t * (color2.green - color1.green));
	result.blue = (uint8)(color1.blue + t * (color2.blue - color1.blue));
	result.alpha = (uint8)(color1.alpha + t * (color2.alpha - color1.alpha));
	return result;
}
