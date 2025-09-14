/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <Alert.h>
#include <AppFileInfo.h>
#include <Resources.h>
#include <MessageRunner.h>
#include <File.h>
#include <Clipboard.h>
#include <TabView.h>
#include <TextView.h>
#include <Directory.h>
#include <GroupView.h>
#include <Catalog.h>

#include <private/interface/AboutWindow.h>

#include "SVGMainWindow.h"
#include "SVGMenuManager.h"
#include "SVGFileManager.h"
#include "SVGView.h"
#include "SVGHVIFView.h"
#include "SVGTextEdit.h"
#include "SVGToolBar.h"
#include "SVGApplication.h"
#include "SVGSettings.h"

#include "HVIFParser.h"
#include "HVIFWriter.h"
#include "SVGParser.h"
#include "SVGRenderer.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SVGMainWindow"

SVGMainWindow::SVGMainWindow(const char* filePath)
	: BWindow(gSettings->GetRect(kWindowFrame, BRect(100, 100, 1200, 800)),
			  "SVGear", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS),
	fSVGView(NULL),
	fIconView(NULL),
	fTabView(NULL),
	fSVGTextView(NULL),
	fRDefTextView(NULL),
	fCPPTextView(NULL),
	fSVGScrollView(NULL),
	fRDefScrollView(NULL),
	fCPPScrollView(NULL),
	fMenuManager(NULL),
	fFileManager(NULL),
	fMenuBar(NULL),
	fMenuContainer(NULL),
	fEditorContainer(NULL),
	fViewerContainer(NULL),
	fStatusView(NULL),
	fSplitView(NULL),
	fToolBar(NULL),
	fEditToolBar(NULL),
	fDocumentModified(false),
	fShowStatView(false),
	fStructureView(NULL),
	fShowStructureView(false),
	fShowSourceView(false),
	fShowBoundingBox(false),
	fBoundingBoxStyle(1),
	fCurrentUIState(0),
	fStateUpdateRunner(NULL),
	fClipboardHasData(false),
	fTextHasSelection(false),
	fOriginalSourceText(),
	fCurrentHVIFData(NULL),
	fCurrentHVIFSize(0),
	fExportPanel(NULL),
	fCurrentExportType(0)
{
	SetSizeLimits(600, 16384, 450, 16384);

	fMenuManager = new SVGMenuManager();
	fFileManager = new SVGFileManager();

	_BuildInterface();

	_RestoreSettings();

	_StartStateMonitoring();

	if (filePath)
		LoadFile(filePath);

	if (fSVGView)
		fSVGView->SetTarget(this);

	_UpdateStatus();
	_UpdateUIState();
}

SVGMainWindow::~SVGMainWindow()
{
	_StopStateMonitoring();
	_SaveSettings();
	delete fMenuManager;
	delete fFileManager;
	delete fExportPanel;
	delete[] fCurrentHVIFData;
	be_app->PostMessage(MSG_WINDOW_CLOSED);
}

void
SVGMainWindow::_BuildInterface()
{
	_BuildToolBars();
	_BuildMainView();
	_BuildStatusBar();

	fMenuBar = fMenuManager->CreateMenuBar(this);

	fIconView = new HVIFView("drag_icon");
	font_height height;
	fMenuBar->GetFontHeight(&height);
	float iconSize = height.ascent + height.descent + 2.0f;
	fIconView->SetExplicitMinSize(BSize(iconSize, B_SIZE_UNSET));
	fIconView->SetExplicitMaxSize(BSize(iconSize, B_SIZE_UNSET));

	fMenuContainer = new BGroupView(B_HORIZONTAL, 0);
	fMenuContainer->GroupLayout()->AddView(fMenuBar);
	fMenuContainer->GroupLayout()->AddView(fIconView);
	fMenuBar->SetBorders(BControlLook::B_ALL_BORDERS & ~BControlLook::B_RIGHT_BORDER);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(fMenuContainer)
		.Add(fToolBar)
		.Add(fSplitView)
		.AddGroup(B_HORIZONTAL, 0)
			.Add(fStatusView)
			.AddGlue()
			.End()
		.End();
}

void
SVGMainWindow::_BuildToolBars()
{
	fToolBar = new SVGToolBar();
	fToolBar->AddAction(MSG_NEW_FILE, this, SVGApplication::GetIcon("document-new", TOOLBAR_ICON_SIZE), B_TRANSLATE("New"));
	fToolBar->AddAction(MSG_OPEN_FILE, this, SVGApplication::GetIcon("document-open", TOOLBAR_ICON_SIZE), B_TRANSLATE("Open"));
	fToolBar->AddAction(MSG_SAVE_FILE, this, SVGApplication::GetIcon("document-save", TOOLBAR_ICON_SIZE), B_TRANSLATE("Save"));
	fToolBar->AddSeparator();
	fToolBar->AddAction(MSG_ZOOM_IN, this, SVGApplication::GetIcon("zoom-in", TOOLBAR_ICON_SIZE), B_TRANSLATE("Zoom in"));
	fToolBar->AddAction(MSG_ZOOM_OUT, this, SVGApplication::GetIcon("zoom-out", TOOLBAR_ICON_SIZE), B_TRANSLATE("Zoom out"));
	fToolBar->AddAction(MSG_ZOOM_ORIGINAL, this, SVGApplication::GetIcon("zoom-original", TOOLBAR_ICON_SIZE), B_TRANSLATE("Zoom original"));
	fToolBar->AddAction(MSG_FIT_WINDOW, this, SVGApplication::GetIcon("zoom-fit-best", TOOLBAR_ICON_SIZE), B_TRANSLATE("Best fit"));
	fToolBar->AddAction(MSG_CENTER, this, SVGApplication::GetIcon("go-center", TOOLBAR_ICON_SIZE), B_TRANSLATE("Center"));
	fToolBar->AddSeparator();
	fToolBar->AddAction(MSG_TOGGLE_BOUNDINGBOX, this, SVGApplication::GetIcon("bounding-box", TOOLBAR_ICON_SIZE), B_TRANSLATE("Show Bounding Box"));
	fToolBar->AddAction(MSG_TOGGLE_SOURCE_VIEW, this, SVGApplication::GetIcon("format-text-code", TOOLBAR_ICON_SIZE), B_TRANSLATE("Show Source Code"));
	fToolBar->AddAction(MSG_TOGGLE_STRUCTURE, this, SVGApplication::GetIcon("structure", TOOLBAR_ICON_SIZE), B_TRANSLATE("Show Structure"));
	fToolBar->AddAction(MSG_TOGGLE_STAT, this, SVGApplication::GetIcon("info", TOOLBAR_ICON_SIZE), B_TRANSLATE("Show statistics"));
	fToolBar->AddGlue();

	fEditToolBar = new SVGToolBar();
	fEditToolBar->AddAction(B_UNDO, this, SVGApplication::GetIcon("edit-undo", TOOLBAR_ICON_SIZE), B_TRANSLATE("Undo"));
	fEditToolBar->AddAction(B_REDO, this, SVGApplication::GetIcon("edit-redo", TOOLBAR_ICON_SIZE), B_TRANSLATE("Redo"));
	fEditToolBar->AddSeparator();
	fEditToolBar->AddAction(MSG_EDIT_COPY, this, SVGApplication::GetIcon("edit-copy", TOOLBAR_ICON_SIZE), B_TRANSLATE("Copy"));
	fEditToolBar->AddAction(MSG_EDIT_PASTE, this, SVGApplication::GetIcon("edit-paste", TOOLBAR_ICON_SIZE), B_TRANSLATE("Paste"));
	fEditToolBar->AddAction(MSG_EDIT_CUT, this, SVGApplication::GetIcon("edit-cut", TOOLBAR_ICON_SIZE), B_TRANSLATE("Cut"));
	fEditToolBar->AddSeparator();
	fEditToolBar->AddAction(MSG_EDIT_WORD_WRAP, this, SVGApplication::GetIcon("text-wrap", TOOLBAR_ICON_SIZE), B_TRANSLATE("Text wrap"));
	fEditToolBar->AddSeparator();
	fEditToolBar->AddAction(MSG_EDIT_APPLY, this, SVGApplication::GetIcon("dialog-ok-apply", TOOLBAR_ICON_SIZE), B_TRANSLATE("Apply (Alt+Enter)"));
	fEditToolBar->AddGlue();
}

