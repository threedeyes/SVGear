/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SVG_STRUCTURE_VIEW_H
#define SVG_STRUCTURE_VIEW_H

#include <View.h>
#include <GroupView.h>
#include <ListView.h>
#include <ScrollView.h>
#include <StringView.h>
#include <String.h>
#include <Font.h>
#include <ListItem.h>
#include <Message.h>
#include <ControlLook.h>
#include <TabView.h>

struct NSVGimage;
struct NSVGshape;
struct NSVGpath;
struct NSVGpaint;

class SVGView;

class SVGShapeItem : public BListItem {
public:
	SVGShapeItem(NSVGshape* shape, int32 index);
	virtual ~SVGShapeItem();

	virtual void DrawItem(BView* owner, BRect frame, bool complete = false);
	virtual void Update(BView* owner, const BFont* font);

	NSVGshape* GetShape() const { return fShape; }
	int32 GetIndex() const { return fIndex; }

private:
	NSVGshape* fShape;
	int32 fIndex;
	float fHeight;
};

class SVGPathItem : public BListItem {
public:
	SVGPathItem(NSVGpath* path, int32 shapeIndex, int32 pathIndex);
	virtual ~SVGPathItem();

	virtual void DrawItem(BView* owner, BRect frame, bool complete = false);
	virtual void Update(BView* owner, const BFont* font);

	NSVGpath* GetPath() const { return fPath; }
	int32 GetShapeIndex() const { return fShapeIndex; }
	int32 GetPathIndex() const { return fPathIndex; }

private:
	NSVGpath* fPath;
	int32 fShapeIndex;
	int32 fPathIndex;
	float fHeight;
};

class SVGPaintItem : public BListItem {
public:
	SVGPaintItem(NSVGpaint* paint, const char* name, int32 shapeIndex, bool isStroke);
	virtual ~SVGPaintItem();

	virtual void DrawItem(BView* owner, BRect frame, bool complete = false);
	virtual void Update(BView* owner, const BFont* font);

	NSVGpaint* GetPaint() const { return fPaint; }
	int32 GetShapeIndex() const { return fShapeIndex; }
	bool IsStroke() const { return fIsStroke; }

private:
	NSVGpaint* fPaint;
	BString fName;
	int32 fShapeIndex;
	bool fIsStroke;
	float fHeight;
};

class SVGStructureView : public BView {
public:
	SVGStructureView(const char* name = "structure_view");
	virtual ~SVGStructureView();

	virtual void Draw(BRect updateRect);
	virtual void MessageReceived(BMessage* message);
	virtual void AttachedToWindow();

	void SetSVGImage(NSVGimage* image);
	void SetSVGView(SVGView* svgView) { fSVGView = svgView; }
	void UpdateStructure();
	void ClearSelection();

private:
	void _BuildInterface();
	void _PopulateShapesList();
	void _PopulatePathsList();
	void _PopulatePaintsList();
	void _ClearListItems(BListView* listView);
	void _HandleShapeSelection(BMessage* message);
	void _HandlePathSelection(BMessage* message);
	void _HandlePaintSelection(BMessage* message);
	const char* _GetPaintTypeName(int paintType);
	rgb_color _GetPaintColor(NSVGpaint* paint);

private:
	BTabView* fTabView;
	BListView* fShapesList;
	BListView* fPathsList;  
	BListView* fPaintsList;
	BScrollView* fShapesScroll;
	BScrollView* fPathsScroll;
	BScrollView* fPaintsScroll;

	NSVGimage* fSVGImage;
	SVGView* fSVGView;
	BFont fFont;

	int32 fSelectedShape;
	int32 fSelectedPath;
};

#endif
