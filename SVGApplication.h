/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SVG_APPLICATION_H
#define SVG_APPLICATION_H

#include <Application.h>
#include <Message.h>
#include <File.h>
#include <Path.h>
#include <MimeType.h>
#include <Resources.h>
#include <AppFileInfo.h>
#include <IconUtils.h>
#include <Window.h>
#include <Bitmap.h>
#include <Roster.h>

#include <private/interface/WindowStack.h>

#include "SVGMainWindow.h"

#define APP_SIGNATURE "application/x-vnd.svgear"
#define STL_SIGNATURE "image/svg+xml"

#define APP_SETTINGS_FILENAME "SVGear_settings"

#define URL_HOMEPAGE		"https://github.com/threedeyes/SVGear"
#define URL_HOMEPAGE_WIKI	"https://github.com/threedeyes/SVGear/wiki"

#define MSG_APP_QUIT					'QAPP'
#define MSG_WINDOW_ACTIVATED			'AWIN'
#define MSG_WINDOW_CLOSED				'CWIN'
#define MSG_EASTER_EGG					'EEGG'

class SVGMainWindow;

class SVGApplication : public BApplication {
	public:
		SVGApplication();
    	~SVGApplication();

		virtual void MessageReceived(BMessage *message);
    	virtual void RefsReceived(BMessage* message);
    	virtual void ArgvReceived(int32 argc, char** argv);
    	virtual void ReadyToRun();

		static BBitmap *GetIcon(const char *iconName, int iconSize);

	private:
		SVGMainWindow *CreateWindow(void);
		SVGMainWindow *lastActivatedWindow;
};

#endif
