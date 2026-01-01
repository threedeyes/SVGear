/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <cstdio>

#include "TagsFlowView.h"

const float TagsFlowView::kHSpacing = 8.0f;
const float TagsFlowView::kVSpacing = 4.0f;
const float TagsFlowView::kPadding = 4.0f;
const float TagsFlowView::kMinHeight = 28.0f;


TagsFlowView::TagsFlowView()
	:
	BView("TagsFlow", B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE),
	fCheckBoxes(20),
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
	BCheckBox* cb = new BCheckBox(name, name, message);
	fCheckBoxes.AddItem(cb);
	AddChild(cb);

	if (Window() != NULL)
		_DoLayout();
}


void
TagsFlowView::ClearTags()
{
	for (int32 i = fCheckBoxes.CountItems() - 1; i >= 0; i--) {
		BCheckBox* cb = fCheckBoxes.ItemAt(i);
		RemoveChild(cb);
	}
	fCheckBoxes.MakeEmpty();
	fCachedHeight = kMinHeight;

	if (Window() != NULL)
		InvalidateLayout();
}


int32
TagsFlowView::CountTags() const
{
	return fCheckBoxes.CountItems();
}


void
TagsFlowView::GetSelectedTags(BString& outTags) const
{
	outTags = "";

	for (int32 i = 0; i < fCheckBoxes.CountItems(); i++) {
		BCheckBox* cb = fCheckBoxes.ItemAt(i);
		if (cb->Value() == B_CONTROL_ON) {
			if (!outTags.IsEmpty())
				outTags << ",";
			outTags << "[" << cb->Label() << "]";
		}
	}
}


float
TagsFlowView::_CalculateHeight(float width) const
{
	if (fCheckBoxes.IsEmpty())
		return kMinHeight;

	float x = kPadding;
	float y = kPadding;
	float rowHeight = 0;
	float maxWidth = width - kPadding * 2;

	for (int32 i = 0; i < fCheckBoxes.CountItems(); i++) {
		BCheckBox* cb = fCheckBoxes.ItemAt(i);

		float cbWidth, cbHeight;
		cb->GetPreferredSize(&cbWidth, &cbHeight);

		if (x + cbWidth > maxWidth && x > kPadding) {
			x = kPadding;
			y += rowHeight + kVSpacing;
			rowHeight = 0;
		}

		if (cbHeight > rowHeight)
			rowHeight = cbHeight;

		x += cbWidth + kHSpacing;
	}

	return y + rowHeight + kPadding;
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

	for (int32 i = 0; i < fCheckBoxes.CountItems(); i++) {
		BCheckBox* cb = fCheckBoxes.ItemAt(i);

		float cbWidth, cbHeight;
		cb->GetPreferredSize(&cbWidth, &cbHeight);

		if (x + cbWidth > maxWidth && x > kPadding) {
			x = kPadding;
			y += rowHeight + kVSpacing;
			rowHeight = 0;
		}

		cb->MoveTo(x, y);
		cb->ResizeTo(cbWidth, cbHeight);

		if (cbHeight > rowHeight)
			rowHeight = cbHeight;

		x += cbWidth + kHSpacing;
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
