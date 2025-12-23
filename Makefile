NAME = ../SVGear
TYPE = APP
APP_MIME_SIG = application/x-vnd.svgear
SRCS = \
	SVGApplication.cpp \
	SVGMainWindow.cpp \
	SVGView.cpp \
	SVGToolBar.cpp \
	SVGTextEdit.cpp \
	SVGHVIFView.cpp \
	SVGFileManager.cpp \
	SVGMenuManager.cpp \
	SVGSettings.cpp \
	SVGStatView.cpp \
	SVGListItem.cpp \
	SVGStructureView.cpp \
	SVGCodeGenerator.cpp \
	SVGVectorizationDialog.cpp \
	SVGVectorizationWorker.cpp \
	External/BSVGView/BSVGView.cpp \
	main.cpp
RDEFS = Resources.rdef
LIBS = be tracker translation shared localestub hviftools imagetracer $(STDCPPLIBS)
SYSTEM_INCLUDE_PATHS = \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/private/interface \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/hviftools \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/hviftools/common \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/hviftools/import \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/hviftools/export \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/imagetracer/core \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/imagetracer/output \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/imagetracer/processing \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/imagetracer/quantization \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/imagetracer/utils
LOCAL_INCLUDE_PATHS = \
	./External/BSVGView \
	./External/nanosvg_ext/src
OPTIMIZE := FULL
LOCALES = en ru
DEFINES =
WARNINGS =
SYMBOLS :=
DEBUGGER :=
COMPILER_FLAGS = -mmmx -msse -msse2
LINKER_FLAGS =
APP_VERSION :=

## Include the Makefile-Engine
DEVEL_DIRECTORY := \
	$(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(DEVEL_DIRECTORY)/etc/makefile-engine
