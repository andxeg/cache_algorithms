#pragma once

#include "defs.h"
#include "lru.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>

template <typename Key, typename Value>
class ARCCache {
    typedef std::unordered_map<std::string, size_t> ContentSizes;
public:
    ARCCache() {};
    explicit ARCCache(size_t size, const size_t & learn_limit = 100, const size_t & period = 1000) :
            cacheSize(size < 1 ? 1 : size),
            splitPoint(0),
            top1Lru(cacheSize),
            top2Lru(cacheSize),
            bottom1Lru(cacheSize),
            bottom2Lru(cacheSize) {}

    Value* find(const Key &key, const size_t & current_time = 0) {
        Value *value = top2Lru.find(key);
        if (value) {
            return value;
        }

        value = top1Lru.find(key);
        if (value) {
            value = top2Lru.put(key, *value);
            top1Lru.erase(key);

            return value;
        }

        return nullptr;
    }

    void prepare_cache() {
        return;
    }

    Value* put(const Key &key, const Value &value, const size_t & current_time = 0) {
        if (bottom1Lru.find(key)) {
            splitPoint = std::min(cacheSize, splitPoint + std::max(bottom2Lru.size() / bottom1Lru.size(), size_t(1)));
            replace(key);
            return top2Lru.put(key, value);
        }

        if (bottom2Lru.find(key)) {
            splitPoint = std::max(size_t(0), splitPoint - std::max(bottom1Lru.size() / bottom2Lru.size(), size_t(1)));
            replace(key);
            return top2Lru.put(key, value);
        }

        size_t l1Size = top1Lru.size() + bottom1Lru.size();
        size_t l2Size = top2Lru.size() + bottom2Lru.size();

        if (l1Size == cacheSize) {
            if (top1Lru.size() < cacheSize) {
                bottom1Lru.erase(bottom1Lru.lruItem()->first);
                replace(key);
            } else {
                evicted(top1Lru.lruItem()->first, top1Lru.lruItem()->first);
                top1Lru.erase(top1Lru.lruItem()->first);
            }
        } else if (l1Size < cacheSize && (l1Size + l2Size >= cacheSize)) {
            if ((l1Size + l2Size) == cacheSize * 2) {
                bottom2Lru.erase(bottom2Lru.lruItem()->first);
            }
            replace(key);
        }

        return top1Lru.put(key, value);
    }

    bool erase(const Key &key) {
        return false;
    }

    void setEvictionCallback(std::function<void(const Key &,const Value &)> callback) {
        evictionCallback = callback;
    }

    size_t size() const {
        return top1Lru.size() + top2Lru.size();
    }

    size_t elementsCount() const {
        return 0;
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
    void replace(const Key &key) {
        bool keyInB2 = (bottom2Lru.find(key) != nullptr);

        if (top1Lru.size() >= 1 && ((keyInB2 && top1Lru.size() == splitPoint) || top1Lru.size() > splitPoint)) {
            assert(top1Lru.size() != 0);
            auto item = top1Lru.lruItem();
            bottom1Lru.put(item->first, 0);
            evicted(item->first, item->second);
            top1Lru.erase(item->first);
        } else {
            if (top2Lru.size() != 0) {
                auto item = top2Lru.lruItem();
                bottom2Lru.put(item->first, 0);
                evicted(item->first, item->second);
                top2Lru.erase(item->first);
            }
        }
    }

    void evicted(const Key &key, const Value &value) {
        if (evictionCallback) {
            evictionCallback(key, value);
        }
    }

private:
    size_t cacheSize;
    size_t currentCacheSize;
    size_t splitPoint;

    LRUCache<Key, Value> top1Lru;
    LRUCache<Key, Value> top2Lru;

    LRUCache<Key, char> bottom1Lru;
    LRUCache<Key, char> bottom2Lru;

    std::function<void(const Key &,const Value &)> evictionCallback;

    ContentSizes contentSizes;
};
