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
	Dialogs/Vectorization/SVGVectorizationDialog.cpp \
	Dialogs/Vectorization/SVGVectorizationWorker.cpp \
	Dialogs/HVIF-Store/HvifStoreClient.cpp \
	Dialogs/HVIF-Store/IconGridView.cpp \
	Dialogs/HVIF-Store/IconInfoView.cpp \
	Dialogs/HVIF-Store/IconSelectionDialog.cpp \
	Dialogs/HVIF-Store/TagsFlowView.cpp \
	Dialogs/HVIF-Store/ChipView.cpp \
	External/BSVGView/BSVGView.cpp \
	main.cpp
RDEFS = Resources.rdef
LIBS = be tracker translation network bnetapi netservices shared localestub hviftools imagetracer agg $(STDCPPLIBS)
SYSTEM_INCLUDE_PATHS = \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/private/interface \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/private/netservices \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/private/shared \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/$(shell getarch -s)/hviftools \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/$(shell getarch -s)/hviftools/common \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/$(shell getarch -s)/hviftools/import \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/$(shell getarch -s)/hviftools/export \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/$(shell getarch -s)/imagetracer/core \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/$(shell getarch -s)/imagetracer/output \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/$(shell getarch -s)/imagetracer/processing \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/$(shell getarch -s)/imagetracer/quantization \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/$(shell getarch -s)/imagetracer/utils \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/$(shell getarch -s)/agg2
LOCAL_INCLUDE_PATHS = \
	./Dialogs/Vectorization \
	./Dialogs/HVIF-Store \
	./External/BSVGView \
	./External/nanosvg_ext/src
OPTIMIZE := FULL
LOCALES = en ru de tr
DEFINES =
WARNINGS =
SYMBOLS :=
DEBUGGER :=
COMPILER_FLAGS = -mmmx -msse
LINKER_FLAGS =
APP_VERSION :=

## Include the Makefile-Engine
DEVEL_DIRECTORY := \
	$(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(DEVEL_DIRECTORY)/etc/makefile-engine
