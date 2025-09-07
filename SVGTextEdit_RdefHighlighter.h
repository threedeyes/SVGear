/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef RDEF_HIGHLIGHTER_H
#define RDEF_HIGHLIGHTER_H

#include <ctype.h>
#include <string.h>

#include "SVGTextEdit_Highlighters.h"

syntax_type
DetectRdefFromFilename(const char* filename)
{
	if (!filename)
		return SYNTAX_NONE;

	const char* ext = strrchr(filename, '.');
	if (!ext)
		return SYNTAX_NONE;

	if (strcmp(ext, ".rdef") == 0)
		return SYNTAX_RDEF;

	return SYNTAX_NONE;
}

syntax_type
DetectRdefFromContent(const char* text, int32 length)
{
	if (!text || length == 0)
		return SYNTAX_NONE;

	if (strstr(text, "resource(") != NULL)
		return SYNTAX_RDEF;

	if (strstr(text, "array {") != NULL)
		return SYNTAX_RDEF;

	if (strstr(text, "#'") != NULL)
		return SYNTAX_RDEF;

	if (strstr(text, "$\"") != NULL)
		return SYNTAX_RDEF;

	if (strstr(text, "R_") != NULL)
		return SYNTAX_RDEF;

	return SYNTAX_NONE;
}

static bool
_IsRdefKeyword(const char* word, int32 length)
{
	static const char* keywords[] = {
		"resource", "array", "message", "archive", "true", "false",
		"enum", "type", "data", "import", "read", "write", "file",
		NULL
	};

	for (int i = 0; keywords[i]; i++) {
		if (strlen(keywords[i]) == (size_t)length &&
			strncmp(word, keywords[i], length) == 0)
			return true;
	}
	return false;
}

static bool
_IsRdefType(const char* word, int32 length)
{
	static const char* types[] = {
		"bool", "int8", "int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64",
		"float", "double", "string", "raw", "point", "rect", "rgb_color", "pattern",
		"message", "mime", "large_icon", "mini_icon", "vector_icon",
		NULL
	};

	for (int i = 0; types[i]; i++) {
		if (strlen(types[i]) == (size_t)length &&
			strncmp(word, types[i], length) == 0)
			return true;
	}
	return false;
}

