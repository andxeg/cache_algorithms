#pragma once

#include "defs.h"

#include <list>
#include <unordered_map>
#include <cstdlib>
#include <iostream>
#include <functional>
#include <utility>

#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))


template <typename Key, typename Value>
class LRUCache {
    typedef std::list<std::pair<Key, Value>> LruList;
    typedef std::unordered_map<std::string, size_t> ContentSizes;
public:
    LRUCache() {};
    explicit LRUCache(size_t size, const size_t & learn_limit = 100, const size_t & period = 1000) :
            cacheSize(size < 1 ? 1 : size),
            currentCacheSize(0) {}

    Value* find(const Key &key, const size_t & current_time = 0) {
        // std::cout << "lru find 1.1" << std::endl;
        // std::cout << "lookup.size() -> " << lookup.size() << std::endl;
        auto it = lookup.find(key);

        // std::cout << "lru find 1.2" << std::endl;

        if (it == lookup.end()) {
            return nullptr;
        }

        // std::cout << "lru find 2" << std::endl;

        return &promote(it->second)->second;
    }

    void prepare_cache() {
        return;
    }

    Value* put(const Key &key, const Value &value, const size_t & current_time = 0) {
        // std::cout << "lru put 1.1" << std::endl;

        Value *result = find(key);
        if (result) {
            return result;
        }

        // std::cout << "lru put 1.2" << std::endl;

        if (contentSizes.find(key) == contentSizes.end()) {
            std::cout << "there is not cid: " << key << " in contentSizes" << std::endl;
        }
        size_t cidSize = contentSizes[key];
        if (cidSize > cacheSize)
            return nullptr;

        // std::cout << "lru put 2" << std::endl;

        makeSizeInvariant(cacheSize - cidSize);

        lruList.push_back(std::make_pair(key, value));
        auto addedIt = --lruList.end();
        lookup[key] = addedIt;

        currentCacheSize += cidSize;

        // std::cout << "lru put 3" << std::endl;

        return &addedIt->second;
    }

    bool erase(const Key &key) {
        auto it = lookup.find(key);

        if (it == lookup.end()) {
            return false;
        }

        size_t cidSize = contentSizes[key];
        currentCacheSize -= cidSize;

        lruList.erase(it->second);
        lookup.erase(it);

        return true;
    }

    void setEvictionCallback(std::function<void(const Key &,const Value &, const size_t & current_time)> callback) {
        evictionCallback = callback;
    }

    size_t size() const {
        // limit of cache size
        return cacheSize;

        // return lookup.size(); previous code
    }

    size_t elementsCount() const {
        return lookup.size();
    }

    void setCacheSize(size_t size) {
        cacheSize = size;
        makeSizeInvariant(cacheSize);
    }

    const std::pair<Key, Value> *mruItem() const {
        return &lruList.back();
    }

    const std::pair<Key, Value> *lruItem() const {
        return &lruList.front();
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
        VecStr hot_content;
        int curr_count = 0;
        int count = MAX((int)(cache_hot_content*elementsCount()), 1);
        typename std::list<std::pair<Key, Value>>::reverse_iterator it = 
                                            lruList.rbegin();
        for (; it != lruList.rend() && curr_count++ < count; ++it) {
            hot_content.push_back(it->first);
        }

        return hot_content;
    }

private:
    void makeSizeInvariant(size_t size) {
        while (getCacheSize() > size) {
            // std::cout << "make size invariant" << std::endl;
            if (evictionCallback) {
                // std::cout << "1" << std::endl;
                evictionCallback(lruList.front().first, lruList.front().second, 0);
            }

            // std::cout << "2" << std::endl;
            size_t cidSize = contentSizes[lruList.front().first];
            currentCacheSize -= cidSize;
            // std::cout << "3" << std::endl;

            lookup.erase(lruList.front().first);

            lruList.pop_front();
        }
    }

    typename LruList::iterator promote(typename LruList::iterator it) {
        lruList.push_back(*it);

        auto addedIt = --lruList.end();
        lookup[it->first] = addedIt;

        lruList.erase(it);

        return addedIt;
    }    

// private:
public:
    LruList lruList;
    std::unordered_map<Key, typename LruList::iterator> lookup;
    size_t cacheSize;
    std::function<Value(const Key&)> getFunction;
    std::function<void(const Key &,const Value &, const size_t & current_time)> evictionCallback;

    size_t currentCacheSize;
    ContentSizes contentSizes;
};
