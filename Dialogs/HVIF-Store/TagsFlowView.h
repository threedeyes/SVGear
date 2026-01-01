/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef TAGS_FLOW_VIEW_H
#define TAGS_FLOW_VIEW_H

#include <View.h>
#include <CheckBox.h>
#include <ObjectList.h>
#include <String.h>

class TagsFlowView : public BView {
public:
							TagsFlowView();
	virtual                 ~TagsFlowView();

	virtual void            AttachedToWindow();
	virtual void            FrameResized(float width, float height);
	virtual void            GetPreferredSize(float* width, float* height);
	virtual BSize           MinSize();
	virtual BSize           MaxSize();
	virtual BSize           PreferredSize();

			void            AddTag(const char* name, BMessage* message);
			void            ClearTags();
			int32           CountTags() const;
			void            GetSelectedTags(BString& outTags) const;

private:
			void            _DoLayout();
			float           _CalculateHeight(float width) const;

#if B_HAIKU_VERSION > B_HAIKU_VERSION_1_BETA_5
			BObjectList<BCheckBox, true> fCheckBoxes;
#else
			BObjectList<BCheckBox> fCheckBoxes;
#endif
			float           fCachedHeight;
			
	static const float      kHSpacing;
	static const float      kVSpacing;
	static const float      kPadding;
	static const float      kMinHeight;
};

#endif
