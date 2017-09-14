#pragma once

#include <list>
#include <unordered_map>
#include <cstdlib>
#include <iostream>
#include <functional>
#include <utility>

template <typename Key, typename Value>
class LRUCache {
    typedef std::list<std::pair<Key, Value>> LruList;
    typedef std::unordered_map<std::string, size_t> ContentSizes;
public:
    explicit LRUCache(size_t size, const size_t & learn_limit = 100, const size_t & period = 1000) :
            cacheSize(size < 1 ? 1 : size),
            currentCacheSize(0) {}

    Value* find(const Key &key, const size_t & current_time = 0) {
        auto it = lookup.find(key);

        if (it == lookup.end()) {
            return nullptr;
        }

        return &promote(it->second)->second;
    }

    Value* put(const Key &key, const Value &value) {
        Value *result = find(key);
        if (result) {
            return result;
        }

        
        size_t cidSize = contentSizes[key];
        if (cidSize > cacheSize)
            return nullptr;

        makeSizeInvariant(cacheSize - cidSize);

        lruList.push_back(std::make_pair(key, value));
        auto addedIt = --lruList.end();
        lookup[key] = addedIt;

        currentCacheSize += cidSize;

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

    void setEvictionCallback(std::function<void(const Key &,const Value &)> callback) {
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

private:
    void makeSizeInvariant(size_t size) {
        while (getCacheSize() > size) {
            if (evictionCallback) {
                evictionCallback(lruList.front().first, lruList.front().second);
            }

            size_t cidSize = contentSizes[lruList.front().first];
            currentCacheSize -= cidSize;

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
    std::function<void(const Key &,const Value &)> evictionCallback;

    size_t currentCacheSize;
    ContentSizes contentSizes;
};
