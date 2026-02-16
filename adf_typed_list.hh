#pragma once

#include <memory>

#include "adf_typed_iterator.hh"

template <typename T>
class AdfTypedList {
public:
    using iterator = AdfTypedIterator<T>;

    explicit AdfTypedList(AdfList* list, void(*deleter)(AdfList*)) : list_(list), deleter_(deleter) {}

    ~AdfTypedList() {
        if (list_) (*deleter_)(list_);
    }

    iterator begin() const {
        if (!list_) return iterator(nullptr);
        return iterator(list_);
    }

    iterator end() const {
        return iterator(nullptr);
    }

private:
    AdfList* list_;
    void(*deleter_)(AdfList*);
};
