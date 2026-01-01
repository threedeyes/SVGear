/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <Alert.h>
#include <Font.h>
#include <SeparatorView.h>
#include <Catalog.h>

#include <cstdlib>
#include <cstdio>

#include "IconSelectionDialog.h"
#include "HvifStoreClient.h"
#include "IconGridView.h"
#include "IconInfoView.h"
#include "TagsFlowView.h"
#include "HvifStoreDefs.h"

#undef  B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT          "HVIFStoreDialog"

IconSelectionDialog::IconSelectionDialog(BMessenger target)
	:
	BWindow(BRect(0, 0, 100, 100), B_TRANSLATE("Select Icon from HVIF Store"),
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fTarget(target),
	fPage(1),
	fLoading(false),
	fSearchRunner(NULL)
{
	float width, height;
	_CalculateWindowSize(&width, &height);

	ResizeTo(width, height);

	float minWidth = width * 0.75f;
	float minHeight = height * 0.75f;
	float maxWidth = width * 1.8f;
	float maxHeight = height * 2.0f;

	SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);

	fClient = new HvifStoreClient(BMessenger(this));
	_InitGUI();

	CenterOnScreen();

	fClient->FetchCategories();
	_Search();
}


IconSelectionDialog::~IconSelectionDialog()
{
	delete fSearchRunner;

	if (fClient->Lock())
		fClient->Quit();
}


bool
IconSelectionDialog::QuitRequested()
{
	if (fTarget.IsValid()) {
		BMessage msg(kMsgDialogClosed);
		fTarget.SendMessage(&msg);
	}
	return true;
}


void
IconSelectionDialog::_CalculateWindowSize(float* width, float* height)
{
	float fontSize = be_plain_font->Size();
	float scale = fontSize / kBaseFontSize;

	if (scale < 1.0f)
		scale = 1.0f;

	*width = kBaseWindowWidth * scale;
	*height = kBaseWindowHeight * scale;
}


void
IconSelectionDialog::_InitGUI()
{
	fSearchEntry = new BTextControl("search", B_TRANSLATE("Search:"), "", new BMessage(kMsgSearch));
	fSearchEntry->SetModificationMessage(new BMessage(kMsgSearch));

	fTagsView = new TagsFlowView();

	fGrid = new IconGridView();

	fGridScroll = new BScrollView("gridScroll", fGrid, 0, B_SUPPORTS_LAYOUT, false, true);
	fGridScroll->SetExplicitMinSize(BSize(300, 200));

	fInfoView = new IconInfoView();
	fGrid->SetInfoView(fInfoView);

	BButton* cancelBtn = new BButton("cancel", B_TRANSLATE("Cancel"), new BMessage(kMsgCancel));

	fOpenBtn = new BButton("open", B_TRANSLATE("Open"), new BMessage(kMsgOpenIcon));
	fOpenBtn->SetEnabled(false);
	fOpenBtn->MakeDefault(true);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fSearchEntry)
		.Add(fTagsView)
		.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.Add(fGridScroll, 3.0f)
			.Add(fInfoView, 0.0f)
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(cancelBtn)
			.Add(fOpenBtn)
		.End()
	.End();
}


void
IconSelectionDialog::_SetLoading(bool loading)
{
	fLoading = loading;
	fGrid->SetLoading(loading);
}


void
IconSelectionDialog::_ScheduleSearch()
{
	delete fSearchRunner;
	fSearchRunner = NULL;

	BMessage msg(kMsgSearchDelayed);
	fSearchRunner = new BMessageRunner(BMessenger(this), &msg, kSearchDebounceDelay, 1);
}


void
IconSelectionDialog::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgSearch: {
			BString currentQuery = fSearchEntry->Text();
			if (currentQuery != fLastSearchQuery) {
				fLastSearchQuery = currentQuery;
				_ScheduleSearch();
			}
			break;
		}

		case kMsgSearchDelayed:
			_Search(true);
			break;

		case kMsgLoadMore:
			if (!fLoading) {
				fPage++;
				_Search(false);
			}
			break;

		case kMsgTagToggled: {
			fTagsView->GetSelectedTags(fCurrentTags);
			_Search(true);
			break;
		}

		case kMsgCategoriesLoaded: {
			BMessage json;
			if (message->FindMessage("json", &json) == B_OK)
				_ParseCategories(&json);
			break;
		}

		case kMsgIconsLoaded: {
			BMessage json;
			if (message->FindMessage("json", &json) == B_OK)
				_ParseIcons(&json);
			break;
		}

		case kMsgIconPreviewReady: {
			BBitmap* bmp = NULL;
			int32 id = 0;
			int32 generation = 0;
			if (message->FindPointer("bitmap", (void**)&bmp) == B_OK &&
				message->FindInt32("id", &id) == B_OK &&
				message->FindInt32("generation", &generation) == B_OK) {
				fGrid->SetIcon(id, bmp, generation);
			}
			break;
		}

		case kMsgSelectIcon:
			fOpenBtn->SetEnabled(fGrid->SelectedItem() != NULL);
			break;

		case kMsgOpenIcon:
			_OpenSelectedIcon();
			break;

		case kMsgCancel:
			PostMessage(B_QUIT_REQUESTED);
			break;

		case kMsgIconDataReady: {
			_SetLoading(false);
			fOpenBtn->SetEnabled(true);

			if (fTarget.IsValid()) {
				fTarget.SendMessage(message);
			}
			PostMessage(B_QUIT_REQUESTED);
			break;
		}

		case kMsgNetworkError: {
			BString error;
			message->FindString("error", &error);
			if (error.IsEmpty())
				error = B_TRANSLATE("Network error occurred");

			BAlert* alert = new BAlert(B_TRANSLATE("Error"), error, B_TRANSLATE("OK"),
				NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->Go();

			_SetLoading(false);
			fOpenBtn->SetEnabled(fGrid->SelectedItem() != NULL);
			break;
		}

		default:
			BWindow::MessageReceived(message);
	}
}


