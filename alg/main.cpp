#include "arccache.h"
#include "fifocache.h"
#include "lfu.h"
#include "lru.h"
#include "midpointlru.h"
#include "mqcache.h"
#include "snlru.h"
#include "twoqcache.h"

#include "pop_caching.h"

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


void printContentSizes(ContentSizes & contentSizes) {
    for (auto& element : contentSizes) 
        std::cout << element.first << ' ' << element.second << std::endl;
}


void printWarmUpItems(std::unordered_set<std::string> warmUpItems) {
    for (auto& element : warmUpItems)
        std::cout << element << std::endl;
}

template <typename Cache>
int test(size_t cacheSize, const std::string& fileName, 
        const size_t & learn_limit = 100, const size_t & period = 1000)
{
    size_t missed = 0;
    size_t count = 0;

    Cache cache(cacheSize, learn_limit, period);

    std::string message = "Start algo.\0";
    struct tm * now = print_current_data_and_time(message);
    int hour_start = now->tm_hour;
    int min_start = now->tm_min;
    int sec_start = now->tm_sec;

    message = "After cache initialization.\0";
    now = print_current_data_and_time(message);

    std::ifstream in(fileName);
    while (true) {
        size_t access_time;
        std::string id;
        size_t size;
        

        in >> access_time;
        in >> id;
        in >> size;


        if (in.eof()) {
            break;
        }
        
        // size = 1; //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        cache.addCidSize(id, size);
        
        // std::cout << "Current cache size put -> " << cache.getCacheSize() << std::endl;
        const std::string *value = cache.find(id, access_time);

        if (!value) {
            ++missed;
            value = cache.put(id, id);
        }

        ++count;

        if (count % 1000 == 0) {
          std::cerr << "Process " << count << "\n";
        }

    }

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
    std::cout << "Cycle -> " << count << std::endl;
    std::cout << "Miss Count -> " << missed << std::endl;
    std::cout << "Cache size -> " << cache.getCacheSize() << std::endl;
    std::cout << "Cache size -> " << cache.elementsCount() << std::endl;

    return 0;
}

int main(int argc, const char* argv[]) {
    std::string cacheType = argv[1];
    std::string::size_type sz = 0;
    size_t cacheSize = std::stoll(std::string(argv[2]), &sz, 0);

    size_t learn_limit = std::stoll(std::string(argv[3]), &sz, 0);
    size_t period = std::stoll(std::string(argv[4]), &sz, 0);

    std::string fileName = argv[5];

    if (cacheType == "mid") {
        return test<MidPointLRUCache<std::string, std::string>>(cacheSize, fileName);
    }

    if (cacheType == "lru") {
        return test<LRUCache<std::string, std::string>>(cacheSize, fileName);
    }

    if (cacheType == "pop_caching") {
        return test<PoPCaching>(cacheSize, fileName, learn_limit, period);
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
