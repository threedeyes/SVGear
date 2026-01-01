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
	App() : BApplication("application/x-vnd.HvifStoreClient") {}
	
	void ReadyToRun() {
		IconSelectionDialog* win = new IconSelectionDialog(BMessenger(this));
		win->Show();
	}
	
	void MessageReceived(BMessage* message) {
		switch (message->what) {
			case kMsgIconDataReady: {
				printf("Icon received!\n");
				printf("  ID: %" B_PRId32 "\n", message->GetInt32("id", 0));
				printf("  Title: %s\n", message->GetString("title", ""));
				printf("  Author: %s\n", message->GetString("author", ""));
				printf("  License: %s\n", message->GetString("license", ""));
				printf("  MIME: %s\n", message->GetString("mime_type", ""));
				printf("  Tags: %s\n", message->GetString("tags", ""));
				
				const void* data;
				ssize_t size;
				
				if (message->FindData("hvif_data", B_RAW_TYPE, &data, &size) == B_OK)
					printf("  HVIF data: %zd bytes\n", size);
					
				if (message->FindData("svg_data", B_RAW_TYPE, &data, &size) == B_OK)
					printf("  SVG data: %zd bytes\n", size);
					
				if (message->FindData("iom_data", B_RAW_TYPE, &data, &size) == B_OK)
					printf("  IOM data: %zd bytes\n", size);
				
				PostMessage(B_QUIT_REQUESTED);
				break;
			}
			
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
