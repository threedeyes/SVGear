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

#include <private/interface/AboutWindow.h>

#include "SVGMainWindow.h"
#include "SVGMenuManager.h"
#include "SVGFileManager.h"
#include "SVGView.h"
#include "SVGHVIFView.h"
#include "SVGTextEdit.h"
#include "SVGToolBar.h"
#include "SVGApplication.h"

#include "HVIFParser.h"
#include "SVGRenderer.h"

SVGMainWindow::SVGMainWindow(const char* filePath)
    : BWindow(BRect(100, 100, 1200, 800), "SVGear", B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS),
      fSVGView(NULL),
      fSourceView(NULL),
      fIconView(NULL),
      fMenuManager(NULL),
      fFileManager(NULL),
      fMenuBar(NULL),
      fMenuContainer(NULL),
      fEditorContainer(NULL),
      fStatusView(NULL),
      fSplitView(NULL),
      fSourceScrollView(NULL),
      fToolBar(NULL),
      fEditToolBar(NULL),
      fDocumentModified(false)
{
    fMenuManager = new SVGMenuManager();
    fFileManager = new SVGFileManager();
    
    _BuildInterface();

    if (filePath) {
        LoadFile(filePath);
    }

    if (fSVGView) {
        fSVGView->SetTarget(this);
    }
    
    _UpdateStatus();
}

SVGMainWindow::~SVGMainWindow()
{
    delete fMenuManager;
    delete fFileManager;
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
    fToolBar->AddAction(MSG_NEW_FILE, this, SVGApplication::GetIcon("document-new", TOOLBAR_ICON_SIZE), "New");
    fToolBar->AddAction(MSG_OPEN_FILE, this, SVGApplication::GetIcon("document-open", TOOLBAR_ICON_SIZE), "Open");
    fToolBar->AddAction(MSG_SAVE_FILE, this, SVGApplication::GetIcon("document-save", TOOLBAR_ICON_SIZE), "Save");
    fToolBar->AddSeparator();
    fToolBar->AddAction(MSG_ZOOM_IN, this, SVGApplication::GetIcon("zoom-in", TOOLBAR_ICON_SIZE), "Zoom in");
    fToolBar->AddAction(MSG_ZOOM_OUT, this, SVGApplication::GetIcon("zoom-out", TOOLBAR_ICON_SIZE), "Zoom out");
    fToolBar->AddAction(MSG_ZOOM_ORIGINAL, this, SVGApplication::GetIcon("zoom-original", TOOLBAR_ICON_SIZE), "Zoom original");
    fToolBar->AddAction(MSG_FIT_WINDOW, this, SVGApplication::GetIcon("zoom-fit-best", TOOLBAR_ICON_SIZE), "Best fit");
    fToolBar->AddSeparator();
    fToolBar->AddAction(MSG_TOGGLE_SOURCE_VIEW, this, SVGApplication::GetIcon("format-text-code", TOOLBAR_ICON_SIZE), "Show Source Code");
    fToolBar->AddGlue();
    
    fEditToolBar = new SVGToolBar();
    fEditToolBar->AddAction(B_UNDO, this, SVGApplication::GetIcon("edit-undo", TOOLBAR_ICON_SIZE), "Undo");
    fEditToolBar->AddSeparator();
    fEditToolBar->AddAction(MSG_EDIT_COPY, this, SVGApplication::GetIcon("edit-copy", TOOLBAR_ICON_SIZE), "Copy");
    fEditToolBar->AddAction(MSG_EDIT_PASTE, this, SVGApplication::GetIcon("edit-paste", TOOLBAR_ICON_SIZE), "Paste");
    fEditToolBar->AddAction(MSG_EDIT_CUT, this, SVGApplication::GetIcon("edit-cut", TOOLBAR_ICON_SIZE), "Cut");
    fEditToolBar->AddSeparator();
    fEditToolBar->AddAction(MSG_EDIT_WORD_WRAP, this, SVGApplication::GetIcon("text-wrap", TOOLBAR_ICON_SIZE), "Text wrap");
    fEditToolBar->AddSeparator();
    fEditToolBar->AddAction(MSG_EDIT_APPLY, this, SVGApplication::GetIcon("dialog-ok-apply", TOOLBAR_ICON_SIZE), "Apply");
    fEditToolBar->AddGlue();
}

