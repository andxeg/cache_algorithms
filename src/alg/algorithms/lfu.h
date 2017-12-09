#pragma once

#include "defs.h"

#include <list>
#include <utility>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <functional>
#include <unordered_map>


template <typename Key, typename Value>
class LFUCache {
    typedef std::list<std::pair<Key, Value>> ItemList;
    typedef std::list<ItemList> LFUList;
    typedef std::unordered_map<std::string, size_t> ContentSizes;

    struct ItemMeta {
        ItemMeta() {}

        ItemMeta(typename ItemList::iterator iIt, typename LFUList::iterator lIt) :
                itemIt(iIt),
                lfuIt(lIt) {}

        typename ItemList::iterator itemIt;
        typename LFUList::iterator lfuIt;
    };

public:
    LFUCache() {};
    explicit LFUCache(size_t size, const size_t & learn_limit = 100, const size_t & period = 1000) :
            cacheSize(size < 1 ? 1 : size),
            currentCacheSize(0) {}

    Value* find(const Key &key, const size_t & current_time = 0) {
        auto it = lookup.find(key);

        if (it == lookup.end()) {
            return nullptr;
        }

        return &promote(it->second)->second;
    }

    void prepare_cache() {
        return;
    }

    Value* put(const Key &key, const Value &value, const size_t & current_time = 0) {
        Value *result = find(key);
        if (result) {
            return result;
        }

        if (lfuList.empty()) {
            lfuList.push_back(ItemList());
        }

        size_t cidSize = contentSizes[key];
        if (cidSize > cacheSize)
            return nullptr;

        makeSizeInvariant(cacheSize - cidSize);

        lfuList.front().push_back(std::make_pair(key, value));
        auto addedItemIt = --lfuList.front().end();
        lookup[key] = ItemMeta(addedItemIt, lfuList.begin());

        currentCacheSize += cidSize;

        return &addedItemIt->second;
    }

    bool erase(const Key &key) {
        auto it = lookup.find(key);

        if (it == lookup.end()) {
            return false;
        }

        ItemMeta &itemMeta = it->second;
        itemMeta.lfuIt->erase(itemMeta.itemIt);
        lookup.erase(it);

        return true;
    }

    void setCacheSize(size_t size) {
        cacheSize = size;
        makeSizeInvariant(cacheSize);
    }

    size_t size() const {
        return lookup.size();
    }

    size_t elementsCount() const {
        return lookup.size();
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
            std::cout << "Size is repeated. Was -> " <<  it->second
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
            auto lfuIt = lfuList.begin();
            while (lfuIt->empty() && lfuIt != lfuList.end()) {
                ++lfuIt;
            }

            assert(lfuIt != lfuList.end());

            if (evictionCallback) {
                evictionCallback(lfuIt->front().first, lfuIt->front().second);
            }

            assert(!lfuIt->empty());

            const Key &key = lfuIt->front().first;

            currentCacheSize -= contentSizes[key];

            lookup.erase(key);

            lfuIt->pop_front();
        }
    }

    typename ItemList::iterator promote(ItemMeta itemMeta) {
        Key key = itemMeta.itemIt->first;

        auto nextLfuIt = itemMeta.lfuIt;
        ++nextLfuIt;

        if (nextLfuIt == lfuList.end()) {
            lfuList.push_back(ItemList());
            nextLfuIt = --lfuList.end();
        }

        assert(itemMeta.lfuIt != nextLfuIt);

        nextLfuIt->push_back(*itemMeta.itemIt);

        auto itemIt = --nextLfuIt->end();

        lookup[key] = ItemMeta(itemIt, nextLfuIt);

        assert(itemIt != itemMeta.itemIt);

        assert(key == itemMeta.itemIt->first);

        itemMeta.lfuIt->erase(itemMeta.itemIt);

        return itemIt;
    }

private:
    std::unordered_map<Key, ItemMeta> lookup;
    LFUList lfuList;
    size_t cacheSize;
    size_t currentCacheSize;

    std::function<void(const Key &,const Value &)> evictionCallback;

    ContentSizes contentSizes;
};
