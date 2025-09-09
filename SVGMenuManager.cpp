/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "SVGMenuManager.h"
#include <MenuItem.h>
#include <Catalog.h>

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
	fSaveItem(NULL),
	fSaveAsItem(NULL),
	fExportSubMenu(NULL),
	fDisplaySubMenu(NULL),
	fBoundingBoxSubMenu(NULL)
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
	_CreateHelpMenu(target);
	_AddShortcuts(target);

	return fMenuBar;
}

void
SVGMenuManager::_CreateFileMenu(BHandler* target)
{
	BMenu* fileMenu = new BMenu(B_TRANSLATE("File"));
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("New"), new BMessage(MSG_NEW_FILE), 'N'));
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Open" B_UTF8_ELLIPSIS), new BMessage(MSG_OPEN_FILE), 'O'));
	fileMenu->AddSeparatorItem();

	fSaveItem = new BMenuItem(B_TRANSLATE("Save"), new BMessage(MSG_SAVE_FILE), 'S');
	fileMenu->AddItem(fSaveItem);

	fSaveAsItem = new BMenuItem(B_TRANSLATE("Save as" B_UTF8_ELLIPSIS), new BMessage(MSG_SAVE_AS_FILE), 'S', B_SHIFT_KEY);
	fileMenu->AddItem(fSaveAsItem);

	fileMenu->AddSeparatorItem();

	fExportSubMenu = new BMenu(B_TRANSLATE("Export"));
	fExportSubMenu->AddItem(new BMenuItem(B_TRANSLATE("HVIF Icon" B_UTF8_ELLIPSIS), new BMessage(MSG_EXPORT_HVIF)));
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
	viewMenu->AddItem(new BMenuItem(B_TRANSLATE("Zoom In"), new BMessage(MSG_ZOOM_IN), '+'));
	viewMenu->AddItem(new BMenuItem(B_TRANSLATE("Zoom Out"), new BMessage(MSG_ZOOM_OUT), '-'));
	viewMenu->AddItem(new BMenuItem(B_TRANSLATE("Zoom Original"), new BMessage(MSG_ZOOM_ORIGINAL), '1'));
	viewMenu->AddItem(new BMenuItem(B_TRANSLATE("Fit to Window"), new BMessage(MSG_FIT_WINDOW), 'F'));
	viewMenu->AddSeparatorItem();
	viewMenu->AddItem(new BMenuItem(B_TRANSLATE("Center"), new BMessage(MSG_CENTER), 'C', B_SHIFT_KEY));
	viewMenu->AddSeparatorItem();
	viewMenu->AddItem(new BMenuItem(B_TRANSLATE("Reset View"), new BMessage(MSG_RESET_VIEW), '0'));
	viewMenu->AddSeparatorItem();

	fDisplaySubMenu = new BMenu(B_TRANSLATE("Display Mode"));

	fNormalItem = new BMenuItem(B_TRANSLATE("Normal"), new BMessage(MSG_DISPLAY_NORMAL));
	fNormalItem->SetMarked(true);
	fDisplaySubMenu->AddItem(fNormalItem);

	fOutlineItem = new BMenuItem(B_TRANSLATE("Outline"), new BMessage(MSG_DISPLAY_OUTLINE));
	fDisplaySubMenu->AddItem(fOutlineItem);

	fFillOnlyItem = new BMenuItem(B_TRANSLATE("Fill Only"), new BMessage(MSG_DISPLAY_FILL_ONLY));
	fDisplaySubMenu->AddItem(fFillOnlyItem);

	fStrokeOnlyItem = new BMenuItem(B_TRANSLATE("Stroke Only"), new BMessage(MSG_DISPLAY_STROKE_ONLY));
	fDisplaySubMenu->AddItem(fStrokeOnlyItem);

	fDisplaySubMenu->SetTargetForItems(target);
	viewMenu->AddItem(fDisplaySubMenu);

	fBoundingBoxSubMenu = new BMenu(B_TRANSLATE("Bounding Box"));

	fBBoxNoneItem = new BMenuItem(B_TRANSLATE("None"), new BMessage(MSG_BBOX_NONE));
	fBBoxNoneItem->SetMarked(true);
	fBoundingBoxSubMenu->AddItem(fBBoxNoneItem);

	fBBoxDocumentItem = new BMenuItem(B_TRANSLATE("Document Style"), new BMessage(MSG_BBOX_DOCUMENT));
	fBoundingBoxSubMenu->AddItem(fBBoxDocumentItem);

	fBBoxSimpleFrameItem = new BMenuItem(B_TRANSLATE("Simple Frame"), new BMessage(MSG_BBOX_SIMPLE_FRAME));
	fBoundingBoxSubMenu->AddItem(fBBoxSimpleFrameItem);

	fBBoxTransparentGrayItem = new BMenuItem(B_TRANSLATE("Transparent Gray"), new BMessage(MSG_BBOX_TRANSPARENT_GRAY));
	fBoundingBoxSubMenu->AddItem(fBBoxTransparentGrayItem);

	fBoundingBoxSubMenu->SetTargetForItems(target);
	viewMenu->AddItem(fBoundingBoxSubMenu);

	viewMenu->AddSeparatorItem();

	fTransparencyItem = new BMenuItem(B_TRANSLATE("Show Transparency Grid"), new BMessage(MSG_TOGGLE_TRANSPARENCY), 'G');
	fTransparencyItem->SetMarked(true);
	viewMenu->AddItem(fTransparencyItem);

	viewMenu->AddSeparatorItem();

	fSourceViewItem = new BMenuItem(B_TRANSLATE("Show Source Code"), new BMessage(MSG_TOGGLE_SOURCE_VIEW), 'S');
	viewMenu->AddItem(fSourceViewItem);
	viewMenu->AddItem(new BMenuItem(B_TRANSLATE("Reload from Source"), new BMessage(MSG_RELOAD_FROM_SOURCE), 'R'));

	viewMenu->SetTargetForItems(target);
	fMenuBar->AddItem(viewMenu);
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
SVGMenuManager::UpdateViewOptions(bool showTransparency, bool showSource, bool showBoundingBox)
{
	if (fTransparencyItem)
		fTransparencyItem->SetMarked(showTransparency);
	if (fSourceViewItem)
		fSourceViewItem->SetMarked(showSource);
}

void
SVGMenuManager::UpdateFileMenu(bool canSave, bool isModified)
{
	if (fSaveItem) {
		fSaveItem->SetEnabled(canSave || isModified);
		if (isModified && canSave) {
			fSaveItem->SetLabel(B_TRANSLATE("Save"));
		} else if (isModified && !canSave) {
			fSaveItem->SetLabel(B_TRANSLATE("Save As" B_UTF8_ELLIPSIS));
		} else {
			fSaveItem->SetLabel(B_TRANSLATE("Save"));
		}
	}

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
