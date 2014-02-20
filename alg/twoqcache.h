#pragma once

#include "fifocache.h"
#include "lru.h"

template <typename Key, typename Value>
class TwoQCache {
public:
    explicit TwoQCache(size_t size, float mainCacheFactor = 0.75, float outCacheFactor = 5) :
            cacheSize(size < 2 ? 2 : size),
            mainCache(cacheSize * mainCacheFactor),
            aIn(size * (1 - mainCacheFactor)),
            aOut(cacheSize * outCacheFactor) {
        aIn.setEvictionCallback([&](const Key &key, const Value &value) {
            aOut.put(key, 0);
            if (evictionCallback) {
                evictionCallback(key, value);
            }
        });
    }

    Value* find(const Key &key) {
        Value *value = mainCache.find(key);

        if (value) {
            return value;
        }

        return aIn.find(key);
    }

    Value* put(const Key &key, const Value &value) {
        Value *result = find(key);

        if (result) {
            return result;
        }

        if (aOut.find(key)) {
            aOut.erase(key);
            return mainCache.put(key, value);
        }

        aIn.setCacheSize(cacheSize - mainCache.size());
        result = aIn.put(key, value);

        return result;
    }

    bool erase(const Key &key) {
        aOut.erase(key);
        if (!mainCache.erase(key)) {
            return aIn.erase(key);
        }

        return true;
    }

    size_t size() const {
        return aIn.size() + mainCache.size();
    }

    void setEvictionCallback(std::function<void(const Key &,const Value &)> callback) {
        evictionCallback = callback;
        mainCache.setEvictionCallback(callback);
    }

private:
    size_t cacheSize;

    LRUCache<Key, Value> mainCache;
    FifoCache<Key, Value> aIn;
    FifoCache<Key, char> aOut;

    std::function<void(const Key &,const Value &)> evictionCallback;
};
