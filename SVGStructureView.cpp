/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Catalog.h>
#include <LayoutBuilder.h>

#include "nanosvg.h"

#include "SVGConstants.h"
#include "SVGStructureView.h"
#include "SVGView.h"
#include "SVGApplication.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SVGStructureView"

SVGStructureView::SVGStructureView(const char* name)
	: BView(name, B_WILL_DRAW),
	fTabView(NULL),
	fShapesList(NULL),
	fPathsList(NULL),
	fPaintsList(NULL),
	fShapesScroll(NULL),
	fPathsScroll(NULL),
	fPaintsScroll(NULL),
	fSVGImage(NULL),
	fSVGView(NULL),
	fShapeIcon(NULL),
	fPathIcon(NULL),
	fColorIcon(NULL),
	fLinearGradientIcon(NULL),
	fRadialGradientIcon(NULL),
	fSelectedShape(-1),
	fSelectedPath(-1)
{
	SetExplicitMaxSize(BSize(256, B_SIZE_UNSET));
	SetExplicitMinSize(BSize(256, B_SIZE_UNSET));
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	GetFont(&fFont);
	fFont.SetSize(fFont.Size() * 0.9);

	_LoadIcons();
	_BuildInterface();
}

SVGStructureView::~SVGStructureView()
{
	_ClearListItems(fShapesList);
	_ClearListItems(fPathsList);
	_ClearListItems(fPaintsList);

	_DeleteIcons();
}

void
SVGStructureView::_LoadIcons()
{
	font_height fh;
	fFont.GetHeight(&fh);
	int32 iconSize = (int32)(fh.ascent + fh.descent + fh.leading);

	if (iconSize < 12) iconSize = 12;
	if (iconSize > 32) iconSize = 32;

	fShapeIcon = SVGApplication::GetIcon("draw-shape", iconSize);
	fPathIcon = SVGApplication::GetIcon("draw-path", iconSize);
	fColorIcon = SVGApplication::GetIcon("colors", iconSize);
	fLinearGradientIcon = SVGApplication::GetIcon("linear-gradients", iconSize);
	fRadialGradientIcon = SVGApplication::GetIcon("radial-gradients", iconSize);
}

void
SVGStructureView::_DeleteIcons()
{
	delete fShapeIcon;
	delete fPathIcon;
	delete fColorIcon;
	delete fLinearGradientIcon;
	delete fRadialGradientIcon;

	fShapeIcon = NULL;
	fPathIcon = NULL;
	fColorIcon = NULL;
	fLinearGradientIcon = NULL;
	fRadialGradientIcon = NULL;
}

void
SVGStructureView::UpdateIcons()
{
	_DeleteIcons();
	_LoadIcons();

	if (fShapesList) {
		for (int32 i = 0; i < fShapesList->CountItems(); i++) {
			SVGListItem* item = dynamic_cast<SVGListItem*>(fShapesList->ItemAt(i));
			if (item && item->GetType() == SVG_ITEM_SHAPE) {
				item->SetIcon(fShapeIcon);
			}
		}
		fShapesList->Invalidate();
	}

	if (fPathsList) {
		for (int32 i = 0; i < fPathsList->CountItems(); i++) {
			SVGListItem* item = dynamic_cast<SVGListItem*>(fPathsList->ItemAt(i));
			if (item && item->GetType() == SVG_ITEM_PATH) {
				item->SetIcon(fPathIcon);
			}
		}
		fPathsList->Invalidate();
	}

	if (fPaintsList) {
		for (int32 i = 0; i < fPaintsList->CountItems(); i++) {
			SVGListItem* item = dynamic_cast<SVGListItem*>(fPaintsList->ItemAt(i));
			if (item && item->GetType() == SVG_ITEM_PAINT) {
				item->SetIcon(_GetPaintIcon(item->GetPaint()));
			}
		}
		fPaintsList->Invalidate();
	}
}

BBitmap*
SVGStructureView::_GetPaintIcon(NSVGpaint* paint)
{
	if (!paint)
		return fColorIcon;

	switch (paint->type) {
		case NSVG_PAINT_COLOR:
			return fColorIcon;
		case NSVG_PAINT_LINEAR_GRADIENT:
			return fLinearGradientIcon;
		case NSVG_PAINT_RADIAL_GRADIENT:
			return fRadialGradientIcon;
		default:
			return fColorIcon;
	}
}