void
SVGMainWindow::_BuildMainView()
{
    fSVGView = new SVGView("svg_view");

    fSourceView = new SVGTextEdit("source_view");
    fSourceView->SetWordWrap(true);
    fSourceScrollView = new BScrollView("source_scroll", fSourceView,
                      B_WILL_DRAW | B_FRAME_EVENTS,
                      true, true, B_NO_BORDER);

    fEditorContainer = new BGroupView(B_VERTICAL, 0);
    fEditorContainer->GroupLayout()->AddView(fEditToolBar);
    fEditorContainer->GroupLayout()->AddView(fSourceScrollView);

    fSplitView = new BSplitView(B_VERTICAL);
    fSplitView->AddChild(fSVGView);
    fSplitView->AddChild(fEditorContainer);
    fSplitView->SetCollapsible(1, true);
    fSplitView->SetCollapsible(0, false);
    fSplitView->SetItemCollapsed(1, true);
}

void
SVGMainWindow::_BuildStatusBar()
{
    fStatusView = new BStringView("status", "Ready");
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

    switch (message->what) {
        case MSG_NEW_FILE:
        case MSG_OPEN_FILE:
        case MSG_SAVE_FILE:
        case MSG_SAVE_AS_FILE:
        case MSG_SAVE_PANEL_SAVE:
        case B_REFS_RECEIVED:
        case B_SAVE_REQUESTED:  // Добавить эту строку
            _HandleFileMessages(message);
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
        case MSG_TOGGLE_SOURCE_VIEW:
            _HandleViewMessages(message);
            break;
            
        case MSG_EDIT_COPY:
        case MSG_EDIT_PASTE:
        case MSG_EDIT_CUT:
        case MSG_EDIT_APPLY:
        case MSG_EDIT_WORD_WRAP:
        case MSG_RELOAD_FROM_SOURCE:
        case B_UNDO:
            _HandleEditMessages(message);
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
        case MSG_SAVE_PANEL_SAVE:  // Обрабатываем оба сообщения
            _HandleSavePanel(message);
            break;
    }
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
            
        case MSG_TOGGLE_SOURCE_VIEW:
            _ToggleSourceView();
            break;
    }
}

void
SVGMainWindow::_HandleEditMessages(BMessage* message)
{
    switch (message->what) {
        case B_UNDO:
            if (fSourceView && !fSplitView->IsItemCollapsed(1)) {
                fSourceView->Undo(be_clipboard);
            }
            break;
            
        case MSG_EDIT_COPY:
            if (fSourceView && !fSplitView->IsItemCollapsed(1)) {
                fSourceView->Copy(be_clipboard);
            }
            break;
            
        case MSG_EDIT_PASTE:
            if (fSourceView && !fSplitView->IsItemCollapsed(1)) {
                fSourceView->Paste(be_clipboard);
            }
            break;
            
        case MSG_EDIT_CUT:
            if (fSourceView && !fSplitView->IsItemCollapsed(1)) {
                fSourceView->Cut(be_clipboard);
            }
            break;
            
        case MSG_EDIT_WORD_WRAP:
            if (fSourceView && !fSplitView->IsItemCollapsed(1)) {
                fSourceView->SetWordWrap(!fSourceView->DoesWordWrap());
            }
            break;
            
        case MSG_EDIT_APPLY:
        case MSG_RELOAD_FROM_SOURCE:
            _ReloadFromSource();
            break;
    }
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
        
        if (fIconView) {
            fIconView->SetIcon(parser.GetIconData(), parser.GetIconDataSize());
        }
        
        const hvif::HVIFIcon& icon = parser.GetIcon();
        hvif::SVGRenderer renderer;
        std::string svg = renderer.RenderIcon(icon, 64, 64);
        fCurrentSource.SetTo(svg.c_str());

        _UpdateStatus();
        _UpdateSourceView();
        _ReloadFromSource();

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
    
    // Сбрасываем путь для нового файла
    fCurrentFilePath = "";
    fDocumentModified = false;
    if (fFileManager) {
        fFileManager->SetLastLoadedFileType(FILE_TYPE_NEW);
    }
    _UpdateFileMenu();
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
    char* data = (char*)res.LoadResource('rSTL', resourceName, &size);
    if (data == NULL || size <= 0)
        return;
        
    data[size] = 0;
    fCurrentSource.SetTo(data);

    if (fSVGView) {
        fSVGView->LoadFromMemory(fCurrentSource.String());
    }

    _UpdateStatus();
    _UpdateSourceView();

    BString windowTitle("SVGear - ");
    windowTitle << title;
    SetTitle(windowTitle.String());
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
        
        fSVGView->ResetView();
        fCurrentFilePath = filePath;
        fDocumentModified = false;
        
        _UpdateStatus();
        _UpdateSourceView();
        _ReloadFromSource();
        _UpdateFileMenu();
    }
}

