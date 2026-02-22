#pragma once

#include <memory>

#include "adf_typed_iterator.hh"

template <typename T>
class AdfTypedList {
 public:
  using iterator = AdfTypedIterator<T>;

  explicit AdfTypedList(AdfList* list, void (*deleter)(AdfList*)) : list_(list), deleter_(deleter) {}

  AdfTypedList(AdfTypedList&& other) noexcept { *this = std::move(other); }

  AdfTypedList<T>& operator=(AdfTypedList&& other) noexcept {
    if (this != &other) {
      std::swap(list_, other.list_);
      std::swap(deleter_, other.deleter_);
    }
    return *this;
  }

  ~AdfTypedList() {
    if (list_)
      (*deleter_)(list_);
  }

  iterator begin() const {
    if (!list_)
      return iterator(nullptr);
    return iterator(list_);
  }

  iterator end() const { return iterator(nullptr); }

 private:
  AdfList* list_{};
  void (*deleter_)(AdfList*){};
};
