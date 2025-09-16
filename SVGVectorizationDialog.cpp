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
#include <ControlLook.h>

#include "SVGVectorizationDialog.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SVGVectorizationDialog"

SVGVectorizationDialog::SVGVectorizationDialog(const char* imagePath, BWindow* target)
	: BWindow(BRect(100, 100, 600, 500), B_TRANSLATE("Vectorization Settings"),
			B_FLOATING_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL,
			B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_CLOSABLE),
	fTarget(target),
	fImagePath(imagePath),
	fFirstShow(true)
{
	fOptions.SetDefaults();
	_BuildInterface();
	_UpdateControls();
	_ApplyPreset();

	SetSizeLimits(320, 32768, 240, 32768);

	if (fTarget != NULL)
		AddToSubset(fTarget);
	else
		SetFeel(B_FLOATING_APP_WINDOW_FEEL);
}

SVGVectorizationDialog::~SVGVectorizationDialog()
{
}

void
SVGVectorizationDialog::Show()
{
	InvalidateLayout();
	Layout(true);

	float padding = be_control_look->DefaultItemSpacing();

	float tabsWidth = 0;
	if (fTabView->CountTabs() > 0) {
		int32 lastTabIndex = fTabView->CountTabs() - 1;
		BRect tabFrame = fTabView->TabFrame(lastTabIndex);
		tabsWidth = tabFrame.right + padding * 2;
	}

	BSize preferredSize = GetLayout()->PreferredSize();
	ResizeTo(MAX(preferredSize.width, tabsWidth), preferredSize.height);

	if (fTarget) {
		BView* parentView = fTarget->ChildAt(0);
		BRect viewRect = parentView == NULL ? fTarget->Frame() : parentView->ConvertToScreen(fTarget->Bounds());
		viewRect.InsetBy(20, 20);
		BWindow::Show();
		MoveTo(viewRect.right - Frame().Width(), viewRect.bottom - Frame().Height());
	} else {
		BWindow::Show();
		CenterOnScreen();
	}

	if (fFirstShow) {
		fFirstShow = false;
		_StartVectorization();
	}
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
			fTarget->Looper()->PostMessage(new BMessage(MSG_VECTORIZATION_CANCEL), fTarget);
			PostMessage(B_QUIT_REQUESTED);
			break;
		case MSG_VECTORIZATION_RESET:
			_ResetToDefaults();
			break;
		case MSG_VECTORIZATION_PRESET:
			_ApplyPreset();
			break;
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
	fTabView = new BTabView("settings_tabs", B_WIDTH_FROM_LABEL);

	_BuildBasicTab();
	_BuildColorsTab();
	_BuildPreprocessingTab();
	_BuildSimplificationTab();
	_BuildGeometryTab();
	_BuildFilteringTab();
	_BuildOutputTab();

	const char* presets[] = {
		B_TRANSLATE("Default"),
		B_TRANSLATE("Fast"),
		B_TRANSLATE("Quality"),
		B_TRANSLATE("Simple"),
		B_TRANSLATE("Detailed"),
		NULL
	};
	fPresetMenu = _CreateMenuField("preset", B_TRANSLATE("Preset"), presets, 0);

	fOKButton = new BButton("ok", B_TRANSLATE("OK"), new BMessage(MSG_VECTORIZATION_OK));
	fCancelButton = new BButton("cancel", B_TRANSLATE("Cancel"), new BMessage(MSG_VECTORIZATION_CANCEL));
	fResetButton = new BButton("reset", B_TRANSLATE("Reset to defaults"), new BMessage(MSG_VECTORIZATION_RESET));

	fOKButton->MakeDefault(true);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL)
			.Add(fPresetMenu)
			.AddGlue()
		.End()
		.Add(fTabView)
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

	fLineThresholdSlider = _CreateSlider("line_threshold", B_TRANSLATE("Line threshold"),
								0.1f, 10.0f, fOptions.fLineThreshold);
	fQuadraticThresholdSlider = _CreateSlider("quad_threshold", B_TRANSLATE("Curve threshold"),
								0.1f, 10.0f, fOptions.fQuadraticThreshold);
	fPathOmitSlider = _CreateSlider("path_omit", B_TRANSLATE("Path omit threshold"),
								0, 250, fOptions.fPathOmitThreshold);

	BLayoutBuilder::Group<>(basicGroup, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
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
SVGVectorizationDialog::_BuildColorsTab()
{
	BGroupView* colorsGroup = new BGroupView(B_VERTICAL, B_USE_DEFAULT_SPACING);

	fColorsSlider = _CreateSlider("colors", B_TRANSLATE("Number of colors"),
								2, 128, fOptions.fNumberOfColors);
	fColorQuantizationCyclesSlider = _CreateSlider("color_cycles", B_TRANSLATE("Quantization cycles"),
								1, 50, fOptions.fColorQuantizationCycles);

	BLayoutBuilder::Group<>(colorsGroup, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fColorsSlider)
		.Add(fColorQuantizationCyclesSlider)
		.AddGlue()
	.End();

	BTab* colorsTab = new BTab();
	fTabView->AddTab(colorsGroup, colorsTab);
	colorsTab->SetLabel(B_TRANSLATE("Colors"));
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
	fMinBackgroundRatioSlider = _CreateSlider("min_bg_ratio", B_TRANSLATE("Min background ratio"),
									0.0f, 1.0f, fOptions.fMinBackgroundRatio);
	fBlurRadiusSlider = _CreateSlider("blur_radius", B_TRANSLATE("Blur radius"),
									0.0f, 10.0f, fOptions.fBlurRadius);
	fBlurDeltaSlider = _CreateSlider("blur_delta", B_TRANSLATE("Blur delta"),
									0, 1024, fOptions.fBlurDelta);

	BLayoutBuilder::Group<>(preprocessGroup, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fRemoveBackgroundCheck)
		.Add(fBackgroundMethodMenu)
		.Add(fBackgroundToleranceSlider)
		.Add(fMinBackgroundRatioSlider)
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
	fDouglasPeuckerCurveProtectionSlider = _CreateSlider("curve_protection", B_TRANSLATE("Curve protection"),
										0.0f, 2.0f, fOptions.fDouglasPeuckerCurveProtection);

	fAggressiveSimplificationCheck = _CreateCheckBox("aggressive_simplify", B_TRANSLATE("Aggressive simplification"),
										fOptions.fAggressiveSimplification);
	fCollinearToleranceSlider = _CreateSlider("collinear_tolerance", B_TRANSLATE("Collinear tolerance"),
										0.1f, 10.0f, fOptions.fCollinearTolerance);
	fMinSegmentLengthSlider = _CreateSlider("min_segment_length", B_TRANSLATE("Min segment length"),
										0.1f, 10.0f, fOptions.fMinSegmentLength);
	fCurveSmoothingSlider = _CreateSlider("curve_smoothing", B_TRANSLATE("Curve smoothing"),
										0.0f, 2.0f, fOptions.fCurveSmoothing);

	BLayoutBuilder::Group<>(simplifyGroup, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fDouglasPeuckerCheck)
		.Add(fDouglasPeuckerToleranceSlider)
		.Add(fDouglasPeuckerCurveProtectionSlider)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.Add(fAggressiveSimplificationCheck)
		.Add(fCollinearToleranceSlider)
		.Add(fMinSegmentLengthSlider)
		.Add(fCurveSmoothingSlider)
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
		.SetInsets(B_USE_DEFAULT_SPACING)
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
SVGVectorizationDialog::_BuildFilteringTab()
{
	BGroupView* filteringGroup = new BGroupView(B_VERTICAL, B_USE_DEFAULT_SPACING);

	fFilterSmallObjectsCheck = _CreateCheckBox("filter_small", B_TRANSLATE("Filter small objects"),
										fOptions.fFilterSmallObjects);
	fMinObjectAreaSlider = _CreateSlider("min_area", B_TRANSLATE("Minimum object area"),
										1, 250, fOptions.fMinObjectArea);
	fMinObjectWidthSlider = _CreateSlider("min_width", B_TRANSLATE("Minimum object width"),
										1.0f, 100.0f, fOptions.fMinObjectWidth);
	fMinObjectHeightSlider = _CreateSlider("min_height", B_TRANSLATE("Minimum object height"),
										1.0f, 100.0f, fOptions.fMinObjectHeight);
	fMinObjectPerimeterSlider = _CreateSlider("min_perimeter", B_TRANSLATE("Minimum object perimeter"),
										1.0f, 500.0f, fOptions.fMinObjectPerimeter);

	BLayoutBuilder::Group<>(filteringGroup, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fFilterSmallObjectsCheck)
		.Add(fMinObjectAreaSlider)
		.Add(fMinObjectWidthSlider)
		.Add(fMinObjectHeightSlider)
		.Add(fMinObjectPerimeterSlider)
		.AddGlue()
	.End();

	BTab* filteringTab = new BTab();
	fTabView->AddTab(filteringGroup, filteringTab);
	filteringTab->SetLabel(B_TRANSLATE("Filtering"));
}

void
SVGVectorizationDialog::_BuildOutputTab()
{
	BGroupView* outputGroup = new BGroupView(B_VERTICAL, B_USE_DEFAULT_SPACING);

	fScaleSlider = _CreateSlider("scale", B_TRANSLATE("Output scale"),
								0.1f, 10.0f, fOptions.fScale);
	fRoundCoordinatesSlider = _CreateSlider("round_coords", B_TRANSLATE("Round coordinates"),
								0, 5, fOptions.fRoundCoordinates);
	fShowDescriptionCheck = _CreateCheckBox("show_description", B_TRANSLATE("Show description"),
								fOptions.fShowDescription);
	fUseViewBoxCheck = _CreateCheckBox("use_viewbox", B_TRANSLATE("Use ViewBox"),
								fOptions.fUseViewBox);
	fOptimizeSvgCheck = _CreateCheckBox("optimize_svg", B_TRANSLATE("Optimize SVG output"),
								fOptions.fOptimizeSvg);
	fRemoveDuplicatesCheck = _CreateCheckBox("remove_duplicates", B_TRANSLATE("Remove duplicate paths"),
								fOptions.fRemoveDuplicates);

	BLayoutBuilder::Group<>(outputGroup, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fScaleSlider)
		.Add(fRoundCoordinatesSlider)
		.AddStrut(B_USE_DEFAULT_SPACING)
		.Add(fShowDescriptionCheck)
		.Add(fUseViewBoxCheck)
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
		BMessage* msg = (strcmp(name, "preset") == 0) ? new BMessage(MSG_VECTORIZATION_PRESET) :
														new BMessage(MSG_VECTORIZATION_SETTINGS_CHANGED);
		BMenuItem* item = new BMenuItem(items[i], msg);
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
	fOptions.fLineThreshold = fLineThresholdSlider->Value() / 100.0f;
	fOptions.fQuadraticThreshold = fQuadraticThresholdSlider->Value() / 100.0f;
	fOptions.fPathOmitThreshold = fPathOmitSlider->Value() / 100.0f;

	fOptions.fNumberOfColors = fColorsSlider->Value() / 100.0f;
	fOptions.fColorQuantizationCycles = fColorQuantizationCyclesSlider->Value() / 100.0f;

	fOptions.fRemoveBackground = (fRemoveBackgroundCheck->Value() == B_CONTROL_ON);

	BMenuItem* bgItem = fBackgroundMethodMenu->Menu()->FindMarked();
	if (bgItem) {
		int32 index = fBackgroundMethodMenu->Menu()->IndexOf(bgItem);
		fOptions.fBackgroundMethod = (BackgroundDetectionMethod)index;
	}

	fOptions.fBackgroundTolerance = fBackgroundToleranceSlider->Value();
	fOptions.fMinBackgroundRatio = fMinBackgroundRatioSlider->Value() / 100.0f;
	fOptions.fBlurRadius = fBlurRadiusSlider->Value() / 100.0f;
	fOptions.fBlurDelta = fBlurDeltaSlider->Value() / 100.0f;

	fOptions.fDouglasPeuckerEnabled = (fDouglasPeuckerCheck->Value() == B_CONTROL_ON);
	fOptions.fDouglasPeuckerTolerance = fDouglasPeuckerToleranceSlider->Value() / 100.0f;
	fOptions.fDouglasPeuckerCurveProtection = fDouglasPeuckerCurveProtectionSlider->Value() / 100.0f;

	fOptions.fAggressiveSimplification = (fAggressiveSimplificationCheck->Value() == B_CONTROL_ON);
	fOptions.fCollinearTolerance = fCollinearToleranceSlider->Value() / 100.0f;
	fOptions.fMinSegmentLength = fMinSegmentLengthSlider->Value() / 100.0f;
	fOptions.fCurveSmoothing = fCurveSmoothingSlider->Value() / 100.0f;

	fOptions.fDetectGeometry = (fDetectGeometryCheck->Value() == B_CONTROL_ON);
	fOptions.fLineTolerance = fLineToleranceSlider->Value() / 100.0f;
	fOptions.fCircleTolerance = fCircleToleranceSlider->Value() / 100.0f;
	fOptions.fMinCircleRadius = fMinCircleRadiusSlider->Value() / 100.0f;
	fOptions.fMaxCircleRadius = fMaxCircleRadiusSlider->Value() / 100.0f;

	fOptions.fFilterSmallObjects = (fFilterSmallObjectsCheck->Value() == B_CONTROL_ON);
	fOptions.fMinObjectArea = fMinObjectAreaSlider->Value() / 100.0f;
	fOptions.fMinObjectWidth = fMinObjectWidthSlider->Value() / 100.0f;
	fOptions.fMinObjectHeight = fMinObjectHeightSlider->Value() / 100.0f;
	fOptions.fMinObjectPerimeter = fMinObjectPerimeterSlider->Value() / 100.0f;

	fOptions.fScale = fScaleSlider->Value() / 100.0f;
	fOptions.fRoundCoordinates = fRoundCoordinatesSlider->Value() / 100.0f;
	fOptions.fLineControlPointRadius = 0;
	fOptions.fQuadraticControlPointRadius = 0;
	fOptions.fShowDescription = (fShowDescriptionCheck->Value() == B_CONTROL_ON);
	fOptions.fUseViewBox = (fUseViewBoxCheck->Value() == B_CONTROL_ON);
	fOptions.fOptimizeSvg = (fOptimizeSvgCheck->Value() == B_CONTROL_ON);
	fOptions.fRemoveDuplicates = (fRemoveDuplicatesCheck->Value() == B_CONTROL_ON);
}

void
SVGVectorizationDialog::_UpdateControls()
{
	fLineThresholdSlider->SetValue((int32)(fOptions.fLineThreshold * 100));
	fQuadraticThresholdSlider->SetValue((int32)(fOptions.fQuadraticThreshold * 100));
	fPathOmitSlider->SetValue((int32)(fOptions.fPathOmitThreshold * 100));

	fColorsSlider->SetValue((int32)(fOptions.fNumberOfColors * 100));
	fColorQuantizationCyclesSlider->SetValue((int32)(fOptions.fColorQuantizationCycles * 100));

	fRemoveBackgroundCheck->SetValue(fOptions.fRemoveBackground ? B_CONTROL_ON : B_CONTROL_OFF);

	BMenuItem* bgItem = fBackgroundMethodMenu->Menu()->ItemAt(fOptions.fBackgroundMethod);
	if (bgItem)
		bgItem->SetMarked(true);

	fBackgroundToleranceSlider->SetValue(fOptions.fBackgroundTolerance);
	fMinBackgroundRatioSlider->SetValue((int32)(fOptions.fMinBackgroundRatio * 100));
	fBlurRadiusSlider->SetValue((int32)(fOptions.fBlurRadius * 100));
	fBlurDeltaSlider->SetValue((int32)(fOptions.fBlurDelta * 100));

	fDouglasPeuckerCheck->SetValue(fOptions.fDouglasPeuckerEnabled ? B_CONTROL_ON : B_CONTROL_OFF);
	fDouglasPeuckerToleranceSlider->SetValue((int32)(fOptions.fDouglasPeuckerTolerance * 100));
	fDouglasPeuckerCurveProtectionSlider->SetValue((int32)(fOptions.fDouglasPeuckerCurveProtection * 100));

	fAggressiveSimplificationCheck->SetValue(fOptions.fAggressiveSimplification ? B_CONTROL_ON : B_CONTROL_OFF);
	fCollinearToleranceSlider->SetValue((int32)(fOptions.fCollinearTolerance * 100));
	fMinSegmentLengthSlider->SetValue((int32)(fOptions.fMinSegmentLength * 100));
	fCurveSmoothingSlider->SetValue((int32)(fOptions.fCurveSmoothing * 100));

	fDetectGeometryCheck->SetValue(fOptions.fDetectGeometry ? B_CONTROL_ON : B_CONTROL_OFF);
	fLineToleranceSlider->SetValue((int32)(fOptions.fLineTolerance * 100));
	fCircleToleranceSlider->SetValue((int32)(fOptions.fCircleTolerance * 100));
	fMinCircleRadiusSlider->SetValue((int32)(fOptions.fMinCircleRadius * 100));
	fMaxCircleRadiusSlider->SetValue((int32)(fOptions.fMaxCircleRadius * 100));

	fFilterSmallObjectsCheck->SetValue(fOptions.fFilterSmallObjects ? B_CONTROL_ON : B_CONTROL_OFF);
	fMinObjectAreaSlider->SetValue((int32)(fOptions.fMinObjectArea * 100));
	fMinObjectWidthSlider->SetValue((int32)(fOptions.fMinObjectWidth * 100));
	fMinObjectHeightSlider->SetValue((int32)(fOptions.fMinObjectHeight * 100));
	fMinObjectPerimeterSlider->SetValue((int32)(fOptions.fMinObjectPerimeter * 100));

	fScaleSlider->SetValue((int32)(fOptions.fScale * 100));
	fRoundCoordinatesSlider->SetValue((int32)(fOptions.fRoundCoordinates * 100));
	fShowDescriptionCheck->SetValue(fOptions.fShowDescription ? B_CONTROL_ON : B_CONTROL_OFF);
	fUseViewBoxCheck->SetValue(fOptions.fUseViewBox ? B_CONTROL_ON : B_CONTROL_OFF);
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
SVGVectorizationDialog::_ApplyPreset()
{
	BMenuItem* item = fPresetMenu->Menu()->FindMarked();
	if (!item)
		return;

	int32 index = fPresetMenu->Menu()->IndexOf(item);
	switch (index) {
		case 0: // Default
			fOptions.SetDefaults();
			fOptions.fFilterSmallObjects = true;
			fOptions.fMinObjectArea = 10.0f;
			fOptions.fLineThreshold = 2.0f;
			fOptions.fQuadraticThreshold = 0.5f;
			fOptions.fNumberOfColors = 8;
			fOptions.fColorQuantizationCycles = 16.0f;
			fOptions.fAggressiveSimplification = true;
			fOptions.fDouglasPeuckerEnabled = true;
			fOptions.fDouglasPeuckerTolerance = 0.5f;
			break;
		case 1: // Fast
			fOptions.SetDefaults();
			fOptions.fNumberOfColors = 16;
			fOptions.fLineThreshold = 2.0f;
			fOptions.fQuadraticThreshold = 2.0f;
			fOptions.fDouglasPeuckerEnabled = true;
			fOptions.fDouglasPeuckerTolerance = 2.0f;
			fOptions.fFilterSmallObjects = true;
			fOptions.fMinObjectArea = 10.0f;
			fOptions.fAggressiveSimplification = true;
			break;
		case 2: // Quality
			fOptions.SetDefaults();
			fOptions.fNumberOfColors = 64;
			fOptions.fLineThreshold = 0.5f;
			fOptions.fQuadraticThreshold = 0.5f;
			fOptions.fColorQuantizationCycles = 20.0f;
			fOptions.fDouglasPeuckerEnabled = true;
			fOptions.fDouglasPeuckerTolerance = 0.5f;
			fOptions.fDetectGeometry = true;
			fOptions.fOptimizeSvg = true;
			break;
		case 3: // Simple
			fOptions.SetDefaults();
			fOptions.fNumberOfColors = 8;
			fOptions.fLineThreshold = 3.0f;
			fOptions.fQuadraticThreshold = 3.0f;
			fOptions.fAggressiveSimplification = true;
			fOptions.fCollinearTolerance = 2.0f;
			fOptions.fFilterSmallObjects = true;
			fOptions.fMinObjectArea = 25.0f;
			break;
		case 4: // Detailed
			fOptions.SetDefaults();
			fOptions.fNumberOfColors = 128;
			fOptions.fLineThreshold = 0.2f;
			fOptions.fQuadraticThreshold = 0.2f;
			fOptions.fColorQuantizationCycles = 5.0f;
			fOptions.fDouglasPeuckerTolerance = 0.2f;
			fOptions.fDetectGeometry = true;
			fOptions.fLineTolerance = 0.5f;
			fOptions.fCircleTolerance = 0.5f;
			break;
	}

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
}
