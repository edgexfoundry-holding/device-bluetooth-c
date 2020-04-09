find_library(LIBCSDK_LIBRARIES NAMES csdk libcsdk PATHS ENV CSDK_DIR PATH_SUFFIXES lib)
if(LIBCSDK_LIBRARIES STREQUAL "LIBCSDK_LIBRARIES-NOTFOUND")
    message(FATAL_ERROR "No C SDK library found in the default paths or $CSDK_DIR/lib. Please check your installation.")
endif()

find_path(LIBCSDK_INCLUDE_DIR edgex/edgex.h edgex/devsdk.h PATHS ENV CSDK_DIR PATH_SUFFIXES include)
if(LIBCSDK_INCLUDE_DIR STREQUAL "LIBCSDK_INCLUDE_DIR-NOTFOUND")
    message(FATAL_ERROR "No C SDK header found in the default paths or $CSDK_DIR/include. Please check your installation.")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBCSDK DEFAULT_MSG LIBCSDK_LIBRARIES LIBCSDK_INCLUDE_DIR)