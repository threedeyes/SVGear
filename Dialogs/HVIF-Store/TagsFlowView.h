/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef TAGS_FLOW_VIEW_H
#define TAGS_FLOW_VIEW_H

#include <View.h>
#include <ObjectList.h>
#include <String.h>

#include "ChipView.h"

struct TagItem {
	BString name;
	bool isMeta;
	bool isSelected;

	TagItem(const char* n) : name(n), isMeta(false), isSelected(false) {
		if (name.StartsWith("[") && name.EndsWith("]"))
			isMeta = true;
	}
};

class TagsFlowView : public BView {
public:
							TagsFlowView();
	virtual					~TagsFlowView();

	virtual void			AttachedToWindow();
	virtual void			FrameResized(float width, float height);
	virtual void			GetPreferredSize(float* width, float* height);
	virtual BSize			MinSize();
	virtual BSize			MaxSize();
	virtual BSize			PreferredSize();

			void			SetTags(const BMessage* tagsList);
			void			GetSelectedTags(BString& outTags) const;

			void			ToggleTag(const char* name);
			void			DeselectAll();

			void			ToggleExpanded();
			bool			IsExpanded() const { return fExpanded; }

			void			Filter(const char* query);

private:
			void			_RebuildViews();
			void			_DoLayout();

#if B_HAIKU_VERSION > B_HAIKU_VERSION_1_BETA_5
			BObjectList<TagItem, true> fTagItems;
			BObjectList<ChipView, false> fChipViews;
#else
			BObjectList<TagItem> fTagItems;
			BObjectList<ChipView> fChipViews;
#endif
			float			fCachedHeight;
			bool			fExpanded;

	static const float		kHSpacing;
	static const float		kVSpacing;
	static const float		kPadding;
	static const float		kMinHeight;
};

#endif
