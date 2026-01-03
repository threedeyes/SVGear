/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <cstdio>
#include <Catalog.h>
#include <Window.h>

#include "TagsFlowView.h"

#undef  B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT          "HVIFStoreDialog"

const float TagsFlowView::kHSpacing = 8.0f;
const float TagsFlowView::kVSpacing = 4.0f;
const float TagsFlowView::kPadding = 4.0f;
const float TagsFlowView::kMinHeight = 28.0f;


TagsFlowView::TagsFlowView()
	:
	BView("TagsFlow", B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE),
	fTags(20),
	fCachedHeight(kMinHeight)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetExplicitMinSize(BSize(100, kMinHeight));
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
}


TagsFlowView::~TagsFlowView()
{
}


void
TagsFlowView::AttachedToWindow()
{
	BView::AttachedToWindow();

	for (int32 i = 0; i < fTags.CountItems(); i++) {
		ChipView* chip = fTags.ItemAt(i);
		chip->SetTarget(Window());
	}

	_DoLayout();
}


void
TagsFlowView::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);
	_DoLayout();
}


void
TagsFlowView::GetPreferredSize(float* width, float* height)
{
	if (width != NULL)
		*width = Bounds().Width();
	if (height != NULL)
		*height = fCachedHeight;
}


BSize
TagsFlowView::MinSize()
{
	return BSize(100, kMinHeight);
}


BSize
TagsFlowView::MaxSize()
{
	return BSize(B_SIZE_UNLIMITED, fCachedHeight);
}


BSize
TagsFlowView::PreferredSize()
{
	return BSize(Bounds().Width(), fCachedHeight);
}


void
TagsFlowView::AddTag(const char* name, BMessage* message)
{
	ChipView* chip = new ChipView(name, name, message, B_CHIP_STYLE_CATEGORY);
	fTags.AddItem(chip);
	AddChild(chip);

	if (Window() != NULL) {
		chip->SetTarget(Window());
		_DoLayout();
	}
}


void
TagsFlowView::ClearTags()
{
	for (int32 i = fTags.CountItems() - 1; i >= 0; i--) {
		ChipView* chip = fTags.ItemAt(i);
		RemoveChild(chip);
	}
	fTags.MakeEmpty();
	fCachedHeight = kMinHeight;

	if (Window() != NULL)
		InvalidateLayout();
}


int32
TagsFlowView::CountTags() const
{
	return fTags.CountItems();
}


void
TagsFlowView::GetSelectedTags(BString& outTags) const
{
	outTags = "";

	for (int32 i = 0; i < fTags.CountItems(); i++) {
		ChipView* chip = fTags.ItemAt(i);
		if (chip->Value() == B_CONTROL_ON) {
			if (!outTags.IsEmpty())
				outTags << ",";
			outTags << "[" << chip->Label() << "]";
		}
	}
}


void
TagsFlowView::ToggleTag(const char* name)
{
	for (int32 i = 0; i < fTags.CountItems(); i++) {
		ChipView* chip = fTags.ItemAt(i);
		if (strcmp(chip->Label(), name) == 0) {
			chip->SetValue(chip->Value() == B_CONTROL_ON ? B_CONTROL_OFF : B_CONTROL_ON);
			chip->Invoke();
			return;
		}
	}
}


void
TagsFlowView::DeselectAll()
{
	for (int32 i = 0; i < fTags.CountItems(); i++) {
		ChipView* chip = fTags.ItemAt(i);
		if (chip->Value() == B_CONTROL_ON) {
			chip->SetValue(B_CONTROL_OFF);
		}
	}
}


void
TagsFlowView::_DoLayout()
{
	float width = Bounds().Width();
	if (width < 50)
		return;

	float x = kPadding;
	float y = kPadding;
	float rowHeight = 0;
	float maxWidth = width - kPadding * 2;

	for (int32 i = 0; i < fTags.CountItems(); i++) {
		ChipView* chip = fTags.ItemAt(i);

		float chipWidth, chipHeight;
		chip->GetPreferredSize(&chipWidth, &chipHeight);

		if (x + chipWidth > maxWidth && x > kPadding) {
			x = kPadding;
			y += rowHeight + kVSpacing;
			rowHeight = 0;
		}

		chip->MoveTo(x, y);
		chip->ResizeTo(chipWidth, chipHeight);

		if (chipHeight > rowHeight)
			rowHeight = chipHeight;

		x += chipWidth + kHSpacing;
	}

	float newHeight = y + rowHeight + kPadding;
	if (newHeight < kMinHeight)
		newHeight = kMinHeight;

	if (fCachedHeight != newHeight) {
		fCachedHeight = newHeight;
		SetExplicitMinSize(BSize(100, fCachedHeight));
		SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, fCachedHeight));
		SetExplicitPreferredSize(BSize(B_SIZE_UNSET, fCachedHeight));
		InvalidateLayout();
	}
}
