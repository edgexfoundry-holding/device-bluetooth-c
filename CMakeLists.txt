cmake_minimum_required(VERSION 3.1)
project(device-ble-c LANGUAGES C)

# Package support
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_package(Libcsdk REQUIRED)
if (NOT LIBCSDK_FOUND)
    message (WARNING "C-SDK library or header not found")
endif ()

find_package(Libdbus REQUIRED)
if (NOT LIBDBUS_FOUND)
    message (WARNING "D-Bus library or header not found")
endif ()

include_directories (${DBUS_INCLUDE_DIR} ${DBUS_ARCH_INCLUDE_DIR})

# Set additional C compiler flags
if (DEFINED CMAKE_C_COMPILER_ID)
    if (CMAKE_C_COMPILER_ID MATCHES GNU)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wstrict-prototypes -Wmissing-prototypes")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -std=c99")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -O0")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -ggdb -O0")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -O3")
    endif ()
endif ()

file(GLOB C_FILES src/c/*.c)
add_executable(device-ble-c ${C_FILES})

FILE(STRINGS "VERSION" VERSION_NUMBER)

target_compile_definitions(device-ble-c PRIVATE VERSION="${VERSION_NUMBER}")
target_link_libraries(device-ble-c PUBLIC ${DBUS_LIBRARIES} PRIVATE ${LIBCSDK_LIBRARIES})