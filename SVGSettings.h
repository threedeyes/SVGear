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

// Settings keys
extern const char* const kWindowFrame;
extern const char* const kSourceViewCollapsed;
extern const char* const kMainViewWeight;
extern const char* const kSourceViewWeight;
extern const char* const kDisplayMode;
extern const char* const kShowTransparency;
extern const char* const kBoundingBoxStyle;
extern const char* const kWordWrap;
extern const char* const kLastOpenPath;
extern const char* const kLastSavePath;
extern const char* const kLastExportPath;
extern const char* const kTabSelection;

class SVGSettings {
public:
	SVGSettings();
	~SVGSettings();

	status_t Load();
	status_t Save();

	// Universal getters
	bool GetBool(const char* name, bool defaultValue = false) const;
	int32 GetInt32(const char* name, int32 defaultValue = 0) const;
	float GetFloat(const char* name, float defaultValue = 0.0f) const;
	BString GetString(const char* name, const char* defaultValue = "") const;
	BRect GetRect(const char* name, const BRect& defaultValue = BRect()) const;

	// Universal setters
	void SetBool(const char* name, bool value);
	void SetInt32(const char* name, int32 value);
	void SetFloat(const char* name, float value);
	void SetString(const char* name, const char* value);
	void SetRect(const char* name, const BRect& value);

	// Utility methods
	void ResetToDefaults();
	status_t GetSettingsPath(BPath& path);

private:
	status_t _CreateSettingsDirectory();
	void _InitializeDefaults();

private:
	BMessage* fSettings;
};

// Global settings instance
extern SVGSettings* gSettings;

// Utility functions
status_t InitializeSettings();
void CleanupSettings();

#endif
