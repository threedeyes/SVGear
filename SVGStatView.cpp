/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "SVGApplication.h"
#include "SVGStatView.h"
#include "nanosvg.h"

#undef  B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT          "SVGStatView"

SVGStatView::SVGStatView(const char* name)
	: BView(name, B_WILL_DRAW), svgImage(NULL)
{
	SetExplicitMinSize(BSize(32,32));

	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	view = new BGroupView("g_stat_view", B_VERTICAL, 1);
	view->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	view->GroupLayout()->SetInsets(8);
	AddChild(view);

	view->GetFont(&font);
	font.SetSize(font.Size() * 0.9);
	font.SetFace(B_BOLD_FACE);

	BGroupLayout *vertLayout = new BGroupLayout(B_VERTICAL);
	vertLayout->SetInsets(1, 0, 0, 0);
	SetLayout(vertLayout);

	BStringView *fileTitle = new BStringView("file", B_TRANSLATE("File"));
	fileTitle->SetAlignment(B_ALIGN_CENTER);
	fileTitle->SetFont(&font, B_FONT_FACE);
	view->AddChild(fileTitle);

	view->AddChild(new BStringView("svg-size", B_TRANSLATE("SVG Size:")));
	view->AddChild(new BStringView("hvif-size", B_TRANSLATE("HVIF Size:")));

	BStringView *imageTitle = new BStringView("image", B_TRANSLATE("Image"));
	imageTitle->SetAlignment(B_ALIGN_CENTER);
	imageTitle->SetFont(&font, B_FONT_FACE);
	view->AddChild(imageTitle);

	view->AddChild(new BStringView("width", B_TRANSLATE("Width:")));
	view->AddChild(new BStringView("height", B_TRANSLATE("Height:")));

	BStringView *contentTitle = new BStringView("content", B_TRANSLATE("Content"));
	contentTitle->SetAlignment(B_ALIGN_CENTER);
	contentTitle->SetFont(&font, B_FONT_FACE);
	view->AddChild(contentTitle);

	view->AddChild(new BStringView("shapes", B_TRANSLATE("Shapes:")));
	view->AddChild(new BStringView("paths", B_TRANSLATE("Paths:")));
	view->AddChild(new BStringView("points", B_TRANSLATE("Points:")));

	BStringView *visualTitle = new BStringView("visual", B_TRANSLATE("Visual"));
	visualTitle->SetAlignment(B_ALIGN_CENTER);
	visualTitle->SetFont(&font, B_FONT_FACE);
	view->AddChild(visualTitle);

	view->AddChild(new BStringView("filled-shapes", B_TRANSLATE("Filled shapes:")));
	view->AddChild(new BStringView("stroked-shapes", B_TRANSLATE("Stroked shapes:")));
	view->AddChild(new BStringView("gradients", B_TRANSLATE("Gradients:")));
	view->AddChild(new BStringView("closed-paths", B_TRANSLATE("Closed paths:")));

	BStringView *boundsTitle = new BStringView("bounds", B_TRANSLATE("Bounds"));
	boundsTitle->SetAlignment(B_ALIGN_CENTER);
	boundsTitle->SetFont(&font, B_FONT_FACE);
	view->AddChild(boundsTitle);

	view->AddChild(new BStringView("min-x", B_TRANSLATE("Min X:")));
	view->AddChild(new BStringView("min-y", B_TRANSLATE("Min Y:")));
	view->AddChild(new BStringView("max-x", B_TRANSLATE("Max X:")));
	view->AddChild(new BStringView("max-y", B_TRANSLATE("Max Y:")));

	view->AddChild(BSpaceLayoutItem::CreateGlue());

	BView *child;
	if ( child = view->ChildAt(0) ) {
		while ( child ) {
			child->SetFont(&font, B_FONT_SIZE);
			child = child->NextSibling();
		}
	}
}

void
SVGStatView::Draw(BRect updateRect)
{
	BRect rect = Bounds();
	rgb_color base = LowColor();

	BView::Draw(rect & updateRect);
	be_control_look->DrawBorder(this, rect, updateRect, base, B_PLAIN_BORDER, 0, BControlLook::B_LEFT_BORDER);
}

void
SVGStatView::SetSVGImage(NSVGimage* image)
{
	svgImage = image;
	UpdateStatistics();
}