void
SVGStructureView::_BuildInterface()
{
	fTabView = new BTabView("structure_tabs", B_WIDTH_FROM_WIDEST);

	fShapesList = new BListView("shapes_list", B_SINGLE_SELECTION_LIST);
	fShapesList->SetSelectionMessage(new BMessage(MSG_SHAPE_SELECTED));
	fShapesScroll = new BScrollView("shapes_scroll", fShapesList,
									B_WILL_DRAW | B_FRAME_EVENTS,
									false, true, B_NO_BORDER);

	BTab* shapesTab = new BTab();
	fTabView->AddTab(fShapesScroll, shapesTab);
	shapesTab->SetLabel(B_TRANSLATE("Shapes"));

	fPathsList = new BListView("paths_list", B_SINGLE_SELECTION_LIST);
	fPathsList->SetSelectionMessage(new BMessage(MSG_PATH_SELECTED));
	fPathsScroll = new BScrollView("paths_scroll", fPathsList,
									B_WILL_DRAW | B_FRAME_EVENTS,
									false, true, B_NO_BORDER);

	BTab* pathsTab = new BTab();
	fTabView->AddTab(fPathsScroll, pathsTab);
	pathsTab->SetLabel(B_TRANSLATE("Paths"));

	fPaintsList = new BListView("paints_list", B_SINGLE_SELECTION_LIST);
	fPaintsList->SetSelectionMessage(new BMessage(MSG_PAINT_SELECTED));
	fPaintsScroll = new BScrollView("paints_scroll", fPaintsList,
									B_WILL_DRAW | B_FRAME_EVENTS,
									false, true, B_NO_BORDER);

	BTab* paintsTab = new BTab();
	fTabView->AddTab(fPaintsScroll, paintsTab);
	paintsTab->SetLabel(B_TRANSLATE("Paints"));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(fTabView)
		.End();
}

void
SVGStructureView::Draw(BRect updateRect)
{
	BRect rect = Bounds();
	rgb_color base = LowColor();

	BView::Draw(rect & updateRect);
	be_control_look->DrawBorder(this, rect, updateRect, base, B_PLAIN_BORDER, 0, 
								BControlLook::B_RIGHT_BORDER);
}

void
SVGStructureView::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (fShapesList)
		fShapesList->SetTarget(this);
	if (fPathsList)
		fPathsList->SetTarget(this);
	if (fPaintsList)
		fPaintsList->SetTarget(this);
}

void
SVGStructureView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SHAPE_SELECTED:
			_HandleShapeSelection(message);
			break;
		case MSG_PATH_SELECTED:
			_HandlePathSelection(message);
			break;
		case MSG_PAINT_SELECTED:
			_HandlePaintSelection(message);
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}

void
SVGStructureView::SetSVGImage(NSVGimage* image)
{
	fSVGImage = image;
	UpdateStructure();
}

void
SVGStructureView::UpdateStructure()
{
	_ClearListItems(fShapesList);
	_ClearListItems(fPathsList);
	_ClearListItems(fPaintsList);

	if (!fSVGImage)
		return;

	_PopulateShapesList();
	_PopulatePathsList();
	_PopulatePaintsList();
}

void
SVGStructureView::_PopulateShapesList()
{
	if (!fShapesList || !fSVGImage)
		return;

	int32 shapeIndex = 0;
	NSVGshape* shape = fSVGImage->shapes;

	while (shape) {
		SVGListItem* item = new SVGListItem(shape, shapeIndex);
		item->SetIcon(fShapeIcon);
		fShapesList->AddItem(item);
		shapeIndex++;
		shape = shape->next;
	}

	fShapesList->Invalidate();
}

void
SVGStructureView::_PopulatePathsList()
{
	if (!fPathsList || !fSVGImage)
		return;

	int32 shapeIndex = 0;
	NSVGshape* shape = fSVGImage->shapes;

	while (shape) {
		int32 pathIndex = 0;
		NSVGpath* path = shape->paths;

		while (path) {
			SVGListItem* item = new SVGListItem(path, shapeIndex, pathIndex);
			item->SetIcon(fPathIcon);
			fPathsList->AddItem(item);
			pathIndex++;
			path = path->next;
		}

		shapeIndex++;
		shape = shape->next;
	}

	fPathsList->Invalidate();
}

