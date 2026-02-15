# ADFLib static library
project(adflib-vs LANGUAGES C)

# Source files
set(ADFLIB_SOURCES
    adflib/src/adflib.c
    adflib/src/adf_bitm.c
    adflib/src/adf_cache.c
    adflib/src/adf_debug.c
    adflib/src/adf_dev.c
    adflib/src/adf_dev_drivers.c
    adflib/src/adf_dev_driver_dump.c
    adflib/src/adf_dev_driver_ramdisk.c
    adflib/src/adf_dev_flop.c
    adflib/src/adf_dev_hd.c
    adflib/src/adf_dev_hdfile.c
    adflib/src/adf_dev_type.c
    adflib/src/adf_dir.c
    adflib/src/adf_env.c
    adflib/src/adf_file.c
    adflib/src/adf_file_block.c
    adflib/src/adf_file_util.c
    adflib/src/adf_link.c
    adflib/src/adf_raw.c
    adflib/src/adf_salv.c
    adflib/src/adf_str.c
    adflib/src/adf_util.c
    adflib/src/adf_vector.c
    adflib/src/adf_vol.c
)

# Header files
set(ADFLIB_HEADERS
    adflib-vs/config.h
    adflib/src/adflib.h
    adflib/src/adf_bitm.h
    adflib/src/adf_blk.h
    adflib/src/adf_blk_hd.h
    adflib/src/adf_byteorder.h
    adflib/src/adf_cache.h
    adflib/src/adf_debug.h
    adflib/src/adf_dev.h
    adflib/src/adf_dev_driver.h
    adflib/src/adf_dev_drivers.h
    adflib/src/adf_dev_driver_dump.h
    adflib/src/adf_dev_driver_nativ.h
    adflib/src/adf_dev_driver_ramdisk.h
    adflib/src/adf_dev_flop.h
    adflib/src/adf_dev_hd.h
    adflib/src/adf_dev_hdfile.h
    adflib/src/adf_dev_type.h
    adflib/src/adf_dir.h
    adflib/src/adf_env.h
    adflib/src/adf_err.h
    adflib/src/adf_file.h
    adflib/src/adf_file_block.h
    adflib/src/adf_file_util.h
    adflib/src/adf_limits.h
    adflib/src/adf_link.h
    adflib/src/adf_prefix.h
    adflib/src/adf_raw.h
    adflib/src/adf_salv.h
    adflib/src/adf_str.h
    adflib/src/adf_types.h
    adflib/src/adf_util.h
    adflib/src/adf_vector.h
    adflib/src/adf_version.h
    adflib/src/adf_vol.h
)

# Create static library
add_library(adflib-vs STATIC ${ADFLIB_SOURCES} ${ADFLIB_HEADERS})

# Include directories
target_include_directories(adflib-vs
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/adflib/src
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/adflib-vs
        ${CMAKE_CURRENT_SOURCE_DIR}/adflib/src/win32
)

# Preprocessor definitions
if(IS_DEBUG)
    target_compile_definitions(adflib-vs PRIVATE
        _DEBUG
        _WINDOWS
    )
else()
    target_compile_definitions(adflib-vs PRIVATE
        NDEBUG
        _WINDOWS
    )
endif()

# Compiler flags
target_compile_options(adflib-vs PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W3>
)

# Force compilation as C
set_target_properties(adflib-vs PROPERTIES
    LINKER_LANGUAGE C
)
