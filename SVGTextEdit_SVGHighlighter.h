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
_AnalyzeXMLTag(const char* text, int32 start, int32 end, BList* ranges)
{
	// Add < and >
	HighlightRange* range1 = new HighlightRange(start, start + 1, HIGHLIGHT_TAG);
	if (range1) ranges->AddItem(range1);

	HighlightRange* range2 = new HighlightRange(end - 1, end, HIGHLIGHT_TAG);
	if (range2) ranges->AddItem(range2);

	int32 pos = start + 1;

	// Handle closing tags
	if (pos < end && text[pos] == '/') {
		HighlightRange* slashRange = new HighlightRange(pos, pos + 1, HIGHLIGHT_TAG);
		if (slashRange) ranges->AddItem(slashRange);
		pos++;
	}

	// Tag name
	int32 tagNameStart = pos;
	while (pos < end - 1 && !isspace(text[pos]) && text[pos] != '>' && text[pos] != '/') {
		pos++;
	}

	if (pos > tagNameStart) {
		HighlightRange* tagRange = new HighlightRange(tagNameStart, pos, HIGHLIGHT_TAG);
		if (tagRange) ranges->AddItem(tagRange);
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
			HighlightRange* attrRange = new HighlightRange(attrStart, pos, HIGHLIGHT_ATTRIBUTE);
			if (attrRange) ranges->AddItem(attrRange);
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
				HighlightRange* valueRange = new HighlightRange(valueStart, pos, HIGHLIGHT_STRING);
				if (valueRange) ranges->AddItem(valueRange);
			}
		}
	}

	// Handle self-closing tag
	if (selfClosing && end >= 2) {
		HighlightRange* closeRange = new HighlightRange(end - 2, end - 1, HIGHLIGHT_TAG);
		if (closeRange) ranges->AddItem(closeRange);
	}
}

void
HighlightWorker::_AnalyzeSVGSyntax(const char* text, int32 length, BList* ranges)
{
	if (!text || length == 0 || !ranges)
		return;

	int32 pos = 0;

	while (pos < length && !fShutdown) {
		// XML Comments
		if (pos + 4 <= length && strncmp(&text[pos], "<!--", 4) == 0) {
			int32 commentEnd = pos + 4;
			while (commentEnd + 3 <= length && strncmp(&text[commentEnd], "-->", 3) != 0) {
				commentEnd++;
			}
			if (commentEnd + 3 <= length) {
				commentEnd += 3;
			}
			_AddRange(ranges, pos, commentEnd, HIGHLIGHT_COMMENT);
			pos = commentEnd;
			continue;
		}

		// XML Declaration
		if (pos + 2 <= length && strncmp(&text[pos], "<?", 2) == 0) {
			int32 declEnd = pos + 2;
			while (declEnd + 2 <= length && strncmp(&text[declEnd], "?>", 2) != 0) {
				declEnd++;
			}
			if (declEnd + 2 <= length) {
				declEnd += 2;
			}
			_AddRange(ranges, pos, declEnd, HIGHLIGHT_PREPROCESSOR);
			pos = declEnd;
			continue;
		}

		// CDATA sections
		if (pos + 9 <= length && strncmp(&text[pos], "<![CDATA[", 9) == 0) {
			int32 cdataEnd = pos + 9;
			while (cdataEnd + 3 <= length && strncmp(&text[cdataEnd], "]]>", 3) != 0) {
				cdataEnd++;
			}
			if (cdataEnd + 3 <= length) {
				cdataEnd += 3;
			}
			_AddRange(ranges, pos, cdataEnd, HIGHLIGHT_STRING);
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
				_AnalyzeXMLTag(text, pos, tagEnd, ranges);
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