void
SVGStatView::UpdateStatistics()
{
	if (svgImage == NULL) {
		SetIntValue("svg-size", 0);
		SetIntValue("hvif-size", 0);
		SetFloatValue("width", 0.0f);
		SetFloatValue("height", 0.0f);
		SetIntValue("shapes", 0);
		SetIntValue("paths", 0);
		SetIntValue("points", 0);
		SetFloatValue("min-x", 0.0f);
		SetFloatValue("min-y", 0.0f);
		SetFloatValue("max-x", 0.0f);
		SetFloatValue("max-y", 0.0f);
		SetIntValue("filled-shapes", 0);
		SetIntValue("stroked-shapes", 0);
		SetIntValue("gradients", 0);
		SetIntValue("closed-paths", 0);
		return;
	}

	// Basic image properties
	SetFloatValue("width", svgImage->width, false);
	SetFloatValue("height", svgImage->height, false);

	// Count shapes and paths
	int shapeCount = CountShapes(svgImage);
	int pathCount = CountPaths(svgImage);
	SetIntValue("shapes", shapeCount);
	SetIntValue("paths", pathCount);

	// Count other statistics
	int totalPoints = 0;
	int filledShapes = 0;
	int strokedShapes = 0;
	int gradientCount = 0;
	int closedPaths = 0;

	NSVGshape* shape = svgImage->shapes;
	while (shape != NULL) {
		// Count filled/stroked shapes
		if (shape->fill.type != NSVG_PAINT_NONE) {
			filledShapes++;
			if (shape->fill.type == NSVG_PAINT_LINEAR_GRADIENT || 
				shape->fill.type == NSVG_PAINT_RADIAL_GRADIENT) {
				gradientCount++;
			}
		}
		if (shape->stroke.type != NSVG_PAINT_NONE) {
			strokedShapes++;
			if (shape->stroke.type == NSVG_PAINT_LINEAR_GRADIENT || 
				shape->stroke.type == NSVG_PAINT_RADIAL_GRADIENT) {
				gradientCount++;
			}
		}

		// Count points and closed paths
		NSVGpath* path = shape->paths;
		while (path != NULL) {
			totalPoints += path->npts;
			if (path->closed) {
				closedPaths++;
			}
			path = path->next;
		}
		shape = shape->next;
	}

	SetIntValue("points", totalPoints);
	SetIntValue("filled-shapes", filledShapes);
	SetIntValue("stroked-shapes", strokedShapes);
	SetIntValue("gradients", gradientCount);
	SetIntValue("closed-paths", closedPaths);

	// Bounds
	float bounds[4];
	GetImageBounds(svgImage, bounds);
	SetFloatValue("min-x", bounds[0], false);
	SetFloatValue("min-y", bounds[1], false);
	SetFloatValue("max-x", bounds[2], false);
	SetFloatValue("max-y", bounds[3], false);
}

int
SVGStatView::CountShapes(NSVGimage* image)
{
	if (image == NULL) return 0;

	int count = 0;
	NSVGshape* shape = image->shapes;
	while (shape != NULL) {
		count++;
		shape = shape->next;
	}
	return count;
}

int
SVGStatView::CountPaths(NSVGimage* image)
{
	if (image == NULL) return 0;

	int count = 0;
	NSVGshape* shape = image->shapes;
	while (shape != NULL) {
		NSVGpath* path = shape->paths;
		while (path != NULL) {
			count++;
			path = path->next;
		}
		shape = shape->next;
	}
	return count;
}

void
SVGStatView::GetImageBounds(NSVGimage* image, float* bounds)
{
	if (image == NULL || image->shapes == NULL) {
		bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0f;
		return;
	}

	NSVGshape* shape = image->shapes;
	bounds[0] = shape->bounds[0];
	bounds[1] = shape->bounds[1];
	bounds[2] = shape->bounds[2];
	bounds[3] = shape->bounds[3];

	shape = shape->next;
	while (shape != NULL) {
		if (shape->bounds[0] < bounds[0]) bounds[0] = shape->bounds[0];
		if (shape->bounds[1] < bounds[1]) bounds[1] = shape->bounds[1];
		if (shape->bounds[2] > bounds[2]) bounds[2] = shape->bounds[2];
		if (shape->bounds[3] > bounds[3]) bounds[3] = shape->bounds[3];
		shape = shape->next;
	}
}

void
SVGStatView::SetFloatValue(const char *param, float value, bool exp)
{
	BStringView *item = (BStringView*)view->FindView(param);
	if (item != NULL) {
		if (item->LockLooper()) {
			BString valueTxt;

			if (exp)
				valueTxt.SetToFormat(" %g", value);
			else
				valueTxt.SetToFormat(" %.2f", value);

			BString text = item->Text();
			text = text.Truncate(text.FindFirst(':') + 1);
			text << valueTxt;
			item->SetText(text);
			item->UnlockLooper();
		}
	}
}

void
SVGStatView::SetIntValue(const char *param, int value)
{
	BStringView *item = (BStringView*)view->FindView(param);
	if (item != NULL) {
		if (item->LockLooper()) {
			BString text = item->Text();
			text = text.Truncate(text.FindFirst(':') + 1);
			text << " ";
			text << value;
			item->SetText(text);
			item->UnlockLooper();
		}
	}
}

void
SVGStatView::SetTextValue(const char *param, const char *value)
{
	BStringView *item = (BStringView*)view->FindView(param);
	if (item != NULL) {
		if (item->LockLooper()) {
			BString text = item->Text();
			text = text.Truncate(text.FindFirst(':') + 1);
			text << " ";
			item->SetToolTip(value);
			BString newValue = value;
			font.TruncateString(&newValue, B_TRUNCATE_SMART, 100);
			text << newValue;
			item->SetText(text);
			item->UnlockLooper();
		}
	}
}
