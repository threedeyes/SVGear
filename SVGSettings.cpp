/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "SVGSettings.h"

#include <Message.h>
#include <File.h>
#include <Path.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <Screen.h>

#include <stdio.h>

const char* const kWindowFrame = "window_frame";
const char* const kSourceViewCollapsed = "source_view_collapsed";
const char* const kMainViewWeight = "main_view_weight";
const char* const kSourceViewWeight = "source_view_weight";
const char* const kDisplayMode = "display_mode";
const char* const kShowTransparency = "show_transparency";
const char* const kShowBoundingBox = "show_bounding_box";
const char* const kShowStatView = "show_stat_view";
const char* const kBoundingBoxStyle = "bounding_box_style";
const char* const kWordWrap = "word_wrap";
const char* const kLastOpenPath = "last_open_path";
const char* const kLastSavePath = "last_save_path";
const char* const kLastExportPath = "last_export_path";

SVGSettings* gSettings = NULL;

SVGSettings::SVGSettings()
	: fSettings(NULL)
{
	fSettings = new BMessage('sett');
	_InitializeDefaults();
}

SVGSettings::~SVGSettings()
{
	delete fSettings;
}

status_t
SVGSettings::Load()
{
	if (!fSettings)
		return B_NO_MEMORY;

	BPath path;
	status_t result = GetSettingsPath(path);
	if (result != B_OK)
		return result;

	BFile file(path.Path(), B_READ_ONLY);
	result = file.InitCheck();
	if (result != B_OK) {
		_InitializeDefaults();
		return B_OK;
	}

	BMessage loaded;
	result = loaded.Unflatten(&file);
	if (result != B_OK) {
		_InitializeDefaults();
		return B_OK;
	}

	delete fSettings;
	fSettings = new BMessage(loaded);

	return B_OK;
}

status_t
SVGSettings::Save()
{
	if (!fSettings)
		return B_NO_MEMORY;

	status_t result = _CreateSettingsDirectory();
	if (result != B_OK)
		return result;

	BPath path;
	result = GetSettingsPath(path);
	if (result != B_OK)
		return result;

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	result = file.InitCheck();
	if (result != B_OK)
		return result;

	return fSettings->Flatten(&file);
}

bool
SVGSettings::GetBool(const char* name, bool defaultValue) const
{
	if (!fSettings || !name)
		return defaultValue;

	bool value;
	if (fSettings->FindBool(name, &value) == B_OK)
		return value;

	return defaultValue;
}

int32
SVGSettings::GetInt32(const char* name, int32 defaultValue) const
{
	if (!fSettings || !name)
		return defaultValue;

	int32 value;
	if (fSettings->FindInt32(name, &value) == B_OK)
		return value;

	return defaultValue;
}

float
SVGSettings::GetFloat(const char* name, float defaultValue) const
{
	if (!fSettings || !name)
		return defaultValue;

	float value;
	if (fSettings->FindFloat(name, &value) == B_OK)
		return value;

	return defaultValue;
}

BString
SVGSettings::GetString(const char* name, const char* defaultValue) const
{
	if (!fSettings || !name)
		return BString(defaultValue ? defaultValue : "");

	BString value;
	if (fSettings->FindString(name, &value) == B_OK)
		return value;

	return BString(defaultValue ? defaultValue : "");
}

BRect
SVGSettings::GetRect(const char* name, const BRect& defaultValue) const
{
	if (!fSettings || !name)
		return defaultValue;

	BRect value;
	if (fSettings->FindRect(name, &value) == B_OK) {
		if (value.IsValid()) {
			if (strcmp(name, kWindowFrame) == 0) {
				BScreen screen;
				BRect screenFrame = screen.Frame();
				if (screenFrame.Contains(value.LeftTop())) {
					return value;
				}
			} else {
				return value;
			}
		}
	}

	return defaultValue;
}

void
SVGSettings::SetBool(const char* name, bool value)
{
	if (!fSettings || !name)
		return;

	fSettings->RemoveName(name);
	fSettings->AddBool(name, value);
}

void
SVGSettings::SetInt32(const char* name, int32 value)
{
	if (!fSettings || !name)
		return;

	fSettings->RemoveName(name);
	fSettings->AddInt32(name, value);
}

void
SVGSettings::SetFloat(const char* name, float value)
{
	if (!fSettings || !name)
		return;

	fSettings->RemoveName(name);
	fSettings->AddFloat(name, value);
}

void
SVGSettings::SetString(const char* name, const char* value)
{
	if (!fSettings || !name || !value)
		return;

	fSettings->RemoveName(name);
	fSettings->AddString(name, value);
}

void
SVGSettings::SetRect(const char* name, const BRect& value)
{
	if (!fSettings || !name || !value.IsValid())
		return;

	fSettings->RemoveName(name);
	fSettings->AddRect(name, value);
}

void
SVGSettings::ResetToDefaults()
{
	if (fSettings) {
		fSettings->MakeEmpty();
		_InitializeDefaults();
	}
}

status_t
SVGSettings::GetSettingsPath(BPath& path)
{
	status_t result = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (result != B_OK)
		return result;

	return path.Append("SVGear_settings");
}

status_t
SVGSettings::_CreateSettingsDirectory()
{
	BPath path;
	status_t result = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (result != B_OK)
		return result;

	BDirectory dir(path.Path());
	return dir.InitCheck();
}

void
SVGSettings::_InitializeDefaults()
{
	if (!fSettings)
		return;

	fSettings->AddRect(kWindowFrame, BRect(50, 50, 900, 700));

	fSettings->AddBool(kSourceViewCollapsed, true);
	fSettings->AddFloat(kMainViewWeight, 0.7f);
	fSettings->AddFloat(kSourceViewWeight, 0.3f);

	fSettings->AddInt32(kDisplayMode, 0);
	fSettings->AddBool(kShowTransparency, true);
	fSettings->AddBool(kShowBoundingBox, false);
	fSettings->AddBool(kShowStatView, false);
	fSettings->AddInt32(kBoundingBoxStyle, 1);

	fSettings->AddBool(kWordWrap, true);

	fSettings->AddString(kLastOpenPath, "");
	fSettings->AddString(kLastSavePath, "");
	fSettings->AddString(kLastExportPath, "");
}

status_t
InitializeSettings()
{
	if (gSettings)
		return B_OK;

	gSettings = new SVGSettings();
	if (!gSettings)
		return B_NO_MEMORY;

	return gSettings->Load();
}

void
CleanupSettings()
{
	if (gSettings) {
		gSettings->Save();
		delete gSettings;
		gSettings = NULL;
	}
}
