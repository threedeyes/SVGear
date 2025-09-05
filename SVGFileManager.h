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

class SVGView;
class HVIFView;
class BHandler;

enum file_type {
    FILE_TYPE_UNKNOWN = 0,
    FILE_TYPE_SVG,
    FILE_TYPE_HVIF,
    FILE_TYPE_FROM_ATTRIBUTES,
    FILE_TYPE_NEW
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

    void ShowOpenPanel(BHandler* target = NULL);
    void ShowSaveAsPanel(BHandler* target = NULL);

    BFilePanel* GetOpenPanel() { return fOpenPanel; }
    BFilePanel* GetSavePanel() { return fSavePanel; }

    file_type GetLastLoadedFileType() const { return fLastFileType; }
    void SetLastLoadedFileType(file_type type) { fLastFileType = type; }

private:
    BFilePanel* fOpenPanel;
    BFilePanel* fSavePanel;
    file_type fLastFileType;

    bool _LoadHVIFFile(const char* filePath, HVIFView* iconView, BString& source);
    bool _LoadSVGFile(const char* filePath, SVGView* svgView, HVIFView* iconView, BString& source);
    bool _LoadFromFileAttributes(const char* filePath, HVIFView* iconView, BString& source);
    void _ShowError(const char* message);
    bool _IsSVGFile(const char* filePath) const;
    bool _EnsureSVGExtension(BString& filePath) const;
};

#endif
