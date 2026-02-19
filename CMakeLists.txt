cmake_minimum_required(VERSION 4.00)
project(opusADF VERSION 1.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Platform detection
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
set(PLATFORM_X64 TRUE)
else()
    set(PLATFORM_X64 FALSE)
endif()

# Configuration-specific settings
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(IS_DEBUG TRUE)
else()
    set(IS_DEBUG FALSE)
endif()

# Download and extract Directory Opus SDK
set(DOPUS_SDK_DIR "${CMAKE_SOURCE_DIR}/external/dopus_sdk")
set(DOPUS_SDK_ZIP "${CMAKE_SOURCE_DIR}/external/opus_sdk.zip")
set(DOPUS_SDK_URL "https://cdn2.gpsoft.com.au/files/Misc/opus_sdk.zip")

if(NOT EXISTS "${DOPUS_SDK_DIR}/headers")
    message(STATUS "Downloading Directory Opus SDK...")

    file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/external")

    if(NOT EXISTS "${DOPUS_SDK_ZIP}")
        file(DOWNLOAD "${DOPUS_SDK_URL}" "${DOPUS_SDK_ZIP}"
             SHOW_PROGRESS
             STATUS DOWNLOAD_STATUS)
        list(GET DOWNLOAD_STATUS 0 DOWNLOAD_RESULT)
        if(NOT DOWNLOAD_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to download Directory Opus SDK")
        endif()
    endif()

    message(STATUS "Extracting Directory Opus SDK...")
    file(ARCHIVE_EXTRACT INPUT "${DOPUS_SDK_ZIP}" DESTINATION "${DOPUS_SDK_DIR}")
endif()

# Add subdirectories
# Disable adflib tests to avoid building unnecessary dependencies
set(ADFLIB_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(ADFLIB_ENABLE_SALVAGE_DIRCACHE OFF CACHE BOOL "" FORCE)
set(ADFLIB_ENABLE_NATIVE_DEV OFF CACHE BOOL "" FORCE)
add_subdirectory(external/adflib EXCLUDE_FROM_ALL)


# Source files
set(OPUSADF_SOURCES
    dllmain.cpp
    opusADF.cpp
    text_utils.cc
)

# Header files
set(OPUSADF_HEADERS
    adf_typed_iterator.hh
    adf_typed_list.hh
    dopus_wstring_view_span.hh
    opusADF.hpp
    stdafx.h
    text_utils.hh
)

# Create shared library (DLL)
add_library(opusADF SHARED ${OPUSADF_SOURCES} ${OPUSADF_HEADERS})

# Include directories
target_include_directories(opusADF PRIVATE
    ${CMAKE_SOURCE_DIR}/external/dopus_sdk/headers
    ${CMAKE_SOURCE_DIR}/external/adflib/src
)

target_link_libraries(opusADF PRIVATE adf)

set (CPP_DIRECTIVES OPUSADF_EXPORTS _WINDOWS _USRDLL _CRT_SECURE_NO_WARNINGS LITT_ENDIAN DOPUS_PLUGIN_HELPER UNICODE _UNICODE)
if (IS_DEBUG)
    list(APPEND CPP_DIRECTIVES _DEBUG)
else()
    list(APPEND CPP_DIRECTIVES NDEBUG)
endif()

if (PLATFORM_X64)
list(APPEND CPP_DIRECTIVES _WIN64)
else()
    list(APPEND CPP_DIRECTIVES WIN32)
endif()

target_compile_definitions(opusADF PRIVATE ${CPP_DIRECTIVES})

# Compiler flags
if(MSVC)
    # Disable runtime checks to allow /O1 in debug builds.
    # string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

    target_compile_options(opusADF PRIVATE
        /W4      # Warning level
        /wd4100  # Unreferenced formal parameter
        #$<$<CONFIG:Debug>:/O4 /Zi /d2Zi+ /Zo /Oy>
        $<$<CONFIG:Debug>:/Od>   # Disable optimization for Debug
    )

    # Conformance mode
    target_compile_options(opusADF PRIVATE /permissive-)

    # SDL checks
    target_compile_options(opusADF PRIVATE /sdl)
else()
    # GCC/Clang flags
    target_compile_options(opusADF PRIVATE
        -Wall
        $<$<CONFIG:Debug>:-O0 -g>
        $<$<CONFIG:Release>:-Os -fomit-frame-pointer>
    )
endif()

# Linker flags
if(MSVC)
    set_target_properties(opusADF PROPERTIES
        LINK_FLAGS_DEBUG "/DEBUG /INCREMENTAL"
        LINK_FLAGS_RELEASE "/DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF"
    )

    # Set subsystem to Windows
    target_link_options(opusADF PRIVATE /SUBSYSTEM:WINDOWS)
endif()

# Set output name
set_target_properties(opusADF PROPERTIES
    OUTPUT_NAME "opusADF"
    PREFIX ""
)

set_target_properties(opusADF PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/Release"
)