// Test file for dopus_wstring_view_array.hh
// This is a standalone test to verify the functionality of the iterator

#include "dopus_wstring_view_array.hh"
#include <iostream>
#include <vector>

int main() {
    // Test 1: Multiple strings
    {
        wchar_t strings[] = L"first\0second\0third\0\0";

        std::vector<std::wstring_view> results;
        for (auto str : dopus_wstring_view_array(strings)) {
            results.push_back(str);
        }

        std::wcout << L"Test 1 - Multiple strings: ";
        if (results.size() == 3 &&
            results[0] == L"first" &&
            results[1] == L"second" &&
            results[2] == L"third") {
            std::wcout << L"PASSED\n";
        } else {
            std::wcout << L"FAILED\n";
            return 1;
        }
    }

    // Test 2: Single string
    {
        wchar_t strings[] = L"only\0\0";

        std::vector<std::wstring_view> results;
        for (auto str : dopus_wstring_view_array(strings)) {
            results.push_back(str);
        }

        std::wcout << L"Test 2 - Single string: ";
        if (results.size() == 1 && results[0] == L"only") {
            std::wcout << L"PASSED\n";
        } else {
            std::wcout << L"FAILED\n";
            return 1;
        }
    }

    // Test 3: Empty sequence (double-NUL at start)
    {
        wchar_t strings[] = L"\0\0";

        std::vector<std::wstring_view> results;
        for (auto str : dopus_wstring_view_array(strings)) {
            results.push_back(str);
        }

        std::wcout << L"Test 3 - Empty sequence: ";
        if (results.empty()) {
            std::wcout << L"PASSED\n";
        } else {
            std::wcout << L"FAILED\n";
            return 1;
        }
    }

    // Test 4: nullptr
    {
        std::vector<std::wstring_view> results;
        for (auto str : dopus_wstring_view_array(nullptr)) {
            results.push_back(str);
        }

        std::wcout << L"Test 4 - nullptr: ";
        if (results.empty()) {
            std::wcout << L"PASSED\n";
        } else {
            std::wcout << L"FAILED\n";
            return 1;
        }
    }

    // Test 5: Strings with different lengths
    {
        wchar_t strings[] = L"a\0longer string\0x\0\0";

        std::vector<std::wstring_view> results;
        for (auto str : dopus_wstring_view_array(strings)) {
            results.push_back(str);
        }

        std::wcout << L"Test 5 - Different lengths: ";
        if (results.size() == 3 &&
            results[0] == L"a" &&
            results[1] == L"longer string" &&
            results[2] == L"x") {
            std::wcout << L"PASSED\n";
        } else {
            std::wcout << L"FAILED\n";
            return 1;
        }
    }

    std::wcout << L"\nAll tests passed!\n";
    return 0;
}
