#pragma once

#include "lru.h"

#include <cmath>
#include <cstdlib>
#include <stdexcept>

template <typename Key, typename Value>
class MidPointLRUCache {
    typedef std::unordered_map<std::string, size_t> ContentSizes;
public:
    explicit MidPointLRUCache(size_t size, const size_t & learn_limit = 100, const size_t & period = 1000, float point = 0.85) :
            cacheSize(size < 2 ? 2 : size),
            currentCacheSize(0),
            headSize(ceil(cacheSize * point)),
            tail(cacheSize - headSize),
            head(headSize) {
        head.setEvictionCallback([&](const Key &key, const Value &value) {
            tail.put(key, value);
        });
    }

    Value* find(const Key &key, const size_t & current_time = 0) {
        Value *value = head.find(key);
        if (value) {
            return value;
        }

        value = tail.find(key);

        if (value) {
            Value tmpValue = *value;
            tail.erase(key);
            value = head.put(key, tmpValue);
            return value;
        }

        return nullptr;
    }

    Value* put(const Key &key, const Value &value) {
        Value *result = find(key);
        if (result) {
            return result;
        }

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

    size_t elementsCount() const {
        return head.elementsCount() + tail.elementsCount();
    }

    void setCacheSize(size_t size) {
        cacheSize = size;
        tail.setCacheSize(cacheSize - head.size());
    }

    ContentSizes getContentSizes() {
        return contentSizes;
    }

    size_t getCacheSize() {
        currentCacheSize = head.getCacheSize() + tail.getCacheSize();
        return currentCacheSize;
    }

    void addCidSize(std::string cid, size_t size) {
        ContentSizes::iterator it = contentSizes.find(cid);
        if (it != contentSizes.end() && it->second != size) {
            std::cout << "Another size for content. Was -> " <<  it->second
                << " now -> "<< size 
                << " for cid -> " << cid << std::endl;
        }

        contentSizes[cid] = size;

        head.addCidSize(cid, size);
        tail.addCidSize(cid, size);
    }

private:
    size_t cacheSize;
    size_t currentCacheSize;
    size_t headSize;
    LRUCache<Key, Value> tail;
    LRUCache<Key, Value> head;
    ContentSizes contentSizes;
};