void
IconSelectionDialog::_Search(bool clear)
{
	_SetLoading(true);

	if (clear) {
		fPage = 1;
		fClient->CancelAllRequests();
		fGrid->Clear();
	}

	fClient->Search(fSearchEntry->Text(), fCurrentTags, fPage, kDefaultPageLimit);
}


void
IconSelectionDialog::_ParseCategories(BMessage* data)
{
	if (data == NULL)
		return;

	fTagsView->ClearTags();

	char indexStr[32];
	int32 i = 0;

	while (true) {
		snprintf(indexStr, sizeof(indexStr), "%" B_PRId32, i);
		BMessage item;

		if (data->FindMessage(indexStr, &item) != B_OK)
			break;

		BString catName = item.GetString("name", "");
		if (!catName.IsEmpty()) {
			fTagsView->AddTag(catName, new BMessage(kMsgTagToggled));
		}
		i++;
	}
}


void
IconSelectionDialog::_ParseIcons(BMessage* data)
{
	_SetLoading(false);

	if (data == NULL)
		return;

	BMessage dataField;
	int32 count = 0;
	type_code type;

	int32 addedCount = 0;

	if (data->GetInfo("data", &type, &count) == B_OK) {
		if (count > 1) {
			for (int32 i = 0; i < count; i++) {
				BMessage item;
				if (data->FindMessage("data", i, &item) == B_OK) {
					_AddIconFromMessage(&item);
					addedCount++;
				}
			}
		} else if (data->FindMessage("data", &dataField) == B_OK) {
			char indexStr[32];
			int32 i = 0;

			while (true) {
				snprintf(indexStr, sizeof(indexStr), "%" B_PRId32, i);
				BMessage item;
				if (dataField.FindMessage(indexStr, &item) != B_OK)
					break;
				_AddIconFromMessage(&item);
				addedCount++;
				i++;
			}
		}
	}

	fGrid->SetHasMore(addedCount == kDefaultPageLimit);
}


void
IconSelectionDialog::_AddIconFromMessage(BMessage* item)
{
	IconItem* icon = new IconItem;

	if (item->HasInt32("id"))
		icon->id = item->GetInt32("id", 0);
	else {
		BString idStr = item->GetString("id", "0");
		icon->id = atoi(idStr.String());
	}

	icon->title = item->GetString("title", B_TRANSLATE("Untitled"));
	icon->author = item->GetString("author", "");
	icon->license = item->GetString("license_name", "");
	icon->mimeType = item->GetString("mime_type", "");

	icon->hvifUrl = item->GetString("hvif_path", "");
	icon->svgUrl = item->GetString("svg_path", "");
	icon->iomUrl = item->GetString("iom_path", "");

	icon->hvifSize = (int32)item->GetDouble("hvif_size", 0.0);
	icon->svgSize = (int32)item->GetDouble("svg_size", 0.0);
	icon->iomSize = (int32)item->GetDouble("iom_size", 0.0);

	BMessage tagsMsg;
	if (item->FindMessage("tags", &tagsMsg) == B_OK) {
		BString tagsStr;
		int32 i = 0;
		char indexStr[32];

		while (true) {
			snprintf(indexStr, sizeof(indexStr), "%" B_PRId32, i);
			BString tag;
			if (tagsMsg.FindString(indexStr, &tag) != B_OK)
				break;

			if (!tagsStr.IsEmpty())
				tagsStr << ", ";
			tagsStr << tag;
			i++;
		}
		icon->tags = tagsStr;
	}

	icon->bitmap = NULL;

	fGrid->AddItem(icon);

	if (!icon->hvifUrl.IsEmpty()) {
		fClient->FetchPreview(icon->id, icon->hvifUrl, 
			fGrid->CurrentGeneration(), fGrid->IconSize());
	}
}


void
IconSelectionDialog::_OpenSelectedIcon()
{
	IconItem* item = fGrid->SelectedItem();
	if (item == NULL)
		return;

	if (item->hvifUrl.IsEmpty() && item->svgUrl.IsEmpty() && 
		item->iomUrl.IsEmpty()) {
		BAlert* alert = new BAlert(B_TRANSLATE("Error"),
			B_TRANSLATE("No icon data available for this icon."),
			B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go();
		return;
	}

	_SetLoading(true);
	fOpenBtn->SetEnabled(false);

	fClient->DownloadIconData(
		item->id,
		item->title,
		item->author,
		item->license,
		item->mimeType,
		item->tags,
		item->hvifUrl,
		item->svgUrl,
		item->iomUrl
	);
}
