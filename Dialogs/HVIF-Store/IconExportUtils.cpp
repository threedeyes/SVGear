/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "IconExportUtils.h"

#include <Clipboard.h>
#include <Message.h>
#include <ctype.h>
#include <cstdio>

static const char kBase64Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


void
IconExportUtils::CopyToClipboardRDef(const uint8* data, size_t size, int32 id, const char* name)
{
	BString text = _GenerateRDef(data, size, id, name);
	if (!text.IsEmpty())
		_CopyToClipboard(text);
}


void
IconExportUtils::CopyToClipboardCPP(const uint8* data, size_t size, const char* name)
{
	BString text = _GenerateCPP(data, size, name);
	if (!text.IsEmpty())
		_CopyToClipboard(text);
}


void
IconExportUtils::CopyToClipboardSVG(const uint8* data, size_t size)
{
	if (data == NULL || size == 0)
		return;

	BString text((const char*)data, size);
	_CopyToClipboard(text);
}


void
IconExportUtils::CopyToClipboardImgTag(const uint8* data, size_t size)
{
	if (data == NULL || size == 0)
		return;

	BString source((const char*)data, size);
	BString base64 = _EncodeBase64(source);

	BString text;
	text.SetToFormat("<img src=\"data:image/svg+xml;base64,%s\" />", base64.String());

	_CopyToClipboard(text);
}


void
IconExportUtils::_CopyToClipboard(const BString& text)
{
	if (be_clipboard->Lock()) {
		be_clipboard->Clear();
		BMessage* clip = be_clipboard->Data();
		clip->AddData("text/plain", B_MIME_TYPE, text.String(), text.Length());
		be_clipboard->Commit();
		be_clipboard->Unlock();
	}
}


BString
IconExportUtils::_GenerateRDef(const uint8* data, size_t size, int32 id, const char* name)
{
	BString result;
	if (data == NULL || size == 0)
		return result;

	result.SetToFormat("resource(%" B_PRId32 ", \"%s\") #'VICN' array {\n", id, name);

	for (size_t i = 0; i < size; i += 32) {
		result << "\t$\"";

		for (size_t j = i; j < i + 32 && j < size; j++) {
			_AppendHexByte(result, data[j], true);
		}

		result << "\"";
		if (i + 32 < size) {
			result << ",";
		}
		result << "\n";
	}

	result << "};";
	return result;
}


BString
IconExportUtils::_GenerateCPP(const uint8* data, size_t size, const char* name)
{
	BString result;
	if (data == NULL || size == 0)
		return result;

	BString varName = _SanitizeName(name);

	result << "const unsigned char k" << varName << "Data[] = {\n";

	for (size_t i = 0; i < size; i++) {
		if (i % 16 == 0)
			result << "\t";

		result << "0x";
		_AppendHexByte(result, data[i], false);

		if (i < size - 1) {
			result << ",";
			if ((i + 1) % 16 == 0)
				result << "\n";
			else
				result << " ";
		}
	}

	result << "\n};\n";
	result << "\nconst size_t k" << varName << "Size = ";

	result << size << ";";

	return result;
}


BString
IconExportUtils::_SanitizeName(const char* name)
{
	BString result;
	if (name == NULL)
		return "Icon";

	int32 len = strlen(name);
	bool capitalizeNext = true;

	for (int32 i = 0; i < len; i++) {
		char c = name[i];
		if (isalnum(c)) {
			if (capitalizeNext) {
				c = toupper(c);
				capitalizeNext = false;
			}
			result << c;
		} else {
			capitalizeNext = true;
		}
	}

	if (result.IsEmpty())
		return "Icon";

	return result;
}


BString
IconExportUtils::_EncodeBase64(const BString& input)
{
	BString output;
	int32 length = input.Length();
	const unsigned char* data = (const unsigned char*)input.String();
	int val = 0, valb = -6;

	for (int32 i = 0; i < length; i++) {
		val = (val << 8) + data[i];
		valb += 8;
		while (valb >= 0) {
			output += kBase64Alphabet[(val >> valb) & 0x3F];
			valb -= 6;
		}
	}

	if (valb > -6)
		output += kBase64Alphabet[((val << 8) >> (valb + 8)) & 0x3F];

	while (output.Length() % 4)
		output += '=';

	return output;
}


void
IconExportUtils::_AppendHexByte(BString& result, uint8 byte, bool uppercase)
{
	BString hex;
	if (uppercase) {
		hex.SetToFormat("%02X", byte);
	} else {
		hex.SetToFormat("%02x", byte);
	}
	result << hex;
}