void
ApplyRdefHighlighting(BTextView* view)
{
	const char* text = view->Text();
	int32 length = view->TextLength();

	if (length == 0)
		return;

	const ColorScheme& colors = GetColorScheme(view);

	SetColorRange(view, 0, length, colors.text);

	int32 pos = 0;

	while (pos < length) {
		// Skip whitespace
		while (pos < length && isspace(text[pos])) {
			pos++;
		}
		if (pos >= length) break;

		// Comments
		if (pos + 1 < length && text[pos] == '/' && text[pos + 1] == '/') {
			int32 lineEnd = pos;
			while (lineEnd < length && text[lineEnd] != '\n') {
				lineEnd++;
			}
			SetColorRange(view, pos, lineEnd, colors.comment);
			pos = lineEnd;
			continue;
		}

		// Multi-line comments
		if (pos + 1 < length && text[pos] == '/' && text[pos + 1] == '*') {
			int32 commentEnd = pos + 2;
			while (commentEnd + 1 < length && 
				   !(text[commentEnd] == '*' && text[commentEnd + 1] == '/')) {
				commentEnd++;
			}
			if (commentEnd + 1 < length) {
				commentEnd += 2;
			}
			SetColorRange(view, pos, commentEnd, colors.comment);
			pos = commentEnd;
			continue;
		}

		// Resource strings ($"string")
		if (text[pos] == '$' && pos + 1 < length && text[pos + 1] == '"') {
			int32 stringEnd = pos + 2;
			while (stringEnd < length && text[stringEnd] != '"') {
				if (text[stringEnd] == '\\' && stringEnd + 1 < length) {
					stringEnd += 2;
				} else {
					stringEnd++;
				}
			}
			if (stringEnd < length) {
				stringEnd++;
			}
			SetColorRange(view, pos, stringEnd, colors.string);
			pos = stringEnd;
			continue;
		}

		// Regular strings
		if (text[pos] == '"') {
			int32 stringEnd = pos + 1;
			while (stringEnd < length && text[stringEnd] != '"') {
				if (text[stringEnd] == '\\' && stringEnd + 1 < length) {
					stringEnd += 2;
				} else {
					stringEnd++;
				}
			}
			if (stringEnd < length) {
				stringEnd++;
			}
			SetColorRange(view, pos, stringEnd, colors.string);
			pos = stringEnd;
			continue;
		}

		// Type codes (#'TYPE')
		if (text[pos] == '#' && pos + 1 < length && text[pos + 1] == '\'') {
			int32 typeEnd = pos + 2;
			while (typeEnd < length && text[typeEnd] != '\'') {
				typeEnd++;
			}
			if (typeEnd < length) {
				typeEnd++;
			}
			SetColorRange(view, pos, typeEnd, colors.number);
			pos = typeEnd;
			continue;
		}

		// Hexadecimal numbers
		if (pos + 1 < length && text[pos] == '0' &&
			(text[pos + 1] == 'x' || text[pos + 1] == 'X')) {
			int32 numEnd = pos + 2;
			while (numEnd < length &&
				   (isdigit(text[numEnd]) || 
					(text[numEnd] >= 'a' && text[numEnd] <= 'f') ||
					(text[numEnd] >= 'A' && text[numEnd] <= 'F'))) {
				numEnd++;
			}
			SetColorRange(view, pos, numEnd, colors.number);
			pos = numEnd;
			continue;
		}

		// Decimal numbers and floats
		if (isdigit(text[pos]) || (text[pos] == '.' && pos + 1 < length && isdigit(text[pos + 1]))) {
			int32 numEnd = pos;
			bool hasDecimal = false;
			
			while (numEnd < length && (isdigit(text[numEnd]) || 
				   (!hasDecimal && text[numEnd] == '.'))) {
				if (text[numEnd] == '.') {
					hasDecimal = true;
				}
				numEnd++;
			}
			
			// Handle scientific notation
			if (numEnd < length && (text[numEnd] == 'e' || text[numEnd] == 'E')) {
				numEnd++;
				if (numEnd < length && (text[numEnd] == '+' || text[numEnd] == '-')) {
					numEnd++;
				}
				while (numEnd < length && isdigit(text[numEnd])) {
					numEnd++;
				}
			}
			
			// Handle float suffix
			if (numEnd < length && (text[numEnd] == 'f' || text[numEnd] == 'F')) {
				numEnd++;
			}
			
			SetColorRange(view, pos, numEnd, colors.number);
			pos = numEnd;
			continue;
		}

		// Resource IDs and constants
		if (text[pos] == 'R' && pos + 1 < length && text[pos + 1] == '_') {
			int32 idEnd = pos;
			while (idEnd < length && (isalnum(text[idEnd]) || text[idEnd] == '_')) {
				idEnd++;
			}
			SetColorRange(view, pos, idEnd, colors.preprocessor);
			pos = idEnd;
			continue;
		}

		// Keywords and identifiers
		if (isalpha(text[pos]) || text[pos] == '_') {
			int32 wordEnd = pos;
			while (wordEnd < length && (isalnum(text[wordEnd]) || text[wordEnd] == '_')) {
				wordEnd++;
			}

			if (_IsRdefKeyword(text + pos, wordEnd - pos)) {
				SetColorRange(view, pos, wordEnd, colors.keyword);
			} else if (_IsRdefType(text + pos, wordEnd - pos)) {
				SetColorRange(view, pos, wordEnd, colors.attribute);
			}
			pos = wordEnd;
			continue;
		}

		// Operators and punctuation
		if (text[pos] == '{' || text[pos] == '}' || text[pos] == '(' || 
			text[pos] == ')' || text[pos] == '[' || text[pos] == ']' ||
			text[pos] == ',' || text[pos] == ';' || text[pos] == '=' ||
			text[pos] == '+' || text[pos] == '-' || text[pos] == '*' ||
			text[pos] == '/' || text[pos] == '%' || text[pos] == '&' ||
			text[pos] == '|' || text[pos] == '^' || text[pos] == '!' ||
			text[pos] == '<' || text[pos] == '>' || text[pos] == '?' ||
			text[pos] == ':' || text[pos] == '~') {
			SetColorRange(view, pos, pos + 1, colors.operator_color);
		}

		pos++;
	}
}

#endif
