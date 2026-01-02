/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef ICON_SELECTION_DIALOG_H
#define ICON_SELECTION_DIALOG_H

#include <Window.h>
#include <TextControl.h>
#include <Button.h>
#include <Messenger.h>
#include <MessageRunner.h>

#include "HvifStoreDefs.h"

class HvifStoreClient;
class IconGridView;
class IconInfoView;
class TagsFlowView;
class BScrollView;

class IconSelectionDialog : public BWindow {
public:
							IconSelectionDialog(BMessenger target = BMessenger());
	virtual                 ~IconSelectionDialog();

	virtual void            MessageReceived(BMessage* message);
	virtual bool            QuitRequested();
	virtual void            Show();

private:
			void            _InitGUI();
			void            _CalculateWindowSize(float* width, float* height);
			void            _Search(bool clear = true);
			void            _ScheduleSearch();
			void            _ParseCategories(BMessage* data);
			void            _ParseIcons(BMessage* data);
			void            _AddIconFromMessage(BMessage* item);
			void            _OpenSelectedIcon();
			void            _SetLoading(bool loading);

			HvifStoreClient* fClient;
			IconGridView*   fGrid;
			BScrollView*    fGridScroll;
			IconInfoView*   fInfoView;
			BTextControl*   fSearchEntry;
			TagsFlowView*   fTagsView;
			BButton*        fOpenBtn;
			BButton*        fResetButton;
			
			BMessenger      fTarget;
			int32           fPage;
			bool            fLoading;
			BString         fCurrentTags;
			BString         fLastSearchQuery;
			BMessageRunner* fSearchRunner;
			int32           fPreserveSelectionId;
};

#endif
