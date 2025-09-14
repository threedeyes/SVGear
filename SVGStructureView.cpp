// Filename: ./SVGStructureView.cpp
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

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SVGStructureView"

SVGShapeItem::SVGShapeItem(NSVGshape* shape, int32 index)
	: BListItem(), fShape(shape), fIndex(index), fHeight(0)
{
}

SVGShapeItem::~SVGShapeItem()
{
}

void
SVGShapeItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	if (!fShape) return;
	
	rgb_color backgroundColor;
	rgb_color textColor;
	
	if (IsSelected()) {
		backgroundColor = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
		textColor = ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR);
	} else {
		backgroundColor = ui_color(B_LIST_BACKGROUND_COLOR);
		textColor = ui_color(B_LIST_ITEM_TEXT_COLOR);
	}

	owner->SetHighColor(backgroundColor);
	owner->FillRect(frame);
	owner->SetHighColor(textColor);

	BString text;
	text.SetToFormat("Shape %ld", fIndex);

	if (fShape->id && strlen(fShape->id) > 0)
		text << " (" << fShape->id << ")";

	font_height fh;
	owner->GetFontHeight(&fh);
	float textY = frame.top + (frame.Height() + fh.ascent - fh.descent) / 2;

	owner->DrawString(text.String(), BPoint(frame.left + 4, textY));

	if (fShape->fill.type != NSVG_PAINT_NONE) {
		BRect colorRect(frame.right - 20, frame.top + 2, frame.right - 2, frame.bottom - 2);

		if (fShape->fill.type == NSVG_PAINT_COLOR) {
			unsigned int color = fShape->fill.color;
			rgb_color fillColor;
			fillColor.red = (color >> 0) & 0xFF;
			fillColor.green = (color >> 8) & 0xFF;
			fillColor.blue = (color >> 16) & 0xFF;
			fillColor.alpha = 255;

			owner->SetHighColor(fillColor);
			owner->FillRect(colorRect);
		} else {
			owner->SetHighColor(128, 128, 128);
			for (int i = 0; i < colorRect.IntegerWidth(); i += 2) {
				owner->StrokeLine(
					BPoint(colorRect.left + i, colorRect.top),
					BPoint(colorRect.left + i, colorRect.bottom)
				);
			}
		}

		owner->SetHighColor(0, 0, 0);
		owner->StrokeRect(colorRect);
	}
}

void
SVGShapeItem::Update(BView* owner, const BFont* font)
{
	font_height fh;
	font->GetHeight(&fh);
	fHeight = fh.ascent + fh.descent + fh.leading + 4;
	SetHeight(fHeight);
}

SVGPathItem::SVGPathItem(NSVGpath* path, int32 shapeIndex, int32 pathIndex)
	: BListItem(), fPath(path), fShapeIndex(shapeIndex), fPathIndex(pathIndex), fHeight(0)
{
}

SVGPathItem::~SVGPathItem()
{
}

void
SVGPathItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	if (!fPath) return;

	rgb_color backgroundColor;
	rgb_color textColor;

	if (IsSelected()) {
		backgroundColor = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
		textColor = ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR);
	} else {
		backgroundColor = ui_color(B_LIST_BACKGROUND_COLOR);
		textColor = ui_color(B_LIST_ITEM_TEXT_COLOR);
	}

	owner->SetHighColor(backgroundColor);
	owner->FillRect(frame);
	owner->SetHighColor(textColor);

	BString text;
	text.SetToFormat("Path %ld.%ld (%d pts)", fShapeIndex, fPathIndex, fPath->npts);

	if (fPath->closed)
		text << " [closed]";

	font_height fh;
	owner->GetFontHeight(&fh);
	float textY = frame.top + (frame.Height() + fh.ascent - fh.descent) / 2;
	
	owner->DrawString(text.String(), BPoint(frame.left + 8, textY));
}

void
SVGPathItem::Update(BView* owner, const BFont* font)
{
	font_height fh;
	font->GetHeight(&fh);
	fHeight = fh.ascent + fh.descent + fh.leading + 4;
	SetHeight(fHeight);
}

SVGPaintItem::SVGPaintItem(NSVGpaint* paint, const char* name, int32 shapeIndex, bool isStroke)
	: BListItem(), fPaint(paint), fName(name), fShapeIndex(shapeIndex), fIsStroke(isStroke), fHeight(0)
{
}

SVGPaintItem::~SVGPaintItem()
{
}

