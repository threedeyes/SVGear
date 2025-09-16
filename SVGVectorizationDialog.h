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
#include <StringView.h>
#include <GroupView.h>
#include <Handler.h>
#include <Message.h>
#include <String.h>

#include "TracingOptions.h"
#include "SVGConstants.h"

class SVGVectorizationDialog : public BWindow {
public:
	SVGVectorizationDialog(const char* imagePath, BHandler* target);
	virtual ~SVGVectorizationDialog();

	virtual void MessageReceived(BMessage* message);
	virtual bool QuitRequested();

	TracingOptions GetCurrentOptions() const;
	void SetOptions(const TracingOptions& options);

	BString GetImagePath() const { return fImagePath; }
	void SetStatusText(const char* text);

private:
	void _BuildInterface();
	void _BuildBasicTab();
	void _BuildPreprocessingTab();
	void _BuildSimplificationTab();
	void _BuildGeometryTab();
	void _BuildOutputTab();

	void _UpdateFromControls();
	void _UpdateControls();
	void _ResetToDefaults();
	void _StartVectorization();

	BSlider* _CreateSlider(const char* name, const char* label, float min, float max, float value);
	BCheckBox* _CreateCheckBox(const char* name, const char* label, bool value);
	BMenuField* _CreateMenuField(const char* name, const char* label, const char* items[], int32 selected);

private:
	BTabView*       fTabView;
	BHandler*       fTarget;
	BString         fImagePath;
	TracingOptions  fOptions;

	// Basic tab controls
	BSlider*        fColorsSlider;
	BSlider*        fLineThresholdSlider;
	BSlider*        fQuadraticThresholdSlider;
	BSlider*        fPathOmitSlider;

	// Preprocessing tab controls
	BCheckBox*      fRemoveBackgroundCheck;
	BMenuField*     fBackgroundMethodMenu;
	BSlider*        fBackgroundToleranceSlider;
	BSlider*        fBlurRadiusSlider;
	BSlider*        fBlurDeltaSlider;

	// Simplification tab controls
	BCheckBox*      fDouglasPeuckerCheck;
	BSlider*        fDouglasPeuckerToleranceSlider;
	BCheckBox*      fFilterSmallObjectsCheck;
	BSlider*        fMinObjectAreaSlider;
	BCheckBox*      fAggressiveSimplificationCheck;

	// Geometry tab controls  
	BCheckBox*      fDetectGeometryCheck;
	BSlider*        fLineToleranceSlider;
	BSlider*        fCircleToleranceSlider;
	BSlider*        fMinCircleRadiusSlider;
	BSlider*        fMaxCircleRadiusSlider;

	// Output tab controls
	BSlider*        fScaleSlider;
	BCheckBox*      fOptimizeSvgCheck;
	BCheckBox*      fRemoveDuplicatesCheck;

	// Buttons
	BButton*        fOKButton;
	BButton*        fCancelButton;
	BButton*        fResetButton;

	// Status
	BStringView*    fStatusView;
};

#endif
