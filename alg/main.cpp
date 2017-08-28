#include "arccache.h"
#include "fifocache.h"
#include "lfu.h"
#include "lru.h"
#include "midpointlru.h"
#include "mqcache.h"
#include "snlru.h"
#include "twoqcache.h"

#include <cmath>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <unordered_set>

#include "timestamps.h"


typedef std::unordered_map<std::string, size_t> ContentSizes;


template <typename T>
T convertFromStringTo(std::string str) {
    T val;
    std::stringstream stream(str);
    stream >> val;
    return val;
}


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
    size_t cyclesCount = 0;

    Cache cache(cacheSize);

    std::unordered_set<std::string> warmUpItems;

    std::unordered_set<std::string> unusedItems;
    std::unordered_set<std::string> evictedItems;
    std::unordered_map<std::string, size_t> addedTime;
    std::vector<size_t> holdTime;
    size_t falseEvicted = 0;
    std::string message = "Start algo.\0";
    struct tm * now = print_current_data_and_time(message);
    int hour_start = now->tm_hour;
    int min_start = now->tm_min;
    int sec_start = now->tm_sec;

    cache.setEvictionCallback([&](const std::string &key, const std::string &value) {
        unusedItems.erase(key);

        // calculate warmUpItems size 
        size_t warmUpItemsSize = 0;
        ContentSizes contentSizes = cache.getContentSizes();
        for (auto&warmUpItem : warmUpItems ) {
            warmUpItemsSize += contentSizes[warmUpItem];
        }

        if (warmUpItemsSize < cacheSize) {
            return;
        }

        evictedItems.insert(key);

        auto addedTimeIt = addedTime.find(key);
        if (addedTimeIt != addedTime.end()) {
            holdTime.push_back(putCount - addedTimeIt->second);
            addedTime.erase(addedTimeIt);
        }
    });



    message = "After cache initialization.\0";
    now = print_current_data_and_time(message);

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

        if (canAppendIdToWarmUpItems<Cache>(warmUpItems, id, cacheSize, cache)) {
            warmUpItems.insert(id);
            unusedItems.insert(id);
            cache.put(id, id);
            ++cyclesCount;
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

            if (count % 1000 == 0) {
              std::cout << "Process " << count << "\n";
            }

            ++cyclesCount;
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

    message = "Algorithm was finished.\0";
    now = print_current_data_and_time(message);
    int time = now->tm_hour * 3600 + now->tm_min * 60 + now->tm_sec - hour_start * 3600 - min_start * 60 - sec_start;
    std::cout << "Algorithm work time -> " <<  time / 3600  << "  hours " << (time % 3600) / 60  << " mins " << (time % 3600) % 60 << " secs" << std::endl;

    std::cout << "\nAlgorithm results:\n";
    std::cout << "Cache size -> " << cacheSize << " Kbyte" << std::endl;
    std::cout << "Hit-rate -> " << 
                (count != 0 ? 100 * (count - missed) / float(count) : -1)  << std::endl;

    std::cout << std::endl;

    std::cout << "More precisely:" << std::endl;
    std::cout << "Cycle -> " << cyclesCount << std::endl;
    std::cout << "Requests without warming -> " << count << std::endl;
    std::cout << "Miss Count -> " << missed << std::endl;

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
