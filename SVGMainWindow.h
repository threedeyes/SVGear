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

#include "SVGConstants.h"
#include "BSVGView.h"

class SVGView;
class SVGTextEdit;
class HVIFView;
class SVGMenuManager;
class SVGFileManager;
class SVGToolBar;
class BMenuBar;

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
    
    // Message handlers
    void _HandleFileMessages(BMessage* message);
    void _HandleViewMessages(BMessage* message);
    void _HandleEditMessages(BMessage* message);
    void _HandleDropMessages(BMessage* message);
    void _HandleSaveMessages(BMessage* message);
    
    // File operations
    void _LoadNewFile();
    void _LoadTemplateFile(const char* resourceName, const char* title);
    void _HandleRefsReceived(BMessage* message);
    void _SaveFile();
    void _SaveAsFile();
    void _HandleSavePanel(BMessage* message);
    bool _UpdateTitleAfterSave(const char* filePath);
    
    // UI Updates
    void _UpdateStatus();
    void _UpdateInterface();
    void _UpdateDisplayModeMenu();
    void _UpdateViewMenu();
    void _UpdateFileMenu();
    
    // Source view management
    void _ToggleSourceView();
    void _UpdateSourceView();
    void _ReloadFromSource();
    
    // Utility functions
    BString _GetDisplayModeName() const;
    void _ShowAbout();
    void _ShowError(const char* message);
    void _ShowSuccess(const char* message);
    BString _GetCurrentSource() const;

private:
    // Core components
    SVGView*         fSVGView;
    SVGTextEdit*     fSourceView;
    HVIFView*        fIconView;
    
    // Managers
    SVGMenuManager*  fMenuManager;
    SVGFileManager*  fFileManager;
    
    // UI elements
    BMenuBar*        fMenuBar;
    BGroupView*      fMenuContainer;
    BGroupView*      fEditorContainer;
    BStringView*     fStatusView;
    BSplitView*      fSplitView;
    BScrollView*     fSourceScrollView;
    SVGToolBar*      fToolBar;
    SVGToolBar*      fEditToolBar;
    
    // State
    BString          fCurrentSource;
    BString          fCurrentFilePath;
    bool             fDocumentModified;
};

#endif
