/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef CPP_HIGHLIGHTER_H
#define CPP_HIGHLIGHTER_H

#include <ctype.h>
#include <string.h>

#include "SVGTextEdit_Highlighters.h"

syntax_type
DetectCppFromFilename(const char* filename)
{
	if (!filename)
		return SYNTAX_NONE;

	const char* ext = strrchr(filename, '.');
	if (!ext)
		return SYNTAX_NONE;

	if (strcmp(ext, ".cpp") == 0 || strcmp(ext, ".h") == 0 ||
		strcmp(ext, ".cc") == 0 || strcmp(ext, ".cxx") == 0 ||
		strcmp(ext, ".hpp") == 0 || strcmp(ext, ".c") == 0)
		return SYNTAX_CPP;

	return SYNTAX_NONE;
}

syntax_type
DetectCppFromContent(const char* text, int32 length)
{
	if (!text || length == 0)
		return SYNTAX_NONE;

	if (strstr(text, "const") != NULL && strstr(text, "[]") != NULL)
		return SYNTAX_CPP;
	if (strstr(text, "0x") != NULL)
		return SYNTAX_CPP;

	return SYNTAX_NONE;
}

static bool
_IsKeyword(const char* word, int32 length)
{
	static const char* keywords[] = {
		"const", "unsigned", "char", "size_t", "int", "long",
		"short", "static", "extern", NULL
	};

	for (int i = 0; keywords[i]; i++) {
		if ((int32)strlen(keywords[i]) == length &&
			strncmp(word, keywords[i], length) == 0)
			return true;
	}
	return false;
}

void
HighlightWorker::_AnalyzeCppSyntax(const char* text, int32 length, BList* ranges)
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

		// Single line comments
		if (pos + 1 < length && text[pos] == '/' && text[pos + 1] == '/') {
			int32 lineEnd = pos;
			while (lineEnd < length && text[lineEnd] != '\n') {
				lineEnd++;
			}
			_AddRange(ranges, pos, lineEnd, HIGHLIGHT_COMMENT);
			pos = lineEnd;
			continue;
		}

		// Array data block
		if (text[pos] == '{') {
			int32 blockEnd = pos + 1;
			int32 braceCount = 1;

			while (blockEnd < length && braceCount > 0 && !fShutdown) {
				if (text[blockEnd] == '{') {
					braceCount++;
				} else if (text[blockEnd] == '}') {
					braceCount--;
				}
				blockEnd++;
			}

			// Add ranges for braces and content
			_AddRange(ranges, pos, pos + 1, HIGHLIGHT_OPERATOR);
			if (blockEnd > pos + 2) {
				_AddRange(ranges, pos + 1, blockEnd - 1, HIGHLIGHT_NUMBER);
			}
			if (blockEnd > pos + 1) {
				_AddRange(ranges, blockEnd - 1, blockEnd, HIGHLIGHT_OPERATOR);
			}

			pos = blockEnd;
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

			if (_IsKeyword(text + pos, wordEnd - pos)) {
				_AddRange(ranges, pos, wordEnd, HIGHLIGHT_KEYWORD);
			}
			pos = wordEnd;
			continue;
		}

		// Simple operators and separators
		if (text[pos] == '[' || text[pos] == ']' || text[pos] == '=' ||
			text[pos] == ',' || text[pos] == ';') {
			_AddRange(ranges, pos, pos + 1, HIGHLIGHT_OPERATOR);
		}

		pos++;
	}
}

#endif
