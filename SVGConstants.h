/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SVG_CONSTANTS_H
#define SVG_CONSTANTS_H

#include <SupportDefs.h>
#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SVGConstants"

// Application constants
#define APP_SIGNATURE "application/x-vnd.svgear"
#define STL_SIGNATURE "image/svg+xml"
#define APP_SETTINGS_FILENAME "SVGear_settings"

// URLs
#define URL_HOMEPAGE "https://github.com/threedeyes/SVGear"
#define URL_HOMEPAGE_WIKI "https://github.com/threedeyes/SVGear/wiki"

// Message constants
const uint32 MSG_NEW_FILE = 'newf';
const uint32 MSG_OPEN_FILE = 'open';
const uint32 MSG_SAVE_FILE = 'save';
const uint32 MSG_SAVE_AS_FILE = 'svas';
const uint32 MSG_SAVE_PANEL_SAVE = 'svps';
const uint32 MSG_CENTER = 'cent';
const uint32 MSG_ZOOM_IN = 'zmin';
const uint32 MSG_ZOOM_OUT = 'zmot';
const uint32 MSG_ZOOM_ORIGINAL = 'acts';
const uint32 MSG_FIT_WINDOW = 'fitw';
const uint32 MSG_RESET_VIEW = 'rstv';
const uint32 MSG_DISPLAY_NORMAL = 'dpnr';
const uint32 MSG_DISPLAY_OUTLINE = 'dpol';
const uint32 MSG_DISPLAY_FILL_ONLY = 'dpfl';
const uint32 MSG_DISPLAY_STROKE_ONLY = 'dpst';
const uint32 MSG_TOGGLE_TRANSPARENCY = 'tgtr';
const uint32 MSG_TOGGLE_BOUNDINGBOX = 'tgbb';
const uint32 MSG_BBOX_NONE = 'bbno';
const uint32 MSG_BBOX_DOCUMENT = 'bbdc';
const uint32 MSG_BBOX_SIMPLE_FRAME = 'bbsf';
const uint32 MSG_BBOX_TRANSPARENT_GRAY = 'bbtg';
const uint32 MSG_ABOUT = 'abou';
const uint32 MSG_TOGGLE_SOURCE_VIEW = 'tgsv';
const uint32 MSG_RELOAD_FROM_SOURCE = 'rfsr';
const uint32 MSG_EDIT_COPY = 'copy';
const uint32 MSG_EDIT_PASTE = 'past';
const uint32 MSG_EDIT_CUT = 'cut_';
const uint32 MSG_EDIT_APPLY = 'appl';
const uint32 MSG_EDIT_WORD_WRAP = 'wrap';
const uint32 MSG_DROP_HVIF = '_RRC';
const uint32 MSG_TOGGLE_STAT = 'tgst';

// Export messages
const uint32 MSG_EXPORT_HVIF = 'exhv';
const uint32 MSG_EXPORT_RDEF = 'exrd';
const uint32 MSG_EXPORT_CPP = 'excx';

// Tab selection message
const uint32 MSG_TAB_SELECTION = 'tabs';

// Application messages
const uint32 MSG_APP_QUIT = 'QAPP';
const uint32 MSG_WINDOW_ACTIVATED = 'AWIN';
const uint32 MSG_WINDOW_CLOSED = 'CWIN';
const uint32 MSG_EASTER_EGG = 'EEGG';
const uint32 MSG_SVG_STATUS_UPDATE = 'svsu';

// State monitoring messages
const uint32 MSG_STATE_UPDATE = 'stup';
const uint32 MSG_TEXT_MODIFIED = 'txmd';
const uint32 MSG_SELECTION_CHANGED = 'slch';

// Structure view messages
const uint32 MSG_TOGGLE_STRUCTURE = 'tgsr';
const uint32 MSG_SHAPE_SELECTED = 'shps';
const uint32 MSG_PATH_SELECTED = 'pths';
const uint32 MSG_PAINT_SELECTED = 'pnsl';
const uint32 MSG_CONTROL_POINTS_SELECTED = 'ctps';
const uint32 MSG_CLEAR_SELECTION = 'clrs';


// UI Constants
const int32 TOOLBAR_ICON_SIZE = 24;
const float SOURCE_VIEW_WEIGHT = 0.3f;
const float MAIN_VIEW_WEIGHT = 0.7f;

// Tab indices
enum {
	TAB_SVG = 0,
	TAB_RDEF = 1,
	TAB_CPP = 2
};

// Global error messages
#define ERROR_FILE_NOT_SPECIFIED B_TRANSLATE("File path not specified")
#define ERROR_READING_SVG B_TRANSLATE("Error reading SVG file")
#define ERROR_PARSING_SVG B_TRANSLATE("Error parsing SVG from source code. Please check the syntax.")
#define ERROR_SOURCE_EMPTY B_TRANSLATE("Source code is empty.")
#define ERROR_SAVE_FAILED B_TRANSLATE("Failed to save file")
#define ERROR_INVALID_PATH B_TRANSLATE("Invalid file path")
#define ERROR_EXPORT_FAILED B_TRANSLATE("Failed to export file")

// Global success messages
#define MSG_FILE_SAVED B_TRANSLATE("File saved successfully")
#define MSG_FILE_EXPORTED B_TRANSLATE("File exported successfully")

#endif
