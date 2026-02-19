#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

/// @brief Convert a latin1 string to UTF-8
/// @param latin1 Input string in latin1 encoding
/// @return widestring in UTF-8 encoding
std::wstring latin1_to_wstring(std::string_view latin1);

/// @brief Convert a wide string to latin1 encoding
/// @param wide Input string in wide character encoding (UTF-16 on Windows)
/// @return latin1 encoded string
std::string wstring_to_latin1(std::wstring_view wide);

/// @brief Convert a wide string to utf8 encoding
/// @param wide Input string in wide character encoding (UTF-16 on Windows)
/// @return latin1 encoded string
std::filesystem::path wstring_to_path(std::wstring_view wide);

/// @brief Split a path string into its components
/// @param path Input string to split
/// @param relative_to Optional base path to make the split relative to. If
/// empty, the split is absolute.
/// @return Vector of string views representing the split parts
std::vector<std::wstring_view> split_path_to_components(std::wstring_view path, std::wstring_view relative_to = L"");

/// @brief Returns the file name from a path
/// @param path Path string to extract the file name from
/// @return the file name component of the path
std::wstring_view file_from_path(std::wstring_view path);