void
SVGMainWindow::_UpdateStatus()
{
    BString status;

    if (fSVGView && fSVGView->IsLoaded()) {
        float scale = fSVGView->Scale();
        float width = fSVGView->SVGWidth();
        float height = fSVGView->SVGHeight();

        status.SetToFormat(" Size: %.0fx%.0f | Scale: %.1f%% | Mode: %s",
                width, height, scale * 100.0f, _GetDisplayModeName().String());
    } else {
        status = "No SVG loaded";
    }

    if (fStatusView) {
        fStatusView->SetText(status.String());
    }
}

void
SVGMainWindow::_UpdateInterface()
{
    _UpdateDisplayModeMenu();
    _UpdateViewMenu();
    _UpdateStatus();
}

void
SVGMainWindow::_UpdateDisplayModeMenu()
{
    if (fMenuManager && fSVGView) {
        fMenuManager->UpdateDisplayMode(fSVGView->DisplayMode());
    }
}

void
SVGMainWindow::_UpdateViewMenu()
{
    if (fMenuManager && fSVGView && fSplitView) {
        bool showTransparency = fSVGView->ShowTransparency();
        bool showSource = !fSplitView->IsItemCollapsed(1);
        fMenuManager->UpdateViewOptions(showTransparency, showSource);
    }
}

BString
SVGMainWindow::_GetDisplayModeName() const
{
    if (!fSVGView)
        return "Unknown";

    switch (fSVGView->DisplayMode()) {
        case SVG_DISPLAY_NORMAL:
            return "Normal";
        case SVG_DISPLAY_OUTLINE:
            return "Outline";
        case SVG_DISPLAY_FILL_ONLY:
            return "Fill Only";
        case SVG_DISPLAY_STROKE_ONLY:
            return "Stroke Only";
        default:
            return "Unknown";
    }
}

void
SVGMainWindow::_ShowAbout()
{
    BAboutWindow* about = new BAboutWindow("SVGear", APP_SIGNATURE);
    about->AddCopyright(2025, "Gerasim Troeglazov (3dEyes**)");
    about->AddDescription(
        "SVGear provides an intuitive interface for viewing and manipulating "
        "SVG (Scalable Vector Graphics) files. \n"
        "The application supports format conversion operations, enabling "
        "users to transform SVG files into other vector formats such as "
        "HVIF (Haiku Vector Icon Format).");
    about->Show();
}

void
SVGMainWindow::_ToggleSourceView()
{
    if (!fSplitView)
        return;

    fSplitView->SetItemCollapsed(1, !fSplitView->IsItemCollapsed(1));

    if (!fSplitView->IsItemCollapsed(1)) {
        _UpdateSourceView();
        fSplitView->SetItemWeight(0, MAIN_VIEW_WEIGHT, false);
        fSplitView->SetItemWeight(1, SOURCE_VIEW_WEIGHT, false);
    }

    _UpdateViewMenu();
    _UpdateStatus();
}

void
SVGMainWindow::_UpdateSourceView()
{
    if (fSourceView && !fCurrentSource.IsEmpty()) {
        fSourceView->SetText(fCurrentSource.String());
        fSourceView->ApplySyntaxHighlighting();
    }
}

void
SVGMainWindow::_ReloadFromSource()
{
    if (!fSourceView || !fSVGView)
        return;

    BString sourceText = fSourceView->Text();
    if (sourceText.IsEmpty()) {
        _ShowError(ERROR_SOURCE_EMPTY);
        return;
    }

    status_t result = fSVGView->LoadFromMemory(sourceText.String());
    if (result != B_OK) {
        _ShowError(ERROR_PARSING_SVG);
    } else {
        // Проверяем, изменился ли документ
        if (fCurrentSource != sourceText) {
            fDocumentModified = true;
            fCurrentSource = sourceText;
        }
        _UpdateStatus();
        _UpdateFileMenu();
    }
}

