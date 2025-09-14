NAME = ../SVGear
TYPE = APP
APP_MIME_SIG = application/x-vnd.svgear
SRCS = \
	SVGApplication.cpp \
	SVGMainWindow.cpp \
	SVGView.cpp \
	SVGToolBar.cpp \
	SVGTextEdit.cpp \
	SVGFileManager.cpp \
	SVGMenuManager.cpp \
	SVGSettings.cpp \
	SVGStatView.cpp \
	SVGStructureView.cpp \
	External/BSVGView/BSVGView.cpp \
	External/HVIF-Tools/src/hvif2svg/HVIFParser.cpp \
	External/HVIF-Tools/src/hvif2svg/SVGRenderer.cpp \
	External/HVIF-Tools/src/svg2hvif/HVIFWriter.cpp \
	External/HVIF-Tools/src/svg2hvif/SVGParser.cpp \
	main.cpp
RDEFS = Resources.rdef
LIBS = be tracker shared localestub $(STDCPPLIBS)
SYSTEM_INCLUDE_PATHS = /system/develop/headers/private/interface
LOCAL_INCLUDE_PATHS = \
	./External/BSVGView \
	./External/NanoSVG/src \
	./External/HVIF-Tools/src/common \
	./External/HVIF-Tools/src/hvif2svg \
	./External/HVIF-Tools/src/svg2hvif
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
