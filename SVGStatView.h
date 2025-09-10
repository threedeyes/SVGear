/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SVGSTAT_VIEW
#define SVGSTAT_VIEW

#include <Window.h>
#include <View.h>
#include <GroupView.h>
#include <String.h>
#include <StringView.h>
#include <SpaceLayoutItem.h>
#include <ControlLook.h>
#include <Font.h>

struct NSVGimage;

class SVGStatView : public BView {
	public:
		SVGStatView(const char* name = "stat_view");
		~SVGStatView() { };

		virtual void Draw(BRect rect);

		void SetSVGImage(NSVGimage* image);
		void SetFloatValue(const char *param, float value, bool exp = true);
		void SetIntValue(const char *param, int value);
		void SetTextValue(const char *param, const char *value);

	private:
		void UpdateStatistics();
		int CountShapes(NSVGimage* image);
		int CountPaths(NSVGimage* image);
		void GetImageBounds(NSVGimage* image, float* bounds);

		BGroupView *view;
		BFont font;
		NSVGimage* svgImage;
};

#endif
