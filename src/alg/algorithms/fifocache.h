#pragma once

#include "defs.h"

#include <cstdlib>
#include <cassert>
#include <functional>
#include <list>
#include <unordered_map>

template <typename Key, typename Value>
class FifoCache {
    typedef std::unordered_map<std::string, size_t> ContentSizes;
public:
    FifoCache() {};
    explicit FifoCache(size_t size, const size_t & learn_limit = 100, const size_t & period = 1000) :
            cacheSize(size < 1 ? 1 : size),
            currentCacheSize(0) {}

    Value* find(const Key &key, const size_t & current_time = 0) const {
        auto it = lookup.find(key);

        if (it == lookup.end()) {
            return nullptr;
        }

        return &it->second->second;
    }

    void prepare_cache() {
        return;
    }

    Value* put(const Key &key, const Value &value, const size_t & current_time = 0) {
        Value *result = find(key);
        if (result) {
            return result;
        }

        size_t cidSize = contentSizes[key];

        if (cidSize > cacheSize)
            return nullptr;

        makeSizeInvariant(cacheSize - cidSize);

        fifo.push_back(std::make_pair(key, value));

        auto addedItemIt = --fifo.end();
        lookup[key] = addedItemIt;

        currentCacheSize += cidSize;

        assert(lookup.find(fifo.front().first) != lookup.end());
        return &addedItemIt->second;
    }

    bool erase(const Key &key) {
        auto it = lookup.find(key);

        if (it == lookup.end()) {
            return false;
        }

        size_t cidSize = contentSizes[key];
        currentCacheSize -= cidSize;
        
        fifo.erase(it->second);
        lookup.erase(it);

        return true;
    }

    size_t size() const {
        return lookup.size();
    }

    size_t elementsCount() const {
        return lookup.size();
    }

    void setCacheSize(size_t size) {
        cacheSize = size;
        makeSizeInvariant(cacheSize);
    }

    void setEvictionCallback(std::function<void(const Key &,const Value &)> callback) {
        evictionCallback = callback;
    }

    ContentSizes getContentSizes() {
        return contentSizes;
    }

    size_t getCacheSize() {
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
    }

    VecStr get_hot_content(const float &cache_hot_content) {
        return VecStr();
    }

private:
    void makeSizeInvariant(size_t size) {
        while (getCacheSize() > size) {
            if (evictionCallback) {
                evictionCallback(fifo.front().first, fifo.front().second);
            }

            auto cid = fifo.front().first;
            auto cidSize = contentSizes[cid];
            lookup.erase(cid);
            fifo.pop_front();
            currentCacheSize -= cidSize;
        }
    }

private:
    typedef std::list<std::pair<Key, Value>> Fifo;
    Fifo fifo;
    std::unordered_map<Key, typename Fifo::iterator> lookup;
    size_t cacheSize;
    size_t currentCacheSize;
    std::function<void(const Key &,const Value &)> evictionCallback;
    ContentSizes contentSizes;
};
