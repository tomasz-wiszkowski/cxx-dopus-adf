#pragma once

#include "external/adflib/src/adf_str.h"
#include <iterator>

template <typename T>
class AdfTypedIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;

    explicit AdfTypedIterator(AdfList* node = nullptr) : current_(node) {}

    T& operator*() const {
        return *static_cast<T*>(current_->content);
    }

    T* operator->() const {
        return static_cast<T*>(current_->content);
    }

    AdfTypedIterator& operator++() {
        if (current_) {
            current_ = current_->next;
        }
        return *this;
    }

    AdfTypedIterator operator++(int) {
        AdfTypedIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    bool operator==(const AdfTypedIterator& other) const {
        return current_ == other.current_;
    }

    bool operator!=(const AdfTypedIterator& other) const {
        return current_ != other.current_;
    }

private:
    AdfList* current_{};
};
