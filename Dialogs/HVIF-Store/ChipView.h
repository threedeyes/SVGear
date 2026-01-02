/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef CHIP_VIEW_H
#define CHIP_VIEW_H

#include <Control.h>

enum chip_style {
	B_CHIP_STYLE_TAG,
	B_CHIP_STYLE_CATEGORY
};

class ChipView : public BControl {
public:
							ChipView(const char* name, const char* label,
								BMessage* message,
								chip_style style = B_CHIP_STYLE_CATEGORY);
	virtual					~ChipView();

	virtual void			SetValue(int32 value);
	virtual void			SetLabel(const char* label);

			chip_style		Style() const { return fStyle; }

			void			SetClickable(bool clickable);
			bool			IsClickable() const { return fClickable; }

	virtual void			GetPreferredSize(float* width, float* height);
	virtual BSize			MinSize();
	virtual BSize			MaxSize();
	virtual BSize			PreferredSize();

	virtual void			Draw(BRect updateRect);
	virtual void			MouseDown(BPoint where);
	virtual void			MouseMoved(BPoint where, uint32 transit,
								const BMessage* dragMessage);
	virtual void			MakeFocus(bool focused = true);
	virtual void			KeyDown(const char* bytes, int32 numBytes);

private:
	float					_CornerRadius() const;
	BRect					_ChipRect() const;
	BRect					_FocusRect() const;

	chip_style				fStyle;
	bool					fClickable;
};

#endif
