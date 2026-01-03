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
#include <File.h>
#include <NodeInfo.h>
#include <Entry.h>
#include <IconUtils.h>

#include <cstdlib>
#include <cstdio>

#include "IconSelectionDialog.h"
#include "HvifStoreClient.h"
#include "IconGridView.h"
#include "IconInfoView.h"
#include "TagsFlowView.h"
#include "HvifStoreDefs.h"
#include "IconsData.h"

#ifdef HVIF_STORE_CLIENT
#include "IconExportUtils.h"
#endif

#undef  B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT          "HVIFStoreDialog"

IconSelectionDialog::IconSelectionDialog(BMessenger target)
	:
	BWindow(BRect(0, 0, 100, 100),
#ifdef HVIF_STORE_CLIENT
	B_TRANSLATE("HVIF-Store Browser"),
#else
	B_TRANSLATE("Select Icon from HVIF Store"),
#endif
			B_TITLED_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
			B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fResetButtonIcon(NULL),
	fTarget(target),
	fPage(1),
	fLoading(false),
	fSearchRunner(NULL),
	fPreserveSelectionId(-1),
	fSavePanel(NULL),
	fPendingSaveFormat(kFormatNone)
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
	delete fSavePanel;
	delete fResetButtonIcon;

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
IconSelectionDialog::Show()
{
	InvalidateLayout();
	Layout(true);

	float w, h;
	fSearchEntry->GetPreferredSize(&w, &h);
	fResetButton->SetExplicitSize(BSize(h, h));

	if (fResetButtonIcon == NULL) {
		fResetButtonIcon = new BBitmap(BRect(0, 0, h - 8, h - 8), B_RGBA32);
		if (fResetButtonIcon->InitCheck() != B_OK
			|| BIconUtils::GetVectorIcon((const uint8*)kClearIconData, kClearIconDataSize, fResetButtonIcon) != B_OK) {
			delete fResetButtonIcon;
			fResetButtonIcon = NULL;
		} else {
			fResetButton->SetIcon(fResetButtonIcon);
		}
	}

	BWindow::Show();
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

	fResetButton = new BButton("reset", "", new BMessage(kMsgClearTags));

	fTagsView = new TagsFlowView();

	fGrid = new IconGridView();

	fGridScroll = new BScrollView("gridScroll", fGrid, 0, B_SUPPORTS_LAYOUT, false, true);
	fGridScroll->SetExplicitMinSize(BSize(300, 200));

	fInfoView = new IconInfoView();
	fInfoView->SetTarget(BMessenger(this));
	fGrid->SetInfoView(fInfoView);

#ifdef HVIF_STORE_CLIENT
	fCopyRDefBtn = new BButton("rdef", "RDEF Array", new BMessage(kMsgCopyRDef));
	fCopyCppBtn = new BButton("cpp", "C++ Array", new BMessage(kMsgCopyCPP));
	fCopySvgBtn = new BButton("svg", "SVG Code", new BMessage(kMsgCopySVG));
	fCopyImgBtn = new BButton("img", "HTML Base64", new BMessage(kMsgCopyImgTag));

	fCopyRDefBtn->SetEnabled(false);
	fCopyCppBtn->SetEnabled(false);
	fCopySvgBtn->SetEnabled(false);
	fCopyImgBtn->SetEnabled(false);
#else
	BButton* cancelBtn = new BButton("cancel", B_TRANSLATE("Cancel"), new BMessage(kMsgCancel));
	fOpenBtn = new BButton("open", B_TRANSLATE("Open"), new BMessage(kMsgOpenIcon));
	fOpenBtn->SetEnabled(false);
	fOpenBtn->MakeDefault(true);
#endif

	AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS)
		.AddGroup(B_HORIZONTAL, B_USE_BORDER_SPACING)
			.Add(fSearchEntry)
			.Add(fResetButton)
		.End()
		.Add(fTagsView)
		.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.Add(fGridScroll, 3.0f)
			.Add(fInfoView, 0.0f)
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
#ifdef HVIF_STORE_CLIENT
			.Add(fCopyRDefBtn)
			.Add(fCopyCppBtn)
			.Add(fCopySvgBtn)
			.Add(fCopyImgBtn)
#else
			.Add(cancelBtn)
			.Add(fOpenBtn)
#endif
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


const char*
IconSelectionDialog::_GetFormatExtension(IconFormat format) const
{
	switch (format) {
		case kFormatHVIF: return "hvif";
		case kFormatSVG:  return "svg";
		case kFormatIOM:  return "iom";
		default:          return "";
	}
}


const char*
IconSelectionDialog::_GetFormatMimeType(IconFormat format) const
{
	switch (format) {
		case kFormatHVIF: return "image/x-hvif";
		case kFormatSVG:  return "image/svg+xml";
		case kFormatIOM:  return "application/x-vnd.Haiku-icon";
		default:          return "application/octet-stream";
	}
}


void
IconSelectionDialog::_SaveFormat(IconFormat format)
{
	IconItem* item = fGrid->SelectedItem();
	if (item == NULL)
		return;

	BString path;
	switch (format) {
		case kFormatHVIF: path = item->hvifUrl; break;
		case kFormatSVG:  path = item->svgUrl; break;
		case kFormatIOM:  path = item->iomUrl; break;
		default: return;
	}

	if (path.IsEmpty()) {
		BAlert* alert = new BAlert(B_TRANSLATE("Error"),
			B_TRANSLATE("This format is not available for this icon."),
			B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go();
		return;
	}

	fPendingSaveFormat = format;

	BString defaultName = item->title;
	defaultName.ReplaceAll("/", "_");
	defaultName.ReplaceAll(":", "_");
	defaultName << "." << _GetFormatExtension(format);

	if (fSavePanel == NULL) {
		BMessage* message = new BMessage(kMsgSaveFormatRef);
		fSavePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this),
			NULL, 0, false, message);
	}

	fSavePanel->SetSaveText(defaultName.String());

	BString panelTitle = B_TRANSLATE("Save icon as ");
	panelTitle << _GetFormatExtension(format);
	fSavePanel->Window()->SetTitle(panelTitle.String());

	SetFeel(B_NORMAL_WINDOW_FEEL);

	fSavePanel->Show();
}


