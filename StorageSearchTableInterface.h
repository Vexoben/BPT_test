#ifndef STORAGE_SEARCH_TABLE_INTERFACE_H
#define STORAGE_SEARCH_TABLE_INTERFACE_H

#include "List.h"

namespace trainsys {

template <class KeyType, class ValueType>
struct DateType {
    KeyType key;
    ValueType value;
};

template <class KeyType, class ValueType>
class StorageSearchTable {
    virtual std::vector<ValueType> find(const KeyType &key) = 0;
    virtual void insert(const std::pair<KeyType, ValueType> &val) = 0;
    virtual void remove(const KeyType &key) = 0;
    virtual ~StorageSearchTable() {};
};

}

#endif // STORAGE_SEARCH_TABLE_INTERFACE_H