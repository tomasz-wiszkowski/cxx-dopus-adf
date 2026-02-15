# CMake Build Instructions

This project now supports building with CMake as an alternative to the Visual Studio solution files.

## Prerequisites

- CMake 3.15 or higher and - preferably - Ninja (use `winget` to install)
- A C/C++ compiler (MSVC, GCC, or Clang)
- Internet connection (for downloading the Directory Opus SDK on first build)

## Building on Windows with Visual Studio

### Using Command Line

1. **Create a build directory:**
   ```bash
   mkdir build
   cd build
   ```

2. **Configure the project:**
   ```bash
   # For 64-bit build (recommended)
   cmake .. -A x64

   # For 32-bit build
   cmake .. -A Win32
   ```

3. **Build the project:**
   ```bash
   # Build Debug configuration
   cmake --build . --config Debug

   # Build Release configuration
   cmake --build . --config Release
   ```

## Project Structure

- **opusADF**: Main DLL/shared library (the Directory Opus plugin)
- **external**: Dependencies

## Troubleshooting

### SDK Download Fails

If the automatic SDK download fails, you can manually:
1. Download `opus_sdk.zip` from https://cdn2.gpsoft.com.au/files/Misc/opus_sdk.zip
2. Place it in `external/opus_sdk.zip`
3. Extract it to `external/dopus_sdk/`

### Build Errors

- Ensure all submodules are properly initialized if using git
- Check that the adflib sources are present in `external/adflib-vs/adflib/src/`
- Verify CMake version is 3.15 or higher: `cmake --version`

## Comparison with Visual Studio Build

The CMake build replicates the Visual Studio project configuration including:
- All source files and headers
- Include directories
- Preprocessor definitions
- Compiler flags and optimizations
- Library dependencies
- Debug/Release configurations
