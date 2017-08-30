#pragma once

#include "lru.h"
#include "fifocache.h"

#include <cmath>
#include <map>
#include <vector>

template <typename Key, typename Value>
class MQCache {
    typedef std::unordered_map<std::string, size_t> ContentSizes;

    struct ValueHolder {
        ValueHolder() :
                expireTime(0),
                reqTime(0),
                reqs(0) {}

        ValueHolder(const Value &v, size_t expTime, size_t rTime, size_t rs) :
                value(v),
                expireTime(expTime),
                reqTime(rTime),
                reqs(rs) {}

        Value value;
        size_t expireTime;
        size_t reqTime;
        size_t reqs;
    };

    static constexpr size_t DEF_LRU_COUNT = 8;
    static constexpr size_t OUT_SIZE_MUL  = 4;

public:
    explicit MQCache(size_t size, size_t lruCount = DEF_LRU_COUNT) :
            cacheSize(size < lruCount ? lruCount : size),
            expireTime(cacheSize * 10),
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

                updateExpireTime(valueHolder->reqTime);

                valueHolder->expireTime = currentTime + expireTime;
                valueHolder->reqTime = currentTime;
                ++valueHolder->reqs;

                size_t lruIndex = index + 1;
                if (lruIndex != index) {
                    valueHolder = lruList[lruIndex].put(key, *valueHolder);
                    lruList[index].erase(key);

                    assert(size() <= cacheSize);
                }

                return &valueHolder->value;
            }
        }

        valueHolder = lruList.back().find(key);
        if (valueHolder) {
            ++currentTime;

            updateExpireTime(valueHolder->reqTime);

            valueHolder->expireTime = currentTime + expireTime;
            valueHolder->reqTime = currentTime;
            ++valueHolder->reqs;

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

        makeSizeInvariant(cacheSize - 1);
        assert(size() <= cacheSize - 1);

        auto *outValue = out.find(key);

        if (outValue) {
            size_t reqs = outValue->first + 1;
            size_t reqTime = outValue->second;
            out.erase(key);

            size_t lruIndex = getLruIndex(reqs);
            updateExpireTime(reqTime);

            out.erase(key);

            return &lruList[lruIndex].put(key, ValueHolder(value, currentTime + expireTime, currentTime, reqs))->value;
        }

        return &lruList.front().put(key, ValueHolder(value, currentTime + expireTime, currentTime, 0))->value;
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

    size_t elementsCount() const {
        return 0;
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

private:
    void checkFrequentExpariation() {
        for (int i = lruList.size() - 1; i > 0; --i) {
            if (lruList[i].size() == 0) {
                continue;
            }

            auto* item = lruList[i].lruItem();
            if (item->second.expireTime < currentTime) {
                lruList[i - 1].put(item->first, ValueHolder(item->second.value, currentTime + expireTime, currentTime, item->second.reqs));
                lruList[i].erase(item->first);
            }
        }

        assert(size() <= cacheSize);
    }

    void makeSizeInvariant(size_t newSize) {
        if (size() <= newSize) {
            return;
        }

        size_t itemsToRemove = size() - newSize;
        for (auto& lru : lruList) {
            while (lru.size() > 0 && itemsToRemove > 0) {
                auto item = lru.lruItem();
                out.put(item->first, std::make_pair(item->second.reqs, item->second.reqTime));

                if (evictionCallback) {
                    evictionCallback(lru.lruItem()->first, lru.lruItem()->second.value);
                }

                lru.erase(lru.lruItem()->first);
                --itemsToRemove;
            }

            if (itemsToRemove == 0) {
                return;
            }
        }
    }

    size_t getLruIndex(size_t reqs) {
        return std::min(size_t(log(reqs)), lruList.size() - 1);
    }

    void updateExpireTime(size_t itemReqTime) {
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
    size_t currentCacheSize;
    size_t expireTime;
    size_t currentTime;

    std::vector<LRUCache<Key, ValueHolder>> lruList;

    std::map<size_t, size_t> temporalDistsMap;

    FifoCache<Key, std::pair<size_t, size_t>> out;

    std::function<void(const Key &,const Value &)> evictionCallback;

    ContentSizes contentSizes;
};
