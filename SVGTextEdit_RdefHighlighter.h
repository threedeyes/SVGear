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
		if ((int32)strlen(keywords[i]) == length &&
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
		if ((int32)strlen(types[i]) == length &&
			strncmp(word, types[i], length) == 0)
			return true;
	}
	return false;
}

void
HighlightWorker::_AnalyzeRdefSyntax(const char* text, int32 length, BList* ranges)
{
	if (!text || length == 0 || !ranges)
		return;

	int32 pos = 0;

	while (pos < length && !fShutdown) {
		// Skip whitespace
		while (pos < length && isspace((unsigned char)text[pos])) {
			pos++;
		}
		if (pos >= length || fShutdown) break;

		// Line comments
		if (pos + 1 < length && text[pos] == '/' && text[pos + 1] == '/') {
			int32 lineEnd = pos;
			while (lineEnd < length && text[lineEnd] != '\n') {
				lineEnd++;
			}
			_AddRange(ranges, pos, lineEnd, HIGHLIGHT_COMMENT);
			pos = lineEnd;
			continue;
		}

		// Block comments
		if (pos + 1 < length && text[pos] == '/' && text[pos + 1] == '*') {
			int32 commentEnd = pos + 2;
			while (commentEnd + 1 < length && 
				   !(text[commentEnd] == '*' && text[commentEnd + 1] == '/')) {
				commentEnd++;
			}
			if (commentEnd + 1 < length) {
				commentEnd += 2;
			}
			_AddRange(ranges, pos, commentEnd, HIGHLIGHT_COMMENT);
			pos = commentEnd;
			continue;
		}

		// Hex strings: $"..."
		if (pos + 1 < length && text[pos] == '$' && text[pos + 1] == '"') {
			int32 stringEnd = pos + 2;
			while (stringEnd < length && text[stringEnd] != '"') {
				stringEnd++;
			}
			if (stringEnd < length) {
				stringEnd++; // Include closing quote
			}
			_AddRange(ranges, pos, stringEnd, HIGHLIGHT_STRING);
			pos = stringEnd;
			continue;
		}

		// Regular strings
		if (text[pos] == '"') {
			int32 stringEnd = pos + 1;
			while (stringEnd < length && text[stringEnd] != '"') {
				if (text[stringEnd] == '\\' && stringEnd + 1 < length) {
					stringEnd += 2; // Skip escaped character
				} else {
					stringEnd++;
				}
			}
			if (stringEnd < length) {
				stringEnd++; // Include closing quote
			}
			_AddRange(ranges, pos, stringEnd, HIGHLIGHT_STRING);
			pos = stringEnd;
			continue;
		}

		// Resource type: #'TYPE'
		if (pos + 2 < length && text[pos] == '#' && text[pos + 1] == '\'') {
			int32 typeEnd = pos + 2;
			while (typeEnd < length && text[typeEnd] != '\'') {
				typeEnd++;
			}
			if (typeEnd < length) {
				typeEnd++; // Include closing quote
			}
			_AddRange(ranges, pos, typeEnd, HIGHLIGHT_PREPROCESSOR);
			pos = typeEnd;
			continue;
		}

		// Numbers
		if (isdigit((unsigned char)text[pos]) || 
			(text[pos] == '0' && pos + 1 < length &&
			 (text[pos + 1] == 'x' || text[pos + 1] == 'X'))) {
			int32 numEnd = pos;
			
			if (text[pos] == '0' && pos + 1 < length &&
				(text[pos + 1] == 'x' || text[pos + 1] == 'X')) {
				numEnd += 2; // Skip 0x
				while (numEnd < length && isxdigit((unsigned char)text[numEnd])) {
					numEnd++;
				}
			} else {
				while (numEnd < length && isdigit((unsigned char)text[numEnd])) {
					numEnd++;
				}
			}
			
			_AddRange(ranges, pos, numEnd, HIGHLIGHT_NUMBER);
			pos = numEnd;
			continue;
		}

		// Keywords and identifiers
		if (isalpha((unsigned char)text[pos]) || text[pos] == '_') {
			int32 wordEnd = pos;
			while (wordEnd < length && !fShutdown) {
				unsigned char ch = (unsigned char)text[wordEnd];
				if (!(isalnum(ch) || ch == '_')) {
					break;
				}
				wordEnd++;
			}

			if (_IsRdefKeyword(text + pos, wordEnd - pos)) {
				_AddRange(ranges, pos, wordEnd, HIGHLIGHT_KEYWORD);
			} else if (_IsRdefType(text + pos, wordEnd - pos)) {
				_AddRange(ranges, pos, wordEnd, HIGHLIGHT_PREPROCESSOR);
			}
			pos = wordEnd;
			continue;
		}

		// Operators and delimiters
		if (text[pos] == '{' || text[pos] == '}' || text[pos] == '(' || 
			text[pos] == ')' || text[pos] == '[' || text[pos] == ']' ||
			text[pos] == ',' || text[pos] == ';' || text[pos] == '=') {
			_AddRange(ranges, pos, pos + 1, HIGHLIGHT_OPERATOR);
		}

		pos++;
	}
}

#endif
