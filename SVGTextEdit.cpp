/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <ctype.h>

#include "SVGTextEdit.h"

SVGTextEdit::SVGTextEdit(const char* name)
	: BTextView(name)
{
	SetWordWrap(false);
	MakeEditable(true);
	SetStylable(true);
	
	SetExplicitMinSize(BSize(32, 32));
	
	BFont sourceFont(be_fixed_font);
	SetFontAndColor(&sourceFont, B_FONT_ALL, &kColorDefault);
	
	SetDoesUndo(true);
}

void
SVGTextEdit::InsertText(const char* text, int32 length, int32 offset, const text_run_array* runs)
{
	BTextView::InsertText(text, length, offset, runs);
	_ApplySyntaxHighlighting();
}

void
SVGTextEdit::DeleteText(int32 start, int32 finish)
{
	BTextView::DeleteText(start, finish);
	_ApplySyntaxHighlighting();
}

void
SVGTextEdit::ApplySyntaxHighlighting()
{
	_ApplySyntaxHighlighting();
}

void
SVGTextEdit::_ApplySyntaxHighlighting()
{
	const char* text = Text();
	int32 length = TextLength();

	if (length == 0)
		return;

	BFont font(be_fixed_font);
	SetFontAndColor(0, length, &font, B_FONT_ALL, &kColorDefault);

	int32 pos = 0;
	while (pos < length) {
		if (text[pos] == '<') {
			if (pos + 3 < length && strncmp(&text[pos], "<!--", 4) == 0) {
				// Comment
				int32 commentEnd = pos + 4;
				while (commentEnd + 2 < length && strncmp(&text[commentEnd], "-->", 3) != 0) {
					commentEnd++;
				}
				if (commentEnd + 2 < length) {
					commentEnd += 3;
					SetFontAndColor(pos, commentEnd, &font, B_FONT_ALL, &kColorComment);
					pos = commentEnd;
					continue;
				}
			}

			// Tag
			int32 tagEnd = pos + 1;
			while (tagEnd < length && text[tagEnd] != '>') {
				tagEnd++;
			}

			if (tagEnd < length) {
				tagEnd++; // Include '>'
				_HighlightTag(pos, tagEnd, font);
				pos = tagEnd;
			} else {
				pos++;
			}
		} else {
			pos++;
		}
	}
}

void
SVGTextEdit::_HighlightTag(int32 start, int32 end, const BFont& font)
{
	const char* text = Text();

	// Highlight brackets
	SetFontAndColor(start, start + 1, &font, B_FONT_ALL, &kColorTag); // '<'
	SetFontAndColor(end - 1, end, &font, B_FONT_ALL, &kColorTag); // '>'

	int32 pos = start + 1;

	// Skip '/' for closing tags
	if (pos < end && text[pos] == '/') {
		SetFontAndColor(pos, pos + 1, &font, B_FONT_ALL, &kColorTag);
		pos++;
	}

	// Find tag name
	int32 tagNameStart = pos;
	while (pos < end - 1 && !isspace(text[pos]) && text[pos] != '>')
		pos++;

	if (pos > tagNameStart)
		SetFontAndColor(tagNameStart, pos, &font, B_FONT_ALL, &kColorTag);

	// Parse attributes
	while (pos < end - 1) {
		while (pos < end - 1 && isspace(text[pos]))
			pos++;

		if (pos >= end - 1)
			break;

		// Attribute name
		int32 attrStart = pos;
		while (pos < end - 1 && !isspace(text[pos]) && text[pos] != '=' && text[pos] != '>')
			pos++;

		if (pos > attrStart)
			SetFontAndColor(attrStart, pos, &font, B_FONT_ALL, &kColorAttribute);

		// Skip whitespace and '='
		while (pos < end - 1 && (isspace(text[pos]) || text[pos] == '='))
			pos++;

		// Attribute value
		if (pos < end - 1 && (text[pos] == '"' || text[pos] == '\'')) {
			char quote = text[pos];
			int32 valueStart = pos;
			pos++; // Skip opening quote

			while (pos < end - 1 && text[pos] != quote)
				pos++;

			if (pos < end - 1) {
				pos++; // Include closing quote
				SetFontAndColor(valueStart, pos, &font, B_FONT_ALL, &kColorString);
			}
		}
	}
}
