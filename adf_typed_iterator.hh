#pragma once

#include <iterator>

#include "external/adflib/src/adf_str.h"

template <typename T>
class AdfTypedIterator {
 public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using reference = T&;

  explicit AdfTypedIterator(AdfList* node = nullptr) { set_current(node); }

  T& operator*() const { return *elem_; }

  T* operator->() const { return elem_; }

  void set_current(AdfList* node) {
    current_ = node;
    elem_ = static_cast<T*>(current_ ? current_->content : nullptr);
  }

  AdfTypedIterator& operator++() {
    if (current_)
      set_current(current_->next);
    return *this;
  }

  AdfTypedIterator operator++(int) {
    AdfTypedIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  bool operator==(const AdfTypedIterator& other) const { return current_ == other.current_; }

  bool operator!=(const AdfTypedIterator& other) const { return current_ != other.current_; }

 private:
  AdfList* current_{};
  T* elem_{};
};
