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
        in >> id;

        if (in.eof()) {
            break;
        }

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

    std::cerr << "Unused items count - " << unusedItems.size() << "\n";
    std::cerr << "False evicted count - " << falseEvicted << "\n";
    std::cerr << "Median hold time - " << medianHoldTime << "\n";
    std::cerr << "Cached items count - " << cache.size() << "\n";
    std::cerr << "Requests - " << count << "\n";
    std::cerr << "Misses - " << missed << "\n";
    std::cerr << "Hit rate - " << 100 * (count - missed) / float(count) << "\n";

    return 0;
}

int main(int argc, const char* argv[]) {
    std::string cacheType = argv[1];
    size_t cacheSize = atoi(argv[2]);
    std::string fileName = argv[3];

    if (cacheType == "mid") {
        std::cout << "Mid point LRU cache\n";
        return test<MidPointLRUCache<std::string, std::string>>(cacheSize, fileName);
    }

    if (cacheType == "lru") {
        std::cout << "LRU cache\n";
        return test<LRUCache<std::string, std::string>>(cacheSize, fileName);
    }

    if (cacheType == "lfu") {
        std::cout << "LFU cache\n";
        return test<LFUCache<std::string, std::string>>(cacheSize, fileName);
    }

    if (cacheType == "2q") {
        std::cout << "2 queue cache\n";
        return test<TwoQCache<std::string, std::string>>(cacheSize, fileName);
    }

    if (cacheType == "s4lru") {
        std::cout << "S4LRU cache\n";
        return test<SNLRUCache<std::string, std::string>>(cacheSize, fileName);
    }

    if (cacheType == "fifo") {
        std::cout << "FIFO cache\n";
        return test<FifoCache<std::string, std::string>>(cacheSize, fileName);
    }

    if (cacheType == "mq") {
        std::cout << "MQ cache\n";
        return test<MQCache<std::string, std::string>>(cacheSize, fileName);
    }

    if (cacheType == "arc") {
        std::cout << "ARC cache\n";
        return test<ARCCache<std::string, std::string>>(cacheSize, fileName);
    }

    std::cout << "Unknown cache type " << cacheType << "\n";

    return 1;
}