void
SVGMainWindow::_BuildTabView()
{
	fTabView = new BTabView("tab_view", B_WIDTH_FROM_WIDEST);

	BGroupView* svgTabGroup = new BGroupView(B_VERTICAL, 0);
	svgTabGroup->GroupLayout()->AddView(fEditToolBar);

	fSVGTextView = new SVGTextEdit("svg_text");
	fSVGTextView->SetWordWrap(true);
	fSVGScrollView = new BScrollView("svg_scroll", fSVGTextView,
					B_WILL_DRAW | B_FRAME_EVENTS,
					true, true, B_NO_BORDER);

	svgTabGroup->GroupLayout()->AddView(fSVGScrollView);

	BTab* svgTab = new BTab();
	fTabView->AddTab(svgTabGroup, svgTab);
	svgTab->SetLabel("SVG");

	fRDefTextView = new SVGTextEdit("rdef_text");
	fRDefTextView->SetWordWrap(true);
	fRDefTextView->MakeEditable(false);

	BFont fixedFont(be_fixed_font);
	fRDefTextView->SetFontAndColor(&fixedFont);

	fRDefScrollView = new BScrollView("rdef_scroll", fRDefTextView,
					B_WILL_DRAW | B_FRAME_EVENTS,
					true, true, B_NO_BORDER);

	BTab* rdefTab = new BTab();
	fTabView->AddTab(fRDefScrollView, rdefTab);
	rdefTab->SetLabel("RDef");

	fCPPTextView = new SVGTextEdit("cpp_text");
	fCPPTextView->SetWordWrap(true);
	fCPPTextView->MakeEditable(false);
	fCPPTextView->SetFontAndColor(&fixedFont);

	fCPPScrollView = new BScrollView("cpp_scroll", fCPPTextView,
					B_WILL_DRAW | B_FRAME_EVENTS,
					true, true, B_NO_BORDER);

	BTab* cppTab = new BTab();
	fTabView->AddTab(fCPPScrollView, cppTab);
	cppTab->SetLabel("C++");
}

void
SVGMainWindow::_BuildMainView()
{
	fViewerContainer = new BGroupView(B_HORIZONTAL, 0);

	fStructureView = new SVGStructureView("structure_view");
	BLayoutItem* structureItem = fViewerContainer->GroupLayout()->AddView(fStructureView);
	structureItem->SetVisible(false);

	fSVGView = new SVGView("svg_view");
	fSVGView->SetBoundingBoxStyle(SVG_BBOX_NONE);
	fViewerContainer->GroupLayout()->AddView(fSVGView);
	fStructureView->SetSVGView(fSVGView);

	fStatView = new SVGStatView("stat_view");
	BLayoutItem* statItem = fViewerContainer->GroupLayout()->AddView(fStatView);
	statItem->SetVisible(false);

	_BuildTabView();

	fEditorContainer = new BGroupView(B_VERTICAL, 0);
	fEditorContainer->GroupLayout()->AddView(fTabView);

	fSplitView = new BSplitView(B_VERTICAL);
	fSplitView->AddChild(fViewerContainer);
	fSplitView->AddChild(fEditorContainer);
	fSplitView->SetCollapsible(1, true);
	fSplitView->SetCollapsible(0, false);
	fSplitView->SetItemCollapsed(1, true);
}

void
SVGMainWindow::_BuildStatusBar()
{
	fStatusView = new BStringView("status", B_TRANSLATE("Ready"));
	BFont font;
	if (fSVGView) {
		fSVGView->GetFont(&font);
		font.SetSize(font.Size() - 2.0);
		fStatusView->SetFont(&font);
	}
	fStatusView->SetAlignment(B_ALIGN_LEFT);
}

bool
SVGMainWindow::QuitRequested()
{
	return true;
}

