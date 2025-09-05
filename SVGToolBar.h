/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SVGEAR_TOOLBAR
#define SVGEAR_TOOLBAR

#include <Rect.h>
#include <private/shared/ToolBar.h>

class SVGToolBar : public BPrivate::BToolBar {
public:
	SVGToolBar(orientation orient = B_HORIZONTAL);
	virtual void Draw(BRect rect);
private:
	orientation fOrientation;
};

#endif
