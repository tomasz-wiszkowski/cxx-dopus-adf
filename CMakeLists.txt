cmake_minimum_required(VERSION 3.15)
project(opusADF VERSION 1.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

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
add_subdirectory(external/adflib)
add_subdirectory(src)
