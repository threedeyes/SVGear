/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "SVGApplication.h"
#include "SVGSettings.h"

SVGApplication::SVGApplication() : BApplication(APP_SIGNATURE),
	lastActivatedWindow(NULL)
{
	InitializeSettings();
}

SVGApplication::~SVGApplication()
{
	CleanupSettings();
	be_app->PostMessage(MSG_WINDOW_CLOSED);
};

SVGMainWindow*
SVGApplication::CreateWindow(void)
{
	SVGMainWindow *activeWindow = NULL;
	SVGMainWindow *lastWindow = NULL;
	for (int32 i = 0; i < CountWindows(); i++) {
		SVGMainWindow* window = dynamic_cast<SVGMainWindow*>(WindowAt(i));
		if (window != NULL) {
			if (window == lastActivatedWindow) {
				activeWindow = window;
				break;
			}
			lastWindow = window;
		}
	}

	SVGMainWindow *svgWindow = new SVGMainWindow();
	if (activeWindow != NULL ) {
		BWindowStack stack(activeWindow);
		stack.AddWindow(svgWindow);
	} else if (lastWindow != NULL ) {
		BWindowStack stack(lastWindow);
		stack.AddWindow(svgWindow);
	}

	svgWindow->Show();

	return svgWindow;
}

void
SVGApplication::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case MSG_WINDOW_ACTIVATED:
		{
			void *winPtr = NULL;
			if (message->FindPointer("window", &winPtr) == B_OK)
				lastActivatedWindow = static_cast<SVGMainWindow*>(winPtr);
			break;
		}
		case MSG_WINDOW_CLOSED:
		{
			lastActivatedWindow = NULL;
			for (int32 i = CountWindows() - 1; i >= 0 ; i--) {
				SVGMainWindow* window = dynamic_cast<SVGMainWindow*>(WindowAt(i));
				if (window != NULL) {
					lastActivatedWindow = window;
					window->Activate();
					break;
				}
			}

			if (lastActivatedWindow == NULL)
				PostMessage(B_QUIT_REQUESTED);

			break;
		}
		default:
			BApplication::MessageReceived(message);
			break;
	}
}

void
SVGApplication::RefsReceived(BMessage* message)
{
	for (int32 i = 0; i < CountWindows(); i++) {
		SVGMainWindow* window = dynamic_cast<SVGMainWindow*>(WindowAt(i));
		if (window != NULL) {
			if (!window->IsLoaded()) {
				window->PostMessage(message);
				return;
			}
		}
	}

	SVGMainWindow *svgWindow = CreateWindow();
	svgWindow->PostMessage(message);
}

void
SVGApplication::ReadyToRun()
{
	if (CountWindows() == 0)
		CreateWindow();
}

void
SVGApplication::ArgvReceived(int32 argc, char** argv)
{
	BMessage *message = NULL;
	for (int32 i = 1; i < argc; i++) {
		entry_ref ref;
		status_t err = get_ref_for_path(argv[i], &ref);
		if (err == B_OK) {
			if (!message) {
				message = new BMessage;
				message->what = B_REFS_RECEIVED;
			}
			message->AddRef("refs", &ref);
		}
	}
	if (message) {
		RefsReceived(message);
		delete message;
	}
}

BBitmap *
SVGApplication::GetIcon(const char *iconName, int iconSize)
{
	if (iconName == NULL) {
		app_info inf;
		be_app->GetAppInfo(&inf);

		BFile file(&inf.ref, B_READ_ONLY);
		BAppFileInfo appMime(&file);
		if (appMime.InitCheck() != B_OK)
			return NULL;
	
		BBitmap* icon = new BBitmap(BRect(0, 0, iconSize - 1, iconSize -1), B_RGBA32);
		if (appMime.GetIcon(icon, (icon_size)iconSize) == B_OK)
			return icon;

		delete icon;
		return NULL;
	} else {
		BResources* resources = AppResources();
		if (resources != NULL) {
			size_t size;
			const void* iconData = resources->LoadResource(B_VECTOR_ICON_TYPE, iconName, &size);
			if (iconData != NULL && size > 0) {
				BBitmap* bitmap = new BBitmap(BRect(0, 0, iconSize - 1, iconSize - 1), B_RGBA32);
				status_t status = BIconUtils::GetVectorIcon((uint8*)iconData, size, bitmap);
				if (status == B_OK)
					return bitmap;
			}
		}
		return NULL;
	}
}
