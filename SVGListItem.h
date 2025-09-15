/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SVG_LIST_ITEM_H
#define SVG_LIST_ITEM_H

#include <ListItem.h>
#include <View.h>
#include <String.h>
#include <Bitmap.h>
#include <Gradient.h>

struct NSVGshape;
struct NSVGpath;
struct NSVGpaint;

enum svg_item_type {
	SVG_ITEM_SHAPE = 0,
	SVG_ITEM_PATH,
	SVG_ITEM_PAINT
};

class SVGListItem : public BListItem {
public:
	SVGListItem(NSVGshape* shape, int32 index);
	SVGListItem(NSVGpath* path, int32 shapeIndex, int32 pathIndex);
	SVGListItem(NSVGpaint* paint, const char* name, int32 shapeIndex, bool isStroke);

	virtual ~SVGListItem();

	virtual void DrawItem(BView* owner, BRect frame, bool complete = false);
	virtual void Update(BView* owner, const BFont* font);

	void SetIcon(BBitmap* bitmap);

	svg_item_type GetType() const { return fType; }
	NSVGshape* GetShape() const { return fShape; }
	NSVGpath* GetPath() const { return fPath; }
	NSVGpaint* GetPaint() const { return fPaint; }
	int32 GetIndex() const { return fIndex; }
	int32 GetShapeIndex() const { return fShapeIndex; }
	int32 GetPathIndex() const { return fPathIndex; }

	bool IsStroke() const { return fIsStroke; }
	float GetRequiredWidth() const { return fRequiredWidth; }

private:
	void _DrawBackground(BView* owner, BRect frame);
	void _DrawColorSwatches(BView* owner, BRect rect, NSVGshape* shape, float swatchSize);
	void _DrawSingleColorSwatch(BView* owner, BRect rect, NSVGpaint* paint);
	void _DrawIcon(BView* owner, BRect rect);
	void _DrawLinearGradient(BView* owner, BRect rect, NSVGpaint* paint);
	void _DrawRadialGradient(BView* owner, BRect rect, NSVGpaint* paint);
	void _DrawTransparencyBackground(BView* owner, BRect rect);
	void _UpdateDisplayText();
	rgb_color _NSVGColorToRGB(unsigned int color);
	rgb_color _InterpolateColor(const rgb_color& color1, const rgb_color& color2, float t);

private:
	svg_item_type fType;
	NSVGshape* fShape;
	NSVGpath* fPath;
	NSVGpaint* fPaint;
	BString fName;
	int32 fIndex;
	int32 fShapeIndex;
	int32 fPathIndex;
	bool fIsStroke;
	float fHeight;
	BBitmap* fIcon;

	float fRequiredWidth;
	float fIconSize;
	float fSwatchSize;
	float fLeftMargin;
	float fRightMargin;
	BString fDisplayText;
};

#endif // SVG_LIST_ITEM_H
