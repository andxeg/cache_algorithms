#pragma once

#include <cstdlib>
#include <cassert>
#include <functional>
#include <list>
#include <unordered_map>

template <typename Key, typename Value>
class FifoCache {
    typedef std::unordered_map<std::string, size_t> ContentSizes;
public:
    explicit FifoCache(size_t size) :
            cacheSize(size < 1 ? 1 : size) {}

    Value* find(const Key &key) const {
        auto it = lookup.find(key);

        if (it == lookup.end()) {
            return nullptr;
        }

        return &it->second->second;
    }

    Value* put(const Key &key, const Value &value) {
        Value *result = find(key);
        if (result) {
            return result;
        }

        makeSizeInvariant(cacheSize - 1);

        fifo.push_back(std::make_pair(key, value));

        auto addedItemIt = --fifo.end();
        lookup[key] = addedItemIt;

        assert(lookup.find(fifo.front().first) != lookup.end());

        assert(lookup.find(fifo.front().first) != lookup.end());

        return &addedItemIt->second;
    }

    bool erase(const Key &key) {
        auto it = lookup.find(key);

        if (it == lookup.end()) {
            return false;
        }

        fifo.erase(it->second);
        lookup.erase(it);

        return true;
    }

    size_t size() const {
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
        size_t result = 0;
        
        for (auto& element : lookup) {
            result += contentSizes[element.first];
        }
        
        result = currentCacheSize;

        return result;
    }

    void addCidSize(std::string cid, size_t size) {
        ContentSizes::iterator it = contentSizes.find(cid);
        if (it != contentSizes.end())
            std::cout << "Size is repeated. Was -> " <<  it->second
                << " now -> "<< size 
                << " for cid -> " << cid << std::endl;

        contentSizes[cid] = size;
    }

private:
    void makeSizeInvariant(size_t size) {
        while (lookup.size() > size) {
            if (evictionCallback) {
                evictionCallback(fifo.front().first, fifo.front().second);
            }

            lookup.erase(fifo.front().first);
            fifo.pop_front();
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
