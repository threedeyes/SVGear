/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Alert.h>
#include <Directory.h>
#include <NodeInfo.h>
#include <Catalog.h>

#include "SVGConstants.h"
#include "SVGFileManager.h"
#include "SVGView.h"
#include "SVGHVIFView.h"
#include "SVGSettings.h"

#include "HVIFParser.h"
#include "HVIFWriter.h"
#include "SVGParser.h"
#include "SVGRenderer.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SVGFileManager"

SVGFileManager::SVGFileManager()
	: fOpenPanel(NULL),
	fSavePanel(NULL),
	fExportPanel(NULL),
	fLastFileType(FILE_TYPE_UNKNOWN),
	fCurrentExportType(0)
{
}

SVGFileManager::~SVGFileManager()
{
	delete fOpenPanel;
	delete fSavePanel;
	delete fExportPanel;
}

bool
SVGFileManager::LoadFile(const char* filePath, SVGView* svgView, HVIFView* iconView, BString& source)
{
	if (!filePath) {
		_ShowError(ERROR_FILE_NOT_SPECIFIED);
		return false;
	}

	fLastFileType = FILE_TYPE_UNKNOWN;

	if (hvif::HVIFParser::IsValidHVIFFile(filePath)) {
		fLastFileType = FILE_TYPE_HVIF;
		return _LoadHVIFFile(filePath, iconView, source);
	} else if (svgView && _IsSVGFile(filePath)) {
		fLastFileType = FILE_TYPE_SVG;
		return _LoadSVGFile(filePath, svgView, iconView, source);
	} else {
		fLastFileType = FILE_TYPE_FROM_ATTRIBUTES;
		return _LoadFromFileAttributes(filePath, iconView, source);
	}
}

bool
SVGFileManager::_LoadHVIFFile(const char* filePath, HVIFView* iconView, BString& source)
{
	hvif::HVIFParser parser;
	if (!parser.ParseFile(filePath)) {
		BString error;
		error.SetToFormat(B_TRANSLATE("Error parsing HVIF file: %s"), parser.GetLastError().c_str());
		_ShowError(error.String());
		return false;
	}

	if (iconView) {
		iconView->SetIcon(parser.GetIconData(), parser.GetIconDataSize());
	}

	const hvif::HVIFIcon& icon = parser.GetIcon();
	hvif::SVGRenderer renderer;
	std::string svg = renderer.RenderIcon(icon, 64, 64);
	source.SetTo(svg.c_str());

	return true;
}

bool
SVGFileManager::_LoadSVGFile(const char* filePath, SVGView* svgView, HVIFView* iconView, BString& source)
{
	if (LoadSourceFromFile(filePath, source) != B_OK) {
		_ShowError(ERROR_READING_SVG);
		return false;
	}

	status_t result = svgView->LoadFromFile(filePath);
	if (result != B_OK) {
		BString error;
		error.SetToFormat(B_TRANSLATE("Error loading SVG file: %s"), filePath);
		_ShowError(error.String());
		return false;
	}

	if (iconView) {
		hvif::HVIFWriter writer;
		hvif::SVGParser parser;
		parser.ParseFile(filePath, writer);
		std::vector<uint8_t> hvifData = writer.WriteToBuffer();
		iconView->SetIcon(hvifData.data(), hvifData.size());
	}

	return true;
}

bool
SVGFileManager::_LoadFromFileAttributes(const char* filePath, HVIFView* iconView, BString& source)
{
	BEntry entry(filePath, true);
	if (entry.InitCheck() != B_OK) {
		BString error;
		error.SetToFormat(B_TRANSLATE("Error accessing file: %s"), filePath);
		_ShowError(error.String());
		return false;
	}

	BNode node(&entry);
	if (node.InitCheck() != B_OK) {
		BString error;
		error.SetToFormat(B_TRANSLATE("Error importing HVIF icon from file: %s"), filePath);
		_ShowError(error.String());
		return false;
	}

	attr_info info;
	if (node.GetAttrInfo("BEOS:ICON", &info) != B_OK) {
		BString error;
		error.SetToFormat(B_TRANSLATE("Error getting HVIF icon info from file: %s"), filePath);
		_ShowError(error.String());
		return false;
	}

	std::vector<uint8_t> data(info.size);
	ssize_t read = node.ReadAttr("BEOS:ICON", B_VECTOR_ICON_TYPE, 0,
								data.data(), data.size());

	if (read != static_cast<ssize_t>(data.size())) {
		BString error;
		error.SetToFormat(B_TRANSLATE("Error reading HVIF icon data from file: %s"), filePath);
		_ShowError(error.String());
		return false;
	}

	hvif::HVIFParser parser;
	if (!parser.ParseData(data, filePath)) {
		_ShowError(B_TRANSLATE("Error parsing HVIF data from file attributes"));
		return false;
	}

	if (iconView) {
		iconView->SetIcon(parser.GetIconData(), parser.GetIconDataSize());
	}

	const hvif::HVIFIcon& icon = parser.GetIcon();
	hvif::SVGRenderer renderer;
	std::string svg = renderer.RenderIcon(icon, 64, 64);
	source.SetTo(svg.c_str());

	return true;
}

