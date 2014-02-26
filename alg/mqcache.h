#pragma once

#include "lru.h"
#include "fifocache.h"

#include <cmath>
#include <map>
#include <vector>

template <typename Key, typename Value>
class MQCache {
    struct ValueHolder {
        ValueHolder() :
                expireTime(0),
                reqTime(0) {}

        ValueHolder(const Value &v, size_t expTime, size_t rTime) :
                value(v),
                expireTime(expTime),
                reqTime(rTime) {}

        Value value;
        size_t expireTime;
        size_t reqTime;
    };

    static constexpr size_t DEF_LRU_COUNT = 8;
    static constexpr size_t OUT_SIZE_MUL  = 4;

public:
    explicit MQCache(size_t size, size_t lruCount = DEF_LRU_COUNT) :
            cacheSize(size < lruCount ? lruCount : size),
            expireTime(cacheSize),
            currentTime(0),
            out(cacheSize * OUT_SIZE_MUL) {
        for (size_t index = 0; index < lruCount; ++index) {
            lruList.push_back(LRUCache<Key, ValueHolder>(cacheSize));
        }
    }

    Value* find(const Key &key) {
        ValueHolder *valueHolder = nullptr;

        checkFrequentExpariation();

        for (size_t index = 0; index < lruList.size() - 1; ++index) {
            valueHolder = lruList[index].find(key);
            if (valueHolder) {
                ++currentTime;

                valueHolder = lruList[index + 1].put(key, *valueHolder);
                lruList[index].erase(key);

                updateExpireTime(valueHolder->reqTime, index);

                valueHolder->expireTime = currentTime + expireTime;
                valueHolder->reqTime = currentTime;

                assert(size() <= cacheSize);

                return &valueHolder->value;
            }
        }

        valueHolder = lruList.back().find(key);
        if (valueHolder) {
            ++currentTime;

            updateExpireTime(valueHolder->reqTime, lruList.size() - 1);

            valueHolder->expireTime = currentTime + expireTime;
            valueHolder->reqTime = currentTime;

            return &valueHolder->value;
        }

        return nullptr;
    }

    Value* put(const Key &key, const Value &value) {
        Value *result = find(key);
        if (result) {
            return result;
        }

        ++currentTime;

        auto *outValue = out.find(key);

        if (outValue) {
            size_t lruIndex = getLruIndex(outValue->first + 1);

            updateExpireTime(outValue->first, outValue->second);

            out.erase(key);

            return putToLru(key, value, lruIndex);
        }

        makeSizeInvariant(cacheSize - 1);
        assert(size() <= cacheSize - 1);

        return &lruList.front().put(key, ValueHolder(value, currentTime + expireTime, currentTime))->value;
    }

    bool erase(const Key &key) {
        for (auto& lru : lruList) {
            if (lru.erase(key)) {
                return true;
            }
        }

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
        evictionCallback = callback;
    }

private:
    void checkFrequentExpariation() {
        for (int i = lruList.size() - 1; i > 0; --i) {
            if (lruList[i].size() == 0) {
                continue;
            }

            auto* headItem = lruList[i].head();
            if (headItem->second.expireTime < currentTime) {
                lruList[i - 1].put(headItem->first, ValueHolder(headItem->second.value, currentTime + expireTime, currentTime));
                lruList[i].erase(headItem->first);
            }
        }

        assert(size() <= cacheSize);
    }

    void makeSizeInvariant(size_t newSize) {
        if (size() <= newSize) {
            return;
        }

        size_t lruIndex = 0;
        size_t itemsToRemove = size() - newSize;
        for (auto& lru : lruList) {
            while (lru.size() > 0 && itemsToRemove > 0) {
                out.put(lru.tail()->first, std::make_pair(lruIndex, lru.tail()->second.reqTime));

                if (evictionCallback) {
                    evictionCallback(lru.tail()->first, lru.tail()->second.value);
                }

                Key key = lru.tail()->first;

                lru.erase(lru.tail()->first);
                --itemsToRemove;
            }

            ++lruIndex;

            if (itemsToRemove == 0) {
                return;
            }
        }
    }

    Value *putToLru(const Key &key, const Value &value, size_t index) {
        makeSizeInvariant(cacheSize - 1);
        assert(size() <= cacheSize - 1);

        return &lruList[index].put(key, ValueHolder(value, currentTime + expireTime, currentTime))->value;
    }

    size_t getLruIndex(size_t reqs) {
        return std::min(reqs, lruList.size() - 1);
    }

    void updateExpireTime(size_t itemReqTime, size_t lruIndex) {
        size_t tempDist = currentTime - itemReqTime;

        size_t pow2Dist = pow(2, ceil(log(tempDist)/log(2)));
        ++temporalDistsMap[pow2Dist];

        size_t reqCount = 0;
        for (auto dist : temporalDistsMap) {
            if (reqCount < dist.second) {
                reqCount = dist.second;
                expireTime = dist.first;
            }
        }
    }

private:
    size_t cacheSize;
    size_t expireTime;
    size_t currentTime;

    std::vector<LRUCache<Key, ValueHolder>> lruList;

    std::map<size_t, size_t> temporalDistsMap;

    FifoCache<Key, std::pair<size_t, size_t>> out;

    std::function<void(const Key &,const Value &)> evictionCallback;
};
