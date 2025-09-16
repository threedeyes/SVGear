/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "SVGCodeGenerator.h"

BString
SVGCodeGenerator::GenerateRDef(const unsigned char* data, size_t size)
{
	BString result;
	
	if (!_IsValidData(data, size)) {
		return result;
	}

	result << "resource(1) #'VICN' array {\n";

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
SVGCodeGenerator::GenerateCPP(const unsigned char* data, size_t size)
{
	BString result;
	
	if (!_IsValidData(data, size)) {
		return result;
	}

	result << "const unsigned char kIconData[] = {\n";

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
	result << "\nconst size_t kIconDataSize = ";
	
	BString sizeStr;
	sizeStr.SetToFormat("%u", (unsigned int)size);
	result << sizeStr << ";";

	return result;
}

BString
SVGCodeGenerator::GenerateHex(const unsigned char* data, size_t size, int32 bytesPerLine)
{
	BString result;
	
	if (!_IsValidData(data, size) || bytesPerLine <= 0) {
		return result;
	}

	for (size_t i = 0; i < size; i++) {
		if (i % bytesPerLine == 0 && i > 0)
			result << "\n";
		
		if (i % bytesPerLine == 0)
			result << "\t";
		
		_AppendHexByte(result, data[i], true);
		
		if (i < size - 1 && (i + 1) % bytesPerLine != 0)
			result << " ";
	}

	return result;
}

void
SVGCodeGenerator::_AppendHexByte(BString& result, unsigned char byte, bool uppercase)
{
	BString hex;
	if (uppercase) {
		hex.SetToFormat("%02X", byte);
	} else {
		hex.SetToFormat("%02x", byte);
	}
	result << hex;
}

bool
SVGCodeGenerator::_IsValidData(const unsigned char* data, size_t size)
{
	return (data != NULL && size > 0);
}
