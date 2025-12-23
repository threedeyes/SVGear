/*
 * Copyright 2025, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SVG_FILE_MANAGER_H
#define SVG_FILE_MANAGER_H

#include <String.h>
#include <FilePanel.h>
#include <Path.h>
#include <File.h>
#include <Entry.h>
#include <Node.h>
#include <fs_attr.h>
#include <vector>

#include "SVGConstants.h"
#include "IconConverter.h"

class SVGView;
class HVIFView;
class BHandler;
class VectorizationWorker;
class VectorizationDialog;

enum file_type {
	FILE_TYPE_UNKNOWN = 0,
	FILE_TYPE_SVG,
	FILE_TYPE_HVIF,
	FILE_TYPE_FROM_ATTRIBUTES,
	FILE_TYPE_NEW,
	FILE_TYPE_RASTER
};

class SVGFileManager {
public:
	SVGFileManager();
	~SVGFileManager();

	bool LoadFile(const char* filePath, SVGView* svgView, HVIFView* iconView, BString& source);
	status_t LoadSourceFromFile(const char* filePath, BString& source);

	status_t SaveFile(const char* filePath, const BString& source, const char* mime);
	bool SaveCurrentFile(const BString& currentPath, const BString& source);
	bool SaveAsFile(const BString& source, BHandler* target);
	bool CanDirectSave(const BString& currentPath) const;
	bool IsRasterImage(const char* filePath) const;

	void ShowOpenPanel(BHandler* target = NULL);
	void ShowSaveAsPanel(BHandler* target = NULL);

	void ShowExportHVIFPanel(BHandler* target);
	void ShowExportRDefPanel(BHandler* target);
	void ShowExportCPPPanel(BHandler* target);
	void ShowExportIOMPanel(BHandler* target);
	void ShowExportPNGPanel(BHandler* target, int32 size);

	status_t ExportHVIF(const char* filePath, const unsigned char* data, size_t size);
	status_t ExportRDef(const char* filePath, const unsigned char* data, size_t size);
	status_t ExportCPP(const char* filePath, const unsigned char* data, size_t size);

	bool HandleExportSavePanel(BMessage* message, const BString& svgSource, const unsigned char* hvifData, size_t hvifSize);

	BFilePanel* GetOpenPanel() { return fOpenPanel; }
	BFilePanel* GetSavePanel() { return fSavePanel; }
	BFilePanel* GetExportPanel() { return fExportPanel; }

	file_type GetLastLoadedFileType() const { return fLastFileType; }
	void SetLastLoadedFileType(file_type type) { fLastFileType = type; }

private:
	BFilePanel* fOpenPanel;
	BFilePanel* fSavePanel;
	BFilePanel* fExportPanel;
	file_type fLastFileType;
	uint32 fCurrentExportType;
	int32 fCurrentExportSize;

	bool _LoadVectorIconFile(const char* filePath, haiku::IconFormat format, HVIFView* iconView, BString& source);
	bool _LoadSVGFile(const char* filePath, SVGView* svgView, HVIFView* iconView, BString& source);
	bool _LoadFromFileAttributes(const char* filePath, HVIFView* iconView, BString& source);
	void _ShowError(const char* message);
	bool _IsSVGFile(const char* filePath) const;
	bool _EnsureSVGExtension(BString& filePath) const;

	status_t _ExportIOM(const char* filePath, const BString& svgSource);
	status_t _ExportPNG(const char* filePath, const BString& svgSource, int32 size);

	void _ShowExportPanel(const char* defaultName, const char* extension, uint32 exportType, BHandler* target);
	status_t _SaveBinaryData(const char* filePath, const unsigned char* data, size_t size, const char* mime);
};

#endif
