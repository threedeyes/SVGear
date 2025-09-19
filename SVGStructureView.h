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
#include <Message.h>
#include <ControlLook.h>
#include <TabView.h>
#include <Bitmap.h>

#include "SVGListItem.h"
#include "SVGTextEdit.h"

struct NSVGimage;
struct NSVGpaint;

class SVGView;

class SVGStructureView : public BView {
public:
	SVGStructureView(const char* name = "structure_view");
	virtual ~SVGStructureView();

	virtual void Draw(BRect updateRect);
	virtual void MessageReceived(BMessage* message);
	virtual void AttachedToWindow();
	virtual void Hide();

	void SetSVGImage(NSVGimage* image);
	void SetSVGView(SVGView* svgView) { fSVGView = svgView; }
	void SetSVGTextEdit(SVGTextEdit *svgTextEdit) { fSVGTextEdit = svgTextEdit; }
	void UpdateStructure();
	void ClearSelection();
	void ForceUpdatePanelWidth();
	void AutoSelect(int32 position);

private:
	void _BuildInterface();
	void _LoadIcons();
	void _PopulateShapesList();
	void _PopulatePathsList();
	void _PopulatePaintsList();
	void _ClearListItems(BListView* listView);
	void _HandleShapeSelection(BMessage* message);
	void _HandlePathSelection(BMessage* message);
	void _HandlePaintSelection(BMessage* message);

	const char* _GetPaintTypeName(int paintType);
	BBitmap* _GetPaintIcon(NSVGpaint* paint);
	float _CalculatePreferredWidth();
	void _UpdatePanelWidth();
	float _CalculateTabsMinWidth();
	float _GetMinimumPanelWidth();
	void _MoveTextCursor(size_t from, size_t to);

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
	SVGTextEdit* fSVGTextEdit;
	BFont fFont;

	BBitmap* fShapeIcon;
	BBitmap* fPathIcon;
	BBitmap* fClosedPathIcon;
	BBitmap* fColorIcon;
	BBitmap* fLinearGradientIcon;
	BBitmap* fRadialGradientIcon;

	int32 fSelectedShape;
	int32 fSelectedPath;

	float fMaxTextItemWidth;

	bool fAutoSelectFlag;
};

#endif
