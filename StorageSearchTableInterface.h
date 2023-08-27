#ifndef STORAGE_SEARCH_TABLE_INTERFACE_H
#define STORAGE_SEARCH_TABLE_INTERFACE_H

#include "List.h"

namespace trainsys {

template <class FirstType, class SecondType> struct Pair;

template <class KeyType, class ValueType>
class StorageSearchTable {
public:
    virtual seqList<ValueType> find(const KeyType &key) = 0;
    virtual void insert(const Pair<KeyType, ValueType> &val) = 0;
    virtual void remove(const Pair<KeyType, ValueType> &val) = 0;
    virtual ~StorageSearchTable() {};
};

}

#endif // STORAGE_SEARCH_TABLE_INTERFACE_H