status_t
SVGFileManager::LoadSourceFromFile(const char* filePath, BString& source)
{
	BFile file(filePath, B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return file.InitCheck();

	off_t size;
	if (file.GetSize(&size) != B_OK)
		return B_ERROR;

	char* buffer = new char[size + 1];
	ssize_t bytesRead = file.Read(buffer, size);
	if (bytesRead != size) {
		delete[] buffer;
		return B_ERROR;
	}

	buffer[size] = '\0';
	source.SetTo(buffer);
	delete[] buffer;

	return B_OK;
}

status_t
SVGFileManager::SaveFile(const char* filePath, const BString& source, const char* mime)
{
	if (!filePath || source.IsEmpty()) {
		printf("SaveFile: Invalid parameters - filePath: %s, source empty: %s\n",
			   filePath ? filePath : "NULL", source.IsEmpty() ? "true" : "false");
		return B_BAD_VALUE;
	}

	printf("SaveFile: Attempting to save to %s\n", filePath);
	printf("SaveFile: Source length: %d\n", source.Length());

	BFile file(filePath, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t initResult = file.InitCheck();
	if (initResult != B_OK) {
		printf("SaveFile: File init failed: %s\n", strerror(initResult));
		return initResult;
	}

	ssize_t bytesWritten = file.Write(source.String(), source.Length());
	if (bytesWritten != source.Length()) {
		printf("SaveFile: Write failed - expected %d bytes, wrote %d\n", 
			   source.Length(), (int)bytesWritten);
		return B_ERROR;
	}

	BNodeInfo nodeInfo(&file);
	if (nodeInfo.InitCheck() == B_OK) {
		status_t mimeResult = nodeInfo.SetType(mime);
		if (mimeResult != B_OK) {
			printf("SaveFile: Warning - could not set MIME type: %s\n", strerror(mimeResult));
		}
	}

	printf("SaveFile: Successfully saved %d bytes to %s\n", (int)bytesWritten, filePath);
	return B_OK;
}

bool
SVGFileManager::SaveCurrentFile(const BString& currentPath, const BString& source)
{
	if (currentPath.IsEmpty() || !CanDirectSave(currentPath)) {
		return false;
	}

	status_t result = SaveFile(currentPath.String(), source, MIME_SVG_SIGNATURE);
	if (result != B_OK) {
		BString error;
		error.SetToFormat(B_TRANSLATE("Error saving file: %s"), strerror(result));
		_ShowError(error.String());
		return false;
	}

	return true;
}

bool
SVGFileManager::SaveAsFile(const BString& source, BHandler* target)
{
	ShowSaveAsPanel(target);
	return true;
}

bool
SVGFileManager::CanDirectSave(const BString& currentPath) const
{
	return !currentPath.IsEmpty() &&
		   _IsSVGFile(currentPath.String()) &&
		   fLastFileType == FILE_TYPE_SVG;
}

void
SVGFileManager::ShowOpenPanel(BHandler* target)
{
	if (!fOpenPanel) {
		fOpenPanel = new BFilePanel(B_OPEN_PANEL, NULL, NULL, 0, false);
		if (target) {
			fOpenPanel->SetTarget(BMessenger(target));
		}
	}
	fOpenPanel->Show();
}

void
SVGFileManager::ShowSaveAsPanel(BHandler* target)
{
	if (!fSavePanel) {
		fSavePanel = new BFilePanel(B_SAVE_PANEL, NULL, NULL, 0, false);
		fSavePanel->SetSaveText("Untitled.svg");
	}
	if (target) {
		fSavePanel->SetTarget(BMessenger(target));
	}

	fSavePanel->Show();
}

void
SVGFileManager::ShowExportHVIFPanel(BHandler* target)
{
	_ShowExportPanel("icon", ".hvif", MSG_EXPORT_HVIF, target);
}

void
SVGFileManager::ShowExportRDefPanel(BHandler* target)
{
	_ShowExportPanel("icon", ".rdef", MSG_EXPORT_RDEF, target);
}

void
SVGFileManager::ShowExportCPPPanel(BHandler* target)
{
	_ShowExportPanel("icon", ".cpp", MSG_EXPORT_CPP, target);
}

status_t
SVGFileManager::ExportHVIF(const char* filePath, const unsigned char* data, size_t size)
{
	if (!data || size == 0)
		return B_BAD_VALUE;

	BString fullPath = filePath;
	if (!fullPath.EndsWith(".hvif"))
		fullPath << ".hvif";

	return _SaveBinaryData(fullPath.String(), data, size, MIME_HVIF_SIGNATURE);
}

status_t
SVGFileManager::ExportRDef(const char* filePath, const unsigned char* data, size_t size)
{
	if (!data || size == 0)
		return B_BAD_VALUE;

	BString fullPath = filePath;
	if (!fullPath.EndsWith(".rdef"))
		fullPath << ".rdef";

	BString rdefContent = _ConvertToRDef(data, size);
	return SaveFile(fullPath.String(), rdefContent, MIME_TXT_SIGNATURE);
}

status_t
SVGFileManager::ExportCPP(const char* filePath, const unsigned char* data, size_t size)
{
	if (!data || size == 0)
		return B_BAD_VALUE;

	BString fullPath = filePath;
	if (!fullPath.EndsWith(".h") && !fullPath.EndsWith(".hpp") && !fullPath.EndsWith(".cpp")) {
		fullPath << ".h";
	}

	BString cppContent = _ConvertToCPP(data, size);
	return SaveFile(fullPath.String(), cppContent, MIME_CPP_SIGNATURE);
}

bool
SVGFileManager::HandleExportSavePanel(BMessage* message, const unsigned char* hvifData, size_t hvifSize)
{
	entry_ref dirRef;
	BString fileName;

	if (message->FindRef("directory", &dirRef) != B_OK ||
		message->FindString("name", &fileName) != B_OK) {
		_ShowError(B_TRANSLATE("Could not get export file information"));
		fCurrentExportType = 0;
		return false;
	}

	BPath dirPath(&dirRef);
	BString fullPath = dirPath.Path();
	fullPath << "/" << fileName;

	if (gSettings) {
		gSettings->SetString(kLastExportPath, dirPath.Path());
	}

	status_t result = B_ERROR;

	switch (fCurrentExportType) {
		case MSG_EXPORT_HVIF:
			result = ExportHVIF(fullPath.String(), hvifData, hvifSize);
			break;

		case MSG_EXPORT_RDEF:
			result = ExportRDef(fullPath.String(), hvifData, hvifSize);
			break;

		case MSG_EXPORT_CPP:
			result = ExportCPP(fullPath.String(), hvifData, hvifSize);
			break;
	}

	fCurrentExportType = 0;
	return (result == B_OK);
}

void
SVGFileManager::_ShowExportPanel(const char* defaultName, const char* extension, uint32 exportType, BHandler* target)
{
	if (!fExportPanel)
		fExportPanel = new BFilePanel(B_SAVE_PANEL, NULL, NULL, 0, false);

	if (target)
		fExportPanel->SetTarget(BMessenger(target));

	if (gSettings) {
		BString lastExportPath = gSettings->GetString(kLastExportPath);
		if (!lastExportPath.IsEmpty()) {
			entry_ref ref;
			if (get_ref_for_path(lastExportPath.String(), &ref) == B_OK) {
				fExportPanel->SetPanelDirectory(&ref);
			}
		}
	}

	fCurrentExportType = exportType;

	BString fileName = defaultName;
	if (!fileName.EndsWith(extension))
		fileName << extension;

	fExportPanel->SetSaveText(fileName.String());
	fExportPanel->Show();
}

status_t
SVGFileManager::_SaveBinaryData(const char* filePath, const unsigned char* data, size_t size, const char* mime)
{
	if (!filePath || !data || size == 0) {
		printf("_SaveBinaryData: Invalid parameters\n");
		return B_BAD_VALUE;
	}

	BFile file(filePath, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t initResult = file.InitCheck();
	if (initResult != B_OK) {
		printf("_SaveBinaryData: File init failed: %s\n", strerror(initResult));
		return initResult;
	}

	ssize_t bytesWritten = file.Write(data, size);
	if (bytesWritten != (ssize_t)size) {
		printf("_SaveBinaryData: Write failed - expected %d, wrote %d\n", (int)size, (int)bytesWritten);
		return B_ERROR;
	}

	BNodeInfo nodeInfo(&file);
	if (nodeInfo.InitCheck() == B_OK) {
		status_t mimeResult = nodeInfo.SetType(mime);
		if (mimeResult != B_OK) {
			printf("SaveFile: Warning - could not set MIME type: %s\n", strerror(mimeResult));
		}
	}

	return B_OK;
}

BString
SVGFileManager::_ConvertToRDef(const unsigned char* data, size_t size)
{
	BString result;
	result << "resource(1) #'VICN' array {\n";

	for (size_t i = 0; i < size; i += 32) {
		result << "\t$\"";

		for (size_t j = i; j < i + 32 && j < size; j++) {
			BString hex;
			hex.SetToFormat("%02X", data[j]);
			result << hex;
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
SVGFileManager::_ConvertToCPP(const unsigned char* data, size_t size)
{
	BString result;
	result << "const unsigned char kIconData[] = {\n";

	for (size_t i = 0; i < size; i++) {
		if (i % 16 == 0)
			result << "\t";

		BString hex;
		hex.SetToFormat("0x%02x", data[i]);
		result << hex;

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

bool
SVGFileManager::_IsSVGFile(const char* filePath) const
{
	if (!filePath)
		return false;

	BString path(filePath);
	path.ToLower();
	return path.EndsWith(".svg");
}

bool
SVGFileManager::_EnsureSVGExtension(BString& filePath) const
{
	if (!filePath.EndsWith(".svg")) {
		filePath << ".svg";
		return true;
	}
	return false;
}

void
SVGFileManager::_ShowError(const char* message)
{
	BAlert* alert = new BAlert(B_TRANSLATE("Error"), message, B_TRANSLATE("OK"), NULL, NULL,
							  B_WIDTH_AS_USUAL, B_STOP_ALERT);
	alert->Go();
}
