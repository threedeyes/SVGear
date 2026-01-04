/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef ICON_SELECTION_DIALOG_H
#define ICON_SELECTION_DIALOG_H

#include <Window.h>
#include <TextControl.h>
#include <Button.h>
#include <Bitmap.h>
#include <Messenger.h>
#include <MessageRunner.h>
#include <FilePanel.h>
#include <Path.h>

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
			void            _ParseTags(BMessage* data);
			void            _ParseIcons(BMessage* data);
			void            _AddIconFromMessage(BMessage* item);
			void            _OpenSelectedIcon();
			void            _SetLoading(bool loading);
			void            _SaveFormat(IconFormat format);
			void            _DoSaveFormat(BMessage* message);

#ifdef HVIF_STORE_CLIENT
			void            _CopyFormat(uint32 command);
			void            _ProcessClipboardData(const uint8* data, size_t size,
								uint32 command, int32 id, const char* name);
#endif

			const char*     _GetFormatExtension(IconFormat format) const;
			const char*     _GetFormatMimeType(IconFormat format) const;

			HvifStoreClient* fClient;
			IconGridView*   fGrid;
			BScrollView*    fGridScroll;
			IconInfoView*   fInfoView;
			BTextControl*   fSearchEntry;
			TagsFlowView*   fTagsView;
			BButton*        fOpenBtn;
			BButton*        fResetButton;

			BBitmap*        fResetButtonIcon;

			BMessenger      fTarget;
			int32           fPage;
			bool            fLoading;
			BString         fCurrentTags;
			BString         fLastSearchQuery;
			BMessageRunner* fSearchRunner;
			int32           fPreserveSelectionId;

			BFilePanel*     fSavePanel;
			IconFormat      fPendingSaveFormat;

#ifdef HVIF_STORE_CLIENT
			BButton*        fCopyRDefBtn;
			BButton*        fCopyCppBtn;
			BButton*        fCopySvgBtn;
			BButton*        fCopyImgBtn;
#endif
};

#endif