void
SVGStructureView::_PopulatePaintsList()
{
	if (!fPaintsList || !fSVGImage)
		return;

	int32 shapeIndex = 0;
	NSVGshape* shape = fSVGImage->shapes;

	while (shape) {
		if (shape->fill.type != NSVG_PAINT_NONE) {
			BString name;
			name.SetToFormat("Shape %ld Fill", shapeIndex);
			SVGListItem* item = new SVGListItem(&shape->fill, name.String(), shapeIndex, false);
			item->SetIcon(_GetPaintIcon(&shape->fill));
			fPaintsList->AddItem(item);
		}

		if (shape->stroke.type != NSVG_PAINT_NONE) {
			BString name;
			name.SetToFormat("Shape %ld Stroke", shapeIndex);
			SVGListItem* item = new SVGListItem(&shape->stroke, name.String(), shapeIndex, true);
			item->SetIcon(_GetPaintIcon(&shape->stroke));
			fPaintsList->AddItem(item);
		}

		shapeIndex++;
		shape = shape->next;
	}

	fPaintsList->Invalidate();
}

void
SVGStructureView::_ClearListItems(BListView* listView)
{
	if (!listView)
		return;

	int32 count = listView->CountItems();
	for (int32 i = 0; i < count; i++) {
		BListItem* item = listView->ItemAt(i);
		delete item;
	}
	listView->MakeEmpty();
}

void
SVGStructureView::_HandleShapeSelection(BMessage* message)
{
	int32 selection = fShapesList->CurrentSelection();
	if (selection < 0)
		return;

	SVGListItem* item = dynamic_cast<SVGListItem*>(fShapesList->ItemAt(selection));
	if (!item || item->GetType() != SVG_ITEM_SHAPE)
		return;

	fSelectedShape = item->GetIndex();
	fSelectedPath = -1;

	if (fSVGView)
		fSVGView->SetHighlightedShape(fSelectedShape);
}

void
SVGStructureView::_HandlePathSelection(BMessage* message)
{
	int32 selection = fPathsList->CurrentSelection();
	if (selection < 0)
		return;

	SVGListItem* item = dynamic_cast<SVGListItem*>(fPathsList->ItemAt(selection));
	if (!item || item->GetType() != SVG_ITEM_PATH)
		return;

	fSelectedShape = item->GetShapeIndex();
	fSelectedPath = item->GetPathIndex();

	if (fSVGView)
		fSVGView->SetHighlightControlPoints(fSelectedShape, fSelectedPath, true);
}

void
SVGStructureView::_HandlePaintSelection(BMessage* message)
{
	int32 selection = fPaintsList->CurrentSelection();
	if (selection < 0)
		return;

	SVGListItem* item = dynamic_cast<SVGListItem*>(fPaintsList->ItemAt(selection));
	if (!item || item->GetType() != SVG_ITEM_PAINT)
		return;

	fSelectedShape = item->GetShapeIndex();

	if (fSVGView)
		fSVGView->SetHighlightedShape(fSelectedShape);
}

void
SVGStructureView::ClearSelection()
{
	if (fShapesList)
		fShapesList->DeselectAll();

	if (fPathsList)
		fPathsList->DeselectAll();

	if (fPaintsList)
		fPaintsList->DeselectAll();

	if (fSVGView)
		fSVGView->ClearHighlight();

	fSelectedShape = -1;
	fSelectedPath = -1;
}

const char*
SVGStructureView::_GetPaintTypeName(int paintType)
{
	switch (paintType) {
		case NSVG_PAINT_COLOR:
			return B_TRANSLATE("Color");
		case NSVG_PAINT_LINEAR_GRADIENT:
			return B_TRANSLATE("Linear Gradient");
		case NSVG_PAINT_RADIAL_GRADIENT:
			return B_TRANSLATE("Radial Gradient");
		default:
			return B_TRANSLATE("None");
	}
}
