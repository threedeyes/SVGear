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

#include "SVGConstants.h"
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

    void LoadFile(const char* filePath);
    bool IsLoaded() const { return !fCurrentFilePath.IsEmpty(); }

    virtual bool QuitRequested();
    virtual void MessageReceived(BMessage* message);

private:
    // UI Building
    void _BuildInterface();
    void _BuildToolBars();
    void _BuildMainView();
    void _BuildStatusBar();
    void _BuildTabView();

    // Message handlers
    void _HandleFileMessages(BMessage* message);
    void _HandleViewMessages(BMessage* message);
    void _HandleEditMessages(BMessage* message);
    void _HandleDropMessages(BMessage* message);
    void _HandleExportMessages(BMessage* message);
    void _HandleExportSavePanel(BMessage* message);

    // File operations
    void _LoadNewFile();
    void _LoadTemplateFile(const char* resourceName, const char* title);
    void _HandleRefsReceived(BMessage* message);
    void _SaveFile();
    void _SaveAsFile();
    void _HandleSavePanel(BMessage* message);
    bool _UpdateTitleAfterSave(const char* filePath);

    // Export operations
    void _ExportHVIF();
    void _ExportRDef();
    void _ExportCPP();
    status_t _SaveBinaryData(const char* filePath, const unsigned char* data, size_t size, const char* mime);
    void _ShowExportPanel(const char* defaultName, const char* extension, uint32 exportType);

    // UI Updates
    void _UpdateStatus();
    void _UpdateInterface();
    void _UpdateDisplayModeMenu();
    void _UpdateViewMenu();
    void _UpdateFileMenu();

    // Tab management
    void _UpdateAllTabs();
    void _UpdateRDefTab();
    void _UpdateCPPTab();
    void _HandleTabSelection();

    // Source view management
    void _ToggleSourceView();
    void _ReloadFromSource();

    // Data conversion and generation
    void _GenerateHVIFFromSVG();
    BString _ConvertToRDef(const unsigned char* data, size_t size);
    BString _ConvertToCPP(const unsigned char* data, size_t size);

    // Utility functions
    BString _GetDisplayModeName() const;
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
    BTextView*       fRDefTextView;
    BTextView*       fCPPTextView;
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
    BStringView*     fStatusView;
    BSplitView*      fSplitView;
    SVGToolBar*      fToolBar;
    SVGToolBar*      fEditToolBar;

    // State
    BString          fCurrentSource;
    BString          fCurrentFilePath;
    bool             fDocumentModified;

    // Current HVIF data for export
    unsigned char*   fCurrentHVIFData;
    size_t           fCurrentHVIFSize;

    // Export panel
    BFilePanel*      fExportPanel;
    uint32           fCurrentExportType;
};

#endif
