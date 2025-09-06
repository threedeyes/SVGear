/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SVG_MENU_MANAGER_H
#define SVG_MENU_MANAGER_H

#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Handler.h>
#include <Message.h>
#include <Window.h>

#include "BSVGView.h"
#include "SVGConstants.h"

class SVGMenuManager {
public:
    SVGMenuManager();
    ~SVGMenuManager();

    BMenuBar* CreateMenuBar(BHandler* target);
	void CreateExportSubMenu(BHandler* target);
    void UpdateDisplayMode(svg_display_mode mode);
    void UpdateViewOptions(bool showTransparency, bool showSource, bool showBoundingBox);
    void UpdateBoundingBoxStyle(svg_boundingbox_style style);
    void UpdateFileMenu(bool canSave, bool isModified);

private:
    BMenuBar* fMenuBar;
    BMenuItem* fNormalItem;
    BMenuItem* fOutlineItem;
    BMenuItem* fFillOnlyItem;
    BMenuItem* fStrokeOnlyItem;
    BMenuItem* fTransparencyItem;
    BMenuItem* fBoundingBoxItem;
    BMenuItem* fBBoxNoneItem;
    BMenuItem* fBBoxDocumentItem;
    BMenuItem* fBBoxSimpleFrameItem;
    BMenuItem* fBBoxTransparentGrayItem;
    BMenuItem* fSourceViewItem;
	BMenuItem* fSaveItem;
    BMenuItem* fSaveAsItem;
    BMenu* fExportSubMenu;
    BMenu* fDisplaySubMenu;
    BMenu* fBoundingBoxSubMenu;

    void _CreateFileMenu(BHandler* target);
    void _CreateViewMenu(BHandler* target);
    void _CreateHelpMenu(BHandler* target);
    void _AddShortcuts(BHandler* target);
};

#endif
