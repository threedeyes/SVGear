/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ChipView.h"
#include <ControlLook.h>
#include <Cursor.h>

static const float kCategoryCornerRadius = 4.0f;
static const float kHPadding = 6.0f;
static const float kVPadding = 3.0f;
static const float kFocusLineWidth = 2.0f;
static const float kFocusInset = -2.0f;


ChipView::ChipView(const char* name, const char* label, BMessage* message,
	chip_style style)
	:
	BControl(name, label, message, B_WILL_DRAW | B_NAVIGABLE),
	fStyle(style),
	fClickable(style == B_CHIP_STYLE_CATEGORY)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(B_PANEL_BACKGROUND_COLOR);
}


ChipView::~ChipView()
{
}


void
ChipView::SetValue(int32 value)
{
	if (Value() != value) {
		BControl::SetValue(value);
		Invalidate();
	}
}


void
ChipView::SetLabel(const char* label)
{
	BControl::SetLabel(label);
	InvalidateLayout();
	Invalidate();
}


void
ChipView::SetClickable(bool clickable)
{
	fClickable = clickable;
}


void
ChipView::GetPreferredSize(float* width, float* height)
{
	font_height fh;
	GetFontHeight(&fh);

	float textWidth = StringWidth(Label());
	float fontHeight = fh.ascent + fh.descent;

	if (width)
		*width = textWidth + kHPadding * 2;
	if (height)
		*height = fontHeight + kVPadding * 2;
}


BSize
ChipView::MinSize()
{
	float width, height;
	GetPreferredSize(&width, &height);
	return BSize(width, height);
}


BSize
ChipView::MaxSize()
{
	float width, height;
	GetPreferredSize(&width, &height);
	return BSize(width, height);
}


BSize
ChipView::PreferredSize()
{
	float width, height;
	GetPreferredSize(&width, &height);
	return BSize(width, height);
}


void
ChipView::Draw(BRect updateRect)
{
	bool selected = (Value() == B_CONTROL_ON);
	bool focused = IsFocus() && fClickable;

	rgb_color bgColor = selected
		? ui_color(B_CONTROL_HIGHLIGHT_COLOR)
		: ui_color(B_CONTROL_BACKGROUND_COLOR);

	SetHighColor(bgColor);
	FillRoundRect(_ChipRect(), _CornerRadius(), _CornerRadius());

	SetHighColor(ui_color(B_CONTROL_BORDER_COLOR));
	StrokeRoundRect(_ChipRect(), _CornerRadius(), _CornerRadius());

	SetHighColor(ui_color(B_CONTROL_TEXT_COLOR));

	font_height fh;
	GetFontHeight(&fh);

	BRect bounds = Bounds();
	float textWidth = StringWidth(Label());
	float textX = (bounds.Width() - textWidth) / 2.0f;
	float textY = (bounds.Height() - (fh.ascent + fh.descent)) / 2.0f + fh.ascent;

	DrawString(Label(), BPoint(textX, textY));

	if (focused) {
		SetHighColor(ui_color(B_CONTROL_MARK_COLOR));
		SetPenSize(kFocusLineWidth);
		StrokeRoundRect(_FocusRect(), _CornerRadius(), _CornerRadius());
		SetPenSize(1.0f);
	}
}


void
ChipView::MouseDown(BPoint where)
{
	if (!IsEnabled() || !fClickable)
		return;

	SetValue(Value() == B_CONTROL_ON ? B_CONTROL_OFF : B_CONTROL_ON);
	Invoke();
}


void
ChipView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if (fClickable) {
		if (transit == B_ENTERED_VIEW) {
			BCursor cursor(B_CURSOR_ID_FOLLOW_LINK);
			SetViewCursor(&cursor);
		} else if (transit == B_EXITED_VIEW) {
			SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
		}
	}

	BControl::MouseMoved(where, transit, dragMessage);
}


void
ChipView::MakeFocus(bool focused)
{
	if (fClickable) {
		BControl::MakeFocus(focused);
		Invalidate();
	}
}


void
ChipView::KeyDown(const char* bytes, int32 numBytes)
{
	if (fClickable && (bytes[0] == B_SPACE || bytes[0] == B_ENTER)) {
		MouseDown(BPoint(0, 0));
		return;
	}

	BControl::KeyDown(bytes, numBytes);
}


float
ChipView::_CornerRadius() const
{
	if (fStyle == B_CHIP_STYLE_TAG) {
		return Bounds().Height() / 2.0f;
	}
	return kCategoryCornerRadius;
}


BRect
ChipView::_ChipRect() const
{
	return Bounds();
}


BRect
ChipView::_FocusRect() const
{
	BRect rect = _ChipRect();
	rect.InsetBy(kFocusInset, kFocusInset);
	return rect;
}
