find_package(PkgConfig)
pkg_check_modules(DBUS dbus-1)

find_library(DBUS_LIBRARIES NAMES dbus-1)

find_path(DBUS_INCLUDE_DIR NAMES dbus/dbus.h
        HINTS
        /usr/include
        /usr/include/dbus-1.0
        /usr/local/include)

if (NOT DBUS_INCLUDE_DIR)
    message(FATAL_ERROR "Couldn't find d-bus includes")
endif ()

find_path(DBUS_ARCH_INCLUDE_DIR NAMES dbus/dbus-arch-deps.h
        HINTS
        /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/dbus-1.0/include)

if (NOT DBUS_ARCH_INCLUDE_DIR)
    message(FATAL_ERROR "Couldn't find d-bus architecture includes")
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBDBUS DEFAULT_MSG DBUS_LIBRARIES DBUS_INCLUDE_DIR)