/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <LayoutBuilder.h>
#include <CardLayout.h>
#include <ScrollView.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Catalog.h>

#include "SVGVectorizationDialog.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SVGVectorizationDialog"

SVGVectorizationDialog::SVGVectorizationDialog(const char* imagePath, BHandler* target)
	: BWindow(BRect(100, 100, 600, 500), B_TRANSLATE("Vectorization Settings"),
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	fTarget(target),
	fImagePath(imagePath)
{
	fOptions.SetDefaults();
	_BuildInterface();
	_UpdateControls();
	CenterOnScreen();
}

SVGVectorizationDialog::~SVGVectorizationDialog()
{
}

void
SVGVectorizationDialog::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_VECTORIZATION_SETTINGS_CHANGED:
			_UpdateFromControls();
			_StartVectorization();
			break;
		case MSG_VECTORIZATION_OK:
			if (fTarget) {
				BMessage msg(MSG_VECTORIZATION_OK);
				msg.AddData("options", B_RAW_TYPE, &fOptions, sizeof(TracingOptions));
				fTarget->Looper()->PostMessage(&msg, fTarget);
			}
			PostMessage(B_QUIT_REQUESTED);
			break;
		case MSG_VECTORIZATION_CANCEL:
			PostMessage(B_QUIT_REQUESTED);
			break;
		case MSG_VECTORIZATION_RESET:
			_ResetToDefaults();
			break;
		case MSG_VECTORIZATION_STATUS_UPDATE:
		{
			BString status;
			if (message->FindString("status", &status) == B_OK) {
				SetStatusText(status.String());
			}
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}

bool
SVGVectorizationDialog::QuitRequested()
{
	return true;
}

TracingOptions
SVGVectorizationDialog::GetCurrentOptions() const
{
	return fOptions;
}

void
SVGVectorizationDialog::SetOptions(const TracingOptions& options)
{
	fOptions = options;
	_UpdateControls();
}

void
SVGVectorizationDialog::_BuildInterface()
{
	fTabView = new BTabView("settings_tabs", B_WIDTH_FROM_WIDEST);

	_BuildBasicTab();
	_BuildPreprocessingTab();
	_BuildSimplificationTab();
	_BuildGeometryTab();
	_BuildOutputTab();

	fStatusView = new BStringView("status", B_TRANSLATE("Ready"));

	fOKButton = new BButton("ok", B_TRANSLATE("OK"), new BMessage(MSG_VECTORIZATION_OK));
	fCancelButton = new BButton("cancel", B_TRANSLATE("Cancel"), new BMessage(MSG_VECTORIZATION_CANCEL));
	fResetButton = new BButton("reset", B_TRANSLATE("Reset to defaults"), new BMessage(MSG_VECTORIZATION_RESET));

	fOKButton->MakeDefault(true);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(fTabView)
		.Add(fStatusView)
		.AddGroup(B_HORIZONTAL)
			.Add(fResetButton)
			.AddGlue()
			.Add(fCancelButton)
			.Add(fOKButton)
		.End()
	.End();
}

void
SVGVectorizationDialog::_BuildBasicTab()
{
	BGroupView* basicGroup = new BGroupView(B_VERTICAL, B_USE_DEFAULT_SPACING);

	fColorsSlider = _CreateSlider("colors", B_TRANSLATE("Number of colors"),
								2, 128, fOptions.fNumberOfColors);
	fLineThresholdSlider = _CreateSlider("line_threshold", B_TRANSLATE("Line threshold"),
								0.1f, 10.0f, fOptions.fLineThreshold);
	fQuadraticThresholdSlider = _CreateSlider("quad_threshold", B_TRANSLATE("Curve threshold"),
								0.1f, 10.0f, fOptions.fQuadraticThreshold);
	fPathOmitSlider = _CreateSlider("path_omit", B_TRANSLATE("Path omit threshold"),
								0, 250, fOptions.fPathOmitThreshold);

	BLayoutBuilder::Group<>(basicGroup, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(fColorsSlider)
		.Add(fLineThresholdSlider)
		.Add(fQuadraticThresholdSlider)
		.Add(fPathOmitSlider)
		.AddGlue()
	.End();

	BTab* basicTab = new BTab();
	fTabView->AddTab(basicGroup, basicTab);
	basicTab->SetLabel(B_TRANSLATE("Basic"));
}

void
SVGVectorizationDialog::_BuildPreprocessingTab()
{
	BGroupView* preprocessGroup = new BGroupView(B_VERTICAL, B_USE_DEFAULT_SPACING);

	fRemoveBackgroundCheck = _CreateCheckBox("remove_bg", B_TRANSLATE("Remove background"),
											fOptions.fRemoveBackground);

	const char* bgMethods[] = {
		B_TRANSLATE("Edge analysis"),
		B_TRANSLATE("Flood fill"), 
		B_TRANSLATE("Dominant color"),
		B_TRANSLATE("Clustering"),
		B_TRANSLATE("Combined"),
		NULL
	};
	fBackgroundMethodMenu = _CreateMenuField("bg_method", B_TRANSLATE("Background method"),
											bgMethods, fOptions.fBackgroundMethod);

	fBackgroundToleranceSlider = _CreateSlider("bg_tolerance", B_TRANSLATE("Background tolerance"),
									1, 50, fOptions.fBackgroundTolerance);
	fBlurRadiusSlider = _CreateSlider("blur_radius", B_TRANSLATE("Blur radius"),
									0.0f, 10.0f, fOptions.fBlurRadius);
	fBlurDeltaSlider = _CreateSlider("blur_delta", B_TRANSLATE("Blur delta"),
									0, 1024, fOptions.fBlurDelta);

	BLayoutBuilder::Group<>(preprocessGroup, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(fRemoveBackgroundCheck)
		.Add(fBackgroundMethodMenu)
		.Add(fBackgroundToleranceSlider)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.Add(fBlurRadiusSlider)
		.Add(fBlurDeltaSlider)
		.AddGlue()
	.End();

	BTab* preprocessTab = new BTab();
	fTabView->AddTab(preprocessGroup, preprocessTab);
	preprocessTab->SetLabel(B_TRANSLATE("Processing"));
}

void
SVGVectorizationDialog::_BuildSimplificationTab()
{
	BGroupView* simplifyGroup = new BGroupView(B_VERTICAL, B_USE_DEFAULT_SPACING);

	fDouglasPeuckerCheck = _CreateCheckBox("douglas_peucker", B_TRANSLATE("Douglas-Peucker simplification"),
										fOptions.fDouglasPeuckerEnabled);
	fDouglasPeuckerToleranceSlider = _CreateSlider("douglas_tolerance", B_TRANSLATE("Simplification tolerance"),
										0.1f, 15.0f, fOptions.fDouglasPeuckerTolerance);
	fFilterSmallObjectsCheck = _CreateCheckBox("filter_small", B_TRANSLATE("Filter small objects"),
										fOptions.fFilterSmallObjects);
	fMinObjectAreaSlider = _CreateSlider("min_area", B_TRANSLATE("Minimum object area"),
										1, 250, fOptions.fMinObjectArea);

	fAggressiveSimplificationCheck = _CreateCheckBox("aggressive_simplify", B_TRANSLATE("Aggressive simplification"),
										fOptions.fAggressiveSimplification);

	BLayoutBuilder::Group<>(simplifyGroup, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(fDouglasPeuckerCheck)
		.Add(fDouglasPeuckerToleranceSlider)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.Add(fFilterSmallObjectsCheck)
		.Add(fMinObjectAreaSlider)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.Add(fAggressiveSimplificationCheck)
		.AddGlue()
	.End();

	BTab* simplifyTab = new BTab();
	fTabView->AddTab(simplifyGroup, simplifyTab);
	simplifyTab->SetLabel(B_TRANSLATE("Simplification"));
}

void
SVGVectorizationDialog::_BuildGeometryTab()
{
	BGroupView* geometryGroup = new BGroupView(B_VERTICAL, B_USE_DEFAULT_SPACING);

	fDetectGeometryCheck = _CreateCheckBox("detect_geometry", B_TRANSLATE("Detect geometric shapes"),
										fOptions.fDetectGeometry);
	fLineToleranceSlider = _CreateSlider("line_tolerance", B_TRANSLATE("Line detection tolerance"),
										0.1f, 20.0f, fOptions.fLineTolerance);
	fCircleToleranceSlider = _CreateSlider("circle_tolerance", B_TRANSLATE("Circle detection tolerance"),
										0.1f, 20.0f, fOptions.fCircleTolerance);
	fMinCircleRadiusSlider = _CreateSlider("min_circle_radius", B_TRANSLATE("Minimum circle radius"),
										1.0f, 100.0f, fOptions.fMinCircleRadius);
	fMaxCircleRadiusSlider = _CreateSlider("max_circle_radius", B_TRANSLATE("Maximum circle radius"),
										10.0f, 1000.0f, fOptions.fMaxCircleRadius);

	BLayoutBuilder::Group<>(geometryGroup, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(fDetectGeometryCheck)
		.Add(fLineToleranceSlider)
		.Add(fCircleToleranceSlider)
		.Add(fMinCircleRadiusSlider)
		.Add(fMaxCircleRadiusSlider)
		.AddGlue()
	.End();

	BTab* geometryTab = new BTab();
	fTabView->AddTab(geometryGroup, geometryTab);
	geometryTab->SetLabel(B_TRANSLATE("Geometry"));
}

void
SVGVectorizationDialog::_BuildOutputTab()
{
	BGroupView* outputGroup = new BGroupView(B_VERTICAL, B_USE_DEFAULT_SPACING);

	fScaleSlider = _CreateSlider("scale", B_TRANSLATE("Output scale"),
								0.1f, 10.0f, fOptions.fScale);
	fOptimizeSvgCheck = _CreateCheckBox("optimize_svg", B_TRANSLATE("Optimize SVG output"),
								fOptions.fOptimizeSvg);
	fRemoveDuplicatesCheck = _CreateCheckBox("remove_duplicates", B_TRANSLATE("Remove duplicate paths"),
								fOptions.fRemoveDuplicates);

	BLayoutBuilder::Group<>(outputGroup, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(fScaleSlider)
		.Add(fOptimizeSvgCheck)
		.Add(fRemoveDuplicatesCheck)
		.AddGlue()
	.End();

	BTab* outputTab = new BTab();
	fTabView->AddTab(outputGroup, outputTab);
	outputTab->SetLabel(B_TRANSLATE("Output"));
}

BSlider*
SVGVectorizationDialog::_CreateSlider(const char* name, const char* label, 
								  float min, float max, float value)
{
	BSlider* slider = new BSlider(name, label, 
								new BMessage(MSG_VECTORIZATION_SETTINGS_CHANGED),
								(int32)(min * 100), (int32)(max * 100), B_HORIZONTAL);
	slider->SetValue((int32)(value * 100));
	slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	slider->SetHashMarkCount(5);
	return slider;
}

BCheckBox*
SVGVectorizationDialog::_CreateCheckBox(const char* name, const char* label, bool value)
{
	BCheckBox* checkBox = new BCheckBox(name, label, new BMessage(MSG_VECTORIZATION_SETTINGS_CHANGED));
	checkBox->SetValue(value ? B_CONTROL_ON : B_CONTROL_OFF);
	return checkBox;
}

BMenuField*
SVGVectorizationDialog::_CreateMenuField(const char* name, const char* label,
										const char* items[], int32 selected)
{
	BPopUpMenu* menu = new BPopUpMenu(name);

	for (int32 i = 0; items[i] != NULL; i++) {
		BMenuItem* item = new BMenuItem(items[i], new BMessage(MSG_VECTORIZATION_SETTINGS_CHANGED));
		menu->AddItem(item);
		if (i == selected)
			item->SetMarked(true);
	}

	BMenuField* menuField = new BMenuField(name, label, menu);
	return menuField;
}

void
SVGVectorizationDialog::_UpdateFromControls()
{
	fOptions.fNumberOfColors = fColorsSlider->Value() / 100.0f;
	fOptions.fLineThreshold = fLineThresholdSlider->Value() / 100.0f;
	fOptions.fQuadraticThreshold = fQuadraticThresholdSlider->Value() / 100.0f;
	fOptions.fPathOmitThreshold = fPathOmitSlider->Value() / 100.0f;

	fOptions.fRemoveBackground = (fRemoveBackgroundCheck->Value() == B_CONTROL_ON);

	BMenuItem* bgItem = fBackgroundMethodMenu->Menu()->FindMarked();
	if (bgItem) {
		int32 index = fBackgroundMethodMenu->Menu()->IndexOf(bgItem);
		fOptions.fBackgroundMethod = (BackgroundDetectionMethod)index;
	}

	fOptions.fBackgroundTolerance = fBackgroundToleranceSlider->Value();
	fOptions.fBlurRadius = fBlurRadiusSlider->Value() / 100.0f;
	fOptions.fBlurDelta = fBlurDeltaSlider->Value() / 100.0f;

	fOptions.fDouglasPeuckerEnabled = (fDouglasPeuckerCheck->Value() == B_CONTROL_ON);
	fOptions.fDouglasPeuckerTolerance = fDouglasPeuckerToleranceSlider->Value() / 100.0f;

	fOptions.fFilterSmallObjects = (fFilterSmallObjectsCheck->Value() == B_CONTROL_ON);
	fOptions.fMinObjectArea = fMinObjectAreaSlider->Value() / 100.0f;

	fOptions.fAggressiveSimplification = (fAggressiveSimplificationCheck->Value() == B_CONTROL_ON);

	fOptions.fDetectGeometry = (fDetectGeometryCheck->Value() == B_CONTROL_ON);
	fOptions.fLineTolerance = fLineToleranceSlider->Value() / 100.0f;
	fOptions.fCircleTolerance = fCircleToleranceSlider->Value() / 100.0f;
	fOptions.fMinCircleRadius = fMinCircleRadiusSlider->Value() / 100.0f;
	fOptions.fMaxCircleRadius = fMaxCircleRadiusSlider->Value() / 100.0f;

	fOptions.fScale = fScaleSlider->Value() / 100.0f;
	fOptions.fOptimizeSvg = (fOptimizeSvgCheck->Value() == B_CONTROL_ON);
	fOptions.fRemoveDuplicates = (fRemoveDuplicatesCheck->Value() == B_CONTROL_ON);
}

void
SVGVectorizationDialog::_UpdateControls()
{
	fColorsSlider->SetValue((int32)(fOptions.fNumberOfColors * 100));
	fLineThresholdSlider->SetValue((int32)(fOptions.fLineThreshold * 100));
	fQuadraticThresholdSlider->SetValue((int32)(fOptions.fQuadraticThreshold * 100));
	fPathOmitSlider->SetValue((int32)(fOptions.fPathOmitThreshold * 100));

	fRemoveBackgroundCheck->SetValue(fOptions.fRemoveBackground ? B_CONTROL_ON : B_CONTROL_OFF);

	BMenuItem* bgItem = fBackgroundMethodMenu->Menu()->ItemAt(fOptions.fBackgroundMethod);
	if (bgItem)
		bgItem->SetMarked(true);

	fBackgroundToleranceSlider->SetValue(fOptions.fBackgroundTolerance);
	fBlurRadiusSlider->SetValue((int32)(fOptions.fBlurRadius * 100));
	fBlurDeltaSlider->SetValue((int32)(fOptions.fBlurDelta * 100));

	fDouglasPeuckerCheck->SetValue(fOptions.fDouglasPeuckerEnabled ? B_CONTROL_ON : B_CONTROL_OFF);
	fDouglasPeuckerToleranceSlider->SetValue((int32)(fOptions.fDouglasPeuckerTolerance * 100));

	fFilterSmallObjectsCheck->SetValue(fOptions.fFilterSmallObjects ? B_CONTROL_ON : B_CONTROL_OFF);
	fMinObjectAreaSlider->SetValue((int32)(fOptions.fMinObjectArea * 100));

	fAggressiveSimplificationCheck->SetValue(fOptions.fAggressiveSimplification ? B_CONTROL_ON : B_CONTROL_OFF);

	fDetectGeometryCheck->SetValue(fOptions.fDetectGeometry ? B_CONTROL_ON : B_CONTROL_OFF);
	fLineToleranceSlider->SetValue((int32)(fOptions.fLineTolerance * 100));
	fCircleToleranceSlider->SetValue((int32)(fOptions.fCircleTolerance * 100));
	fMinCircleRadiusSlider->SetValue((int32)(fOptions.fMinCircleRadius * 100));
	fMaxCircleRadiusSlider->SetValue((int32)(fOptions.fMaxCircleRadius * 100));

	fScaleSlider->SetValue((int32)(fOptions.fScale * 100));
	fOptimizeSvgCheck->SetValue(fOptions.fOptimizeSvg ? B_CONTROL_ON : B_CONTROL_OFF);
	fRemoveDuplicatesCheck->SetValue(fOptions.fRemoveDuplicates ? B_CONTROL_ON : B_CONTROL_OFF);
}

void
SVGVectorizationDialog::_ResetToDefaults()
{
	fOptions.SetDefaults();
	_UpdateControls();
	_StartVectorization();
}

void
SVGVectorizationDialog::_StartVectorization()
{
	if (fTarget) {
		BMessage msg(MSG_VECTORIZATION_PREVIEW);
		msg.AddString("image_path", fImagePath);
		msg.AddData("options", B_RAW_TYPE, &fOptions, sizeof(TracingOptions));
		fTarget->Looper()->PostMessage(&msg, fTarget);
	}

	fStatusView->SetText(B_TRANSLATE("Vectorizing..."));
}

void
SVGVectorizationDialog::SetStatusText(const char* text)
{
	if (fStatusView) {
		fStatusView->SetText(text);
	}
}