void
SVGPaintItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	if (!fPaint) return;

	rgb_color backgroundColor;
	rgb_color textColor;

	if (IsSelected()) {
		backgroundColor = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
		textColor = ui_color(B_LIST_SELECTED_ITEM_TEXT_COLOR);
	} else {
		backgroundColor = ui_color(B_LIST_BACKGROUND_COLOR);
		textColor = ui_color(B_LIST_ITEM_TEXT_COLOR);
	}

	owner->SetHighColor(backgroundColor);
	owner->FillRect(frame);
	owner->SetHighColor(textColor);

	font_height fh;
	owner->GetFontHeight(&fh);
	float textY = frame.top + (frame.Height() + fh.ascent - fh.descent) / 2;

	owner->DrawString(fName.String(), BPoint(frame.left + 4, textY));

	BRect colorRect(frame.right - 20, frame.top + 2, frame.right - 2, frame.bottom - 2);

	if (fPaint->type == NSVG_PAINT_COLOR) {
		unsigned int color = fPaint->color;
		rgb_color paintColor;
		paintColor.red = (color >> 0) & 0xFF;
		paintColor.green = (color >> 8) & 0xFF;
		paintColor.blue = (color >> 16) & 0xFF;
		paintColor.alpha = 255;

		owner->SetHighColor(paintColor);
		owner->FillRect(colorRect);
	} else if (fPaint->type == NSVG_PAINT_LINEAR_GRADIENT || 
			   fPaint->type == NSVG_PAINT_RADIAL_GRADIENT) {
		for (int i = 0; i < colorRect.IntegerWidth(); i++) {
			float t = (float)i / colorRect.Width();
			int grayLevel = (int)(128 + 64 * sin(t * 6.28));
			owner->SetHighColor(grayLevel, grayLevel, grayLevel);
			owner->StrokeLine(
				BPoint(colorRect.left + i, colorRect.top),
				BPoint(colorRect.left + i, colorRect.bottom)
			);
		}
	}

	owner->SetHighColor(0, 0, 0);
	owner->StrokeRect(colorRect);
}

void
SVGPaintItem::Update(BView* owner, const BFont* font)
{
	font_height fh;
	font->GetHeight(&fh);
	fHeight = fh.ascent + fh.descent + fh.leading + 4;
	SetHeight(fHeight);
}

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
	  fSelectedShape(-1),
	  fSelectedPath(-1)
{
	SetExplicitMaxSize(BSize(256, B_SIZE_UNSET));
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	GetFont(&fFont);
	fFont.SetSize(fFont.Size() * 0.9);

	_BuildInterface();
}

SVGStructureView::~SVGStructureView()
{
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
	if (fShapesList)
		fShapesList->MakeEmpty();

	if (fPathsList)
		fPathsList->MakeEmpty();

	if (fPaintsList)
		fPaintsList->MakeEmpty();

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
		SVGShapeItem* item = new SVGShapeItem(shape, shapeIndex);
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
			SVGPathItem* item = new SVGPathItem(path, shapeIndex, pathIndex);
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
			name.SetToFormat("Shape %ld Fill (%s)", shapeIndex, _GetPaintTypeName(shape->fill.type));
			SVGPaintItem* item = new SVGPaintItem(&shape->fill, name.String(), shapeIndex, false);
			fPaintsList->AddItem(item);
		}

		if (shape->stroke.type != NSVG_PAINT_NONE) {
			BString name;
			name.SetToFormat("Shape %ld Stroke (%s)", shapeIndex, _GetPaintTypeName(shape->stroke.type));
			SVGPaintItem* item = new SVGPaintItem(&shape->stroke, name.String(), shapeIndex, true);
			fPaintsList->AddItem(item);
		}

		shapeIndex++;
		shape = shape->next;
	}

	fPaintsList->Invalidate();
}

void
SVGStructureView::_HandleShapeSelection(BMessage* message)
{
	int32 selection = fShapesList->CurrentSelection();
	if (selection < 0)
		return;

	SVGShapeItem* item = dynamic_cast<SVGShapeItem*>(fShapesList->ItemAt(selection));
	if (!item)
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

	SVGPathItem* item = dynamic_cast<SVGPathItem*>(fPathsList->ItemAt(selection));
	if (!item)
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

	SVGPaintItem* item = dynamic_cast<SVGPaintItem*>(fPaintsList->ItemAt(selection));
	if (!item)
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

rgb_color
SVGStructureView::_GetPaintColor(NSVGpaint* paint)
{
	rgb_color color = { 0, 0, 0, 255 };

	if (paint && paint->type == NSVG_PAINT_COLOR) {
		unsigned int c = paint->color;
		color.red = (c >> 0) & 0xFF;
		color.green = (c >> 8) & 0xFF;
		color.blue = (c >> 16) & 0xFF;
	}

	return color;
}
