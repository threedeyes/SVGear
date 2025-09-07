/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SVG_HIGHLIGHTER_H
#define SVG_HIGHLIGHTER_H

#include <ctype.h>
#include <string.h>

#include "SVGTextEdit_Highlighters.h"

syntax_type
DetectSVGFromFilename(const char* filename)
{
	if (!filename)
		return SYNTAX_NONE;

	const char* ext = strrchr(filename, '.');
	if (!ext)
		return SYNTAX_NONE;

	if (strcmp(ext, ".svg") == 0 || strcmp(ext, ".xml") == 0)
		return SYNTAX_SVG_XML;

	return SYNTAX_NONE;
}

syntax_type
DetectSVGFromContent(const char* text, int32 length)
{
	if (!text || length == 0)
		return SYNTAX_NONE;

	if (length > 5 && strncmp(text, "<?xml", 5) == 0)
		return SYNTAX_SVG_XML;

	if (strstr(text, "<svg") != NULL)
		return SYNTAX_SVG_XML;

	if (strstr(text, "</") != NULL && strstr(text, "/>") != NULL)
		return SYNTAX_SVG_XML;

	if (strstr(text, "xmlns") != NULL)
		return SYNTAX_SVG_XML;

	return SYNTAX_NONE;
}

static void
_HighlightXMLTag(BTextView* view, int32 start, int32 end, const ColorScheme& colors)
{
	const char* text = view->Text();

	// Highlight < and >
	SetColorRange(view, start, start + 1, colors.tag);
	SetColorRange(view, end - 1, end, colors.tag);

	int32 pos = start + 1;

	// Handle closing tags
	if (pos < end && text[pos] == '/') {
		SetColorRange(view, pos, pos + 1, colors.tag);
		pos++;
	}

	// Tag name
	int32 tagNameStart = pos;
	while (pos < end - 1 && !isspace(text[pos]) && text[pos] != '>' && text[pos] != '/') {
		pos++;
	}

	if (pos > tagNameStart) {
		SetColorRange(view, tagNameStart, pos, colors.tag);
	}

	// Self-closing tag
	bool selfClosing = false;
	if (pos < end - 1 && text[pos] == '/') {
		selfClosing = true;
	}

	// Attributes
	while (pos < end - 1) {
		// Skip whitespace
		while (pos < end - 1 && isspace(text[pos])) {
			pos++;
		}

		if (pos >= end - 1 || text[pos] == '/' || text[pos] == '>') {
			break;
		}

		// Attribute name
		int32 attrStart = pos;
		while (pos < end - 1 && !isspace(text[pos]) && text[pos] != '=' && text[pos] != '>' && text[pos] != '/') {
			pos++;
		}

		if (pos > attrStart) {
			SetColorRange(view, attrStart, pos, colors.attribute);
		}

		// Skip whitespace and =
		while (pos < end - 1 && (isspace(text[pos]) || text[pos] == '=')) {
			pos++;
		}

		// Attribute value
		if (pos < end - 1 && (text[pos] == '"' || text[pos] == '\'')) {
			char quote = text[pos];
			int32 valueStart = pos;
			pos++;

			while (pos < end - 1 && text[pos] != quote) {
				pos++;
			}

			if (pos < end - 1) {
				pos++;
				SetColorRange(view, valueStart, pos, colors.string);
			}
		}
	}

	// Handle self-closing tag
	if (selfClosing && end >= 2) {
		SetColorRange(view, end - 2, end - 1, colors.tag);
	}
}

void
ApplySVGHighlighting(BTextView* view)
{
	const char* text = view->Text();
	int32 length = view->TextLength();

	if (length == 0)
		return;

	const ColorScheme& colors = GetColorScheme(view);

	SetColorRange(view, 0, length, colors.text);

	int32 pos = 0;
	while (pos < length) {
		// XML Comments
		if (pos + 3 < length && strncmp(&text[pos], "<!--", 4) == 0) {
			int32 commentEnd = pos + 4;
			while (commentEnd + 2 < length && strncmp(&text[commentEnd], "-->", 3) != 0) {
				commentEnd++;
			}
			if (commentEnd + 2 < length) {
				commentEnd += 3;
			}
			SetColorRange(view, pos, commentEnd, colors.comment);
			pos = commentEnd;
			continue;
		}

		// XML Declaration
		if (pos + 1 < length && strncmp(&text[pos], "<?", 2) == 0) {
			int32 declEnd = pos + 2;
			while (declEnd + 1 < length && strncmp(&text[declEnd], "?>", 2) != 0) {
				declEnd++;
			}
			if (declEnd + 1 < length) {
				declEnd += 2;
			}
			SetColorRange(view, pos, declEnd, colors.preprocessor);
			pos = declEnd;
			continue;
		}

		// CDATA sections
		if (pos + 8 < length && strncmp(&text[pos], "<![CDATA[", 9) == 0) {
			int32 cdataEnd = pos + 9;
			while (cdataEnd + 2 < length && strncmp(&text[cdataEnd], "]]>", 3) != 0) {
				cdataEnd++;
			}
			if (cdataEnd + 2 < length) {
				cdataEnd += 3;
			}
			SetColorRange(view, pos, cdataEnd, colors.string);
			pos = cdataEnd;
			continue;
		}

		// XML Tags
		if (text[pos] == '<') {
			int32 tagEnd = pos + 1;
			while (tagEnd < length && text[tagEnd] != '>') {
				tagEnd++;
			}

			if (tagEnd < length) {
				tagEnd++;
				_HighlightXMLTag(view, pos, tagEnd, colors);
				pos = tagEnd;
			} else {
				pos++;
			}
		} else {
			pos++;
		}
	}
}

#endif
