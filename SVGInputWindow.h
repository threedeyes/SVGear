/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SVG_INPUT_WINDOW_H
#define SVG_INPUT_WINDOW_H

#include <Messenger.h>
#include <Mime.h>
#include <String.h>
#include <View.h>
#include <Window.h>
#include <Button.h>
#include <StringList.h>
#include <TextControl.h>
#include <StringView.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <ControlLook.h>
#include <GraphicsDefs.h>
#include <Spinner.h>
#include <CheckBox.h>
#include <TabView.h>
#include <Slider.h>
#include <List.h>
#include <InterfaceDefs.h>
#include <ScrollView.h>

#include <iostream>
#include <sstream>
#include <vector>
#include <climits>
#include <cfloat>

#define MSG_INPUT_OK				'InOK'
#define MSG_INPUT_CANCEL			'InCl'
#define MSG_INPUT_RESET				'InRs'
#define MSG_INPUT_VALUE_UPDATED		'InVU'
#define MSG_HELPER_VALUE_UPDATED	'HlVU'

#define BUTTON_OK		0x01
#define BUTTON_CANCEL	0x02
#define BUTTON_RESET	0x04
#define BUTTON_CLOSE	0x08

#define SLIDER_SCALE_FACTOR			100
#define DEFAULT_HASH_MARK_DIVISOR	10
#define LOCK_TIMEOUT				1000

enum FieldType {
	TEXT_FIELD,
	INTEGER_FIELD,
	FLOAT_FIELD,
	SLIDER_FIELD,
	GROUP_FIELD,
	CHECKBOX_FIELD,
	MENU_FIELD,
	TAB_FIELD
};

struct FieldInfo {
	FieldType type;
	BString name;
	BString label;
	BString defaultValue;
	BView* control;
	BView* helper;
	float minValue;
	float maxValue;
	rgb_color backgroundColor;
	bool hasCustomBackgroundColor;
	bool editable;
	int32 groupCount;
	BStringList* menuItems;
	int32 defaultSelection;
	bool defaultChecked;
	BString tabName;
	
	FieldInfo() {
		type = TEXT_FIELD;
		control = NULL;
		helper = NULL;
		minValue = 0.0f;
		maxValue = 0.0f;
		backgroundColor.red = 255;
		backgroundColor.green = 255;
		backgroundColor.blue = 255;
		backgroundColor.alpha = 255;
		hasCustomBackgroundColor = false;
		editable = true;
		groupCount = 0;
		menuItems = NULL;
		defaultSelection = 0;
		defaultChecked = false;
		tabName = "";
	}
};

static bool
IsFloat(const char* text)
{
	if (!text) return false;
	std::string str = text;
	std::istringstream iss(str);
	float f;
	iss >> std::noskipws >> f;
	return iss.eof() && !iss.fail();
}

class SVGInputWindow : public BWindow {
	public:
		SVGInputWindow(const char* title, BWindow* target, uint32 messageId,
				uint32 buttons = BUTTON_OK | BUTTON_CANCEL | BUTTON_RESET);
		virtual ~SVGInputWindow();

		virtual void MessageReceived(BMessage* message);
		virtual	bool QuitRequested();
		virtual void Show();

		void AddTextField(const char* name, const char* label, const char* defaultValue = "");
		void AddIntegerField(const char* name, const char* label, int defaultValue = 0,
				int minValue = INT_MIN, int maxValue = INT_MAX);
		void AddFloatField(const char* name, const char* label, float defaultValue = 0.0f,
				float minValue = -FLT_MAX, float maxValue = FLT_MAX);
		void AddSliderField(const char* name, const char* label, float defaultValue = 0.0f,
				float minValue = 0.0f, float maxValue = 100.0f);
		void AddCheckBoxField(const char* name, const char* label, bool defaultChecked = false);
		void AddMenuField(const char* name, const char* label, BStringList* items, int32 defaultSelection = 0);
		void AddGroup(const char* name, const char* label, int32 count);

		void AddTabView(const char* name);
		void AddTab(const char* name, const char* label);
		void SetActiveTab(const char* tabName);

		void SetTextFieldValue(const char* name, const char* value);
		void SetIntegerFieldValue(const char* name, int value);
		void SetFloatFieldValue(const char* name, float value);
		void SetSliderFieldValue(const char* name, float value);
		void SetCheckBoxFieldValue(const char* name, bool checked);
		void SetMenuFieldValue(const char* name, int32 selection);

		void SetFieldBackgroundColor(const char* name, rgb_color color);
		void SetFieldEditable(const char*name, bool editable);

	private:
		FieldInfo* FindField(const char* name);
		void CreateLayout();
		void CreateTabbedLayout();
		void CreateSimpleLayout();
		void ApplyBackgroundColor(BView* control, rgb_color color);
		void ApplyFieldEditable(BView* control, bool editable);
		BMessage* MakeMessage(uint32 what, uint32 extended = 0);
		bool IsValid();

		BWindow* fParentWindow;
		BMessenger fTargetMessenger;
		BButton* fOkButton;
		uint32 fMessageId;
		uint32 fButtons;
		std::vector<FieldInfo> fFields;

		BTabView* fCurrentTabView;
		BTab* fCurrentTab;
		BView* fCurrentTabContentView;
		BString fCurrentTabName;
		std::vector<BString> fTabNames;
};

#endif
