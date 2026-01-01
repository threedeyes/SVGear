/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef ICON_INFO_VIEW_H
#define ICON_INFO_VIEW_H

#include <View.h>
#include <String.h>
#include <Bitmap.h>

struct IconItem;

class IconInfoView : public BView {
public:
							IconInfoView();
	virtual                 ~IconInfoView();

	virtual void            AttachedToWindow();
	virtual void            Draw(BRect updateRect);
	virtual BSize           MinSize();
	virtual BSize           MaxSize();
	virtual BSize           PreferredSize();

			void            SetIcon(IconItem* item);
			void            Clear();

private:
			void            _CalculateSizes();
			float           _DrawField(const char* label, const char* value,
								float y, float maxWidth);
			float           _DrawWrappedText(const char* text, float x, float y,
								float maxWidth);
			BString         _FormatSize(int32 bytes) const;

			IconItem*       fCurrentItem;
			
			float           fPreviewSize;
			float           fPadding;
			float           fPanelWidth;
			
	static const float      kBasePreviewSize;
	static const float      kBasePadding;
	static const float      kBasePanelWidth;
	static const float      kBaseFontSize;
};

#endif
