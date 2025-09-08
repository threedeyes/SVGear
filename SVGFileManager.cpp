/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Alert.h>
#include <Directory.h>
#include <NodeInfo.h>
#include <Catalog.h>

#include "SVGFileManager.h"
#include "SVGView.h"
#include "SVGHVIFView.h"

#include "HVIFParser.h"
#include "HVIFWriter.h"
#include "SVGParser.h"
#include "SVGRenderer.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SVGFileManager"

SVGFileManager::SVGFileManager()
	: fOpenPanel(NULL),
	  fSavePanel(NULL),
	  fLastFileType(FILE_TYPE_UNKNOWN)
{
}

SVGFileManager::~SVGFileManager()
{
	delete fOpenPanel;
	delete fSavePanel;
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

	status_t result = SaveFile(currentPath.String(), source, "image/svg+xml");
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