void
SVGMainWindow::MessageReceived(BMessage* message)
{
	if (message->WasDropped()) {
		_HandleDropMessages(message);
		return;
	}

	if (message->what == B_SAVE_REQUESTED && fCurrentExportType != 0) {
		_HandleExportSavePanel(message);
		return;
	}

	switch (message->what) {
		case MSG_NEW_FILE:
		case MSG_OPEN_FILE:
		case MSG_SAVE_FILE:
		case MSG_SAVE_AS_FILE:
		case MSG_SAVE_PANEL_SAVE:
		case B_REFS_RECEIVED:
		case B_SAVE_REQUESTED:
			_HandleFileMessages(message);
			break;

		case MSG_EXPORT_HVIF:
		case MSG_EXPORT_RDEF:
		case MSG_EXPORT_CPP:
			_HandleExportMessages(message);
			break;

		case MSG_TAB_SELECTION:
			_HandleTabSelection();
			break;

		case MSG_FIT_WINDOW:
		case MSG_ZOOM_ORIGINAL:
		case MSG_CENTER:
		case MSG_ZOOM_IN:
		case MSG_ZOOM_OUT:
		case MSG_RESET_VIEW:
		case MSG_DISPLAY_NORMAL:
		case MSG_DISPLAY_OUTLINE:
		case MSG_DISPLAY_FILL_ONLY:
		case MSG_DISPLAY_STROKE_ONLY:
		case MSG_TOGGLE_TRANSPARENCY:
		case MSG_TOGGLE_BOUNDINGBOX:
		case MSG_BBOX_NONE:
		case MSG_BBOX_DOCUMENT:
		case MSG_BBOX_SIMPLE_FRAME:
		case MSG_BBOX_TRANSPARENT_GRAY:
		case MSG_TOGGLE_SOURCE_VIEW:
		case MSG_TOGGLE_STAT:
		case MSG_TOGGLE_STRUCTURE:
			_HandleViewMessages(message);
			break;

		case MSG_EDIT_COPY:
		case MSG_EDIT_PASTE:
		case MSG_EDIT_CUT:
		case MSG_EDIT_APPLY:
		case MSG_EDIT_WORD_WRAP:
		case MSG_RELOAD_FROM_SOURCE:
		case B_UNDO:
		case B_REDO:
			_HandleEditMessages(message);
			break;

		case MSG_STATE_UPDATE:
			_CheckClipboardState();
			_CheckTextSelectionState();
			_UpdateUIState();
			break;

		case MSG_TEXT_MODIFIED:
			_OnTextModified();
			break;

		case MSG_SELECTION_CHANGED:
			_OnSelectionChanged();
			break;

		case MSG_ABOUT:
			_ShowAbout();
			break;

		case MSG_EASTER_EGG:
			_LoadTemplateFile("Teapot.svg", "Teapot.svg");
			break;

		case MSG_SVG_STATUS_UPDATE:
			_UpdateStatus();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

void
SVGMainWindow::_HandleFileMessages(BMessage* message)
{
	switch (message->what) {
		case MSG_NEW_FILE:
			_LoadNewFile();
			break;

		case MSG_OPEN_FILE:
			if (fFileManager) {
				fFileManager->ShowOpenPanel(this);
			}
			break;

		case B_REFS_RECEIVED:
			_HandleRefsReceived(message);
			break;

		case MSG_SAVE_FILE:
			_SaveFile();
			break;

		case MSG_SAVE_AS_FILE:
			_SaveAsFile();
			break;

		case B_SAVE_REQUESTED:
		case MSG_SAVE_PANEL_SAVE:
			_HandleSavePanel(message);
			break;
	}
}

void
SVGMainWindow::_HandleExportMessages(BMessage* message)
{
	switch (message->what) {
		case MSG_EXPORT_HVIF:
			_ExportHVIF();
			break;
		case MSG_EXPORT_RDEF:
			_ExportRDef();
			break;
		case MSG_EXPORT_CPP:
			_ExportCPP();
			break;
	}
}

void
SVGMainWindow::_HandleExportSavePanel(BMessage* message)
{
	entry_ref dirRef;
	BString fileName;

	if (message->FindRef("directory", &dirRef) != B_OK ||
		message->FindString("name", &fileName) != B_OK) {
		_ShowError(B_TRANSLATE("Could not get export file information"));
		fCurrentExportType = 0;
		return;
	}

	BPath dirPath(&dirRef);
	BString fullPath = dirPath.Path();
	fullPath << "/" << fileName;

	gSettings->SetString(kLastExportPath, dirPath.Path());

	status_t result = B_ERROR;

	switch (fCurrentExportType) {
		case MSG_EXPORT_HVIF:
		{
			if (!fCurrentHVIFData || fCurrentHVIFSize == 0) {
				_ShowError(B_TRANSLATE("No HVIF data available"));
				break;
			}

			if (!fileName.EndsWith(".hvif"))
				fullPath << ".hvif";

			result = _SaveBinaryData(fullPath.String(), fCurrentHVIFData, fCurrentHVIFSize, "image/x-hvif");
			break;
		}

		case MSG_EXPORT_RDEF:
		{
			if (!fCurrentHVIFData || fCurrentHVIFSize == 0) {
				_ShowError(B_TRANSLATE("No HVIF data available"));
				break;
			}

			if (!fileName.EndsWith(".rdef"))
				fullPath << ".rdef";

			BString rdefContent = _ConvertToRDef(fCurrentHVIFData, fCurrentHVIFSize);
			result = fFileManager->SaveFile(fullPath.String(), rdefContent, "text/plain");
			break;
		}

		case MSG_EXPORT_CPP:
		{
			if (!fCurrentHVIFData || fCurrentHVIFSize == 0) {
				_ShowError(B_TRANSLATE("No HVIF data available"));
				break;
			}

			if (!fileName.EndsWith(".h") && !fileName.EndsWith(".hpp") && !fileName.EndsWith(".cpp")) {
				fullPath << ".h";
			}

			BString cppContent = _ConvertToCPP(fCurrentHVIFData, fCurrentHVIFSize);
			result = fFileManager->SaveFile(fullPath.String(), cppContent, "text/x-source-code");
			break;
		}
	}

	if (result == B_OK) {
		_ShowSuccess(MSG_FILE_EXPORTED);
	} else {
		BString error;
		error.SetToFormat("%s: %s", B_TRANSLATE("Failed to export file"), strerror(result));
		_ShowError(error.String());
	}

	fCurrentExportType = 0;
}

void
SVGMainWindow::_HandleViewMessages(BMessage* message)
{
	if (!fSVGView)
		return;

	switch (message->what) {
		case MSG_FIT_WINDOW:
			fSVGView->ZoomToFit();
			break;

		case MSG_ZOOM_ORIGINAL:
			fSVGView->ZoomToOriginal();
			break;

		case MSG_CENTER:
			fSVGView->CenterImage();
			break;

		case MSG_ZOOM_IN:
			fSVGView->ZoomIn();
			break;

		case MSG_ZOOM_OUT:
			fSVGView->ZoomOut();
			break;

		case MSG_RESET_VIEW:
			fSVGView->ResetView();
			break;

		case MSG_DISPLAY_NORMAL:
			fSVGView->SetDisplayMode(SVG_DISPLAY_NORMAL);
			_UpdateDisplayModeMenu();
			break;

		case MSG_DISPLAY_OUTLINE:
			fSVGView->SetDisplayMode(SVG_DISPLAY_OUTLINE);
			_UpdateDisplayModeMenu();
			break;

		case MSG_DISPLAY_FILL_ONLY:
			fSVGView->SetDisplayMode(SVG_DISPLAY_FILL_ONLY);
			_UpdateDisplayModeMenu();
			break;

		case MSG_DISPLAY_STROKE_ONLY:
			fSVGView->SetDisplayMode(SVG_DISPLAY_STROKE_ONLY);
			_UpdateDisplayModeMenu();
			break;

		case MSG_TOGGLE_TRANSPARENCY:
			fSVGView->SetShowTransparency(!fSVGView->ShowTransparency());
			_UpdateViewMenu();
			break;

		case MSG_TOGGLE_BOUNDINGBOX:
			fShowBoundingBox = !fShowBoundingBox;
			_UpdateBoundingBoxMenu();
			_UpdateViewMenu();
			_UpdateUIState();
			break;

		case MSG_BBOX_NONE:
			fShowBoundingBox = false;
			_UpdateBoundingBoxMenu();
			_UpdateViewMenu();
			_UpdateUIState();
			break;

		case MSG_BBOX_DOCUMENT:
			fShowBoundingBox = true;
			fBoundingBoxStyle = SVG_BBOX_DOCUMENT;
			_UpdateBoundingBoxMenu();
			_UpdateViewMenu();
			_UpdateUIState();
			break;

		case MSG_BBOX_SIMPLE_FRAME:
			fShowBoundingBox = true;
			fBoundingBoxStyle = SVG_BBOX_SIMPLE_FRAME;
			_UpdateBoundingBoxMenu();
			_UpdateViewMenu();
			_UpdateUIState();
			break;

		case MSG_BBOX_TRANSPARENT_GRAY:
			fShowBoundingBox = true;
			fBoundingBoxStyle = SVG_BBOX_TRANSPARENT_GRAY;
			_UpdateBoundingBoxMenu();
			_UpdateViewMenu();
			_UpdateUIState();
			break;

		case MSG_TOGGLE_SOURCE_VIEW:
			_ToggleSourceView();
			break;

		case MSG_TOGGLE_STRUCTURE:
			_ToggleStructureView();
			break;

		case MSG_TOGGLE_STAT:
			_ToggleStatView();
			break;
	}
}

void
SVGMainWindow::_HandleEditMessages(BMessage* message)
{
	BTextView* currentTextView = NULL;

	if (fTabView && !fSplitView->IsItemCollapsed(1)) {
		int32 selection = fTabView->Selection();
		switch (selection) {
			case TAB_SVG:
				currentTextView = fSVGTextView;
				break;
			case TAB_RDEF:
				currentTextView = fRDefTextView;
				break;
			case TAB_CPP:
				currentTextView = fCPPTextView;
				break;
		}
	}

	switch (message->what) {
		case B_UNDO:
			if (currentTextView && currentTextView == fSVGTextView)
				fSVGTextView->Undo(be_clipboard);
			break;

		case B_REDO:
			if (currentTextView && currentTextView == fSVGTextView)
				fSVGTextView->Redo();
			break;

		case MSG_EDIT_COPY:
			if (currentTextView)
				currentTextView->Copy(be_clipboard);
			break;

		case MSG_EDIT_PASTE:
			if (currentTextView && currentTextView == fSVGTextView)
				fSVGTextView->Paste(be_clipboard);
			break;

		case MSG_EDIT_CUT:
			if (currentTextView && currentTextView == fSVGTextView)
				fSVGTextView->Cut(be_clipboard);
			break;

		case MSG_EDIT_WORD_WRAP:
			if (currentTextView) {
				currentTextView->SetWordWrap(!currentTextView->DoesWordWrap());
			}
			break;

		case MSG_EDIT_APPLY:
		case MSG_RELOAD_FROM_SOURCE:
			_ReloadFromSource();
			break;
	}

	_UpdateUIState();
}

void
SVGMainWindow::_HandleDropMessages(BMessage* message)
{
	if (message->HasData("icon", B_VECTOR_ICON_TYPE)) {
		ssize_t size;
		const void* data;
		if (message->FindData("icon", B_VECTOR_ICON_TYPE, &data, &size) != B_OK)
			return;

		std::vector<uint8_t> icondata((uint8_t*)data, (uint8_t*)data + size);

		hvif::HVIFParser parser;
		if (!parser.ParseData(icondata)) {
			return;
		}

		delete[] fCurrentHVIFData;
		fCurrentHVIFSize = parser.GetIconDataSize();
		fCurrentHVIFData = new unsigned char[fCurrentHVIFSize];
		memcpy(fCurrentHVIFData, parser.GetIconData(), fCurrentHVIFSize);

		const hvif::HVIFIcon& icon = parser.GetIcon();
		hvif::SVGRenderer renderer;
		std::string svg = renderer.RenderIcon(icon, 64, 64);
		fCurrentSource.SetTo(svg.c_str());
		fOriginalSourceText = fCurrentSource;
		fDocumentModified = true;

		_UpdateStatus();
		_UpdateAllTabs();
		_ReloadFromSource();
		_UpdateUIState();
		_UpdateStatView();

		fSVGTextView->ClearUndoHistory();

		SetTitle("SVGear - Untitled.svg");
	} else if (message->HasRef("refs")) {
		message->what = B_REFS_RECEIVED;
		_HandleRefsReceived(message);
	}
}

void
SVGMainWindow::_LoadNewFile()
{
	_LoadTemplateFile("Untitled.svg", "Untitled.svg");
	if (fIconView) {
		fIconView->RemoveIcon();
	}

	delete[] fCurrentHVIFData;
	fCurrentHVIFData = NULL;
	fCurrentHVIFSize = 0;

	fCurrentFilePath = "";
	fOriginalSourceText = fCurrentSource;
	fDocumentModified = true;

	_GenerateHVIFFromSVG();

	if (fFileManager) {
		fFileManager->SetLastLoadedFileType(FILE_TYPE_NEW);
	}
	_UpdateUIState();
	_UpdateStatView();
	_UpdateStatView();
}

void
SVGMainWindow::_LoadTemplateFile(const char* resourceName, const char* title)
{
	app_info info;
	be_app->GetAppInfo(&info);
	BFile file(&info.ref, B_READ_ONLY);

	BResources res;
	if (res.SetTo(&file) != B_OK)
		return;

	size_t size;
	char* data = (char*)res.LoadResource('rSVG', resourceName, &size);
	if (data == NULL || size <= 0)
		return;

	data[size] = 0;
	fCurrentSource.SetTo(data);
	fOriginalSourceText = fCurrentSource;

	if (fSVGView)
		fSVGView->LoadFromMemory(fCurrentSource.String());

	_GenerateHVIFFromSVG();
	_UpdateStatus();
	_UpdateAllTabs();

	if (fSVGTextView)
		fSVGTextView->ClearUndoHistory();

	BString windowTitle("SVGear - ");
	windowTitle << title;
	SetTitle(windowTitle.String());

	_UpdateUIState();
	_UpdateStatView();
}

void
SVGMainWindow::_HandleRefsReceived(BMessage* message)
{
	entry_ref ref;
	if (message->FindRef("refs", &ref) == B_OK) {
		BPath path(&ref);
		if (path.InitCheck() == B_OK) {
			LoadFile(path.Path());
		}
	}
}

void
SVGMainWindow::LoadFile(const char* filePath)
{
	if (!fFileManager || !fSVGView)
		return;

	if (fFileManager->LoadFile(filePath, fSVGView, fIconView, fCurrentSource)) {
		BPath path(filePath);
		BString title("SVGear - ");
		title << path.Leaf();
		SetTitle(title.String());

		BPath dirPath;
		path.GetParent(&dirPath);
		gSettings->SetString(kLastOpenPath, dirPath.Path());

		fSVGView->ResetView();
		fCurrentFilePath = filePath;
		fDocumentModified = fFileManager->GetLastLoadedFileType() != FILE_TYPE_SVG;
		fOriginalSourceText = fCurrentSource;

		_GenerateHVIFFromSVG();

		_UpdateStatus();
		_UpdateAllTabs();
		_ReloadFromSource();
		fSVGTextView->ClearUndoHistory();
		_UpdateUIState();
		_UpdateStatView();
	}
}

void
SVGMainWindow::_GenerateHVIFFromSVG()
{
	if (fCurrentSource.IsEmpty())
		return;

	try {
		hvif::HVIFWriter writer;
		hvif::SVGParser parser;

		if (parser.ParseString(fCurrentSource.String(), writer)) {
			std::vector<uint8_t> hvifData = writer.WriteToBuffer();

			delete[] fCurrentHVIFData;
			fCurrentHVIFData = NULL;
			fCurrentHVIFSize = hvifData.size();
			if (!hvifData.empty()) {
				fCurrentHVIFData = new unsigned char[fCurrentHVIFSize];
				memcpy(fCurrentHVIFData, &hvifData[0], fCurrentHVIFSize);
			}
		}
	} catch (...) {
	}

	if (fIconView)
		fIconView->SetIcon(fCurrentHVIFData, fCurrentHVIFSize);
}

void
SVGMainWindow::_UpdateStatus()
{
	BString status;

	if (fSVGView && fSVGView->IsLoaded()) {
		float scale = fSVGView->Scale();
		float width = fSVGView->SVGWidth();
		float height = fSVGView->SVGHeight();

		status.SetToFormat(B_TRANSLATE(" Size: %.0fx%.0f | Scale: %.1f%% | Mode: %s | BBox: %s"),
				width, height, scale * 100.0f, 
				_GetDisplayModeName().String(), 
				_GetBoundingBoxStyleName().String());
	} else {
		status = B_TRANSLATE("No SVG loaded");
	}

	if (fStatusView)
		fStatusView->SetText(status.String());
}

void
SVGMainWindow::_UpdateInterface()
{
	_UpdateDisplayModeMenu();
	_UpdateBoundingBoxMenu();
	_UpdateViewMenu();
	_UpdateStatus();
}

void
SVGMainWindow::_UpdateStatView()
{
	fStatView->SetSVGImage(fSVGView->SVGImage());
	fStatView->SetIntValue("svg-size", fCurrentSource.Length());
	fStatView->SetIntValue("hvif-size", fCurrentHVIFSize);
	_UpdateStructureView();
}

void
SVGMainWindow::_UpdateDisplayModeMenu()
{
	if (fMenuManager && fSVGView)
		fMenuManager->UpdateDisplayMode(fSVGView->DisplayMode());
}

void
SVGMainWindow::_UpdateBoundingBoxMenu()
{
	fSVGView->SetBoundingBoxStyle(fShowBoundingBox ? (svg_boundingbox_style)fBoundingBoxStyle : SVG_BBOX_NONE);

	if (fMenuManager && fSVGView)
		fMenuManager->UpdateBoundingBoxStyle(fShowBoundingBox ? (svg_boundingbox_style)fBoundingBoxStyle : SVG_BBOX_NONE);

	_UpdateToggleButtonStates();
}

void
SVGMainWindow::_UpdateViewMenu()
{
	if (fMenuManager && fSVGView && fSplitView) {
		bool showTransparency = fSVGView->ShowTransparency();
		bool showBoundingBox = fSVGView->BoundingBoxStyle() != SVG_BBOX_NONE;
		fMenuManager->UpdateViewOptions(showTransparency, fShowSourceView, showBoundingBox, fShowStructureView, fShowStatView);
	}
}

BString
SVGMainWindow::_GetDisplayModeName() const
{
	if (!fSVGView)
		return B_TRANSLATE("Unknown");

	switch (fSVGView->DisplayMode()) {
		case SVG_DISPLAY_NORMAL:
			return B_TRANSLATE("Normal");
		case SVG_DISPLAY_OUTLINE:
			return B_TRANSLATE("Outline");
		case SVG_DISPLAY_FILL_ONLY:
			return B_TRANSLATE("Fill Only");
		case SVG_DISPLAY_STROKE_ONLY:
			return B_TRANSLATE("Stroke Only");
		default:
			return B_TRANSLATE("Unknown");
	}
}

BString
SVGMainWindow::_GetBoundingBoxStyleName() const
{
	if (!fSVGView)
		return B_TRANSLATE("Unknown");

	switch (fSVGView->BoundingBoxStyle()) {
		case SVG_BBOX_NONE:
			return B_TRANSLATE("None");
		case SVG_BBOX_DOCUMENT:
			return B_TRANSLATE("Document");
		case SVG_BBOX_SIMPLE_FRAME:
			return B_TRANSLATE("Simple");
		case SVG_BBOX_TRANSPARENT_GRAY:
			return B_TRANSLATE("Gray");
		default:
			return B_TRANSLATE("Unknown");
	}
}

void
SVGMainWindow::_ShowAbout()
{
	BAboutWindow* about = new BAboutWindow("SVGear", APP_SIGNATURE);
	about->AddCopyright(2025, "Gerasim Troeglazov (3dEyes**)");
	about->AddDescription(B_TRANSLATE(
		"SVGear provides an intuitive interface for viewing and manipulating "
		"SVG (Scalable Vector Graphics) files. \n"
		"The application supports format conversion operations, enabling "
		"users to transform SVG files into other vector formats such as "
		"HVIF (Haiku Vector Icon Format)."));
	about->Show();
}

void
SVGMainWindow::_ToggleSourceView()
{
	if (!fSplitView)
		return;

	fShowSourceView = !fShowSourceView;

	fSplitView->SetItemCollapsed(1, !fShowSourceView);

	if (fShowSourceView) {
		float mainWeight = gSettings->GetFloat(kMainViewWeight, 0.7f);
		float sourceWeight = gSettings->GetFloat(kSourceViewWeight, 0.3f);
		fSplitView->SetItemWeight(0, mainWeight, false);
		fSplitView->SetItemWeight(1, sourceWeight, false);
	}

	_UpdateViewMenu();
	_UpdateStatus();
	_UpdateUIState();
}

void
SVGMainWindow::_ToggleStructureView()
{
	fShowStructureView = !fShowStructureView;

	BLayoutItem* structureItem = fViewerContainer->GroupLayout()->ItemAt(0);
	if (structureItem) {
		structureItem->SetVisible(fShowStructureView);
	}

	_UpdateToolBarStates();
	_UpdateUIState();
}

void
SVGMainWindow::_ToggleStatView()
{
	fShowStatView = !fShowStatView;

	BLayoutItem* statItem = fViewerContainer->GroupLayout()->ItemAt(2);
	if (statItem) {
		statItem->SetVisible(fShowStatView);
	}

	_UpdateToolBarStates();
	_UpdateUIState();
}

void
SVGMainWindow::_UpdateAllTabs()
{
	if (fSVGTextView && !fCurrentSource.IsEmpty()) {
		fSVGTextView->SetText(fCurrentSource.String());
		fOriginalSourceText = fCurrentSource;
	}

	_UpdateRDefTab();
	_UpdateCPPTab();

	_UpdateUIState();
}

void
SVGMainWindow::_UpdateRDefTab()
{
	if (!fRDefTextView || !fCurrentHVIFData || fCurrentHVIFSize == 0) {
		if (fRDefTextView)
			fRDefTextView->SetText(B_TRANSLATE("No HVIF data available"));
		return;
	}

	BString rdefContent = _ConvertToRDef(fCurrentHVIFData, fCurrentHVIFSize);
	fRDefTextView->SetText(rdefContent.String());
}

void
SVGMainWindow::_UpdateCPPTab()
{
	if (!fCPPTextView || !fCurrentHVIFData || fCurrentHVIFSize == 0) {
		if (fCPPTextView)
			fCPPTextView->SetText(B_TRANSLATE("No HVIF data available"));
		return;
	}

	BString cppContent = _ConvertToCPP(fCurrentHVIFData, fCurrentHVIFSize);
	fCPPTextView->SetText(cppContent.String());
}

void
SVGMainWindow::_HandleTabSelection()
{
}

void
SVGMainWindow::_ReloadFromSource()
{
	if (!fSVGTextView || !fSVGView)
		return;

	BString sourceText = fSVGTextView->Text();
	if (sourceText.IsEmpty()) {
		_ShowError(ERROR_SOURCE_EMPTY);
		return;
	}

	status_t result = fSVGView->LoadFromMemory(sourceText.String());
	if (result != B_OK) {
		_ShowError(ERROR_PARSING_SVG);
	} else {
		if (fCurrentSource != sourceText)
			fDocumentModified = true;

		fCurrentSource = sourceText;

		_GenerateHVIFFromSVG();
		_UpdateRDefTab();
		_UpdateCPPTab();
		_UpdateStatus();
		_UpdateUIState();
	}

	fStatView->SetSVGImage(fSVGView->SVGImage());
	_UpdateStatView();
}

void
SVGMainWindow::_ShowError(const char* message)
{
	BAlert* alert = new BAlert(B_TRANSLATE("Error"), message, B_TRANSLATE("OK"), NULL, NULL,
							  B_WIDTH_AS_USUAL, B_STOP_ALERT);
	alert->Go();
}

void
SVGMainWindow::_SaveFile()
{
	if (!fFileManager)
		return;

	BString currentSource = _GetCurrentSource();
	if (currentSource.IsEmpty()) {
		_ShowError(ERROR_SOURCE_EMPTY);
		return;
	}

	if (fFileManager->CanDirectSave(fCurrentFilePath)) {
		if (fFileManager->SaveCurrentFile(fCurrentFilePath, currentSource)) {
			fOriginalSourceText = currentSource;
			if (fSVGTextView && !fSplitView->IsItemCollapsed(1)) {
				BString editorText = fSVGTextView->Text();
				if (editorText == currentSource) {
					fCurrentSource = currentSource;
				}
			} else {
				fCurrentSource = currentSource;
			}
			fDocumentModified = false;
			_ShowSuccess(MSG_FILE_SAVED);
			_UpdateUIState();
		}
	} else {
		_SaveAsFile();
	}
}

void
SVGMainWindow::_SaveAsFile()
{
	if (!fFileManager)
		return;

	BString currentSource = _GetCurrentSource();
	if (currentSource.IsEmpty()) {
		_ShowError(ERROR_SOURCE_EMPTY);
		return;
	}

	fFileManager->ShowSaveAsPanel(this);
}

void
SVGMainWindow::_HandleSavePanel(BMessage* message)
{
	entry_ref dirRef;
	BString fileName;

	if (message->FindRef("directory", &dirRef) != B_OK) {
		_ShowError(B_TRANSLATE("Could not find directory reference"));
		return;
	}

	if (message->FindString("name", &fileName) != B_OK) {
		_ShowError(B_TRANSLATE("Could not find file name"));
		return;
	}

	BDirectory dir(&dirRef);
	if (dir.InitCheck() != B_OK) {
		_ShowError(ERROR_INVALID_PATH);
		return;
	}

	BPath dirPath(&dirRef);
	if (dirPath.InitCheck() != B_OK) {
		_ShowError(ERROR_INVALID_PATH);
		return;
	}

	BString fullPath = dirPath.Path();
	fullPath << "/" << fileName;

	if (!fileName.EndsWith(".svg")) {
		fullPath << ".svg";
	}

	gSettings->SetString(kLastSavePath, dirPath.Path());

	BString currentSource = _GetCurrentSource();
	if (currentSource.IsEmpty()) {
		_ShowError(ERROR_SOURCE_EMPTY);
		return;
	}

	status_t result = fFileManager->SaveFile(fullPath.String(), currentSource, "image/svg+xml");

	if (result == B_OK) {
		fCurrentFilePath = fullPath;
		fOriginalSourceText = currentSource;
		fCurrentSource = currentSource;
		if (fFileManager) {
			fFileManager->SetLastLoadedFileType(FILE_TYPE_SVG);
		}
		fDocumentModified = false;
		_UpdateTitleAfterSave(fullPath.String());
		_ShowSuccess(MSG_FILE_SAVED);
		_UpdateUIState();
	} else {
		BString error;
		error.SetToFormat("%s: %s", B_TRANSLATE("Failed to save file"), strerror(result));
		_ShowError(error.String());
	}
}

bool
SVGMainWindow::_UpdateTitleAfterSave(const char* filePath)
{
	if (!filePath)
		return false;

	BPath path(filePath);
	if (path.InitCheck() != B_OK)
		return false;

	BString title("SVGear - ");
	title << path.Leaf();
	SetTitle(title.String());
	
	return true;
}

BString
SVGMainWindow::_GetCurrentSource() const
{
	if (fSVGTextView && !fSplitView->IsItemCollapsed(1)) {
		return BString(fSVGTextView->Text());
	} else {
		return fCurrentSource;
	}
}

void
SVGMainWindow::_ShowSuccess(const char* message)
{
	if (fStatusView) {
		BString status = fStatusView->Text();
		fStatusView->SetText(message);

		BMessage restoreMsg(MSG_SVG_STATUS_UPDATE);
		BMessageRunner* runner = new BMessageRunner(this, &restoreMsg, 3000000, 1);
	}
}

void
SVGMainWindow::_ExportHVIF()
{
	if (!fCurrentHVIFData || fCurrentHVIFSize == 0) {
		_ShowError(B_TRANSLATE("No HVIF data available for export"));
		return;
	}

	_ShowExportPanel("icon", ".hvif", MSG_EXPORT_HVIF);
}

void
SVGMainWindow::_ExportRDef()
{
	if (!fCurrentHVIFData || fCurrentHVIFSize == 0) {
		_ShowError(B_TRANSLATE("No HVIF data available for export"));
		return;
	}

	_ShowExportPanel("icon", ".rdef", MSG_EXPORT_RDEF);
}

void
SVGMainWindow::_ExportCPP()
{
	if (!fCurrentHVIFData || fCurrentHVIFSize == 0) {
		_ShowError(B_TRANSLATE("No HVIF data available for export"));
		return;
	}

	_ShowExportPanel("icon", ".cpp", MSG_EXPORT_CPP);
}

void
SVGMainWindow::_ShowExportPanel(const char* defaultName, const char* extension, uint32 exportType)
{
	if (!fExportPanel) {
		fExportPanel = new BFilePanel(B_SAVE_PANEL, NULL, NULL, 0, false);
		fExportPanel->SetTarget(BMessenger(this));
	}

	BString lastExportPath = gSettings->GetString(kLastExportPath);
	if (!lastExportPath.IsEmpty()) {
		entry_ref ref;
		if (get_ref_for_path(lastExportPath.String(), &ref) == B_OK) {
			fExportPanel->SetPanelDirectory(&ref);
		}
	}

	fCurrentExportType = exportType;

	BString fileName = defaultName;
	if (!fileName.EndsWith(extension))
		fileName << extension;

	fExportPanel->SetSaveText(fileName.String());
	fExportPanel->Show();
}

status_t
SVGMainWindow::_SaveBinaryData(const char* filePath, const unsigned char* data, size_t size, const char* mime)
{
	if (!filePath || !data || size == 0) {
		printf("_SaveBinaryData: Invalid parameters\n");
		return B_BAD_VALUE;
	}

	BFile file(filePath, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t initResult = file.InitCheck();
	if (initResult != B_OK) {
		printf("_SaveBinaryData: File init failed: %s\n", strerror(initResult));
		return initResult;
	}

	ssize_t bytesWritten = file.Write(data, size);
	if (bytesWritten != (ssize_t)size) {
		printf("_SaveBinaryData: Write failed - expected %d, wrote %d\n", (int)size, (int)bytesWritten);
		return B_ERROR;
	}

	BNodeInfo nodeInfo(&file);
	if (nodeInfo.InitCheck() == B_OK) {
		status_t mimeResult = nodeInfo.SetType(mime);
		if (mimeResult != B_OK) {
			printf("SaveFile: Warning - could not set MIME type: %s\n", strerror(mimeResult));
		}
	}

	return B_OK;
}

BString
SVGMainWindow::_ConvertToRDef(const unsigned char* data, size_t size)
{
	BString result;
	result << "resource(1) #'VICN' array {\n";

	for (size_t i = 0; i < size; i += 32) {
		result << "\t$\"";

		for (size_t j = i; j < i + 32 && j < size; j++) {
			BString hex;
			hex.SetToFormat("%02X", data[j]);
			result << hex;
		}

		result << "\"";
		if (i + 32 < size) {
			result << ",";
		}
		result << "\n";
	}

	result << "};";
	return result;
}

BString
SVGMainWindow::_ConvertToCPP(const unsigned char* data, size_t size)
{
	BString result;
	result << "const unsigned char kIconData[] = {\n";

	for (size_t i = 0; i < size; i++) {
		if (i % 16 == 0)
			result << "\t";

		BString hex;
		hex.SetToFormat("0x%02x", data[i]);
		result << hex;

		if (i < size - 1) {
			result << ",";
			if ((i + 1) % 16 == 0)
				result << "\n";
			else
				result << " ";
		}
	}

	result << "\n};\n";
	result << "\nconst size_t kIconDataSize = ";
	BString sizeStr;
	sizeStr.SetToFormat("%u", (unsigned int)size);
	result << sizeStr << ";";

	return result;
}

void
SVGMainWindow::_UpdateUIState()
{
	uint32 newState = _CalculateCurrentUIState();
	fCurrentUIState = newState;
	_UpdateToolBarStates();
	_UpdateMenuStates();
}

uint32
SVGMainWindow::_CalculateCurrentUIState() const
{
	uint32 state = UI_STATE_NO_DOCUMENT;

	if (!fCurrentFilePath.IsEmpty() || !fCurrentSource.IsEmpty())
		state |= UI_STATE_DOCUMENT_LOADED;

	if ((!fOriginalSourceText.IsEmpty() && fCurrentSource != fOriginalSourceText) || fDocumentModified)
		state |= UI_STATE_DOCUMENT_MODIFIED;

	if (_HasUnAppliedEditorChanges())
		state |= UI_STATE_HAS_UNAPPLIED_CHANGES;

	if (fCurrentHVIFData && fCurrentHVIFSize > 0)
		state |= UI_STATE_HAS_HVIF_DATA;

	if (fSplitView && !fSplitView->IsItemCollapsed(1))
		state |= UI_STATE_SOURCE_VIEW_VISIBLE;

	if (fFileManager && fFileManager->CanDirectSave(fCurrentFilePath))
		state |= UI_STATE_CAN_SAVE_DIRECT;

	if (fSVGTextView && (state & UI_STATE_SOURCE_VIEW_VISIBLE)) {
		if (fSVGTextView->CanUndo())
			state |= UI_STATE_CAN_UNDO;

		if (fSVGTextView->CanRedo())
			state |= UI_STATE_CAN_REDO;
	}

	if (fTextHasSelection)
		state |= UI_STATE_HAS_SELECTION;

	if (fClipboardHasData)
		state |= UI_STATE_HAS_CLIPBOARD_DATA;

	return state;
}

void
SVGMainWindow::_UpdateToolBarStates()
{
	bool hasDocument = (fCurrentUIState & UI_STATE_DOCUMENT_LOADED) != 0;
	bool isModified = (fCurrentUIState & UI_STATE_DOCUMENT_MODIFIED) != 0;
	bool canSaveDirect = (fCurrentUIState & UI_STATE_CAN_SAVE_DIRECT) != 0;
	bool sourceVisible = (fCurrentUIState & UI_STATE_SOURCE_VIEW_VISIBLE) != 0;
	bool hasSelection = (fCurrentUIState & UI_STATE_HAS_SELECTION) != 0;
	bool hasClipboardData = (fCurrentUIState & UI_STATE_HAS_CLIPBOARD_DATA) != 0;
	bool canUndo = (fCurrentUIState & UI_STATE_CAN_UNDO) != 0;
	bool canRedo = (fCurrentUIState & UI_STATE_CAN_REDO) != 0;
	bool hasUnappliedChanges = (fCurrentUIState & UI_STATE_HAS_UNAPPLIED_CHANGES) != 0;

	if (fToolBar) {
		_SetToolBarItemEnabled(fToolBar, MSG_SAVE_FILE, hasDocument && (isModified || fDocumentModified));
		_SetToolBarItemEnabled(fToolBar, MSG_ZOOM_IN, hasDocument);
		_SetToolBarItemEnabled(fToolBar, MSG_ZOOM_OUT, hasDocument);
		_SetToolBarItemEnabled(fToolBar, MSG_ZOOM_ORIGINAL, hasDocument);
		_SetToolBarItemEnabled(fToolBar, MSG_CENTER, hasDocument);
		_SetToolBarItemEnabled(fToolBar, MSG_FIT_WINDOW, hasDocument);
		_SetToolBarItemEnabled(fToolBar, MSG_TOGGLE_BOUNDINGBOX, hasDocument);
	}

	if (fEditToolBar) {
		_SetToolBarItemEnabled(fEditToolBar, B_UNDO, sourceVisible && canUndo);
		_SetToolBarItemEnabled(fEditToolBar, B_REDO, sourceVisible && canRedo);
		_SetToolBarItemEnabled(fEditToolBar, MSG_EDIT_COPY, sourceVisible && hasSelection);
		_SetToolBarItemEnabled(fEditToolBar, MSG_EDIT_CUT, sourceVisible && hasSelection);
		_SetToolBarItemEnabled(fEditToolBar, MSG_EDIT_PASTE, sourceVisible && hasClipboardData);
		_SetToolBarItemEnabled(fEditToolBar, MSG_EDIT_WORD_WRAP, sourceVisible);
		_SetToolBarItemEnabled(fEditToolBar, MSG_EDIT_APPLY, sourceVisible && hasDocument && hasUnappliedChanges);
	}

	_UpdateToggleButtonStates();
}

void
SVGMainWindow::_UpdateMenuStates()
{
	bool hasDocument = (fCurrentUIState & UI_STATE_DOCUMENT_LOADED) != 0;
	bool isModified = (fCurrentUIState & UI_STATE_DOCUMENT_MODIFIED) != 0;
	bool canSaveDirect = (fCurrentUIState & UI_STATE_CAN_SAVE_DIRECT) != 0;
	bool hasHVIF = (fCurrentUIState & UI_STATE_HAS_HVIF_DATA) != 0;
	bool sourceVisible = (fCurrentUIState & UI_STATE_SOURCE_VIEW_VISIBLE) != 0;

	if (fMenuManager) {
		fMenuManager->UpdateFileMenu(canSaveDirect, isModified || fDocumentModified);
		fMenuManager->UpdateExportMenu(hasHVIF);

		_SetMenuItemEnabled(MSG_ZOOM_IN, hasDocument);
		_SetMenuItemEnabled(MSG_ZOOM_OUT, hasDocument);
		_SetMenuItemEnabled(MSG_ZOOM_ORIGINAL, hasDocument);
		_SetMenuItemEnabled(MSG_FIT_WINDOW, hasDocument);
		_SetMenuItemEnabled(MSG_CENTER, hasDocument);
		_SetMenuItemEnabled(MSG_RESET_VIEW, hasDocument);
		_SetMenuItemEnabled(MSG_DISPLAY_NORMAL, hasDocument);
		_SetMenuItemEnabled(MSG_DISPLAY_OUTLINE, hasDocument);
		_SetMenuItemEnabled(MSG_DISPLAY_FILL_ONLY, hasDocument);
		_SetMenuItemEnabled(MSG_DISPLAY_STROKE_ONLY, hasDocument);
		_SetMenuItemEnabled(MSG_TOGGLE_TRANSPARENCY, hasDocument);
		_SetMenuItemEnabled(MSG_TOGGLE_BOUNDINGBOX, hasDocument);
		_SetMenuItemEnabled(MSG_BBOX_NONE, hasDocument);
		_SetMenuItemEnabled(MSG_BBOX_DOCUMENT, hasDocument);
		_SetMenuItemEnabled(MSG_BBOX_SIMPLE_FRAME, hasDocument);
		_SetMenuItemEnabled(MSG_BBOX_TRANSPARENT_GRAY, hasDocument);
		_SetMenuItemEnabled(MSG_RELOAD_FROM_SOURCE, sourceVisible && hasDocument);
	}
}

void
SVGMainWindow::_SetToolBarItemEnabled(SVGToolBar* toolbar, uint32 command, bool enabled)
{
	if (!toolbar)
		return;

	for (int32 i = 0; i < toolbar->CountChildren(); i++) {
		BView* child = toolbar->ChildAt(i);
		if (!child)
			continue;

		BControl* control = dynamic_cast<BControl*>(child);
		if (control) {
			BMessage* message = control->Message();
			if (message && message->what == command) {
				control->SetEnabled(enabled);
				break;
			}
		}
	}
}

void
SVGMainWindow::_SetMenuItemEnabled(uint32 command, bool enabled)
{
	if (!fMenuBar)
		return;

	BMenuItem* item = _FindMenuItem(fMenuBar, command);
	if (item)
		item->SetEnabled(enabled);
}

BMenuItem*
SVGMainWindow::_FindMenuItem(BMenu* menu, uint32 command)
{
	if (!menu)
		return NULL;

	for (int32 i = 0; i < menu->CountItems(); i++) {
		BMenuItem* item = menu->ItemAt(i);
		if (!item)
			continue;

		if (item->Command() == command)
			return item;

		if (item->Submenu()) {
			BMenuItem* found = _FindMenuItem(item->Submenu(), command);
			if (found)
				return found;
		}
	}

	return NULL;
}

void
SVGMainWindow::_StartStateMonitoring()
{
	BMessage updateMsg(MSG_STATE_UPDATE);
	fStateUpdateRunner = new BMessageRunner(BMessenger(this), &updateMsg, 500000, -1);
}

void
SVGMainWindow::_StopStateMonitoring()
{
	delete fStateUpdateRunner;
	fStateUpdateRunner = NULL;
}

void
SVGMainWindow::_CheckClipboardState()
{
	bool hasData = false;

	if (be_clipboard->Lock()) {
		BMessage* clipData = be_clipboard->Data();
		hasData = clipData && (clipData->HasString("text/plain") ||
			  clipData->HasData("text/plain", B_MIME_TYPE));
		be_clipboard->Unlock();
	}

	if (hasData != fClipboardHasData)
		fClipboardHasData = hasData;
}

void
SVGMainWindow::_CheckTextSelectionState()
{
	bool hasSelection = false;

	if (fSVGTextView && fSplitView && !fSplitView->IsItemCollapsed(1)) {
		int32 selStart, selEnd;
		fSVGTextView->GetSelection(&selStart, &selEnd);
		hasSelection = (selStart != selEnd);
	}

	if (hasSelection != fTextHasSelection)
		fTextHasSelection = hasSelection;
}

void
SVGMainWindow::_OnTextModified()
{
	_UpdateUIState();
}

void
SVGMainWindow::_OnSelectionChanged()
{
	_CheckTextSelectionState();
	_UpdateUIState();
}

void
SVGMainWindow::_SetToolBarButtonPressed(SVGToolBar* toolbar, uint32 command, bool pressed)
{
	if (!toolbar)
		return;

	for (int32 i = 0; i < toolbar->CountChildren(); i++) {
		BView* child = toolbar->ChildAt(i);
		if (!child)
			continue;

		BControl* control = dynamic_cast<BControl*>(child);
		if (control) {
			BMessage* message = control->Message();
			if (message && message->what == command) {
				if (control->Value() != (pressed ? B_CONTROL_ON : B_CONTROL_OFF)) {
					control->SetValue(pressed ? B_CONTROL_ON : B_CONTROL_OFF);
					control->Invalidate();
				}
				break;
			}
		}
	}
}

void
SVGMainWindow::_UpdateToggleButtonStates()
{
	if (fToolBar) {
		_SetToolBarButtonPressed(fToolBar, MSG_TOGGLE_SOURCE_VIEW, fShowSourceView);

		bool boundingBoxVisible = fSVGView && (fSVGView->BoundingBoxStyle() != SVG_BBOX_NONE);
		_SetToolBarButtonPressed(fToolBar, MSG_TOGGLE_BOUNDINGBOX, boundingBoxVisible);

		_SetToolBarButtonPressed(fToolBar, MSG_TOGGLE_STAT, fShowStatView);
		_SetToolBarButtonPressed(fToolBar, MSG_TOGGLE_STRUCTURE, fShowStructureView);
	}

	if (fEditToolBar && fSVGTextView) {
		bool wordWrapEnabled = fSVGTextView->DoesWordWrap();
		_SetToolBarButtonPressed(fEditToolBar, MSG_EDIT_WORD_WRAP, wordWrapEnabled);
	}
}

void
SVGMainWindow::_SaveSettings()
{
	if (!gSettings)
		return;

	gSettings->SetRect(kWindowFrame, Frame());

	if (fSplitView) {
		gSettings->SetBool(kSourceViewCollapsed, !fShowSourceView);
		if (fShowSourceView) {
			gSettings->SetFloat(kMainViewWeight, fSplitView->ItemWeight(0));
			gSettings->SetFloat(kSourceViewWeight, fSplitView->ItemWeight(1));
		}
	}

	if (fSVGView) {
		gSettings->SetInt32(kDisplayMode, (int32)fSVGView->DisplayMode());
		gSettings->SetBool(kShowTransparency, fSVGView->ShowTransparency());
		gSettings->SetBool(kShowBoundingBox, fShowBoundingBox);
		gSettings->SetInt32(kBoundingBoxStyle, fBoundingBoxStyle);
	}

	// Сохранение состояния панелей
	gSettings->SetBool(kShowStructureView, fShowStructureView);
	gSettings->SetBool(kShowStatView, fShowStatView);
	gSettings->SetBool(kShowSourceView, fShowSourceView);

	if (fSVGTextView) {
		gSettings->SetBool(kWordWrap, fSVGTextView->DoesWordWrap());
	}

	gSettings->Save();
}

void
SVGMainWindow::_RestoreSettings()
{
	if (!gSettings)
		return;

	// Восстановление состояния исходного кода
	if (fSplitView) {
		fShowSourceView = !gSettings->GetBool(kSourceViewCollapsed, true);
		fSplitView->SetItemCollapsed(1, !fShowSourceView);

		if (fShowSourceView) {
			float mainWeight = gSettings->GetFloat(kMainViewWeight, 0.7f);
			float sourceWeight = gSettings->GetFloat(kSourceViewWeight, 0.3f);
			fSplitView->SetItemWeight(0, mainWeight, false);
			fSplitView->SetItemWeight(1, sourceWeight, false);
		}
	}

	if (fSVGView) {
		int32 displayMode = gSettings->GetInt32(kDisplayMode, 0);
		fSVGView->SetDisplayMode((svg_display_mode)displayMode);

		bool showTransparency = gSettings->GetBool(kShowTransparency, true);
		fSVGView->SetShowTransparency(showTransparency);

		fShowBoundingBox = gSettings->GetBool(kShowBoundingBox, false);
		fBoundingBoxStyle = gSettings->GetInt32(kBoundingBoxStyle, 1);
		fSVGView->SetBoundingBoxStyle(fShowBoundingBox ? (svg_boundingbox_style)fBoundingBoxStyle : SVG_BBOX_NONE);
	}

	// Восстановление состояния панели статистики
	if (fStatView && fViewerContainer) {
		fShowStatView = gSettings->GetBool(kShowStatView, false);
		BLayoutItem* statItem = fViewerContainer->GroupLayout()->ItemAt(2);
		if (statItem) {
			statItem->SetVisible(fShowStatView);
		}
	}

	// Восстановление состояния панели структуры
	if (fStructureView && fViewerContainer) {
		fShowStructureView = gSettings->GetBool(kShowStructureView, false);
		BLayoutItem* structureItem = fViewerContainer->GroupLayout()->ItemAt(0);
		if (structureItem) {
			structureItem->SetVisible(fShowStructureView);
		}
	}

	if (fSVGTextView) {
		bool wordWrap = gSettings->GetBool(kWordWrap, true);
		fSVGTextView->SetWordWrap(wordWrap);
		if (fRDefTextView)
			fRDefTextView->SetWordWrap(wordWrap);
		if (fCPPTextView)
			fCPPTextView->SetWordWrap(wordWrap);
	}

	BString lastOpenPath = gSettings->GetString(kLastOpenPath);
	if (!lastOpenPath.IsEmpty() && fFileManager) {
		BFilePanel* openPanel = fFileManager->GetOpenPanel();
		if (openPanel) {
			entry_ref ref;
			if (get_ref_for_path(lastOpenPath.String(), &ref) == B_OK) {
				openPanel->SetPanelDirectory(&ref);
			}
		}
	}

	BString lastSavePath = gSettings->GetString(kLastSavePath);
	if (!lastSavePath.IsEmpty() && fFileManager) {
		BFilePanel* savePanel = fFileManager->GetSavePanel();
		if (savePanel) {
			entry_ref ref;
			if (get_ref_for_path(lastSavePath.String(), &ref) == B_OK) {
				savePanel->SetPanelDirectory(&ref);
			}
		}
	}

	_UpdateInterface();
	_UpdateUIState();
}

bool
SVGMainWindow::_HasUnAppliedEditorChanges() const
{
	if (!fSVGTextView || fSplitView->IsItemCollapsed(1))
		return false;

	BString editorText = fSVGTextView->Text();
	return (editorText != fCurrentSource);
}

void
SVGMainWindow::_UpdateStructureView()
{
	if (fStructureView) {
		fStructureView->SetSVGImage(fSVGView->SVGImage());
	}
}
