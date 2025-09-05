/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <ControlLook.h>

#include "SVGToolBar.h"

SVGToolBar::SVGToolBar(orientation orient) : BToolBar(orient),
	fOrientation(orient)
{
	if (orient == B_HORIZONTAL)
		GroupLayout()->SetInsets(0.0f, 0.0f, 0.0f, 1.0f);
	else {
		SetResizingMode(B_FOLLOW_TOP_BOTTOM);
		GroupLayout()->SetInsets(0.0f, 0.0f, 1.0f, 0.0f);
	}
	SetFlags(Flags() | B_WILL_DRAW);
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

void
SVGToolBar::Draw(BRect updateRect)
{
	BRect rect = Bounds();
	rgb_color base = LowColor();
	be_control_look->DrawBorder(this, rect, updateRect, base, B_PLAIN_BORDER, 0,
		fOrientation == B_HORIZONTAL ? BControlLook::B_BOTTOM_BORDER :  BControlLook::B_RIGHT_BORDER);
	BToolBar::Draw(rect & updateRect);
}
