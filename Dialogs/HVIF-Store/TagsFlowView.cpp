/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <cstdio>
#include <Catalog.h>
#include <Window.h>
#include <LayoutUtils.h>

#include "TagsFlowView.h"
#include "HvifStoreDefs.h"

#undef  B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT          "HVIFStoreDialog"

const float TagsFlowView::kHSpacing = 4.0f;
const float TagsFlowView::kVSpacing = 4.0f;
const float TagsFlowView::kPadding = 3.0f;
const float TagsFlowView::kMinHeight = 22.0f;


TagsFlowView::TagsFlowView()
	:
	BView("TagsFlow", B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE),
#if B_HAIKU_VERSION > B_HAIKU_VERSION_1_BETA_5
	fTagItems(20),
	fChipViews(20),
#else
	fTagItems(20, true),
	fChipViews(20, false),
#endif
	fCachedHeight(kMinHeight),
	fExpanded(false)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetExplicitMinSize(BSize(100, kMinHeight));
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
}


TagsFlowView::~TagsFlowView()
{
	while (!fChipViews.IsEmpty()) {
		ChipView* chip = fChipViews.RemoveItemAt(0);
		chip->RemoveSelf();
		delete chip;
	}
}


void
TagsFlowView::AttachedToWindow()
{
	BView::AttachedToWindow();
	_RebuildViews();
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
TagsFlowView::SetTags(const BMessage* tagsList)
{
	fTagItems.MakeEmpty();
	if (tagsList == NULL)
		return;

	int32 i = 0;
	char indexStr[32];
	while (true) {
		snprintf(indexStr, sizeof(indexStr), "%" B_PRId32, i);
		BMessage item;
		if (tagsList->FindMessage(indexStr, &item) != B_OK)
			break;

		BString tagName = item.GetString("name", "");
		if (!tagName.IsEmpty()) {
			fTagItems.AddItem(new TagItem(tagName.String()));
		}
		i++;
	}

	_RebuildViews();
}


void
TagsFlowView::GetSelectedTags(BString& outTags) const
{
	outTags = "";
	for (int32 i = 0; i < fTagItems.CountItems(); i++) {
		TagItem* item = fTagItems.ItemAt(i);
		if (item->isSelected) {
			if (!outTags.IsEmpty())
				outTags << ",";
			outTags << item->name;
		}
	}
}


void
TagsFlowView::ToggleTag(const char* name)
{
	bool changed = false;
	for (int32 i = 0; i < fTagItems.CountItems(); i++) {
		TagItem* item = fTagItems.ItemAt(i);
		if (item->name == name) {
			item->isSelected = !item->isSelected;
			changed = true;
			break;
		}
	}

	if (changed) {
		_RebuildViews();
		if (Window()) {
			Window()->PostMessage(kMsgTagToggled);
		}
	}
}


void
TagsFlowView::DeselectAll()
{
	bool changed = false;
	for (int32 i = 0; i < fTagItems.CountItems(); i++) {
		TagItem* item = fTagItems.ItemAt(i);
		if (item->isSelected) {
			item->isSelected = false;
			changed = true;
		}
	}

	if (changed) {
		if (fExpanded) {
			fExpanded = false;
			if (Window()) Window()->PostMessage(kMsgToggleTagsExpansion);
		}
		_RebuildViews();
	}
}


void
TagsFlowView::ToggleExpanded()
{
	fExpanded = !fExpanded;
	_RebuildViews();
}


void
TagsFlowView::Filter(const char* query)
{
	BString q(query);
	q.ToLower();

	for (int32 i = 0; i < fChipViews.CountItems(); i++) {
		ChipView* chip = fChipViews.ItemAt(i);

		if (chip->Style() == B_CHIP_STYLE_ACTION) {
			if (chip->IsHidden())
				chip->Show();
			continue;
		}

		BString label(chip->Label());
		label.ToLower();

		if (q.IsEmpty() || label.FindFirst(q) >= 0 || chip->Value() == B_CONTROL_ON) {
			if (chip->IsHidden())
				chip->Show();
		} else {
			if (!chip->IsHidden())
				chip->Hide();
		}
	}

	_DoLayout();
	Invalidate();
}


void
TagsFlowView::_RebuildViews()
{
	while (!fChipViews.IsEmpty()) {
		ChipView* chip = fChipViews.RemoveItemAt(0);
		chip->RemoveSelf();
		delete chip;
	}

	bool hasHiddenItems = false;
	for (int32 i = 0; i < fTagItems.CountItems(); i++) {
		TagItem* item = fTagItems.ItemAt(i);
		if (!item->isMeta && !item->isSelected) {
			hasHiddenItems = true;
			break;
		}
	}

	for (int32 i = 0; i < fTagItems.CountItems(); i++) {
		TagItem* item = fTagItems.ItemAt(i);
		if (item->isMeta) {
			BString label = item->name;
			label.Remove(0, 1);
			label.Truncate(label.Length() - 1);

			BMessage* msg = new BMessage(kMsgMetaTagClicked);
			msg->AddString("tag", item->name);

			ChipView* chip = new ChipView(label.String(), label.String(), msg, B_CHIP_STYLE_CATEGORY);
			chip->SetClickable(true);
			chip->SetTarget(Window());
			if (item->isSelected)
				chip->SetValue(B_CONTROL_ON);

			fChipViews.AddItem(chip);
			AddChild(chip);
		}
	}

	if (!fExpanded) {
		for (int32 i = 0; i < fTagItems.CountItems(); i++) {
			TagItem* item = fTagItems.ItemAt(i);
			if (!item->isMeta && item->isSelected) {
				BMessage* msg = new BMessage(kMsgMetaTagClicked);
				msg->AddString("tag", item->name);

				ChipView* chip = new ChipView(item->name.String(), item->name.String(), msg, B_CHIP_STYLE_TAG);
				chip->SetClickable(true);
				chip->SetTarget(Window());
				chip->SetValue(B_CONTROL_ON);

				fChipViews.AddItem(chip);
				AddChild(chip);
			}
		}
	}

	if (fExpanded) {
		for (int32 i = 0; i < fTagItems.CountItems(); i++) {
			TagItem* item = fTagItems.ItemAt(i);
			if (!item->isMeta) {
				BMessage* msg = new BMessage(kMsgMetaTagClicked);
				msg->AddString("tag", item->name);

				ChipView* chip = new ChipView(item->name.String(), item->name.String(), msg, B_CHIP_STYLE_TAG);
				chip->SetClickable(true);
				chip->SetTarget(Window());
				if (item->isSelected)
					chip->SetValue(B_CONTROL_ON);

				fChipViews.AddItem(chip);
				AddChild(chip);
			}
		}
	}

	if (hasHiddenItems || fExpanded) {
		BMessage* msg = new BMessage(kMsgToggleTagsExpansion);
		const char* label = fExpanded ? B_TRANSLATE("Hide tags") : B_TRANSLATE("Show all tags...");

		ChipView* toggleChip = new ChipView("toggle", label, msg, B_CHIP_STYLE_ACTION);
		toggleChip->SetClickable(true);
		toggleChip->SetTarget(Window());
		toggleChip->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));

		fChipViews.AddItem(toggleChip);
		AddChild(toggleChip);
	}

	_DoLayout();
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

	for (int32 i = 0; i < fChipViews.CountItems(); i++) {
		ChipView* chip = fChipViews.ItemAt(i);

		if (chip->IsHidden())
			continue;

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
		InvalidateLayout(true);
	}
}
