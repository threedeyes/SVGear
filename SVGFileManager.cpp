/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Alert.h>
#include <Directory.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Catalog.h>
#include <TranslationUtils.h>

#include "SVGConstants.h"
#include "SVGFileManager.h"
#include "SVGView.h"
#include "SVGHVIFView.h"
#include "SVGSettings.h"
#include "SVGCodeGenerator.h"
#include "SVGVectorizationWorker.h"
#include "SVGVectorizationDialog.h"

#include "IconConverter.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SVGFileManager"

SVGFileManager::SVGFileManager()
	: fOpenPanel(NULL),
	fSavePanel(NULL),
	fExportPanel(NULL),
	fLastFileType(FILE_TYPE_UNKNOWN),
	fCurrentExportType(0),
	fCurrentExportSize(64)
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

	haiku::IconFormat format = haiku::IconConverter::DetectFormatBySignature(filePath);

	switch (format) {
		case haiku::FORMAT_HVIF:
			fLastFileType = FILE_TYPE_HVIF;
			return _LoadVectorIconFile(filePath, haiku::FORMAT_HVIF, iconView, source);

		case haiku::FORMAT_IOM:
			fLastFileType = FILE_TYPE_HVIF;
			return _LoadVectorIconFile(filePath, haiku::FORMAT_IOM, iconView, source);

		case haiku::FORMAT_SVG:
			fLastFileType = FILE_TYPE_SVG;
			return _LoadSVGFile(filePath, svgView, iconView, source);

		case haiku::FORMAT_PNG:
			fLastFileType = FILE_TYPE_RASTER;
			return false;

		default:
			break;
	}

	BBitmap* bitmap = BTranslationUtils::GetBitmap(filePath);
	if (bitmap != NULL) {
		delete bitmap;
		fLastFileType = FILE_TYPE_RASTER;
		return false;
	}

	fLastFileType = FILE_TYPE_FROM_ATTRIBUTES;
	if (_LoadFromFileAttributes(filePath, iconView, source)) {
		return true;
	}

	BString error;
	error.SetToFormat(B_TRANSLATE("Unable to load file: %s\nUnknown or unsupported format"), filePath);
	_ShowError(error.String());
	return false;
}

bool
SVGFileManager::_LoadVectorIconFile(const char* filePath, haiku::IconFormat format,	HVIFView* iconView, BString& source)
{
	haiku::Icon icon = haiku::IconConverter::Load(filePath, format);

	std::string errorMsg = haiku::IconConverter::GetLastError();
	if (!errorMsg.empty()) {
		BString error;
		const char* formatName = (format == haiku::FORMAT_HVIF) ? "HVIF" : "IOM";
		error.SetToFormat(B_TRANSLATE("Error loading %s file: %s"), formatName, errorMsg.c_str());
		_ShowError(error.String());
		return false;
	}

	if (iconView) {
		std::vector<uint8_t> hvifData;

		if (format == haiku::FORMAT_HVIF) {
			BFile file(filePath, B_READ_ONLY);
			if (file.InitCheck() == B_OK) {
				off_t size;
				if (file.GetSize(&size) == B_OK && size > 0) {
					hvifData.resize(size);
					file.Read(hvifData.data(), size);
				}
			}
		} else {
			haiku::ConvertOptions opts;
			haiku::IconConverter::SaveToBuffer(icon, hvifData, haiku::FORMAT_HVIF, opts);
		}

		if (!hvifData.empty()) {
			iconView->SetIcon(hvifData.data(), hvifData.size());
		}
	}

	haiku::ConvertOptions opts;
	opts.svgWidth = 64;
	opts.svgHeight = 64;
	opts.preserveNames = false; //TODO: fix hvif-tools naming

	std::vector<uint8_t> svgBuffer;
	if (!haiku::IconConverter::SaveToBuffer(icon, svgBuffer, haiku::FORMAT_SVG, opts)) {
		const char* formatName = (format == haiku::FORMAT_HVIF) ? "HVIF" : "IOM";
		BString error;
		error.SetToFormat(B_TRANSLATE("Error converting %s to SVG"), formatName);
		_ShowError(error.String());
		return false;
	}

	source.SetTo(reinterpret_cast<const char*>(svgBuffer.data()), svgBuffer.size());
	return true;
}

bool
SVGFileManager::_LoadSVGFile(const char* filePath, SVGView* svgView, HVIFView* iconView, BString& source)
{
	if (LoadSourceFromFile(filePath, source) != B_OK) {
		_ShowError(ERROR_READING_SVG);
		return false;
	}

	if (svgView) {
		status_t result = svgView->LoadFromFile(filePath);
		if (result != B_OK) {
			BString error;
			error.SetToFormat(B_TRANSLATE("Error loading SVG file: %s"), filePath);
			_ShowError(error.String());
			return false;
		}
	}

	if (iconView) {
		std::vector<uint8_t> svgData(source.String(), source.String() + source.Length());
		haiku::Icon icon = haiku::IconConverter::LoadFromBuffer(svgData, haiku::FORMAT_SVG);

		if (haiku::IconConverter::GetLastError().empty()) {
			std::vector<uint8_t> hvifData;
			haiku::ConvertOptions opts;
			if (haiku::IconConverter::SaveToBuffer(icon, hvifData, haiku::FORMAT_HVIF, opts)) {
				iconView->SetIcon(hvifData.data(), hvifData.size());
			}
		}
	}

	return true;
}