void
IconSelectionDialog::_DoSaveFormat(BMessage* message)
{
	if (fPendingSaveFormat == kFormatNone)
		return;

	entry_ref dirRef;
	BString name;

	if (message->FindRef("directory", &dirRef) != B_OK ||
		message->FindString("name", &name) != B_OK) {
		fPendingSaveFormat = kFormatNone;
		return;
	}

	IconItem* item = fGrid->SelectedItem();
	if (item == NULL) {
		fPendingSaveFormat = kFormatNone;
		return;
	}

	BString urlPath;
	switch (fPendingSaveFormat) {
		case kFormatHVIF: urlPath = item->hvifUrl; break;
		case kFormatSVG:  urlPath = item->svgUrl; break;
		case kFormatIOM:  urlPath = item->iomUrl; break;
		default:
			fPendingSaveFormat = kFormatNone;
			return;
	}

	BPath dirPath(&dirRef);
	BPath filePath(dirPath);
	filePath.Append(name.String());

	BMessage downloadMsg(kMsgDownloadIcon);
	downloadMsg.AddInt32("id", item->id);
	downloadMsg.AddString("title", item->title);
	downloadMsg.AddString("save_path", filePath.Path());
	downloadMsg.AddInt32("save_format", fPendingSaveFormat);

	switch (fPendingSaveFormat) {
		case kFormatHVIF:
			downloadMsg.AddString("hvif_path", urlPath);
			downloadMsg.AddString("svg_path", "");
			downloadMsg.AddString("iom_path", "");
			break;
		case kFormatSVG:
			downloadMsg.AddString("hvif_path", "");
			downloadMsg.AddString("svg_path", urlPath);
			downloadMsg.AddString("iom_path", "");
			break;
		case kFormatIOM:
			downloadMsg.AddString("hvif_path", "");
			downloadMsg.AddString("svg_path", "");
			downloadMsg.AddString("iom_path", urlPath);
			break;
		default:
			break;
	}

	fClient->PostMessage(&downloadMsg);
	fPendingSaveFormat = kFormatNone;
}

