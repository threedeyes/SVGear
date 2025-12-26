/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>
#include <MenuItem.h>
#include <Catalog.h>
#include <RecentItems.h>

#include "SVGMenuManager.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SVGMenuManager"

SVGMenuManager::SVGMenuManager()
	: fMenuBar(NULL),
	fNormalItem(NULL),
	fOutlineItem(NULL),
	fFillOnlyItem(NULL),
	fStrokeOnlyItem(NULL),
	fTransparencyItem(NULL),
	fBBoxNoneItem(NULL),
	fBBoxDocumentItem(NULL),
	fBBoxSimpleFrameItem(NULL),
	fBBoxTransparentGrayItem(NULL),
	fSourceViewItem(NULL),
	fStructureViewItem(NULL),
	fStatViewItem(NULL),
	fSaveItem(NULL),
	fSaveAsItem(NULL),
	fOpenInIconOMaticItem(NULL),
	fExportSubMenu(NULL),
	fDisplaySubMenu(NULL),
	fBoundingBoxSubMenu(NULL),
	fToolsMenu(NULL)
{
}

SVGMenuManager::~SVGMenuManager()
{
}

BMenuBar*
SVGMenuManager::CreateMenuBar(BHandler* target)
{
	fMenuBar = new BMenuBar("menubar");

	_CreateFileMenu(target);
	_CreateViewMenu(target);
	_CreateToolsMenu(target);
	_CreateHelpMenu(target);
	_AddShortcuts(target);

	return fMenuBar;
}

void
SVGMenuManager::_CreateFileMenu(BHandler* target)
{
	BMenu* fileMenu = new BMenu(B_TRANSLATE("File"));
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("New"), new BMessage(MSG_NEW_FILE), 'N'));

	BMenuItem* fMenuItemOpen = new BMenuItem(BRecentFilesList::NewFileListMenu(B_TRANSLATE("Open" B_UTF8_ELLIPSIS),
		NULL, NULL, be_app, 10, true, NULL, APP_SIGNATURE), new BMessage(MSG_OPEN_FILE));
	fMenuItemOpen->SetShortcut('O', 0);
	fileMenu->AddItem(fMenuItemOpen);
	fileMenu->AddSeparatorItem();

	fSaveItem = new BMenuItem(B_TRANSLATE("Save"), new BMessage(MSG_SAVE_FILE), 'S');
	fileMenu->AddItem(fSaveItem);

	fSaveAsItem = new BMenuItem(B_TRANSLATE("Save as" B_UTF8_ELLIPSIS), new BMessage(MSG_SAVE_AS_FILE), 'S', B_SHIFT_KEY);
	fileMenu->AddItem(fSaveAsItem);

	fileMenu->AddSeparatorItem();

	fExportSubMenu = new BMenu(B_TRANSLATE("Export"));
	fExportSubMenu->AddItem(new BMenuItem(B_TRANSLATE("HVIF Icon" B_UTF8_ELLIPSIS), new BMessage(MSG_EXPORT_HVIF)));
	fExportSubMenu->AddItem(new BMenuItem(B_TRANSLATE("Icon-O-Matic" B_UTF8_ELLIPSIS), new BMessage(MSG_EXPORT_IOM)));

	BMenu* pngSubMenu = new BMenu(B_TRANSLATE("PNG image"));
	const int32 sizes[] = { 16, 24, 32, 48, 64, 128, 256, 512 };
	for (int i = 0; i < 8; i++) {
		BMessage* msg = new BMessage(MSG_EXPORT_PNG);
		msg->AddInt32("size", sizes[i]);
		BString label;
		label.SetToFormat(B_TRANSLATE_COMMENT("%ld px","Image size, 'pixels'"), sizes[i]);
		pngSubMenu->AddItem(new BMenuItem(label.String(), msg));
	}

	pngSubMenu->AddSeparatorItem();
	BMessage* originalPngMsg = new BMessage(MSG_EXPORT_PNG);
	originalPngMsg->AddInt32("size", -1);
	pngSubMenu->AddItem(new BMenuItem(B_TRANSLATE("Original size"), originalPngMsg));

	pngSubMenu->SetTargetForItems(target);
	fExportSubMenu->AddItem(pngSubMenu);

	fExportSubMenu->AddSeparatorItem();

	fExportSubMenu->AddItem(new BMenuItem(B_TRANSLATE("RDef resource" B_UTF8_ELLIPSIS), new BMessage(MSG_EXPORT_RDEF)));
	fExportSubMenu->AddItem(new BMenuItem(B_TRANSLATE("C++ array" B_UTF8_ELLIPSIS), new BMessage(MSG_EXPORT_CPP)));
	fExportSubMenu->SetTargetForItems(target);

	fileMenu->AddItem(fExportSubMenu);
	fileMenu->AddSeparatorItem();

	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED), 'Q'));

	fileMenu->SetTargetForItems(target);
	fMenuBar->AddItem(fileMenu);
}

