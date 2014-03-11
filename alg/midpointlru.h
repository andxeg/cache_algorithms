#pragma once

#include "lru.h"

#include <cmath>
#include <cstdlib>
#include <stdexcept>

template <typename Key, typename Value>
class MidPointLRUCache {
public:
    explicit MidPointLRUCache(size_t size, float point = 0.85) :
            cacheSize(size < 2 ? 2 : size),
            headSize(ceil(cacheSize * point)),
            tail(cacheSize),
            head(headSize) {
        head.setEvictionCallback([&](const Key &key, const Value &value) {
            tail.put(key, value);
        });
    }

    Value* find(const Key &key) {
        Value *value = head.find(key);
        if (value) {
            return value;
        }

        value = tail.find(key);

        if (value) {
            Value tmpValue = *value;
            value = head.put(key, tmpValue);
            tail.erase(key);
            return value;
        }

        return nullptr;
    }

    Value* put(const Key &key, const Value &value) {
        Value *result = find(key);
        if (result) {
            return result;
        }

        tail.setCacheSize(cacheSize - head.size());
        return tail.put(key, value);
    }

    bool erase(const Key &key) {
        if (!tail.erase(key)) {
            return head.erase(key);
        }

        return true;
    }

    void setEvictionCallback(std::function<void(const Key &,const Value &)> callback) {
        tail.setEvictionCallback(callback);
    }

    size_t size() const {
        return tail.size() + head.size();
    }

    void setCacheSize(size_t size) {
        cacheSize = size;
        tail.setCacheSize(cacheSize - head.size());
    }

private:
    size_t cacheSize;
    size_t headSize;
    LRUCache<Key, Value> tail;
    LRUCache<Key, Value> head;
};