void
SVGMainWindow::_ShowError(const char* message)
{
    BAlert* alert = new BAlert("Error", message, "OK", NULL, NULL,
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

    // Проверяем, можем ли мы сохранить напрямую
    if (fFileManager->CanDirectSave(fCurrentFilePath)) {
        // Прямое сохранение SVG файла
        if (fFileManager->SaveCurrentFile(fCurrentFilePath, currentSource)) {
            fDocumentModified = false;
            _ShowSuccess(MSG_FILE_SAVED);
            _UpdateFileMenu();
        }
    } else {
        // Если это новый файл, HVIF или из атрибутов - предлагаем Save As
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
    // Добавим отладочную информацию
    printf("_HandleSavePanel called\n");
    message->PrintToStream();
    
    entry_ref dirRef;
    BString fileName;
    
    if (message->FindRef("directory", &dirRef) != B_OK) {
        printf("Error: Could not find directory ref\n");
        _ShowError("Could not find directory reference");
        return;
    }
    
    if (message->FindString("name", &fileName) != B_OK) {
        printf("Error: Could not find file name\n");
        _ShowError("Could not find file name");
        return;
    }

    printf("Directory found, file name: %s\n", fileName.String());

    BDirectory dir(&dirRef);
    if (dir.InitCheck() != B_OK) {
        printf("Error: Directory init failed\n");
        _ShowError(ERROR_INVALID_PATH);
        return;
    }

    BPath dirPath(&dirRef);
    if (dirPath.InitCheck() != B_OK) {
        printf("Error: Could not get directory path\n");
        _ShowError(ERROR_INVALID_PATH);
        return;
    }
    
    BString fullPath = dirPath.Path();
    fullPath << "/" << fileName;

    // Убедимся, что файл имеет расширение .svg
    if (!fileName.EndsWith(".svg") && !fileName.EndsWith(".svgz")) {
        fullPath << ".svg";
    }

    printf("Full path: %s\n", fullPath.String());

    BString currentSource = _GetCurrentSource();
    if (currentSource.IsEmpty()) {
        printf("Error: Current source is empty\n");
        _ShowError(ERROR_SOURCE_EMPTY);
        return;
    }

    printf("Attempting to save file...\n");
    status_t result = fFileManager->SaveFile(fullPath.String(), currentSource);
    
    if (result == B_OK) {
        printf("File saved successfully\n");
        // Обновляем состояние после успешного сохранения
        fCurrentFilePath = fullPath;
        fDocumentModified = false;
        if (fFileManager) {
            fFileManager->SetLastLoadedFileType(FILE_TYPE_SVG);
        }
        
        _UpdateTitleAfterSave(fullPath.String());
        _ShowSuccess(MSG_FILE_SAVED);
        _UpdateFileMenu();
    } else {
        printf("Error saving file: %s\n", strerror(result));
        BString error;
        error.SetToFormat("%s: %s", ERROR_SAVE_FAILED, strerror(result));
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
    if (fSourceView && !fSplitView->IsItemCollapsed(1)) {
        // Если редактор источников открыт, берем текст оттуда
        return BString(fSourceView->Text());
    } else {
        // Иначе используем сохраненный источник
        return fCurrentSource;
    }
}

void
SVGMainWindow::_ShowSuccess(const char* message)
{
    // Можно показать в статус-баре или как уведомление
    if (fStatusView) {
        BString status = fStatusView->Text();
        fStatusView->SetText(message);
        
        // Восстановить обычный статус через 3 секунды
        BMessage restoreMsg(MSG_SVG_STATUS_UPDATE);
        BMessageRunner* runner = new BMessageRunner(this, &restoreMsg, 3000000, 1);
    }
}

void
SVGMainWindow::_UpdateFileMenu()
{
    // Обновляем состояние меню в зависимости от возможности прямого сохранения
    if (fMenuManager) {
        // TODO: Добавить метод в SVGMenuManager для обновления состояния Save/Save As
    }
}
