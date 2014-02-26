#pragma once

#include <functional>
#include <list>
#include <unordered_map>
#include <utility>

template <typename Key, typename Value>
class LRUCache {
    typedef std::list<std::pair<Key, Value>> LruList;
public:
    explicit LRUCache(size_t size) :
            cacheSize(size < 1 ? 1 : size) {}

    Value* find(const Key &key) {
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

        makeSizeInvariant(cacheSize - 1);

        lruList.push_back(std::make_pair(key, value));
        auto addedIt = --lruList.end();
        lookup[key] = addedIt;

        return &addedIt->second;
    }

    bool erase(const Key &key) {
        auto it = lookup.find(key);

        if (it == lookup.end()) {
            return false;
        }

        lruList.erase(it->second);
        lookup.erase(it);

        return true;
    }

    void setEvictionCallback(std::function<void(const Key &,const Value &)> callback) {
        evictionCallback = callback;
    }

    size_t size() const {
        return lookup.size();
    }

    void setCacheSize(size_t size) {
        cacheSize = size;
        makeSizeInvariant(cacheSize);
    }

    const std::pair<Key, Value> *head() const {
        return &lruList.back();
    }

    const std::pair<Key, Value> *tail() const {
        return &lruList.front();
    }

private:
    void makeSizeInvariant(size_t size) {
        while (lookup.size() > size) {
            if (evictionCallback) {
                evictionCallback(lruList.front().first, lruList.front().second);
            }

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

private:
    LruList lruList;
    std::unordered_map<Key, typename LruList::iterator> lookup;
    size_t cacheSize;
    std::function<Value(const Key&)> getFunction;

    std::function<void(const Key &,const Value &)> evictionCallback;
};
