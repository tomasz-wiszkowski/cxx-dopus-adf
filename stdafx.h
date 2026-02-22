// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <regex>
#include <span>
#include <sstream>
#include <string>
#include <vector>

extern "C" {
#include "external/dopus_sdk/headers/vfs plugins.h"
#include "external/dopus_sdk/headers/plugin support.h"

#define class devClass
#include "external/adflib/src/adflib.h"
#undef class
}
#include "opusADF.hpp"
