#pragma once

#include "fifocache.h"
#include "lru.h"

#include <cmath>
#include <cstdlib>

template <typename Key, typename Value>
class TwoQCache {
    typedef std::unordered_map<std::string, size_t> ContentSizes;
public:
    explicit TwoQCache(size_t size, float mainCacheFactor = 0.60, 
                                    float outCacheFactor = 0.20,
                                    float inCacheFactor = 0.20) :
            cacheSize(size < 2 ? 2 : size),
            currentCacheSize(0),
            mainCache(floor(cacheSize * mainCacheFactor)),
            aIn(floor(cacheSize * inCacheFactor)),
            aOut(floor(cacheSize * outCacheFactor)) 
    {
        aIn.setEvictionCallback([&](const Key &key, const Value &value) {
            aOut.put(key, value);
            if (evictionCallback) {
                evictionCallback(key, value);
            }
        });
    }

    Value* find(const Key &key, const size_t & current_time = 0) {
        Value *value = mainCache.find(key);

        if (value) {
            return value;
        }

        value = aOut.find(key);

        if (value) {
            Value tmpValue = *value;
            aOut.erase(key);
            return mainCache.put(key, tmpValue);
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

        return aIn.put(key, value);
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

    size_t elementsCount() const {
        return  aIn.elementsCount() + 
                aOut.elementsCount() + 
                mainCache.elementsCount();
    }

    void setEvictionCallback(std::function<void(const Key &,const Value &)> callback) {
        evictionCallback = callback;
        mainCache.setEvictionCallback(callback);
    }

    ContentSizes getContentSizes() {
        return contentSizes;
    }

    size_t getCacheSize() {
        currentCacheSize = 0;
        currentCacheSize += aIn.getCacheSize() + 
                            aOut.getCacheSize() + 
                            mainCache.getCacheSize();

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

        aIn.addCidSize(cid, size);
        aOut.addCidSize(cid, size);
        mainCache.addCidSize(cid, size);
    }

private:
    size_t cacheSize;
    size_t currentCacheSize;

    LRUCache<Key, Value> mainCache;
    FifoCache<Key, Value> aIn;
    FifoCache<Key, Value> aOut;

    std::function<void(const Key &,const Value &)> evictionCallback;

    ContentSizes contentSizes;
};
