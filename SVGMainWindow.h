/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SVG_MAIN_WINDOW_H
#define SVG_MAIN_WINDOW_H

#include <Window.h>
#include <String.h>
#include <StringView.h>
#include <SplitView.h>
#include <GroupView.h>
#include <ScrollView.h>
#include <TabView.h>
#include <FilePanel.h>
#include <MenuItem.h>
#include <Menu.h>
#include <MessageRunner.h>

#include "SVGConstants.h"
#include "SVGStatView.h"
#include "SVGStructureView.h"
#include "BSVGView.h"

class SVGView;
class SVGTextEdit;
class HVIFView;
class SVGMenuManager;
class SVGFileManager;
class SVGToolBar;
class BMenuBar;
class BTextView;

class SVGMainWindow : public BWindow {
public:
	SVGMainWindow(const char* filePath = NULL);
	virtual ~SVGMainWindow();

	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage* message);

	void LoadFile(const char* filePath);
	bool IsLoaded() const { return !fCurrentFilePath.IsEmpty(); }

	enum ui_state {
		UI_STATE_NO_DOCUMENT = 0,
		UI_STATE_DOCUMENT_LOADED = 1,
		UI_STATE_DOCUMENT_MODIFIED = 2,
		UI_STATE_HAS_HVIF_DATA = 4,
		UI_STATE_SOURCE_VIEW_VISIBLE = 8,
		UI_STATE_HAS_SELECTION = 16,
		UI_STATE_CAN_UNDO = 32,
		UI_STATE_CAN_REDO = 64,
		UI_STATE_CAN_SAVE_DIRECT = 128,
		UI_STATE_HAS_CLIPBOARD_DATA = 256,
		UI_STATE_HAS_UNAPPLIED_CHANGES = 512
	};

private:
	// UI Building
	void _BuildInterface();
	void _BuildToolBars();
	void _BuildMainView();
	void _BuildTabView();
	void _BuildStatusBar();

	// Message handlers
	void _HandleFileMessages(BMessage* message);
	void _HandleViewMessages(BMessage* message);
	void _HandleEditMessages(BMessage* message);
	void _HandleDropMessages(BMessage* message);
	void _HandleExportMessages(BMessage* message);

	// File operations
	void _LoadNewFile();
	void _LoadTemplateFile(const char* resourceName, const char* title);
	void _HandleRefsReceived(BMessage* message);
	void _SaveFile();
	void _SaveAsFile();
	void _HandleSavePanel(BMessage* message);
	bool _UpdateTitleAfterSave(const char* filePath);

	// Data generation
	void _GenerateHVIFFromSVG();

	// Tab management
	void _UpdateAllTabs();
	void _UpdateRDefTab();
	void _UpdateCPPTab();
	void _HandleTabSelection();

	// View management
	void _ToggleSourceView();
	void _ToggleStructureView();
	void _ToggleStatView();
	void _ReloadFromSource();
	void _UpdateStructureView();

	// UI Updates
	void _UpdateStatus();
	void _UpdateInterface();
	void _UpdateStatView();
	void _UpdateDisplayModeMenu();
	void _UpdateBoundingBoxMenu();
	void _UpdateViewMenu();

	// UI State management
	void _UpdateUIState();
	void _UpdateToolBarStates();
	void _UpdateMenuStates();
	void _UpdateToggleButtonStates();
	uint32 _CalculateCurrentUIState() const;
	void _SetToolBarItemEnabled(SVGToolBar* toolbar, uint32 command, bool enabled);
	void _SetToolBarButtonPressed(SVGToolBar* toolbar, uint32 command, bool pressed);
	void _SetMenuItemEnabled(uint32 command, bool enabled);
	BMenuItem* _FindMenuItem(BMenu* menu, uint32 command);
	bool _HasUnAppliedEditorChanges() const;

	// State monitoring
	void _StartStateMonitoring();
	void _StopStateMonitoring();
	void _CheckClipboardState();
	void _CheckTextSelectionState();
	void _OnTextModified();
	void _OnSelectionChanged();

	// Settings management
	void _SaveSettings();
	void _RestoreSettings();

	// Utility functions
	BString _GetDisplayModeName() const;
	BString _GetBoundingBoxStyleName() const;
	void _ShowAbout();
	void _ShowError(const char* message);
	void _ShowSuccess(const char* message);
	BString _GetCurrentSource() const;

private:
	// Core components
	SVGView*         fSVGView;
	HVIFView*        fIconView;

	// Tab components
	BTabView*        fTabView;
	SVGTextEdit*     fSVGTextView;
	SVGTextEdit*     fRDefTextView;
	SVGTextEdit*     fCPPTextView;
	BScrollView*     fSVGScrollView;
	BScrollView*     fRDefScrollView;
	BScrollView*     fCPPScrollView;

	// Managers
	SVGMenuManager*  fMenuManager;
	SVGFileManager*  fFileManager;

	// UI elements
	BMenuBar*        fMenuBar;
	BGroupView*      fMenuContainer;
	BGroupView*      fEditorContainer;
	BGroupView*      fViewerContainer;
	BStringView*     fStatusView;
	BSplitView*      fSplitView;
	SVGToolBar*      fToolBar;
	SVGToolBar*      fEditToolBar;
	SVGStatView*     fStatView;
	SVGStructureView* fStructureView;

	// Document state
	BString          fCurrentSource;
	BString          fCurrentFilePath;
	BString          fOriginalSourceText;
	bool             fDocumentModified;

	// View state
	bool             fShowStatView;
	bool             fShowStructureView;
	bool             fShowSourceView;
	bool             fShowBoundingBox;
	int32            fBoundingBoxStyle;

	// UI state
	uint32           fCurrentUIState;

	// State monitoring
	BMessageRunner*  fStateUpdateRunner;
	bool             fClipboardHasData;
	bool             fTextHasSelection;

	// HVIF data for export
	unsigned char*   fCurrentHVIFData;
	size_t           fCurrentHVIFSize;
};

#endif
