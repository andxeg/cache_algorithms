#pragma once

#include "lru.h"

#include <cstdlib>
#include <vector>

template <typename Key, typename Value>
class SNLRUCache {
    typedef std::unordered_map<std::string, size_t> ContentSizes;
public:
    explicit SNLRUCache(size_t size, size_t lruCount = 4) :
            cacheSize(size < lruCount ? lruCount : size) {
        for (size_t index = 0; index < lruCount; ++index) {
            lruList.push_back(LRUCache<Key, Value>(cacheSize));
            if (index != 0) {
                lruList.back().setEvictionCallback([=](const Key &key, const Value &value) {
                    this->lruList[index - 1].setCacheSize(cacheSize - this->size() + this->lruList[index - 1].size() + 1);
                    this->lruList[index - 1].put(key, value);
                });
            }
        }
    }

    Value* find(const Key &key) {
        for (size_t index = 0; index < lruList.size() - 1; ++index) {
            Value *value = lruList[index].find(key);
            if (value) {
                lruList[index + 1].setCacheSize(cacheSize - size() + this->lruList[index].size() + 1);
                value = lruList[index + 1].put(key, *value);
                lruList[index].erase(key);
                return value;
            }
        }

        return lruList.back().find(key);
    }

    Value* put(const Key &key, const Value &value) {
        Value *result = find(key);
        if (result) {
            return result;
        }

        lruList.front().setCacheSize(cacheSize - size() + lruList.front().size());
        return lruList.front().put(key, value);
    }

    bool erase(const Key &key) {
        return false;
    }

    size_t size() const {
        size_t sz = 0;
        for (const auto& lru : lruList) {
            sz += lru.size();
        }

        return sz;
    }

    void setEvictionCallback(std::function<void(const Key &,const Value &)> callback) {
        lruList.front().setEvictionCallback(callback);
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
    size_t cacheSize;
    size_t currentCacheSize;

    std::vector<LRUCache<Key, Value>> lruList;

    ContentSizes contentSizes;
};
