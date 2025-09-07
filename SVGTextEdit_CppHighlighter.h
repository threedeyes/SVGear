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
		strcmp(ext, ".hpp") == 0 || strcmp(ext, ".c") == 0 ||
		strcmp(ext, ".C") == 0 || strcmp(ext, ".hh") == 0)
		return SYNTAX_CPP;

	return SYNTAX_NONE;
}

syntax_type
DetectCppFromContent(const char* text, int32 length)
{
	if (!text || length == 0)
		return SYNTAX_NONE;

	if (strstr(text, "#include") != NULL)
		return SYNTAX_CPP;
	if (strstr(text, "#define") != NULL)
		return SYNTAX_CPP;
	if (strstr(text, "#ifndef") != NULL)
		return SYNTAX_CPP;

	if (strstr(text, "class ") != NULL)
		return SYNTAX_CPP;
	if (strstr(text, "namespace ") != NULL)
		return SYNTAX_CPP;
	if (strstr(text, "template") != NULL)
		return SYNTAX_CPP;

	if (strstr(text, "const ") != NULL)
		return SYNTAX_CPP;
	if (strstr(text, "struct ") != NULL)
		return SYNTAX_CPP;
	if (strstr(text, "typedef ") != NULL)
		return SYNTAX_CPP;

	if (strstr(text, "void ") != NULL && strstr(text, "(") != NULL)
		return SYNTAX_CPP;

	return SYNTAX_NONE;
}

static bool
_IsCppKeyword(const char* word, int32 length)
{
	static const char* keywords[] = {
		"auto", "break", "case", "char", "const", "continue", "default", "do",
		"double", "else", "enum", "extern", "float", "for", "goto", "if",
		"int", "long", "register", "return", "short", "signed", "sizeof", "static",
		"struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while",
		"class", "private", "protected", "public", "friend", "inline", "operator",
		"overload", "template", "this", "virtual", "bool", "true", "false", "new",
		"delete", "namespace", "using", "try", "catch", "throw", "const_cast",
		"dynamic_cast", "reinterpret_cast", "static_cast", "typeid", "typename",
		"explicit", "export", "mutable", "wchar_t", "size_t", "NULL",
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
_IsCppType(const char* word, int32 length)
{
	static const char* types[] = {
		"int8", "int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64",
		"bigtime_t", "thread_id", "area_id", "port_id", "sem_id", "team_id",
		"status_t", "type_code", "ssize_t", "off_t", "dev_t", "ino_t",
		"BString", "BMessage", "BView", "BWindow", "BApplication", "BRect",
		"BPoint", "BSize", "rgb_color", "pattern", "drawing_mode",
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
ApplyCppHighlighting(BTextView* view)
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

		// Single line comments
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

		// Preprocessor directives
		if (text[pos] == '#') {
			int32 lineEnd = pos;
			while (lineEnd < length && text[lineEnd] != '\n') {
				lineEnd++;
			}
			SetColorRange(view, pos, lineEnd, colors.preprocessor);
			pos = lineEnd;
			continue;
		}

		// String literals
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

		// Character literals
		if (text[pos] == '\'') {
			int32 charEnd = pos + 1;
			while (charEnd < length && text[charEnd] != '\'') {
				if (text[charEnd] == '\\' && charEnd + 1 < length) {
					charEnd += 2;
				} else {
					charEnd++;
				}
			}
			if (charEnd < length) {
				charEnd++;
			}
			SetColorRange(view, pos, charEnd, colors.string);
			pos = charEnd;
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
			if (numEnd < length && (text[numEnd] == 'L' || text[numEnd] == 'l')) {
				numEnd++;
			}
			if (numEnd < length && (text[numEnd] == 'U' || text[numEnd] == 'u')) {
				numEnd++;
			}
			SetColorRange(view, pos, numEnd, colors.number);
			pos = numEnd;
			continue;
		}

		// Decimal numbers
		if (isdigit(text[pos])) {
			int32 numEnd = pos;
			while (numEnd < length && (isdigit(text[numEnd]) || text[numEnd] == '.')) {
				numEnd++;
			}
			if (numEnd < length && (text[numEnd] == 'e' || text[numEnd] == 'E')) {
				numEnd++;
				if (numEnd < length && (text[numEnd] == '+' || text[numEnd] == '-')) {
					numEnd++;
				}
				while (numEnd < length && isdigit(text[numEnd])) {
					numEnd++;
				}
			}
			if (numEnd < length && (text[numEnd] == 'f' || text[numEnd] == 'F' ||
									text[numEnd] == 'L' || text[numEnd] == 'l')) {
				numEnd++;
			}
			SetColorRange(view, pos, numEnd, colors.number);
			pos = numEnd;
			continue;
		}

		// Keywords and identifiers
		if (isalpha(text[pos]) || text[pos] == '_') {
			int32 wordEnd = pos;
			while (wordEnd < length && (isalnum(text[wordEnd]) || text[wordEnd] == '_')) {
				wordEnd++;
			}

			if (_IsCppKeyword(text + pos, wordEnd - pos)) {
				SetColorRange(view, pos, wordEnd, colors.keyword);
			} else if (_IsCppType(text + pos, wordEnd - pos)) {
				SetColorRange(view, pos, wordEnd, colors.attribute);
			}
			pos = wordEnd;
			continue;
		}

		// Operators
		if (text[pos] == '{' || text[pos] == '}' || text[pos] == '[' ||
			text[pos] == ']' || text[pos] == '(' || text[pos] == ')' ||
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
