#pragma once

#include <windows.h>

#include <iterator>
#include <string_view>

namespace dopus {

/// @brief Wrapper for iterating over a double-NUL-terminated sequence of NUL-delimited wide strings.
/// @details Provides a range-based for loop interface for LPWSTR parameters that point to
/// multiple consecutive NUL-terminated strings, terminated by an additional NUL (double-NUL).
/// Example format: L"string1\0string2\0string3\0\0"
class wstring_view_span {
 public:
  class iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::wstring_view;
    using difference_type = std::ptrdiff_t;
    using pointer = const std::wstring_view*;
    using reference = std::wstring_view;

    explicit iterator(LPWSTR ptr = nullptr) : current_(ptr) {
      // If pointing to an empty string (double-NUL start), treat as end
      if (current_ && *current_ == L'\0') {
        current_ = nullptr;
      }
    }

    std::wstring_view operator*() const { return std::wstring_view(current_); }

    iterator& operator++() {
      if (current_) {
        // Move to the character after the current string's NUL terminator
        current_ += std::wstring_view(current_).length() + 1;
        // Check if we've reached the end (empty string = double-NUL)
        if (*current_ == L'\0') {
          current_ = nullptr;
        }
      }
      return *this;
    }

    iterator operator++(int) {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    bool operator==(const iterator& other) const { return current_ == other.current_; }

    bool operator!=(const iterator& other) const { return current_ != other.current_; }

   private:
    LPWSTR current_{};
  };

  explicit wstring_view_span(LPWSTR strings) : strings_(strings) {}

  iterator begin() const { return iterator(strings_); }

  iterator end() const { return iterator(nullptr); }

 private:
  LPWSTR strings_{};
};

}  // namespace dopus
