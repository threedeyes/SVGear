/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Application.h>

#include <stdio.h>

#include "IconSelectionDialog.h"
#include "HvifStoreDefs.h"

class App : public BApplication {
public:
	App() : BApplication("application/x-vnd.HVIFStoreBrowser") {}
	
	void ReadyToRun() {
		IconSelectionDialog* win = new IconSelectionDialog(BMessenger(this));
		win->Show();
	}
	
	void MessageReceived(BMessage* message) {
		switch (message->what) {
			case kMsgDialogClosed:
				PostMessage(B_QUIT_REQUESTED);
				break;
				
			default:
				BApplication::MessageReceived(message);
		}
	}
};

int main() {
	App app;
	app.Run();
	return 0;
}
