NAME = HVIFStoreBrowser
TYPE = APP
APP_MIME_SIG = application/x-vnd.HvifStoreBrowser
SRCS = \
	HvifStoreClient.cpp \
	IconGridView.cpp \
	IconSelectionDialog.cpp \
	TagsFlowView.cpp \
	IconInfoView.cpp \
	ChipView.cpp \
	IconCache.cpp \
	HVIFStoreBrowser.cpp
RDEFS =
RSRCS =
LIBS = be tracker network bnetapi netservices shared localestub $(STDCPPLIBS)
LIBPATHS =
SYSTEM_INCLUDE_PATHS = \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/private/netservices \
	$(shell finddir B_SYSTEM_HEADERS_DIRECTORY)/private/shared
LOCAL_INCLUDE_PATHS =
OPTIMIZE :=
LOCALES =
DEFINES = HVIF_STORE_CLIENT
WARNINGS =
SYMBOLS :=
DEBUGGER :=
COMPILER_FLAGS =
LINKER_FLAGS =
APP_VERSION :=
DRIVER_PATH =

## Include the Makefile-Engine
DEVEL_DIRECTORY := \
	$(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(DEVEL_DIRECTORY)/etc/makefile-engine
