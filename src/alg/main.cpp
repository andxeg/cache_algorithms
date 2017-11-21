#include "lfu.h"
#include "lru.h"
#include "lru_K.h"
#include "snlru.h"
#include "fifocache.h"
#include "twoqcache.h"
#include "midpointlru.h"
#include "pop_caching.h"
#include "timestamps.h"


#include "arccache.h"
#include "mqcache.h"

#include <cmath>
#include <iomanip>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <unordered_set>


#define PERIOD_SIZE 86400

struct PeriodStat;
typedef std::vector<PeriodStat> PeriodsStatistics;
typedef std::unordered_map<std::string, size_t> ContentSizes;


struct PeriodStat {
    size_t start, end;
    size_t hit, requests;
    size_t hit_bytes, requests_bytes;
    PeriodStat() : 
        start(0), end(0), 
        hit(0), requests(0),
        hit_bytes(0), requests_bytes(0) {}

    size_t duration() const {
        return end - start;
    }

    float object_hit_rate() const {
        return (requests != 0) ? static_cast<float>(hit) / requests : 0.0;
    }

    float byte_hit_rate() const {
        return (requests_bytes != 0) ? static_cast<float>(hit_bytes) / requests_bytes : 0.0;
    }
};

struct TotalStat {
    size_t hit, requests;
    size_t hit_bytes, requests_bytes;

    TotalStat() :
        hit(0), requests(0),
        hit_bytes(0), requests_bytes(0) {}

    float object_hit_rate() const {
        return (requests != 0) ? static_cast<float>(hit) / requests : -1.0;
    }

    float byte_hit_rate() const {
        return (requests_bytes != 0) ? static_cast<float>(hit_bytes) / requests_bytes : -1.0;
    }

    float average_object_hit_rate(const PeriodsStatistics & stats) const {
        float sum_hit_rate = 0.0;
        for (auto & period : stats)
            sum_hit_rate += period.object_hit_rate();

        return (stats.size() != 0) ? sum_hit_rate / stats.size() : -1.0;
    }

    float average_byte_hit_rate(const PeriodsStatistics & stats) const {
        float sum_hit_rate = 0.0;
        for (auto & period : stats)
            sum_hit_rate += period.byte_hit_rate();            
        return (stats.size() != 0) ? sum_hit_rate / stats.size() : -1.0;
    }
};

template <typename Cache>
void print_algorithm_results(const TotalStat & total_stat, 
    const PeriodsStatistics & periods_stat, Cache & cache, const size_t & cache_size)
{
    std::cout << "Algorithm results:" << std::endl;
    std::cout << "Cache size " << cache_size << " Kbytes" << std::endl;
    std::cout << "Total Object Hit Rate " << total_stat.object_hit_rate() << std::endl;
    std::cout << "Total Byte Hit Rate " << total_stat.byte_hit_rate() << std::endl;
    std::cout << "Average Object Hit Rate " << total_stat.average_object_hit_rate(periods_stat) << std::endl;
    std::cout << "Average Byte Hit Rate " << total_stat.average_byte_hit_rate(periods_stat) << std::endl;
    std::cout << "Cache status:" << std::endl;
    std::cout << "Total Requests " << total_stat.requests << std::endl;
    std::cout << "Cache size (in objects) " << cache.getCacheSize() << std::endl;
    std::cout << "Cache size (in Kbytes) " << cache.elementsCount() << std::endl;
    std::cout << std::endl; std::cout << std::endl; std::cout << std::endl;

    std::cout << "Statistics for " << periods_stat.size() << " days" << std::endl;
    for (auto & period : periods_stat) {
        std::cout << "Start " << period.start << " End " << period.end << " Duration " << period.duration() << std::endl;
        std::cout << "Hit " << period.hit << " Requests " << period.requests << std::endl;
        std::cout << "Hit bytes " << period.hit_bytes << " Requests bytes " << period.requests_bytes << std::endl;
        std::cout << "OHR " << period.object_hit_rate() << " BHR " << period.byte_hit_rate() << std::endl;
    }

    std::cout << std::endl; std::cout << std::endl; std::cout << std::endl;
}


template <typename Cache>
int test(size_t cacheSize, const std::string& fileName, 
        const size_t & learn_limit = 100, const size_t & period = 1000)
{
    TotalStat total_stat;
    PeriodsStatistics periods_stat;
    periods_stat.push_back(PeriodStat());
    Cache cache(cacheSize, learn_limit, period);
    
    struct tm * now = print_current_data_and_time("Start algo.");
    int hour_start = now->tm_hour;
    int min_start = now->tm_min;
    int sec_start = now->tm_sec;

    print_current_data_and_time("After cache initialization.");

    std::ifstream in(fileName);
    
    std::string id;
    size_t access_time, size;
    in >> access_time >> id >> size;
    size_t prev_period_end = access_time;
    periods_stat.back().start = access_time;

    while (true) {
        
        if (in.eof()) {break;}

        if ((access_time - prev_period_end) >= PERIOD_SIZE) {
            periods_stat.back().end = access_time;
            periods_stat.push_back(PeriodStat());
            periods_stat.back().start = access_time;
            prev_period_end = access_time;
        }

        cache.addCidSize(id, size);
        
        if (cache.find(id, access_time) == nullptr) {
            cache.put(id, id, access_time);
        } else {
            total_stat.hit += 1;
            total_stat.hit_bytes += size;
            periods_stat.back().hit += 1;
            periods_stat.back().hit_bytes += size;
        }

        total_stat.requests += 1;
        total_stat.requests_bytes += size;
        periods_stat.back().requests += 1;
        periods_stat.back().requests_bytes += size;

        if (total_stat.requests % 1000 == 0) {
          std::cerr << "Process " << total_stat.requests << std::endl;;
        }

        in >> access_time >> id >> size;
    }

    now = print_current_data_and_time("Algorithm was finished.");
    int time =  now->tm_hour * 3600 + now->tm_min * 60 + now->tm_sec - 
                hour_start * 3600 - min_start * 60 - sec_start;
    std::cout   << "Algorithm work time -> " 
                <<  time / 3600  << "  hours " 
                << (time % 3600) / 60  << " mins " 
                << (time % 3600) % 60 << " secs" << std::endl;

    print_algorithm_results<Cache>(total_stat, periods_stat, cache, cacheSize);
    return 0;
}

int main(int argc, const char* argv[]) {
    if (argc != 6) {
        std::cerr << "Error in input paramters" << std::endl;
        return -1;
    }

    // TODO add config file and put all parameters in it

    std::string cacheType = argv[1];
    std::string::size_type sz = 0;
    size_t cacheSize = std::stoll(std::string(argv[2]), &sz, 0);

    // Parameters for PoPCaching and LRU_K
    size_t learn_limit = std::stoll(std::string(argv[3]), &sz, 0);
    size_t period = std::stoll(std::string(argv[4]), &sz, 0);

    std::string fileName = argv[5];

    if (cacheType == "mid") {
        return test<MidPointLRUCache<std::string, std::string>>(cacheSize, fileName);
    }

    if (cacheType == "lru") {
        return test<LRUCache<std::string, std::string>>(cacheSize, fileName);
    }

    if (cacheType == "lru_k") {
        return test<LRU_K_Cache<std::string, std::string>>(cacheSize, fileName, learn_limit, period);
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