#ifdef HVIF_STORE_CLIENT
void
IconSelectionDialog::_CopyFormat(uint32 command)
{
	IconItem* item = fGrid->SelectedItem();
	if (item == NULL)
		return;

	if (command == kMsgCopyRDef || command == kMsgCopyCPP) {
		if (item->hvifData != NULL && item->hvifSize > 0) {
			_ProcessClipboardData(item->hvifData, item->hvifSize, command,
				item->id, item->title);
		} else {
			BAlert* alert = new BAlert(B_TRANSLATE("Error"),
				B_TRANSLATE("HVIF data is missing in memory."),
				B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->Go();
		}
		return;
	}

	if (command == kMsgCopySVG || command == kMsgCopyImgTag) {
		if (item->svgUrl.IsEmpty()) {
			BAlert* alert = new BAlert(B_TRANSLATE("Error"),
				B_TRANSLATE("SVG format is not available for this icon."),
				B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->Go();
			return;
		}

		_SetLoading(true);

		fCopyRDefBtn->SetEnabled(false);
		fCopyCppBtn->SetEnabled(false);
		fCopySvgBtn->SetEnabled(false);
		fCopyImgBtn->SetEnabled(false);

		BMessage msg(kMsgDownloadIcon);
		msg.AddInt32("id", item->id);
		msg.AddString("title", item->title);
		msg.AddInt32("clipboard_action", command);
		msg.AddString("svg_path", item->svgUrl);

		fClient->PostMessage(&msg);
	}
}


void
IconSelectionDialog::_ProcessClipboardData(const uint8* data, size_t size,
	uint32 command, int32 id, const char* name)
{
	switch (command) {
		case kMsgCopyRDef:
			IconExportUtils::CopyToClipboardRDef(data, size, id, name);
			break;
		case kMsgCopyCPP:
			IconExportUtils::CopyToClipboardCPP(data, size, name);
			break;
		case kMsgCopySVG:
			IconExportUtils::CopyToClipboardSVG(data, size);
			break;
		case kMsgCopyImgTag:
			IconExportUtils::CopyToClipboardImgTag(data, size);
			break;
	}
}
#endif


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
			fInfoView->SetFilterTags(fCurrentTags);
			_Search(true);
			break;
		}

		case kMsgMetaTagClicked: {
			BString tag;
			if (message->FindString("tag", &tag) == B_OK) {
				fTagsView->ToggleTag(tag);
			}
			break;
		}

		case kMsgClearTags: {
			fTagsView->DeselectAll();
			fSearchEntry->SetText("");
			fTagsView->GetSelectedTags(fCurrentTags);
			fInfoView->SetFilterTags(fCurrentTags);
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
		    const void* hvifData = NULL;
		    ssize_t hvifSize = 0;

		    if (message->FindPointer("bitmap", (void**)&bmp) == B_OK &&
		        message->FindInt32("id", &id) == B_OK &&
		        message->FindInt32("generation", &generation) == B_OK) {
		        message->FindData("hvif_data", B_RAW_TYPE, &hvifData, &hvifSize);
		        fGrid->SetIcon(id, bmp, generation, (const uint8*)hvifData, (size_t)hvifSize);
		    }
		    break;
		}

		case kMsgSelectIcon: {
			IconItem* item = fGrid->SelectedItem();
			bool hasSelection = (item != NULL);
#ifdef HVIF_STORE_CLIENT
			fCopyRDefBtn->SetEnabled(hasSelection && !item->hvifUrl.IsEmpty());
			fCopyCppBtn->SetEnabled(hasSelection && !item->hvifUrl.IsEmpty());

			bool hasSVG = hasSelection && !item->svgUrl.IsEmpty();
			fCopySvgBtn->SetEnabled(hasSVG);
			fCopyImgBtn->SetEnabled(hasSVG);
#else
			fOpenBtn->SetEnabled(hasSelection);
#endif
			fPreserveSelectionId = -1;
			break;
		}

		case kMsgOpenIcon:
			_OpenSelectedIcon();
			break;

		case kMsgCancel:
			PostMessage(B_QUIT_REQUESTED);
			break;

#ifdef HVIF_STORE_CLIENT
		case kMsgCopyRDef:
		case kMsgCopyCPP:
		case kMsgCopySVG:
		case kMsgCopyImgTag:
			_CopyFormat(message->what);
			break;
#endif

		case kMsgSaveFormat: {
			int32 format;
			if (message->FindInt32("format", &format) == B_OK) {
				_SaveFormat((IconFormat)format);
			}
			break;
		}

		case kMsgSaveFormatRef:
			SetFeel(B_MODAL_APP_WINDOW_FEEL);
			_DoSaveFormat(message);
			break;

		case B_CANCEL: {
			void* source = NULL;
			if (message->FindPointer("source", &source) == B_OK
				&& source == fSavePanel) {
				SetFeel(B_MODAL_APP_WINDOW_FEEL);
				fPendingSaveFormat = kFormatNone;
			}
			break;
		}

		case kMsgIconDataReady: {
#ifdef HVIF_STORE_CLIENT
			int32 clipboardAction;
			if (message->FindInt32("clipboard_action", &clipboardAction) == B_OK) {
				const void* data = NULL;
				ssize_t size = 0;
				const char* dataField = NULL;

				if (clipboardAction == kMsgCopySVG || clipboardAction == kMsgCopyImgTag)
					dataField = "svg_data";
				else
					dataField = "hvif_data";

				if (dataField && message->FindData(dataField, B_RAW_TYPE, &data, &size) == B_OK) {
					int32 id = message->GetInt32("id", 0);
					BString title = message->GetString("title", "");
					_ProcessClipboardData((const uint8*)data, size, clipboardAction, id, title);
				}

				_SetLoading(false);
				PostMessage(kMsgSelectIcon);
				break;
			}
#endif

			BString savePath;
			int32 saveFormat;

			if (message->FindString("save_path", &savePath) == B_OK &&
				message->FindInt32("save_format", &saveFormat) == B_OK) {

				const void* data = NULL;
				ssize_t size = 0;
				const char* dataField = NULL;

				switch ((IconFormat)saveFormat) {
					case kFormatHVIF: dataField = "hvif_data"; break;
					case kFormatSVG:  dataField = "svg_data"; break;
					case kFormatIOM:  dataField = "iom_data"; break;
					default: break;
				}

				if (dataField != NULL &&
					message->FindData(dataField, B_RAW_TYPE, &data, &size) == B_OK) {

					BFile file(savePath.String(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);

					if (file.InitCheck() == B_OK) {
						file.Write(data, size);

						BNodeInfo nodeInfo(&file);
						nodeInfo.SetType(_GetFormatMimeType((IconFormat)saveFormat));
					} else {
						BAlert* alert = new BAlert(B_TRANSLATE("Error"),
							B_TRANSLATE("Failed to save file."),
							B_TRANSLATE("OK"), NULL, NULL,
							B_WIDTH_AS_USUAL, B_WARNING_ALERT);
						alert->Go();
					}
				}
				break;
			}

			_SetLoading(false);
#ifdef HVIF_STORE_CLIENT
			PostMessage(kMsgSelectIcon);
#else
			fOpenBtn->SetEnabled(true);
#endif

			if (fTarget.IsValid()) {
				fTarget.SendMessage(message);
			}

#ifndef HVIF_STORE_CLIENT
			PostMessage(B_QUIT_REQUESTED);
#else
			if (!fTarget.IsValid())
				PostMessage(B_QUIT_REQUESTED);
#endif
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
			PostMessage(kMsgSelectIcon);
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
		IconItem* selected = fGrid->SelectedItem();
		if (selected != NULL) {
			fPreserveSelectionId = selected->id;
		} else {
			fPreserveSelectionId = -1;
		}

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

	if (fPreserveSelectionId >= 0) {
		if (fGrid->SelectIcon(fPreserveSelectionId)) {
#ifndef HVIF_STORE_CLIENT
			fOpenBtn->SetEnabled(true);
#else
			PostMessage(kMsgSelectIcon);
#endif
			fPreserveSelectionId = -1;
		}
	}
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
#ifndef HVIF_STORE_CLIENT
	fOpenBtn->SetEnabled(false);
#endif

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
