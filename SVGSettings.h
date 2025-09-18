/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SVG_SETTINGS_H
#define SVG_SETTINGS_H

#include <SupportDefs.h>
#include <Rect.h>
#include <String.h>
#include <Message.h>

class BMessage;
class BPath;

extern const char* const kWindowFrame;
extern const char* const kSourceViewCollapsed;
extern const char* const kMainViewWeight;
extern const char* const kSourceViewWeight;
extern const char* const kDisplayMode;
extern const char* const kShowTransparency;
extern const char* const kShowBoundingBox;
extern const char* const kShowStatView;
extern const char* const kShowStructureView;
extern const char* const kShowSourceView;
extern const char* const kBoundingBoxStyle;
extern const char* const kWordWrap;
extern const char* const kLastOpenPath;
extern const char* const kLastSavePath;
extern const char* const kLastExportPath;
extern const char* const kVectorizationCustomLineThreshold;
extern const char* const kVectorizationCustomQuadraticThreshold;
extern const char* const kVectorizationCustomPathOmitThreshold;
extern const char* const kVectorizationCustomNumberOfColors;
extern const char* const kVectorizationCustomColorQuantizationCycles;
extern const char* const kVectorizationCustomRemoveBackground;
extern const char* const kVectorizationCustomBackgroundMethod;
extern const char* const kVectorizationCustomBackgroundTolerance;
extern const char* const kVectorizationCustomMinBackgroundRatio;
extern const char* const kVectorizationCustomBlurRadius;
extern const char* const kVectorizationCustomBlurDelta;
extern const char* const kVectorizationCustomDouglasPeuckerEnabled;
extern const char* const kVectorizationCustomDouglasPeuckerTolerance;
extern const char* const kVectorizationCustomDouglasPeuckerCurveProtection;
extern const char* const kVectorizationCustomAggressiveSimplification;
extern const char* const kVectorizationCustomCollinearTolerance;
extern const char* const kVectorizationCustomMinSegmentLength;
extern const char* const kVectorizationCustomCurveSmoothing;
extern const char* const kVectorizationCustomDetectGeometry;
extern const char* const kVectorizationCustomLineTolerance;
extern const char* const kVectorizationCustomCircleTolerance;
extern const char* const kVectorizationCustomMinCircleRadius;
extern const char* const kVectorizationCustomMaxCircleRadius;
extern const char* const kVectorizationCustomFilterSmallObjects;
extern const char* const kVectorizationCustomMinObjectArea;
extern const char* const kVectorizationCustomMinObjectWidth;
extern const char* const kVectorizationCustomMinObjectHeight;
extern const char* const kVectorizationCustomMinObjectPerimeter;
extern const char* const kVectorizationCustomScale;
extern const char* const kVectorizationCustomRoundCoordinates;
extern const char* const kVectorizationCustomShowDescription;
extern const char* const kVectorizationCustomUseViewBox;
extern const char* const kVectorizationCustomOptimizeSvg;
extern const char* const kVectorizationCustomRemoveDuplicates;
extern const char* const kVectorizationSelectedPreset;

class SVGSettings {
public:
	SVGSettings();
	~SVGSettings();

	status_t Load();
	status_t Save();

	bool GetBool(const char* name, bool defaultValue = false) const;
	int32 GetInt32(const char* name, int32 defaultValue = 0) const;
	float GetFloat(const char* name, float defaultValue = 0.0f) const;
	BString GetString(const char* name, const char* defaultValue = "") const;
	BRect GetRect(const char* name, const BRect& defaultValue = BRect()) const;

	void SetBool(const char* name, bool value);
	void SetInt32(const char* name, int32 value);
	void SetFloat(const char* name, float value);
	void SetString(const char* name, const char* value);
	void SetRect(const char* name, const BRect& value);

	void ResetToDefaults();
	status_t GetSettingsPath(BPath& path);

private:
	status_t _CreateSettingsDirectory();
	void _InitializeDefaults();

private:
	BMessage* fSettings;
};

extern SVGSettings* gSettings;

status_t InitializeSettings();
void CleanupSettings();

#endif
