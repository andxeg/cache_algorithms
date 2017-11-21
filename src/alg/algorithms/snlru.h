#pragma once

#include "lru.h"

#include <cstdlib>
#include <vector>
#include <cmath>

#define REQUESTS_TRESHOLD 2


template <typename Key, typename Value>
class SNLRUCache {
    typedef std::unordered_map<std::string, size_t> ContentSizes;
    typedef std::unordered_map<std::string, size_t> CandidateList;
public:
    explicit SNLRUCache(size_t size, const size_t & learn_limit = 100, const size_t & period = 1000, size_t lruCount = 4) :
            cacheSize(size < lruCount ? lruCount : size),
            currentCacheSize(0) {
        for (size_t index = 0; index < lruCount; ++index) {
            lruList.push_back(LRUCache<Key, Value>(floor(float(cacheSize) / lruCount)));
            if (index != 0) {
                lruList.back().setEvictionCallback([=](const Key &key, const Value &value, const size_t & current_time ) {
                    this->lruList[index - 1].put(key, value, current_time);
                });
            }
        }
    }

    Value* find(const Key &key, const size_t & current_time = 0) {
        // CandidateList::iterator it = candidateList.find(key);
        // if (it == candidateList.end()) {
        //     candidateList[key] = 0;
        // }

        // candidateList[key] += 1;

        for (size_t index = 0; index < lruList.size() - 1; ++index) {
            Value *value = lruList[index].find(key, current_time);
            if (value) {
                value = lruList[index + 1].put(key, *value, current_time);
                lruList[index].erase(key);
                return value;
            }
        }

        return lruList.back().find(key, current_time);
    }

    Value* put(const Key &key, const Value &value, const size_t & current_time = 0) {
        // Value *result = find(key);
        // if (result) {
        //     return result;
        // }

        // size_t requests_count = candidateList[key];
        
        // if (requests_count <= REQUESTS_TRESHOLD && requests_count > 1) {
        //     return nullptr;
        // }


        return lruList.front().put(key, value, current_time);
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

    size_t elementsCount() const {
        size_t result = 0;
        for (auto & lruCache : lruList) {
            result += lruCache.elementsCount();
        }

        return result;
    }

    void setEvictionCallback(std::function<void(const Key &,const Value &)> callback) {
        lruList.front().setEvictionCallback(callback);
    }

    ContentSizes getContentSizes() {
        return contentSizes;
    }

    size_t getCacheSize() {
        currentCacheSize = 0;
        for (auto & lruCache : lruList) {
            currentCacheSize += lruCache.getCacheSize();
        }
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

        for (auto & lruCache : lruList) {
            lruCache.addCidSize(cid, size);
        }

    }

private:
    size_t cacheSize;
    size_t currentCacheSize;

    std::vector<LRUCache<Key, Value>> lruList;

    ContentSizes contentSizes;

    // CandidateList candidateList;
};