void
SVGMenuManager::_CreateViewMenu(BHandler* target)
{
	BMenu* viewMenu = new BMenu(B_TRANSLATE("View"));
	viewMenu->AddItem(new BMenuItem(B_TRANSLATE("Zoom in"), new BMessage(MSG_ZOOM_IN), '+'));
	viewMenu->AddItem(new BMenuItem(B_TRANSLATE("Zoom out"), new BMessage(MSG_ZOOM_OUT), '-'));
	viewMenu->AddItem(new BMenuItem(B_TRANSLATE("Zoom original"), new BMessage(MSG_ZOOM_ORIGINAL), '1'));
	viewMenu->AddItem(new BMenuItem(B_TRANSLATE("Fit to window"), new BMessage(MSG_FIT_WINDOW), 'F'));
	viewMenu->AddSeparatorItem();
	viewMenu->AddItem(new BMenuItem(B_TRANSLATE("Center"), new BMessage(MSG_CENTER), 'C', B_SHIFT_KEY));
	viewMenu->AddSeparatorItem();
	viewMenu->AddItem(new BMenuItem(B_TRANSLATE("Reset view"), new BMessage(MSG_RESET_VIEW), '0'));
	viewMenu->AddSeparatorItem();

	fDisplaySubMenu = new BMenu(B_TRANSLATE("Display mode"));

	fNormalItem = new BMenuItem(B_TRANSLATE("Normal"), new BMessage(MSG_DISPLAY_NORMAL));
	fNormalItem->SetMarked(true);
	fDisplaySubMenu->AddItem(fNormalItem);

	fOutlineItem = new BMenuItem(B_TRANSLATE("Outline"), new BMessage(MSG_DISPLAY_OUTLINE));
	fDisplaySubMenu->AddItem(fOutlineItem);

	fFillOnlyItem = new BMenuItem(B_TRANSLATE("Fill only"), new BMessage(MSG_DISPLAY_FILL_ONLY));
	fDisplaySubMenu->AddItem(fFillOnlyItem);

	fStrokeOnlyItem = new BMenuItem(B_TRANSLATE("Stroke only"), new BMessage(MSG_DISPLAY_STROKE_ONLY));
	fDisplaySubMenu->AddItem(fStrokeOnlyItem);

	fDisplaySubMenu->SetTargetForItems(target);
	viewMenu->AddItem(fDisplaySubMenu);

	fBoundingBoxSubMenu = new BMenu(B_TRANSLATE("Bounding box"));

	fBBoxNoneItem = new BMenuItem(B_TRANSLATE("None"), new BMessage(MSG_BBOX_NONE));
	fBBoxNoneItem->SetMarked(true);
	fBoundingBoxSubMenu->AddItem(fBBoxNoneItem);

	fBBoxDocumentItem = new BMenuItem(B_TRANSLATE("Document style"), new BMessage(MSG_BBOX_DOCUMENT));
	fBoundingBoxSubMenu->AddItem(fBBoxDocumentItem);

	fBBoxSimpleFrameItem = new BMenuItem(B_TRANSLATE("Simple frame"), new BMessage(MSG_BBOX_SIMPLE_FRAME));
	fBoundingBoxSubMenu->AddItem(fBBoxSimpleFrameItem);

	fBBoxTransparentGrayItem = new BMenuItem(B_TRANSLATE("Transparent gray"), new BMessage(MSG_BBOX_TRANSPARENT_GRAY));
	fBoundingBoxSubMenu->AddItem(fBBoxTransparentGrayItem);

	fBoundingBoxSubMenu->SetTargetForItems(target);
	viewMenu->AddItem(fBoundingBoxSubMenu);

	viewMenu->AddSeparatorItem();

	fTransparencyItem = new BMenuItem(B_TRANSLATE("Show transparency grid"), new BMessage(MSG_TOGGLE_TRANSPARENCY), 'G');
	fTransparencyItem->SetMarked(true);
	viewMenu->AddItem(fTransparencyItem);

	viewMenu->AddSeparatorItem();

	fSourceViewItem = new BMenuItem(B_TRANSLATE("Show sources panel"), new BMessage(MSG_TOGGLE_SOURCE_VIEW), 'U');
	viewMenu->AddItem(fSourceViewItem);

	fStructureViewItem = new BMenuItem(B_TRANSLATE("Show structure panel"), new BMessage(MSG_TOGGLE_STRUCTURE));
	viewMenu->AddItem(fStructureViewItem);

	fStatViewItem = new BMenuItem(B_TRANSLATE("Show statistics panel"), new BMessage(MSG_TOGGLE_STAT));
	viewMenu->AddItem(fStatViewItem);

	viewMenu->SetTargetForItems(target);
	fMenuBar->AddItem(viewMenu);
}

