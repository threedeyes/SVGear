/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SVG_CODE_GENERATOR_H
#define SVG_CODE_GENERATOR_H

#include <String.h>
#include <SupportDefs.h>

class SVGCodeGenerator {
public:
	static BString GenerateRDef(const unsigned char* data, size_t size);
	static BString GenerateCPP(const unsigned char* data, size_t size);
	static BString GenerateHex(const unsigned char* data, size_t size, 
					int32 bytesPerLine = 16);

private:
	static void _AppendHexByte(BString& result, unsigned char byte, bool uppercase = false);
	static bool _IsValidData(const unsigned char* data, size_t size);
};

#endif
