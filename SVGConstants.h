/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SVG_CONSTANTS_H
#define SVG_CONSTANTS_H

#include <SupportDefs.h>

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
const uint32 MSG_ABOUT = 'abou';
const uint32 MSG_TOGGLE_SOURCE_VIEW = 'tgsv';
const uint32 MSG_RELOAD_FROM_SOURCE = 'rfsr';
const uint32 MSG_EDIT_COPY = 'copy';
const uint32 MSG_EDIT_PASTE = 'past';
const uint32 MSG_EDIT_CUT = 'cut_';
const uint32 MSG_EDIT_APPLY = 'appl';
const uint32 MSG_EDIT_WORD_WRAP = 'wrap';
const uint32 MSG_DROP_HVIF = '_RRC';

// Application messages
const uint32 MSG_APP_QUIT = 'QAPP';
const uint32 MSG_WINDOW_ACTIVATED = 'AWIN';
const uint32 MSG_WINDOW_CLOSED = 'CWIN';
const uint32 MSG_EASTER_EGG = 'EEGG';
const uint32 MSG_SVG_STATUS_UPDATE = 'svsu';

// UI Constants
const int32 TOOLBAR_ICON_SIZE = 24;
const float SOURCE_VIEW_WEIGHT = 0.3f;
const float MAIN_VIEW_WEIGHT = 0.7f;

// Error messages
const char* const ERROR_FILE_NOT_SPECIFIED = "File path not specified";
const char* const ERROR_READING_SVG = "Error reading SVG file";
const char* const ERROR_PARSING_SVG = "Error parsing SVG from source code. Please check the syntax.";
const char* const ERROR_SOURCE_EMPTY = "Source code is empty.";
const char* const ERROR_SAVE_FAILED = "Failed to save file";
const char* const ERROR_INVALID_PATH = "Invalid file path";

// Success messages
const char* const MSG_FILE_SAVED = "File saved successfully";

#endif
