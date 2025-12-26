/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SVG_VECTORIZATION_DIALOG_H
#define SVG_VECTORIZATION_DIALOG_H

#include <Window.h>
#include <TabView.h>
#include <CheckBox.h>
#include <Slider.h>
#include <MenuField.h>
#include <Button.h>
#include <GroupView.h>
#include <Handler.h>
#include <Message.h>
#include <String.h>
#include <StringView.h>
#include <StatusBar.h>
#include <Path.h>

#include "TracingOptions.h"
#include "SVGConstants.h"
#include "VectorizationProgress.h"

class SVGVectorizationDialog : public BWindow {
public:
	SVGVectorizationDialog(const char* imagePath, BWindow* target);
	virtual ~SVGVectorizationDialog();

	virtual void Show();
	virtual void MessageReceived(BMessage* message);
	virtual bool QuitRequested();

	void SetVectorizationCompleted();
	void SetVectorizationError(const char* errorMessage = NULL);
	void ResetProgress(bigtime_t delay = 0); 

	TracingOptions GetCurrentOptions() const;
	void SetOptions(const TracingOptions& options);

	BString GetImagePath() const { return fImagePath; }

	static void ProgressCallbackStatic(int stage, int percent, void* userData);

private:
	void _BuildInterface();
	void _BuildBasicTab();
	void _BuildColorsTab();
	void _BuildPreprocessingTab();
	void _BuildSimplificationTab();
	void _BuildGeometryTab();
	void _BuildFilteringTab();
	void _BuildGradientsTab();
	void _BuildOutputTab();

	void _UpdateFromControls();
	void _UpdateControls();
	void _UpdateControlStates();
	void _ApplyPreset();
	void _StartVectorization();

	void _SaveCustomPreset();
	void _LoadCustomPreset();
	void _SwitchToCustomPreset();
	void _SaveSelectedPreset(int32 presetIndex);
	void _LoadSelectedPreset();

	void _UpdateVectorizationProgress(int stage, int percent);

	BSlider* _CreateSlider(const char* name, const char* label, float min, float max, float value);
	BCheckBox* _CreateCheckBox(const char* name, const char* label, bool value);
	BMenuField* _CreateMenuField(const char* name, const char* label, const char* items[], int32 selected);
	BView* _CreateSliderWithLabels(const char* name, const char* label, float min, float max, float value,
									BSlider** outSlider, BStringView** outValueLabel);
	void _UpdateSliderLabels();
	BString _FormatSliderValue(float value, int decimals = 2);

private:
	BTabView*        fTabView;
	BWindow*         fTarget;
	BString          fImagePath;
	BFont*           fBoldFont;
	TracingOptions   fOptions;
	BStatusBar*      fProgressBar;
	bool             fFirstShow;
	bool             fUpdatingControls;

	static const char* kSVGDescription;

	// Preset control
	BMenuField*     fPresetMenu;

	// Basic tab controls
	BSlider*        fLineThresholdSlider;
	BSlider*        fQuadraticThresholdSlider;
	BSlider*        fPathOmitSlider;

	// Colors tab controls
	BSlider*        fColorsSlider;
	BSlider*        fColorQuantizationCyclesSlider;

	// Preprocessing tab controls
	BCheckBox*      fRemoveBackgroundCheck;
	BMenuField*     fBackgroundMethodMenu;
	BSlider*        fBackgroundToleranceSlider;
	BSlider*        fMinBackgroundRatioSlider;
	BSlider*        fBlurRadiusSlider;
	BSlider*        fBlurDeltaSlider;

	// Simplification tab controls
	BCheckBox*      fVisvalingamWhyattCheck;
	BSlider*        fVisvalingamWhyattToleranceSlider;
	BCheckBox*      fDouglasPeuckerCheck;
	BSlider*        fDouglasPeuckerToleranceSlider;
	BSlider*        fDouglasPeuckerCurveProtectionSlider;
	BCheckBox*      fAggressiveSimplificationCheck;
	BSlider*        fCollinearToleranceSlider;
	BSlider*        fMinSegmentLengthSlider;
	BSlider*        fCurveSmoothingSlider;

	// Geometry tab controls
	BCheckBox*      fDetectGeometryCheck;
	BSlider*        fLineToleranceSlider;
	BSlider*        fCircleToleranceSlider;
	BSlider*        fMinCircleRadiusSlider;
	BSlider*        fMaxCircleRadiusSlider;

	// Filtering tab controls
	BCheckBox*      fFilterSmallObjectsCheck;
	BSlider*        fMinObjectAreaSlider;
	BSlider*        fMinObjectWidthSlider;
	BSlider*        fMinObjectHeightSlider;
	BSlider*        fMinObjectPerimeterSlider;

	// Gradients tab controls
	BCheckBox*      fDetectGradientsCheck;
	BSlider*        fGradientSampleStrideSlider;
	BSlider*        fGradientMinR2Slider;
	BSlider*        fGradientMinDeltaSlider;
	BSlider*        fGradientMinSizeSlider;
	BSlider*        fGradientMaxSubdivSlider;
	BSlider*        fGradientMinSamplesSlider;

	// Output tab controls
	BSlider*        fScaleSlider;
	BSlider*        fRoundCoordinatesSlider;
	BCheckBox*      fShowDescriptionCheck;
	BCheckBox*      fUseViewBoxCheck;
	BCheckBox*      fOptimizeSvgCheck;
	BCheckBox*      fRemoveDuplicatesCheck;

	// Buttons
	BButton*        fOKButton;
	BButton*        fCancelButton;

	// Labels
	BStringView*    fLineThresholdValueLabel;
	BStringView*    fQuadraticThresholdValueLabel;
	BStringView*    fPathOmitValueLabel;
	BStringView*    fColorsValueLabel;
	BStringView*    fColorQuantizationCyclesValueLabel;
	BStringView*    fBackgroundToleranceValueLabel;
	BStringView*    fMinBackgroundRatioValueLabel;
	BStringView*    fBlurRadiusValueLabel;
	BStringView*    fBlurDeltaValueLabel;
	BStringView*    fVisvalingamWhyattToleranceValueLabel;
	BStringView*    fDouglasPeuckerToleranceValueLabel;
	BStringView*    fDouglasPeuckerCurveProtectionValueLabel;
	BStringView*    fCollinearToleranceValueLabel;
	BStringView*    fMinSegmentLengthValueLabel;
	BStringView*    fCurveSmoothingValueLabel;
	BStringView*    fLineToleranceValueLabel;
	BStringView*    fCircleToleranceValueLabel;
	BStringView*    fMinCircleRadiusValueLabel;
	BStringView*    fMaxCircleRadiusValueLabel;
	BStringView*    fMinObjectAreaValueLabel;
	BStringView*    fMinObjectWidthValueLabel;
	BStringView*    fMinObjectHeightValueLabel;
	BStringView*    fMinObjectPerimeterValueLabel;
	BStringView*    fGradientSampleStrideValueLabel;
	BStringView*    fGradientMinR2ValueLabel;
	BStringView*    fGradientMinDeltaValueLabel;
	BStringView*    fGradientMinSizeValueLabel;
	BStringView*    fGradientMaxSubdivValueLabel;
	BStringView*    fGradientMinSamplesValueLabel;
	BStringView*    fScaleValueLabel;
	BStringView*    fRoundCoordinatesValueLabel;
};

#endif