void
SVGMenuManager::_CreateToolsMenu(BHandler* target)
{
	fToolsMenu = new BMenu(B_TRANSLATE("Tools"));

	fOpenInIconOMaticItem = new BMenuItem(B_TRANSLATE("Icon-O-Matic" B_UTF8_ELLIPSIS), new BMessage(MSG_OPEN_IN_ICON_O_MATIC));
	fOpenInIconOMaticItem->SetEnabled(false);
	fToolsMenu->AddItem(fOpenInIconOMaticItem);

	fToolsMenu->SetTargetForItems(target);
	fMenuBar->AddItem(fToolsMenu);
}

void
SVGMenuManager::_CreateHelpMenu(BHandler* target)
{
	BMenu* helpMenu = new BMenu(B_TRANSLATE("Help"));
	helpMenu->AddItem(new BMenuItem(B_TRANSLATE("About SVGear" B_UTF8_ELLIPSIS), new BMessage(MSG_ABOUT)));

	helpMenu->SetTargetForItems(target);
	fMenuBar->AddItem(helpMenu);
}

void
SVGMenuManager::_AddShortcuts(BHandler* target)
{
	BWindow* window = dynamic_cast<BWindow*>(target);
	if (window) {
		window->AddShortcut('T', B_COMMAND_KEY, new BMessage(MSG_EASTER_EGG));
		window->AddShortcut('B', B_COMMAND_KEY, new BMessage(MSG_TOGGLE_BOUNDINGBOX));
		window->AddShortcut('Z', B_COMMAND_KEY, new BMessage(B_UNDO));
		window->AddShortcut('Z', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(B_REDO));
		window->AddShortcut(B_ENTER, B_COMMAND_KEY, new BMessage(MSG_RELOAD_FROM_SOURCE));
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
SVGMenuManager::UpdateBoundingBoxStyle(svg_boundingbox_style style)
{
	if (!fBBoxNoneItem || !fBBoxDocumentItem || !fBBoxSimpleFrameItem || !fBBoxTransparentGrayItem)
		return;

	fBBoxNoneItem->SetMarked(style == SVG_BBOX_NONE);
	fBBoxDocumentItem->SetMarked(style == SVG_BBOX_DOCUMENT);
	fBBoxSimpleFrameItem->SetMarked(style == SVG_BBOX_SIMPLE_FRAME);
	fBBoxTransparentGrayItem->SetMarked(style == SVG_BBOX_TRANSPARENT_GRAY);
}

void
SVGMenuManager::UpdateViewOptions(bool showTransparency, bool showSource,
								bool showBoundingBox, bool showStructure, bool showStat)
{
	if (fTransparencyItem)
		fTransparencyItem->SetMarked(showTransparency);
	if (fSourceViewItem)
		fSourceViewItem->SetMarked(showSource);
	if (fStructureViewItem)
		fStructureViewItem->SetMarked(showStructure);
	if (fStatViewItem)
		fStatViewItem->SetMarked(showStat);
}

void
SVGMenuManager::UpdateFileMenu(bool canSave, bool isModified)
{
	if (fSaveItem)
		fSaveItem->SetEnabled(canSave || isModified);
	if (fSaveAsItem)
		fSaveAsItem->SetEnabled(true);
}

void
SVGMenuManager::UpdateExportMenu(bool hasHVIFData)
{
	if (fExportSubMenu) {
		fExportSubMenu->SetEnabled(hasHVIFData);
		for (int32 i = 0; i < fExportSubMenu->CountItems(); i++) {
			BMenuItem* item = fExportSubMenu->ItemAt(i);
			if (item) {
				item->SetEnabled(hasHVIFData);
			}
		}
	}
}

void
SVGMenuManager::UpdateToolsMenu(bool hasHVIFData)
{
	if (fOpenInIconOMaticItem) {
		fOpenInIconOMaticItem->SetEnabled(hasHVIFData);
	}
}

void
SVGMenuManager::SetMenuItemEnabled(uint32 command, bool enabled)
{
	if (!fMenuBar)
		return;

	BMenuItem* item = _FindMenuItem(fMenuBar, command);
	if (item)
		item->SetEnabled(enabled);
}

BMenuItem*
SVGMenuManager::_FindMenuItem(BMenu* menu, uint32 command)
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
