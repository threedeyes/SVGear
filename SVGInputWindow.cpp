/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "SVGInputWindow.h"

#undef  B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT          "SVGInputWindow"

SVGInputWindow::SVGInputWindow(const char* title, BWindow* target, uint32 messageId, uint32 buttons)
	: BWindow(BRect(0, 0, 600, 100), title, B_FLOATING_WINDOW_LOOK, B_FLOATING_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fParentWindow(target),
	fTargetMessenger(target),
	fMessageId(messageId),
	fButtons(buttons),
	fOkButton(NULL),
	fCurrentTabView(NULL),
	fCurrentTab(NULL),
	fCurrentTabContentView(NULL),
	fCurrentTabName("")
{
	fOkButton = new BButton("OK", new BMessage(MSG_INPUT_OK));
	fOkButton->SetEnabled(false);

	fTabNames.clear();

	SetSizeLimits(500, 32768, 100, 32768);

	if (fParentWindow != NULL)
		AddToSubset(fParentWindow);
	else
		SetFeel(B_FLOATING_APP_WINDOW_FEEL);
}

SVGInputWindow::~SVGInputWindow()
{
	for (int32 i = 0; i < (int32)fFields.size(); i++) {
		if (fFields[i].menuItems != NULL) {
			delete fFields[i].menuItems;
		}
	}
}

void
SVGInputWindow::AddTextField(const char* name, const char* label, const char* defaultValue)
{
	FieldInfo field;
	field.type = TEXT_FIELD;
	field.name = name;
	field.label = label;
	field.defaultValue = defaultValue;
	field.tabName = fCurrentTabName;

	BTextControl* control = new BTextControl("", defaultValue, NULL);
	control->SetModificationMessage(new BMessage(MSG_INPUT_VALUE_UPDATED));
	control->SetExplicitMinSize(BSize(200, B_SIZE_UNSET));
	control->SetExplicitPreferredSize(BSize(250, B_SIZE_UNSET));
	field.control = control;
	fFields.push_back(field);
}

void
SVGInputWindow::AddIntegerField(const char* name, const char* label, int defaultValue, int minValue, int maxValue)
{
	BString defaultValueStr;
	defaultValueStr << defaultValue;

	FieldInfo field;
	field.type = INTEGER_FIELD;
	field.name = name;
	field.label = label;
	field.defaultValue = defaultValueStr;
	field.minValue = (float)minValue;
	field.maxValue = (float)maxValue;
	field.tabName = fCurrentTabName;

	BSpinner* spinner = new BSpinner(name, "", new BMessage(MSG_INPUT_VALUE_UPDATED));
	spinner->SetRange(minValue, maxValue);
	spinner->SetValue(defaultValue);
	spinner->SetExplicitMinSize(BSize(100, B_SIZE_UNSET));
	spinner->SetExplicitPreferredSize(BSize(120, B_SIZE_UNSET));
	field.control = spinner;
	fFields.push_back(field);
}

void
SVGInputWindow::AddFloatField(const char* name, const char* label, float defaultValue, float minValue, float maxValue)
{
	BString defaultValueStr;
	defaultValueStr.SetToFormat("%.2f", defaultValue);

	FieldInfo field;
	field.type = FLOAT_FIELD;
	field.name = name;
	field.label = label;
	field.defaultValue = defaultValueStr;
	field.minValue = minValue;
	field.maxValue = maxValue;
	field.tabName = fCurrentTabName;

	BTextControl* control = new BTextControl("", defaultValueStr, NULL);
	control->SetModificationMessage(new BMessage(MSG_INPUT_VALUE_UPDATED));
	control->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	control->SetExplicitMinSize(BSize(120, B_SIZE_UNSET));
	control->SetExplicitPreferredSize(BSize(150, B_SIZE_UNSET));
	field.control = control;
	fFields.push_back(field);
}

void
SVGInputWindow::AddSliderField(const char* name, const char* label, float defaultValue, float minValue, float maxValue)
{
	BString defaultValueStr;
	defaultValueStr << defaultValue;
	BString minValueStr;
	minValueStr.SetToFormat("%g", minValue);
	BString maxValueStr;
	maxValueStr.SetToFormat("%g", maxValue);

	FieldInfo field;
	field.type = SLIDER_FIELD;
	field.name = name;
	field.label = label;
	field.defaultValue = defaultValueStr;
	field.minValue = minValue;
	field.maxValue = maxValue;
	field.tabName = fCurrentTabName;

	BSlider* slider = new BSlider(name, "", new BMessage(MSG_INPUT_VALUE_UPDATED), 
		minValue * SLIDER_SCALE_FACTOR, maxValue * SLIDER_SCALE_FACTOR,
		B_HORIZONTAL, B_TRIANGLE_THUMB);
	slider->SetModificationMessage(new BMessage(MSG_INPUT_VALUE_UPDATED));
	slider->SetLimitLabels(minValueStr.String(), maxValueStr.String());
	slider->SetHashMarks(B_HASH_MARKS_TOP);
	slider->SetHashMarkCount((maxValue - minValue) / DEFAULT_HASH_MARK_DIVISOR);
	slider->SetValue(defaultValue * SLIDER_SCALE_FACTOR);
	slider->SetExplicitMinSize(BSize(150, B_SIZE_UNSET));
	slider->SetExplicitPreferredSize(BSize(200, B_SIZE_UNSET));
	field.control = slider;

	BTextControl* control = new BTextControl("", defaultValueStr, NULL);
	control->SetModificationMessage(new BMessage(MSG_HELPER_VALUE_UPDATED));
	control->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	control->SetExplicitMinSize(BSize(80, B_SIZE_UNSET));
	control->SetExplicitPreferredSize(BSize(100, B_SIZE_UNSET));
	field.helper = control;

	fFields.push_back(field);
}

void
SVGInputWindow::AddCheckBoxField(const char* name, const char* label, bool defaultChecked)
{
	FieldInfo field;
	field.type = CHECKBOX_FIELD;
	field.name = name;
	field.label = label;
	field.defaultChecked = defaultChecked;
	field.defaultValue = defaultChecked ? "true" : "false";
	field.tabName = fCurrentTabName;

	BCheckBox* checkBox = new BCheckBox(name, label, new BMessage(MSG_INPUT_VALUE_UPDATED));
	checkBox->SetValue(defaultChecked ? B_CONTROL_ON : B_CONTROL_OFF);
	field.control = checkBox;
	fFields.push_back(field);
}

void
SVGInputWindow::AddMenuField(const char* name, const char* label, BStringList* items, int32 defaultSelection)
{
	if (!items || items->CountStrings() == 0) return;

	FieldInfo field;
	field.type = MENU_FIELD;
	field.name = name;
	field.label = label;
	field.defaultSelection = defaultSelection;
	field.menuItems = new BStringList(*items);
	field.defaultValue = items->StringAt(defaultSelection);
	field.tabName = fCurrentTabName;

	BPopUpMenu* popUpMenu = new BPopUpMenu("menu");
	for (int32 i = 0; i < items->CountStrings(); i++) {
		BMenuItem* item = new BMenuItem(items->StringAt(i), new BMessage(MSG_INPUT_VALUE_UPDATED));
		popUpMenu->AddItem(item);
		if (i == defaultSelection) {
			item->SetMarked(true);
		}
	}

	BMenuField* menuField = new BMenuField(name, label, popUpMenu);
	menuField->SetExplicitMinSize(BSize(150, B_SIZE_UNSET));
	menuField->SetExplicitPreferredSize(BSize(200, B_SIZE_UNSET));
	field.control = menuField;
	fFields.push_back(field);
}

void
SVGInputWindow::AddGroup(const char* name, const char* label, int32 count)
{
	FieldInfo field;
	field.type = GROUP_FIELD;
	field.name = name;
	field.label = label;
	field.groupCount = count;
	field.tabName = fCurrentTabName;

	BGroupView* group = new BGroupView(name, B_HORIZONTAL, 1);
	field.control = group;
	fFields.push_back(field);
}

void
SVGInputWindow::AddTabView(const char* name)
{
	FieldInfo field;
	field.type = TAB_FIELD;
	field.name = name;
	field.label = name;

	fCurrentTabView = new BTabView(name, B_WIDTH_FROM_LABEL);
	field.control = fCurrentTabView;
	fFields.push_back(field);
}

void
SVGInputWindow::AddTab(const char* name, const char* label)
{
	if (!fCurrentTabView)
		AddTabView("defaultTabView");

	fCurrentTabContentView = new BView(name, B_WILL_DRAW);
	fCurrentTabContentView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fCurrentTab = new BTab(fCurrentTabContentView);
	fCurrentTab->SetLabel(label);

	fCurrentTabView->AddTab(fCurrentTabContentView, fCurrentTab);
	fCurrentTabName = name;

	fTabNames.push_back(name);
}

void
SVGInputWindow::SetActiveTab(const char* tabName)
{
	if (fCurrentTabView) {
		for (int32 i = 0; i < fCurrentTabView->CountTabs(); i++) {
			BTab* tab = fCurrentTabView->TabAt(i);
			if (tab && tab->View() && BString(tab->View()->Name()) == tabName) {
				fCurrentTabContentView = tab->View();
				fCurrentTabName = tabName;
				break;
			}
		}
	}
}

void
SVGInputWindow::CreateLayout()
{
	bool hasTabView = false;

	for (int32 i = 0; i < (int32)fFields.size(); i++) {
		if (fFields[i].type == TAB_FIELD) {
			hasTabView = true;
			break;
		}
	}

	if (hasTabView)
		CreateTabbedLayout();
	else
		CreateSimpleLayout();
}

void
SVGInputWindow::CreateTabbedLayout()
{
	float padding = be_control_look->DefaultItemSpacing();
	BTabView* tabView = NULL;

	for (int32 i = 0; i < (int32)fFields.size(); i++) {
		if (fFields[i].type == TAB_FIELD) {
			tabView = (BTabView*)fFields[i].control;
			break;
		}
	}

	if (!tabView)
		return;

	int32 maxFieldsInTab = 0;	
	for (int32 tabIndex = 0; tabIndex < tabView->CountTabs(); tabIndex++) {
		BTab* tab = tabView->TabAt(tabIndex);
		if (!tab || !tab->View()) continue;

		BView* tabContent = tab->View();
		BString tabName = "";
		if (tabIndex < (int32)fTabNames.size())
			tabName = fTabNames[tabIndex];

		if (tabName.IsEmpty())
			continue;

		int32 fieldCount = 0;
		for (int32 i = 0; i < (int32)fFields.size(); i++) {
			if (fFields[i].type != TAB_FIELD && fFields[i].tabName == tabName) {
				fieldCount++;
			}
		}

		if (fieldCount > maxFieldsInTab)
			maxFieldsInTab = fieldCount;

		if (fieldCount == 0) {
			BLayoutBuilder::Group<>(tabContent, B_VERTICAL, padding)
				.SetInsets(padding)
				.AddGlue()
			.End();
			continue;
		}

		BLayoutBuilder::Group<> groupBuilder(tabContent, B_VERTICAL, padding);
		groupBuilder.SetInsets(padding);

		for (int32 i = 0; i < (int32)fFields.size(); i++) {
			FieldInfo field = fFields[i];

			if (field.type == TAB_FIELD) continue;
			if (field.tabName != tabName) continue;

			if (field.type == CHECKBOX_FIELD) {
				groupBuilder.Add(field.control);
			} else {
				BStringView* labelView = new BStringView("label", field.label);
				labelView->SetExplicitMinSize(BSize(120, B_SIZE_UNSET));
				labelView->SetAlignment(B_ALIGN_LEFT);

				if (field.helper) {
					groupBuilder.AddGroup(B_HORIZONTAL, padding)
						.Add(labelView)
						.Add(field.control, 2.0f)
						.Add(field.helper, 0.5f)
					.End();
				} else {
					groupBuilder.AddGroup(B_HORIZONTAL, padding)
						.Add(labelView)
						.Add(field.control, 1.0f)
					.End();
				}
			}

			ApplyFieldEditable(field.control, field.editable);
			if (field.hasCustomBackgroundColor) {
				ApplyBackgroundColor(field.control, field.backgroundColor);
				if (field.helper)
					ApplyBackgroundColor(field.helper, field.backgroundColor);
			}
		}
		
		if (fieldCount < 8)
			groupBuilder.AddGlue();

		groupBuilder.End();
	}

	float tabsWidth = 0;
	if (tabView->CountTabs() > 0) {
		int32 lastTabIndex = tabView->CountTabs() - 1;
		BRect tabFrame = tabView->TabFrame(lastTabIndex);
		tabsWidth = tabFrame.right + padding * 2;
	}

	float fieldHeight = 35.0f;
	float tabHeaderHeight = 30.0f;
	float buttonsHeight = 40.0f;
	float paddingTotal = padding * 6;

	float calculatedHeight = (maxFieldsInTab * fieldHeight) + tabHeaderHeight + buttonsHeight + paddingTotal;
	float minWidth = MAX(tabsWidth + 50, 500.0f);
	float minHeight = MAX(calculatedHeight, 200.0f);

	tabView->SetExplicitMinSize(BSize(minWidth, B_SIZE_UNSET));

	BLayoutBuilder::Group<>(this, B_VERTICAL, padding)
		.SetInsets(padding)
		.Add(tabView)
		.AddGroup(B_HORIZONTAL, padding)
			.AddGlue()
			.Add(new BButton("Reset", new BMessage(MSG_INPUT_RESET)))
			.Add(new BButton("Cancel", new BMessage(MSG_INPUT_CANCEL)))
			.Add(fOkButton)
		.End()
	.End();

	SetSizeLimits(minWidth + padding * 4, 32768, minHeight + 60, 32768);

	fOkButton->MakeDefault(true);
	SetDefaultButton(fOkButton);
}

void
SVGInputWindow::CreateSimpleLayout()
{
	float padding = be_control_look->DefaultItemSpacing();

	BLayoutBuilder::Grid<> layoutBuilder(this, padding, padding);
	layoutBuilder.SetInsets(padding, padding, padding, padding);
	layoutBuilder.SetColumnWeight(0, 0.0f);
	layoutBuilder.SetColumnWeight(1, 0.0f);
	layoutBuilder.SetColumnWeight(2, 0.0f);
	layoutBuilder.SetColumnWeight(3, 0.0f);
	layoutBuilder.SetColumnWeight(4, 1.0f);
	layoutBuilder.SetColumnWeight(5, 0.0f);

	int32 row = 0;
	for (int32 i = 0; i < (int32)fFields.size(); i++) {
		FieldInfo field = fFields[i];

		if (field.type == GROUP_FIELD) {
			BStringView* labelView = new BStringView("label", field.label);
			labelView->SetExplicitMinSize(BSize(100, B_SIZE_UNSET));
			layoutBuilder.Add(labelView, 0, row);
			BGroupView *group = (BGroupView*)field.control;
			layoutBuilder.Add(group, 4, row, 2);
			int32 groupSize = field.groupCount;
			for (int32 j = 1; j <= groupSize; j++) {
				if (i + j < (int32)fFields.size()) {
					FieldInfo groupField = fFields[i + j];
					group->AddChild(groupField.control);
					ApplyFieldEditable(groupField.control, groupField.editable);
					if (groupField.hasCustomBackgroundColor) {
						ApplyBackgroundColor(groupField.control, groupField.backgroundColor);
						if (groupField.helper)
							ApplyBackgroundColor(groupField.helper, groupField.backgroundColor);
					}
				}
			}
			i += groupSize;
		} else if (field.type == CHECKBOX_FIELD) {
			layoutBuilder.Add(field.control, 0, row, 6);
		} else {
			BStringView* labelView = new BStringView("label", field.label);
			labelView->SetExplicitMinSize(BSize(100, B_SIZE_UNSET));
			labelView->SetExplicitPreferredSize(BSize(120, B_SIZE_UNSET));
			layoutBuilder.Add(labelView, 0, row);

			if (field.helper) {
				layoutBuilder.Add(field.helper, 3, row);
				layoutBuilder.Add(field.control, 4, row, 2);
			} else {
				layoutBuilder.Add(field.control, 4, row, 2);
			}
			ApplyFieldEditable(field.control, field.editable);
			if (field.hasCustomBackgroundColor) {
				ApplyBackgroundColor(field.control, field.backgroundColor);
				if (field.helper)
					ApplyBackgroundColor(field.helper, field.backgroundColor);
			}
		}
		row++;
	}

	BButton* cancelButton = new BButton("Cancel", new BMessage(MSG_INPUT_CANCEL));
	BButton* resetButton = new BButton("Reset", new BMessage(MSG_INPUT_RESET));
	BButton* closeButton = new BButton("Close", new BMessage(MSG_INPUT_OK));

	if (fButtons & BUTTON_RESET)
		layoutBuilder.Add(resetButton, 0, row);

	layoutBuilder.Add(BSpaceLayoutItem::CreateGlue(), 1, row, 3);

	if (fButtons & BUTTON_CANCEL)
		layoutBuilder.Add(cancelButton, 4, row);

	if (fButtons & BUTTON_OK) {
		layoutBuilder.Add(fOkButton, 5, row);
		fOkButton->MakeDefault(true);
	} else if (fButtons & BUTTON_CLOSE) {
		layoutBuilder.Add(closeButton, 5, row);
		closeButton->MakeDefault(true);
	}

	if (!fFields.empty())
		fFields[0].control->MakeFocus(true);

	SetDefaultButton(fOkButton);
}

void
SVGInputWindow::Show()
{
	CreateLayout();

	InvalidateLayout();
	Layout(true);

	BSize preferredSize = GetLayout()->PreferredSize();
	ResizeTo(preferredSize.width, preferredSize.height);

	if (fParentWindow) {
		BView* parentView = fParentWindow->FindView("MainView");
		BRect viewRect = parentView == NULL ? fParentWindow->Frame() : parentView->ConvertToScreen(parentView->Bounds());
		viewRect.InsetBy(20, 20);
		BWindow::Show();
		MoveTo(viewRect.right - Frame().Width(), viewRect.bottom - Frame().Height());
	} else {
		BWindow::Show();
		CenterOnScreen();
	}

	fTargetMessenger.SendMessage(MakeMessage(MSG_INPUT_VALUE_UPDATED));
}

bool
SVGInputWindow::QuitRequested()
{
	if (fButtons & BUTTON_CLOSE)
		fTargetMessenger.SendMessage(MakeMessage(fMessageId));
	else
		fTargetMessenger.SendMessage(MSG_INPUT_CANCEL);

	return true;
}

BMessage*
SVGInputWindow::MakeMessage(uint32 what, uint32 extended)
{
	BMessage* message = new BMessage(what);

	message->AddInt32("extended", extended);

	if (what == MSG_INPUT_VALUE_UPDATED)
		message->AddInt32("action", fMessageId);

	for (int32 i = 0; i < (int32)fFields.size(); i++) {
		FieldInfo field = fFields[i];
		switch (field.type) {
			case TEXT_FIELD:
				message->AddString(field.name, ((BTextControl*)field.control)->Text());
				break;
			case FLOAT_FIELD:
				message->AddFloat(field.name, atof(((BTextControl*)field.control)->Text()));
				break;
			case INTEGER_FIELD:
				message->AddInt32(field.name, ((BSpinner*)field.control)->Value());
				break;
			case SLIDER_FIELD:
				message->AddFloat(field.name, ((BSlider*)field.control)->Value() / SLIDER_SCALE_FACTOR);
				break;
			case CHECKBOX_FIELD:
				message->AddBool(field.name, ((BCheckBox*)field.control)->Value() == B_CONTROL_ON);
				break;
			case MENU_FIELD:
			{
				BMenuField* menuField = (BMenuField*)field.control;
				BMenuItem* marked = menuField->Menu()->FindMarked();
				if (marked) {
					message->AddString(field.name, marked->Label());
					message->AddInt32(BString(field.name).Append("_index"), menuField->Menu()->IndexOf(marked));
				}
				break;
			}
		}
	}

	return message;
}

void
SVGInputWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_INPUT_VALUE_UPDATED:
		{
			for (int32 i = 0; i < (int32)fFields.size(); i++) {
				FieldInfo field = fFields[i];
				if (field.type == SLIDER_FIELD) {
					BSlider* slider = (BSlider*)field.control;
					BTextControl* helper = (BTextControl*)field.helper;
					if (helper)
						SetSliderFieldValue(field.name, slider->Value() / SLIDER_SCALE_FACTOR);
				}
			}

			if (IsValid())
				fTargetMessenger.SendMessage(MakeMessage(MSG_INPUT_VALUE_UPDATED));

			fOkButton->SetEnabled(IsValid());
			break;
		}
		case MSG_HELPER_VALUE_UPDATED:
		{
			for (int32 i = 0; i < (int32)fFields.size(); i++) {
				FieldInfo field = fFields[i];
				if (field.type == SLIDER_FIELD) {
					BSlider* slider = (BSlider*)field.control;
					BTextControl* helper = (BTextControl*)field.helper;
					if (helper) {
						if (IsFloat(helper->Text())) {
							slider->SetValue(atof(helper->Text()) * SLIDER_SCALE_FACTOR);
						}
					}
				}
			}

			if (IsValid())
				fTargetMessenger.SendMessage(MakeMessage(MSG_INPUT_VALUE_UPDATED));

			fOkButton->SetEnabled(IsValid());
			break;
		}
		case MSG_INPUT_OK:
		{
			fTargetMessenger.SendMessage(MakeMessage(fMessageId));
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case MSG_INPUT_CANCEL:
		{
			fTargetMessenger.SendMessage(MSG_INPUT_CANCEL);
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case MSG_INPUT_RESET:
		{
			for (int32 i = 0; i < (int32)fFields.size(); i++) {
				FieldInfo field = fFields[i];
				switch (field.type) {
				case TEXT_FIELD:
				case FLOAT_FIELD:
					((BTextControl*)field.control)->SetText(field.defaultValue);
					break;
				case INTEGER_FIELD:
					((BSpinner*)field.control)->SetValue(atoi(field.defaultValue));
					break;
				case SLIDER_FIELD:
					SetSliderFieldValue(field.name, atof(field.defaultValue));
					break;
				case CHECKBOX_FIELD:
					((BCheckBox*)field.control)->SetValue(field.defaultChecked ? B_CONTROL_ON : B_CONTROL_OFF);
					break;
				case MENU_FIELD:
				{
					BMenuField* menuField = (BMenuField*)field.control;
					BMenuItem* item = menuField->Menu()->ItemAt(field.defaultSelection);
					if (item) {
						item->SetMarked(true);
					}
					break;
				}
				}
			}
			fTargetMessenger.SendMessage(MakeMessage(MSG_INPUT_VALUE_UPDATED, message->what));
			fOkButton->SetEnabled(IsValid());
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}

FieldInfo*
SVGInputWindow::FindField(const char* name)
{
	if (!name) return NULL;

	for (int32 i = 0; i < (int32)fFields.size(); i++) {
		if (fFields[i].name == name) {
			return &fFields[i];
		}
	}
	return NULL;
}

void
SVGInputWindow::SetTextFieldValue(const char* name, const char* value)
{
	if (LockWithTimeout(LOCK_TIMEOUT) == B_OK) {
		FieldInfo* field = FindField(name);
		if (field && field->type == TEXT_FIELD) {
			BTextControl* control = dynamic_cast<BTextControl*>(field->control);
			if (control)
				control->SetText(value);
		}
		UnlockLooper();
	}
}

void
SVGInputWindow::SetIntegerFieldValue(const char* name, int value)
{
	if (LockWithTimeout(LOCK_TIMEOUT) == B_OK) {
		FieldInfo* field = FindField(name);
		if (field && field->type == INTEGER_FIELD) {
			BSpinner* spinner = dynamic_cast<BSpinner*>(field->control);
			if (spinner)
				spinner->SetValue(value);
		}
		UnlockLooper();
	}
}

void
SVGInputWindow::SetFloatFieldValue(const char* name, float value)
{
	if (LockWithTimeout(LOCK_TIMEOUT) == B_OK) {
		FieldInfo* field = FindField(name);
		if (field && field->type == FLOAT_FIELD) {
			BTextControl* control = dynamic_cast<BTextControl*>(field->control);
			if (control) {
				BString text;
				text.SetToFormat("%.2f", value);
				control->SetText(text);
			}
		}
		UnlockLooper();
	}
}

void
SVGInputWindow::SetSliderFieldValue(const char* name, float value)
{
	if (LockWithTimeout(LOCK_TIMEOUT) == B_OK) {
		FieldInfo* field = FindField(name);
		if (field && field->type == SLIDER_FIELD) {
			BSlider* slider = dynamic_cast<BSlider*>(field->control);
			BTextControl* helper = dynamic_cast<BTextControl*>(field->helper);

			if (slider) {
				if (slider->Value() != value * SLIDER_SCALE_FACTOR)
					slider->SetValue(value * SLIDER_SCALE_FACTOR);
			}

			if (helper) {
				BString label;
				label.SetToFormat("%.2f", value);
				if (BString(helper->Text()) != label)
					helper->SetText(label);
			}
		}
		UnlockLooper();
	}
}

void
SVGInputWindow::SetCheckBoxFieldValue(const char* name, bool checked)
{
	if (LockWithTimeout(LOCK_TIMEOUT) == B_OK) {
		FieldInfo* field = FindField(name);
		if (field && field->type == CHECKBOX_FIELD) {
			BCheckBox* checkBox = dynamic_cast<BCheckBox*>(field->control);
			if (checkBox)
				checkBox->SetValue(checked ? B_CONTROL_ON : B_CONTROL_OFF);
		}
		UnlockLooper();
	}
}

void
SVGInputWindow::SetMenuFieldValue(const char* name, int32 selection)
{
	if (LockWithTimeout(LOCK_TIMEOUT) == B_OK) {
		FieldInfo* field = FindField(name);
		if (field && field->type == MENU_FIELD) {
			BMenuField* menuField = dynamic_cast<BMenuField*>(field->control);
			if (menuField) {
				BMenuItem* item = menuField->Menu()->ItemAt(selection);
				if (item) {
					item->SetMarked(true);
				}
			}
		}
		UnlockLooper();
	}
}

void
SVGInputWindow::SetFieldBackgroundColor(const char* name, rgb_color color)
{
	FieldInfo* field = FindField(name);
	if (field) {
		field->backgroundColor = color;
		field->hasCustomBackgroundColor = true;
		if (field->control) {
			ApplyBackgroundColor(field->control, color);
		}
		if (field->helper) {
			ApplyBackgroundColor(field->helper, color);
		}
	}
}

void
SVGInputWindow::SetFieldEditable(const char*name, bool editable)
{
	FieldInfo* field = FindField(name);
	if (field) {
		field->editable = editable;
		if (field->control) {
			ApplyFieldEditable(field->control, editable);
		}
	}
}

void
SVGInputWindow::ApplyBackgroundColor(BView* control, rgb_color color)
{
	if (!control)
		return;

	BTextControl* textControl = dynamic_cast<BTextControl*>(control);
	if (textControl) {
		textControl->TextView()->SetViewColor(color);
		textControl->TextView()->SetLowColor(color);
	} else {
		BSlider* slider = dynamic_cast<BSlider*>(control);
		if (slider) {
			slider->SetBarColor(color);
		} else {
			BSpinner* spinner = dynamic_cast<BSpinner*>(control);
			if (spinner) {
				spinner->TextView()->SetViewColor(color);
				spinner->TextView()->SetLowColor(color);
			}
		}
	}

	if (control->Parent())
		control->Parent()->Invalidate();
}

void
SVGInputWindow::ApplyFieldEditable(BView* control, bool editable)
{
	if (!control)
		return;

	BTextControl* textControl = dynamic_cast<BTextControl*>(control);
	if (textControl) {
		textControl->TextView()->MakeEditable(editable);
	} else {
		BSlider* slider = dynamic_cast<BSlider*>(control);
		if (slider) {
			slider->SetEnabled(editable);
		} else {
			BSpinner* spinner = dynamic_cast<BSpinner*>(control);
			if (spinner) {
				spinner->TextView()->MakeEditable(editable);
			} else {
				BCheckBox* checkBox = dynamic_cast<BCheckBox*>(control);
				if (checkBox) {
					checkBox->SetEnabled(editable);
				} else {
					BMenuField* menuField = dynamic_cast<BMenuField*>(control);
					if (menuField) {
						menuField->SetEnabled(editable);
					}
				}
			}
		}
	}
}

bool
SVGInputWindow::IsValid()
{
	bool allFieldsValid = true;
	for (int32 i = 0; i < (int32)fFields.size(); i++) {
		FieldInfo field = fFields[i];
		switch (field.type) {
			case FLOAT_FIELD:
			{
				BTextControl* control = dynamic_cast<BTextControl*>(field.control);
				if (control) {
					if (!IsFloat(control->Text())) {
						allFieldsValid = false;
					} else {
						float value = atof(control->Text());
						if (value < field.minValue || value > field.maxValue) {
							allFieldsValid = false;
						}
					}
				}
				break;
			}
			case TEXT_FIELD:
			case INTEGER_FIELD:
			case SLIDER_FIELD:
			case CHECKBOX_FIELD:
			case MENU_FIELD:
				break;
		}
		if (!allFieldsValid)
			break;
	}
	return allFieldsValid;
}
