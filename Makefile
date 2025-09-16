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
	SVGListItem.cpp \
	SVGStructureView.cpp \
	SVGCodeGenerator.cpp \
	SVGVectorizationDialog.cpp \
	SVGVectorizationWorker.cpp \
	External/BSVGView/BSVGView.cpp \
	External/HVIF-Tools/src/hvif2svg/HVIFParser.cpp \
	External/HVIF-Tools/src/hvif2svg/SVGRenderer.cpp \
	External/HVIF-Tools/src/svg2hvif/HVIFWriter.cpp \
	External/HVIF-Tools/src/svg2hvif/SVGParser.cpp \
	External/HVIF-Tools/src/img2svg/core/BitmapData.cpp \
	External/HVIF-Tools/src/img2svg/core/ImageTracer.cpp \
	External/HVIF-Tools/src/img2svg/core/IndexedBitmap.cpp \
	External/HVIF-Tools/src/img2svg/core/TracingOptions.cpp \
	External/HVIF-Tools/src/img2svg/output/SvgWriter.cpp \
	External/HVIF-Tools/src/img2svg/processing/BackgroundRemover.cpp \
	External/HVIF-Tools/src/img2svg/processing/GeometryDetector.cpp \
	External/HVIF-Tools/src/img2svg/processing/PathScanner.cpp \
	External/HVIF-Tools/src/img2svg/processing/PathSimplifier.cpp \
	External/HVIF-Tools/src/img2svg/processing/PathTracer.cpp \
	External/HVIF-Tools/src/img2svg/processing/SelectiveBlur.cpp \
	External/HVIF-Tools/src/img2svg/quantization/ColorCube.cpp \
	External/HVIF-Tools/src/img2svg/quantization/ColorNode.cpp \
	External/HVIF-Tools/src/img2svg/quantization/ColorQuantizer.cpp \
	main.cpp
RDEFS = Resources.rdef
LIBS = be tracker translation shared localestub $(STDCPPLIBS)
SYSTEM_INCLUDE_PATHS = /system/develop/headers/private/interface
LOCAL_INCLUDE_PATHS = \
	./External/BSVGView \
	./External/NanoSVG/src \
	./External/HVIF-Tools/src/common \
	./External/HVIF-Tools/src/hvif2svg \
	./External/HVIF-Tools/src/svg2hvif \
	./External/HVIF-Tools/src/img2svg/core \
	./External/HVIF-Tools/src/img2svg/output \
	./External/HVIF-Tools/src/img2svg/processing \
	./External/HVIF-Tools/src/img2svg/quantization
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
