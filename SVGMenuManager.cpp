/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "SVGMenuManager.h"
#include <MenuItem.h>

SVGMenuManager::SVGMenuManager()
    : fMenuBar(NULL),
      fNormalItem(NULL),
      fOutlineItem(NULL),
      fFillOnlyItem(NULL),
      fStrokeOnlyItem(NULL),
      fTransparencyItem(NULL),
      fSourceViewItem(NULL)
{
}

SVGMenuManager::~SVGMenuManager()
{
    // BMenuBar owns the menu items, so we don't need to delete them
}

BMenuBar*
SVGMenuManager::CreateMenuBar(BHandler* target)
{
    fMenuBar = new BMenuBar("menubar");

    _CreateFileMenu(target);
    _CreateViewMenu(target);
    _CreateDisplayMenu(target);
    _CreateHelpMenu(target);
    _AddShortcuts(target);

    return fMenuBar;
}

void
SVGMenuManager::_CreateViewMenu(BHandler* target)
{
    BMenu* viewMenu = new BMenu("View");
    viewMenu->AddItem(new BMenuItem("Zoom In", new BMessage(MSG_ZOOM_IN), '+'));
    viewMenu->AddItem(new BMenuItem("Zoom Out", new BMessage(MSG_ZOOM_OUT), '-'));
    viewMenu->AddItem(new BMenuItem("Zoom Original", new BMessage(MSG_ZOOM_ORIGINAL), '1'));
    viewMenu->AddItem(new BMenuItem("Fit to Window", new BMessage(MSG_FIT_WINDOW), 'F'));
    viewMenu->AddSeparatorItem();
    viewMenu->AddItem(new BMenuItem("Center", new BMessage(MSG_CENTER), 'C', B_SHIFT_KEY));
    viewMenu->AddSeparatorItem();
    viewMenu->AddItem(new BMenuItem("Reset View", new BMessage(MSG_RESET_VIEW), '0'));
    viewMenu->AddSeparatorItem();
    
    fTransparencyItem = new BMenuItem("Show Transparency Grid", new BMessage(MSG_TOGGLE_TRANSPARENCY), 'G');
    fTransparencyItem->SetMarked(true);
    viewMenu->AddItem(fTransparencyItem);
    viewMenu->AddSeparatorItem();
    
    fSourceViewItem = new BMenuItem("Show Source Code", new BMessage(MSG_TOGGLE_SOURCE_VIEW), 'S');
    viewMenu->AddItem(fSourceViewItem);
    viewMenu->AddItem(new BMenuItem("Reload from Source", new BMessage(MSG_RELOAD_FROM_SOURCE), 'R'));
    
    viewMenu->SetTargetForItems(target);
    fMenuBar->AddItem(viewMenu);
}

void
SVGMenuManager::_CreateDisplayMenu(BHandler* target)
{
    BMenu* displayMenu = new BMenu("Display");
    
    fNormalItem = new BMenuItem("Normal", new BMessage(MSG_DISPLAY_NORMAL));
    fNormalItem->SetMarked(true);
    displayMenu->AddItem(fNormalItem);
    
    fOutlineItem = new BMenuItem("Outline", new BMessage(MSG_DISPLAY_OUTLINE));
    displayMenu->AddItem(fOutlineItem);
    
    fFillOnlyItem = new BMenuItem("Fill Only", new BMessage(MSG_DISPLAY_FILL_ONLY));
    displayMenu->AddItem(fFillOnlyItem);
    
    fStrokeOnlyItem = new BMenuItem("Stroke Only", new BMessage(MSG_DISPLAY_STROKE_ONLY));
    displayMenu->AddItem(fStrokeOnlyItem);
    
    displayMenu->SetTargetForItems(target);
    fMenuBar->AddItem(displayMenu);
}

void
SVGMenuManager::_CreateHelpMenu(BHandler* target)
{
    BMenu* helpMenu = new BMenu("Help");
    helpMenu->AddItem(new BMenuItem("About SVGear...", new BMessage(MSG_ABOUT)));
    
    helpMenu->SetTargetForItems(target);
    fMenuBar->AddItem(helpMenu);
}

void
SVGMenuManager::_AddShortcuts(BHandler* target)
{
    BWindow* window = dynamic_cast<BWindow*>(target);
    if (window) {
        window->AddShortcut('T', B_COMMAND_KEY, new BMessage(MSG_EASTER_EGG));
        window->AddShortcut('Z', B_COMMAND_KEY, new BMessage(B_UNDO));
    }
}

void
SVGMenuManager::UpdateDisplayMode(svg_display_mode mode)
{
    if (!fNormalItem || !fOutlineItem || !fFillOnlyItem || !fStrokeOnlyItem)
        return;

    fNormalItem->SetMarked(mode == SVG_DISPLAY_NORMAL);
    fOutlineItem->SetMarked(mode == SVG_DISPLAY_OUTLINE);
    fFillOnlyItem->SetMarked(mode == SVG_DISPLAY_FILL_ONLY);
    fStrokeOnlyItem->SetMarked(mode == SVG_DISPLAY_STROKE_ONLY);
}

void
SVGMenuManager::UpdateViewOptions(bool showTransparency, bool showSource)
{
    if (fTransparencyItem)
        fTransparencyItem->SetMarked(showTransparency);
    if (fSourceViewItem)
        fSourceViewItem->SetMarked(showSource);
}

void
SVGMenuManager::_CreateFileMenu(BHandler* target)
{
    BMenu* fileMenu = new BMenu("File");
    fileMenu->AddItem(new BMenuItem("New", new BMessage(MSG_NEW_FILE), 'N'));
    fileMenu->AddItem(new BMenuItem("Open" B_UTF8_ELLIPSIS, new BMessage(MSG_OPEN_FILE), 'O'));
    fileMenu->AddSeparatorItem();
    
    fSaveItem = new BMenuItem("Save", new BMessage(MSG_SAVE_FILE), 'S');
    fileMenu->AddItem(fSaveItem);
    
    fSaveAsItem = new BMenuItem("Save as" B_UTF8_ELLIPSIS, new BMessage(MSG_SAVE_AS_FILE), 'S', B_SHIFT_KEY);
    fileMenu->AddItem(fSaveAsItem);
    
    fileMenu->AddSeparatorItem();
    fileMenu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
    
    fileMenu->SetTargetForItems(target);
    fMenuBar->AddItem(fileMenu);
}

void
SVGMenuManager::UpdateFileMenu(bool canSave, bool isModified)
{
    if (fSaveItem) {
        fSaveItem->SetEnabled(canSave || isModified);
        if (isModified && canSave) {
            fSaveItem->SetLabel("Save");
        } else if (isModified && !canSave) {
            fSaveItem->SetLabel("Save As" B_UTF8_ELLIPSIS);
        } else {
            fSaveItem->SetLabel("Save");
        }
    }
    
    if (fSaveAsItem) {
        fSaveAsItem->SetEnabled(true); // Save As всегда доступен
    }
}
