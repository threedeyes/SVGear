/*
 * Copyright 2025-2026, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef ICON_EXPORT_UTILS_H
#define ICON_EXPORT_UTILS_H

#include <SupportDefs.h>
#include <String.h>

class IconExportUtils {
public:
	static void		CopyToClipboardRDef(const uint8* data, size_t size, int32 id, const char* name);
	static void		CopyToClipboardCPP(const uint8* data, size_t size, const char* name);
	static void		CopyToClipboardSVG(const uint8* data, size_t size);
	static void		CopyToClipboardImgTag(const uint8* data, size_t size);

private:
	static void		_CopyToClipboard(const BString& text);
	
	static BString	_GenerateRDef(const uint8* data, size_t size, int32 id, const char* name);
	static BString	_GenerateCPP(const uint8* data, size_t size, const char* name);
	static BString	_SanitizeName(const char* name);
	static BString	_EncodeBase64(const BString& input);
	static void		_AppendHexByte(BString& result, uint8 byte, bool uppercase);
};

#endif
