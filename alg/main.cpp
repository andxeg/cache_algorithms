#include "arccache.h"
#include "fifocache.h"
#include "lfu.h"
#include "lru.h"
#include "midpointlru.h"
#include "mqcache.h"
#include "snlru.h"
#include "twoqcache.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <vector>


typedef std::unordered_map<std::string, size_t> ContentSizes;

template <typename Cache>
bool canAppendIdToWarmUpItems(  std::unordered_set<std::string> & warmUpItems,
                                std::string id,
                                size_t & cacheSize,
                                Cache & cache)
{
    
    ContentSizes contentSizes = cache.getContentSizes();

    bool result = true;
    size_t warmUpItemsSize = 0;
    size_t idSize = contentSizes[id];

    for (auto& warmUpItem : warmUpItems) {
        warmUpItemsSize += contentSizes[warmUpItem];
    }

    if ((warmUpItemsSize + idSize) > cacheSize) {
        result = false;
    }

    return result;
}


template <typename Cache>
int test(size_t cacheSize, const std::string& fileName) {
    size_t missed = 0;
    size_t count = 0;
    size_t putCount = 0;

    Cache cache(cacheSize);

    std::unordered_set<std::string> warmUpItems;

    std::unordered_set<std::string> unusedItems;
    std::unordered_set<std::string> evictedItems;
    std::unordered_map<std::string, size_t> addedTime;
    std::vector<size_t> holdTime;
    size_t falseEvicted = 0;

    cache.setEvictionCallback([&](const std::string &key, const std::string &value) {
        unusedItems.erase(key);

        if (warmUpItems.size() < cacheSize) {
            return;
        }

        evictedItems.insert(key);

        auto addedTimeIt = addedTime.find(key);
        if (addedTimeIt != addedTime.end()) {
            holdTime.push_back(putCount - addedTimeIt->second);
            addedTime.erase(addedTimeIt);
        }
    });

    std::ifstream in(fileName);
    while (true) {
        std::string id;
        size_t size;

        in >> id;
        in >> size;

        if (in.eof()) {
            break;
        }

        cache.addCidSize(id, size);

        // canAppendIdToWarmUpItems<Cache>(warmUpItems, id, cacheSize, cache);


        if (warmUpItems.size() < cacheSize) {
            warmUpItems.insert(id);
            unusedItems.insert(id);
            cache.put(id, id);
        } else {
            const std::string *value = cache.find(id);

            if (evictedItems.find(id) != evictedItems.end()) {
                ++falseEvicted;
                evictedItems.erase(id);

                assert(value == nullptr);
            }

            if (value) {
                unusedItems.erase(id);

                auto addedTimeIt = addedTime.find(id);
                if (addedTimeIt != addedTime.end()) {
                    addedTime.erase(addedTimeIt);
                }
            } else {
                ++missed;

                unusedItems.insert(id);

                value = cache.put(id, id);
                addedTime[id] = putCount;
                ++putCount;
            }

            if (*value != id) {
                std::cout << "Invalid value " << *value << " " << id << "\n";
                return 1;
            }

            ++count;
        }
    }

    size_t medianHoldTime = 0;

    if (!holdTime.empty()) {
        auto middleIt = holdTime.begin() + holdTime.size() / 2;
        std::nth_element(holdTime.begin(), middleIt, holdTime.end());
        medianHoldTime = *middleIt;
    }

/*    std::cout << "Unused items count - " << unusedItems.size() << "\n";
    std::cout << "False evicted count - " << falseEvicted << "\n";
    std::cout << "Median hold time - " << medianHoldTime << "\n";
    std::cout << "Cached items count - " << cache.size() << "\n";
    std::cout << "Requests - " << count << "\n";
    std::cout << "Misses - " << missed << "\n";
    std::cout << "Hit rate - " << 100 * (count - missed) / float(count) << "\n";
*/
    std::cout << cache.size() << ' ' <<
    (count != 0 ? 100 * (count - missed) / float(count) : -1) << std::endl;

    return 0;
}

int main(int argc, const char* argv[]) {
    std::string cacheType = argv[1];
    std::string::size_type sz = 0;
    size_t cacheSize = std::stoll(std::string(argv[2]), &sz, 0);

    std::string fileName = argv[3];

    if (cacheType == "mid") {
        return test<MidPointLRUCache<std::string, std::string>>(cacheSize, fileName);
    }

    if (cacheType == "lru") {
        return test<LRUCache<std::string, std::string>>(cacheSize, fileName);
    }

    if (cacheType == "lfu") {
        return test<LFUCache<std::string, std::string>>(cacheSize, fileName);
    }

    if (cacheType == "2q") {
        return test<TwoQCache<std::string, std::string>>(cacheSize, fileName);
    }

    if (cacheType == "s4lru") {
        return test<SNLRUCache<std::string, std::string>>(cacheSize, fileName);
    }

    if (cacheType == "fifo") {
        return test<FifoCache<std::string, std::string>>(cacheSize, fileName);
    }

    if (cacheType == "mq") {
        return test<MQCache<std::string, std::string>>(cacheSize, fileName);
    }

    if (cacheType == "arc") {
        return test<ARCCache<std::string, std::string>>(cacheSize, fileName);
    }

    std::cout << "Unknown cache type " << cacheType << "\n";

    return 1;
}
