# opusADF shared library (DLL)
project(opusADF LANGUAGES CXX)

# Source files
set(OPUSADF_SOURCES
    ../dllmain.cpp
    ../opusADF.cpp
    ../stdafx.cpp
)

# Header files
set(OPUSADF_HEADERS
    ../opusADF.hpp
    ../stdafx.h
)

# Create shared library (DLL)
add_library(opusADF SHARED ${OPUSADF_SOURCES} ${OPUSADF_HEADERS})

# Include directories
target_include_directories(opusADF PRIVATE
    ${CMAKE_SOURCE_DIR}/external/dopus_sdk/headers
    ${CMAKE_SOURCE_DIR}/external/adflib/src
)

target_link_libraries(opusADF PRIVATE adf)

# Preprocessor definitions - x64 only configurations
if(PLATFORM_X64)
    if(IS_DEBUG)
        target_compile_definitions(opusADF PRIVATE
            _DEBUG
            OPUSADF_EXPORTS
            _WINDOWS
            _USRDLL
            _CRT_SECURE_NO_WARNINGS
            LITT_ENDIAN
            DOPUS_PLUGIN_HELPER
            UNICODE
            _UNICODE
        )
    else()
        target_compile_definitions(opusADF PRIVATE
            NDEBUG
            OPUSADF_EXPORTS
            _WINDOWS
            _USRDLL
            _CRT_SECURE_NO_WARNINGS
            LITT_ENDIAN
            DOPUS_PLUGIN_HELPER
            UNICODE
            _UNICODE
        )
    endif()
else()
    # Win32 configurations
    if(IS_DEBUG)
        target_compile_definitions(opusADF PRIVATE
            WIN32
            _DEBUG
            OPUSADF_EXPORTS
            _WINDOWS
            _USRDLL
            UNICODE
            _UNICODE
        )
    else()
        target_compile_definitions(opusADF PRIVATE
            WIN32
            NDEBUG
            OPUSADF_EXPORTS
            _WINDOWS
            _USRDLL
            UNICODE
            _UNICODE
        )
    endif()
endif()

# Compiler flags
if(MSVC)
    target_compile_options(opusADF PRIVATE
        /W3      # Warning level 3
        $<$<CONFIG:Debug>:/Od>   # Disable optimization for Debug
        $<$<CONFIG:Release>:/O1> # Optimize for size
        $<$<CONFIG:Release>:/Oy> # Omit frame pointers
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

# Optional: Set custom output directory (similar to VS project)
# Uncomment if you want to output to a specific directory
# set_target_properties(opusADF PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/Debug"
#     RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/Release"
# )