bool
SVGFileManager::_LoadFromFileAttributes(const char* filePath, HVIFView* iconView, BString& source)
{
	BEntry entry(filePath, true);
	if (entry.InitCheck() != B_OK) {
		return false;
	}

	BNode node(&entry);
	if (node.InitCheck() != B_OK) {
		return false;
	}

	attr_info info;
	if (node.GetAttrInfo("BEOS:ICON", &info) != B_OK) {
		return false;
	}

	std::vector<uint8_t> data(info.size);
	ssize_t read = node.ReadAttr("BEOS:ICON", B_VECTOR_ICON_TYPE, 0,
								data.data(), data.size());

	if (read != static_cast<ssize_t>(data.size())) {
		return false;
	}

	haiku::Icon icon = haiku::IconConverter::LoadFromBuffer(data, haiku::FORMAT_HVIF);

	std::string errorMsg = haiku::IconConverter::GetLastError();
	if (!errorMsg.empty()) {
		return false;
	}

	if (iconView) {
		iconView->SetIcon(data.data(), data.size());
	}

	haiku::ConvertOptions opts;
	opts.svgWidth = 64;
	opts.svgHeight = 64;
	opts.preserveNames = false;

	std::vector<uint8_t> svgBuffer;
	if (!haiku::IconConverter::SaveToBuffer(icon, svgBuffer, haiku::FORMAT_SVG, opts)) {
		return false;
	}

	source.SetTo(reinterpret_cast<const char*>(svgBuffer.data()), svgBuffer.size());
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
		return B_BAD_VALUE;
	}

	BFile file(filePath, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t initResult = file.InitCheck();
	if (initResult != B_OK) {
		return initResult;
	}

	ssize_t bytesWritten = file.Write(source.String(), source.Length());
	if (bytesWritten != source.Length()) {
		return B_ERROR;
	}

	BNodeInfo nodeInfo(&file);
	if (nodeInfo.InitCheck() == B_OK) {
		nodeInfo.SetType(mime);
	}

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

bool
SVGFileManager::IsRasterImage(const char* filePath) const
{
	if (!filePath)
		return false;

	BBitmap* bitmap = BTranslationUtils::GetBitmap(filePath);
	if (bitmap != NULL) {
		delete bitmap;
		return true;
	}

	return false;
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

void
SVGFileManager::ShowExportIOMPanel(BHandler* target)
{
	_ShowExportPanel("icon", ".iom", MSG_EXPORT_IOM, target);
}

void
SVGFileManager::ShowExportPNGPanel(BHandler* target, int32 size)
{
	fCurrentExportSize = size;
	BString defaultName;
	defaultName.SetToFormat("icon_%ldx%ld", size, size);
	_ShowExportPanel(defaultName.String(), ".png", MSG_EXPORT_PNG, target);
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

	BString rdefContent = SVGCodeGenerator::GenerateRDef(data, size);
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

	BString cppContent = SVGCodeGenerator::GenerateCPP(data, size);
	return SaveFile(fullPath.String(), cppContent, MIME_CPP_SIGNATURE);
}

status_t
SVGFileManager::_ExportIOM(const char* filePath, const BString& svgSource)
{
	if (svgSource.IsEmpty())
		return B_BAD_VALUE;

	BString fullPath = filePath;
	if (!fullPath.EndsWith(".iom"))
		fullPath << ".iom";

	std::vector<uint8_t> svgData(svgSource.String(), svgSource.String() + svgSource.Length());
	haiku::Icon icon = haiku::IconConverter::LoadFromBuffer(svgData, haiku::FORMAT_SVG);

	if (!haiku::IconConverter::GetLastError().empty())
		return B_ERROR;

	std::vector<uint8_t> iomData;
	if (!haiku::IconConverter::SaveToBuffer(icon, iomData, haiku::FORMAT_IOM))
		return B_ERROR;

	return _SaveBinaryData(fullPath.String(), iomData.data(), iomData.size(), "application/x-vnd.haiku-icon");
}

status_t
SVGFileManager::_ExportPNG(const char* filePath, const BString& svgSource, int32 size)
{
	if (svgSource.IsEmpty())
		return B_BAD_VALUE;

	BString fullPath = filePath;
	if (!fullPath.EndsWith(".png"))
		fullPath << ".png";

	std::vector<uint8_t> svgData(svgSource.String(), svgSource.String() + svgSource.Length());
	haiku::Icon icon = haiku::IconConverter::LoadFromBuffer(svgData, haiku::FORMAT_SVG);

	if (!haiku::IconConverter::GetLastError().empty())
		return B_ERROR;

	haiku::ConvertOptions opts;
	opts.pngWidth = size;
	opts.pngHeight = size;
	opts.pngScale = 1.0f;

	std::vector<uint8_t> pngData;
	if (!haiku::IconConverter::SaveToBuffer(icon, pngData, haiku::FORMAT_PNG, opts))
		return B_ERROR;

	return _SaveBinaryData(fullPath.String(), pngData.data(), pngData.size(), "image/png");
}

bool
SVGFileManager::HandleExportSavePanel(BMessage* message, const BString& svgSource, const unsigned char* hvifData, size_t hvifSize)
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

		case MSG_EXPORT_IOM:
			result = _ExportIOM(fullPath.String(), svgSource);
			break;

		case MSG_EXPORT_PNG:
			result = _ExportPNG(fullPath.String(), svgSource, fCurrentExportSize);
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
		return B_BAD_VALUE;
	}

	BFile file(filePath, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t initResult = file.InitCheck();
	if (initResult != B_OK) {
		return initResult;
	}

	ssize_t bytesWritten = file.Write(data, size);
	if (bytesWritten != (ssize_t)size) {
		return B_ERROR;
	}

	BNodeInfo nodeInfo(&file);
	if (nodeInfo.InitCheck() == B_OK) {
		nodeInfo.SetType(mime);
	}

	return B_OK;
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
