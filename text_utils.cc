#include "text_utils.hh"

#include <windows.h>

std::wstring latin1_to_wstring(std::string_view latin1) {
    int wide_len = MultiByteToWideChar(
        28591, // ISO-8859-1 code page
        /* flags= */0,
        latin1.data(),
        static_cast<int>(latin1.size()),
        nullptr,
        0);

    std::wstring wide(wide_len, 0);

    MultiByteToWideChar(
        28591,
        0,
        latin1.data(),
        static_cast<int>(latin1.size()),
        &wide[0],
        wide_len);

    // Truncate the null terminator added by MultiByteToWideChar
    if (!wide.empty() && wide.back() == L'\0') {
        wide.pop_back();
    }
    
    return wide;
}

std::string wstring_to_latin1(std::wstring_view wide) {
    int latin1_len = WideCharToMultiByte(
        28591,  // ISO-8859-1 code page
        WC_NO_BEST_FIT_CHARS,
        wide.data(),
        static_cast<int>(wide.size()),
        nullptr,
        0,
        "?",
        nullptr);

    std::string latin1(latin1_len, 0);

    WideCharToMultiByte(
        28591,
        WC_NO_BEST_FIT_CHARS,
        wide.data(),
        static_cast<int>(wide.size()),
        &latin1[0],
        latin1_len,
        "?",
        nullptr);

    // Truncate the null terminator added by WideCharToMultiByte
    if (!latin1.empty() && latin1.back() == '\0') {
        latin1.pop_back();
    }

    return latin1;
}

std::vector<std::wstring_view> split_path_to_components(std::wstring_view path, std::wstring_view relative_to) {
    std::vector<std::wstring_view> components;
    std::wstring::size_type pos_begin, pos_end = 0;

    if (!relative_to.empty()) {
        if (path.find(relative_to) == 0) {
            path.remove_prefix(relative_to.size());
        }
        else {
            // relative_to is not a prefix of path, return empty components
            return components;
        }
    }

    // TODO: there must be a more efficient way to do this
    while ((pos_begin = path.find_first_not_of(L"\\\\", pos_end)) != std::wstring::npos) {
        pos_end = path.find_first_of(L"\\\\", pos_begin);
        if (pos_end == std::wstring::npos) pos_end = path.size();
        components.push_back(path.substr(pos_begin, pos_end - pos_begin));
    }

    return components;
}

std::wstring_view file_from_path(std::wstring_view path) {
    auto pos = path.find_last_of(L"\\\\");
    if (pos == std::wstring::npos) {
        return path;
    }
    else {
        return path.substr(pos + 1);
    }
}
