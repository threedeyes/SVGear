/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SYNTAX_HIGHLIGHTERS_H
#define SYNTAX_HIGHLIGHTERS_H

#include <TextView.h>
#include <Font.h>
#include <List.h>

struct ColorScheme {
	rgb_color text;
	rgb_color keyword;
	rgb_color string;
	rgb_color comment;
	rgb_color number;
	rgb_color operator_color;
	rgb_color tag;
	rgb_color attribute;
	rgb_color preprocessor;
};

static const ColorScheme kLightColors = {
	{0, 0, 0, 255},         // text
	{0, 0, 255, 255},       // keyword
	{0, 128, 0, 255},       // string
	{128, 128, 128, 255},   // comment
	{255, 0, 0, 255},       // number
	{255, 140, 0, 255},     // operator
	{0, 0, 128, 255},       // tag
	{128, 0, 128, 255},     // attribute
	{128, 0, 255, 255}      // preprocessor
};

static const ColorScheme kDarkColors = {
	{220, 220, 220, 255},   // text
	{100, 150, 255, 255},   // keyword
	{150, 255, 150, 255},   // string
	{150, 150, 150, 255},   // comment
	{255, 100, 100, 255},   // number
	{255, 200, 100, 255},   // operator
	{150, 150, 255, 255},   // tag
	{255, 150, 255, 255},   // attribute
	{200, 150, 255, 255}    // preprocessor
};

enum syntax_type;
class HighlightWorker;

bool IsBackgroundDark(BTextView* view);
const ColorScheme& GetColorScheme(BTextView* view);
void SetColorRange(BTextView* view, int32 start, int32 end, const rgb_color& color);

inline bool IsBackgroundDark(BTextView* view)
{
	rgb_color bg = view->ViewColor();
	int brightness = (bg.red * 299 + bg.green * 587 + bg.blue * 114) / 1000;
	return brightness < 128;
}

inline const ColorScheme& GetColorScheme(BTextView* view)
{
	return IsBackgroundDark(view) ? kDarkColors : kLightColors;
}

inline void SetColorRange(BTextView* view, int32 start, int32 end, const rgb_color& color)
{
	BFont font(be_fixed_font);
	view->SetFontAndColor(start, end, &font, B_FONT_ALL, &color);
}

#define DECLARE_HIGHLIGHTER(name, type) \
	syntax_type Detect##name##FromFilename(const char* filename); \
	syntax_type Detect##name##FromContent(const char* text, int32 length);

#define REGISTER_FILENAME_DETECTOR(name) \
	{ \
		syntax_type detected = Detect##name##FromFilename(filename); \
		if (detected != SYNTAX_NONE) return detected; \
	}

#define REGISTER_CONTENT_DETECTOR(name) \
	{ \
		syntax_type detected = Detect##name##FromContent(text, length); \
		if (detected != SYNTAX_NONE) return detected; \
	}

DECLARE_HIGHLIGHTER(SVG, SYNTAX_SVG_XML)
DECLARE_HIGHLIGHTER(Cpp, SYNTAX_CPP)
DECLARE_HIGHLIGHTER(Rdef, SYNTAX_RDEF)

#include "SVGTextEdit_SVGHighlighter.h"
#include "SVGTextEdit_CppHighlighter.h"
#include "SVGTextEdit_RdefHighlighter.h"

#endif
