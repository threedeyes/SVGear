/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef ICON_INFO_VIEW_H
#define ICON_INFO_VIEW_H

#include <View.h>
#include <String.h>
#include <Bitmap.h>
#include <Messenger.h>
#include <Cursor.h>

#include "HvifStoreDefs.h"

struct IconItem;

class IconInfoView : public BView {
public:
							IconInfoView();
	virtual                 ~IconInfoView();

	virtual void            AttachedToWindow();
	virtual void            Draw(BRect updateRect);
	virtual void            MouseDown(BPoint where);
	virtual void            MouseMoved(BPoint where, uint32 transit,
								const BMessage* dragMessage);
	virtual BSize           MinSize();
	virtual BSize           MaxSize();
	virtual BSize           PreferredSize();

			void            SetIcon(IconItem* item);
			void            Clear();

			void            SetTarget(BMessenger target);
			void            SetFilterTags(const char* tags);

private:
			void            _CalculateSizes();
			float           _DrawField(const char* label, const char* value,
								float y, float maxWidth);
			float           _DrawTags(const char* tags, float x, float y,
								float maxWidth);
			float           _DrawFormats(float x, float y, float maxWidth);
			BString         _FormatSize(int32 bytes) const;
			BString         _GetTagAt(BPoint point) const;
			IconFormat      _GetFormatAt(BPoint point) const;
			bool            _IsOverClickable(BPoint point) const;
			void            _UpdateCursor(BPoint where);

			IconItem*       fCurrentItem;
			BMessenger      fTarget;
			BString         fCurrentFilterTags;
			
			float           fPreviewSize;
			float           fPadding;
			float           fPanelWidth;
			
			BRect           fFormatRects[kFormatCount];
			bool            fCursorOverLink;

	static const float      kBasePreviewSize;
	static const float      kBasePadding;
	static const float      kBasePanelWidth;
	static const float      kBaseFontSize;
};

#endif
