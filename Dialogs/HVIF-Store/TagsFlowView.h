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

			void			AddTag(const char* name, BMessage* message);
			void			ClearTags();
			int32			CountTags() const;
			void			GetSelectedTags(BString& outTags) const;

			void			ToggleTag(const char* name);
			void			DeselectAll();

private:
			void			_DoLayout();

#if B_HAIKU_VERSION > B_HAIKU_VERSION_1_BETA_5
			BObjectList<ChipView, true> fTags;
#else
			BObjectList<ChipView> fTags;
#endif
			float			fCachedHeight;

	static const float		kHSpacing;
	static const float		kVSpacing;
	static const float		kPadding;
	static const float		kMinHeight;
};

#endif
