#include "lfu.h"
#include "lru.h"
#include "lru_K.h"
#include "snlru.h"
#include "fifocache.h"
#include "twoqcache.h"
#include "midpointlru.h"
#include "pop_caching.h"
#include "timestamps.h"

#include "defs.h"
#include "config.h"
#include "history_manager.h"
#include "pre_push.h"
#include "size_filter.h"


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
#include <unordered_map>


struct PeriodStat;
struct TotalStat;
typedef long long PoPId;
typedef long long PoPSize;
typedef std::vector<PoPId> VecPoP;
typedef std::vector<PoPSize> VecPoPSize;
typedef std::vector<PeriodStat> PeriodsStatistics;
typedef std::unordered_map<std::string, size_t> ContentSizes;
typedef std::unordered_map<PoPId, PeriodsStatistics> PIDsPeriodStatistics;
typedef std::unordered_map<PoPId, TotalStat> PIDsTotalStats;
typedef std::unordered_map<PoPId, HistoryManager> PIDsHistoryManagers;
typedef std::unordered_map<PoPId, SizeFilter> PIDsSizeFilters;
typedef std::unordered_map<PoPId, size_t> PIDsPeriodEnds;


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
void print_algorithm_results(PIDsTotalStats &pids_total_statistics, 
                             PIDsPeriodStatistics &pids_period_statistics, 
                             std::unordered_map<PoPId, Cache> &pids_caches, 
                             VecPoP &pids,
                             VecPoPSize &pids_sizes)
{
    std::cout << "Algorithm results:" << std::endl;

    for (size_t i = 0; i < pids.size(); ++i) {
        PoPId pid = pids[i];
        PoPSize pid_size = pids_sizes[i];
        std::cout << "PID: " << pid << " PID size: " << pid_size << std::endl;
        Cache cache = pids_caches[pid];
        std::cout << "Cache size " 
                  << pid_size * 1024 * 1024 
                  << " Kbytes" 
                  << std::endl;

        std::cout << "Total Object Hit Rate " 
                  << pids_total_statistics[pid].object_hit_rate()
                  << std::endl;

        std::cout << "Total Byte Hit Rate "
                  << pids_total_statistics[pid].byte_hit_rate() 
                  << std::endl;

        std::cout << "Average Object Hit Rate " <<
                  pids_total_statistics[pid].average_object_hit_rate(pids_period_statistics[pid])
                  << std::endl;

        std::cout << "Average Byte Hit Rate " 
                  << pids_total_statistics[pid].average_byte_hit_rate(pids_period_statistics[pid])
                  << std::endl;

        std::cout << "Cache status:" << std::endl;
        std::cout << "Total Requests " << pids_total_statistics[pid].requests << std::endl;
        std::cout << "Cache size (in Kbytes) "  << cache.getCacheSize() << std::endl;
        std::cout << "Cache size (in objects) " << cache.elementsCount() << std::endl;
        std::cout << std::endl; std::cout << std::endl; std::cout << std::endl;


        std::cout << "Statistics for " << pids_period_statistics[pid].size() << " days" << std::endl;

        for (auto &period : pids_period_statistics[pid]) {
            std::cout << "Start " << period.start << " End " << period.end
                      << " Duration " << period.duration()
                      << std::endl;

            std::cout << "Hit " << period.hit 
                      << " Requests " << period.requests 
                      << std::endl;

            std::cout << "Hit bytes " << period.hit_bytes
                      << " Requests bytes " << period.requests_bytes 
                      << std::endl;

            std::cout << "OHR " << period.object_hit_rate()
                      << " BHR " << period.byte_hit_rate()
                      << std::endl;
        }
        std::cout << std::endl; std::cout << std::endl; std::cout << std::endl;
    }

    std::cout << std::endl; std::cout << std::endl; std::cout << std::endl;
}

template<typename Cache>
void read_cids_size(Cache &cache, const std::string &filename) {
    std::fstream input(filename);
    if (!input.is_open()) {
        std::cerr << "[ERROR] Error while opening file " 
                    << filename << std::endl;
        return;
    }
    std::string id;
    size_t access_time;
    PoPId pid;
    size_t size = 0;
    while (true) {
        input >> access_time >> pid >> id >> size;
        if (input.eof() && size == 0) break;
        cache.addCidSize(id, size);
        size = 0;
    }
    input.close();
}

template<typename Cache>
void make_pre_push(Cache &cache, PrePush &pre_push,
                   HistoryManager &history_manager, 
                   SizeFilter &size_filter,
                   Config &config) {
    float cache_hot_content = config.get_float_by_name("CACHE_HOT_CONTENT");

    /* take hot content from cache */
    VecStr hot_content = cache.get_hot_content(cache_hot_content);

    hot_content = pre_push.get_pre_push_list(hot_content, history_manager);

    /* add hot_content to cache */
    /* hot_content may contains elements which are already in cache */
    ContentSizes contentSizes = cache.getContentSizes();
    int count  = 0;
    float size = 0.0;
    for (auto &content : hot_content) {
        if (cache.find(content) == nullptr) {
            ++count;
            size = (float)contentSizes[content];
            if (size_filter.admit_object(content, size, history_manager) == true)
                cache.put(content, content);
        }
    }

    print_current_data_and_time(std::string("Pre Push added: ") +
                                ToString<size_t>(hot_content.size()) +
                                std::string(", new elements: ") +
                                ToString<int>(count));
}


template <typename Cache>
int test(size_t cacheSize, const std::string& filename, Config &config,
         const size_t & learn_limit = 100, const size_t & period = 1000)
{
    /* Point of Presence (PoP) identifiers */
    VecPoP pids       = config.get_vector_by_name<PoPId>("PIDS");
    VecPoPSize pids_sizes = config.get_vector_by_name<PoPSize>("PIDS_CACHE_SIZES");

    /* create all data structures for each PoP */
    PIDsPeriodStatistics pids_period_statistics;
    PIDsTotalStats pids_total_statistics;
    PIDsHistoryManagers pids_history_managers;
    PIDsSizeFilters pids_size_filters;
    std::unordered_map<PoPId, Cache> pids_caches;

    for (size_t i = 0; i < pids.size(); ++i) {
        PoPId pop_id = pids[i];
        PoPSize pid_size = pids_sizes[i];
        pids_period_statistics[pop_id] = PeriodsStatistics();
        pids_period_statistics[pop_id].push_back(PeriodStat());
        pids_total_statistics[pop_id]  = TotalStat();
        pids_history_managers[pop_id]  = HistoryManager(config);
        pids_size_filters[pop_id] = SizeFilter(config);
        pids_caches[pop_id] = Cache(pid_size * 1024 * 1024, learn_limit, period);
        pids_caches[pop_id].prepare_cache();
    }

    size_t period_size = config.get_int_by_name("STAT_PERIOD_SIZE");
    size_t start_pre_push = config.get_int_by_name("START_PRE_PUSH");
    
    PrePush pre_push(config);
    std::cout << "Total PIDs count: " << pids_caches.size() << std::endl;
    for (size_t i = 0; i < pids.size(); ++i) {
        std::cout << "Pid: " << pids[i] << " with cache size: " << pids_caches[pids[i]].size() << std::endl;
    }
    
    /* read cids sizes for each caches */
    print_current_data_and_time("Start read cids sizes.");
    read_cids_size<Cache>(pids_caches[pids[0]], filename);
    ContentSizes contentSizes = pids_caches[pids[0]].getContentSizes();
    for (size_t i = 1; i < pids.size(); ++i) {
        PoPId pid = pids[i];
        for (auto &elem : contentSizes) {
            pids_caches[pid].addCidSize(elem.first, elem.second);
        }
    }
    print_current_data_and_time("After cache initialization.");

    // std::cout << "config start" << std::endl;
    // config.print();
    // std::cout << "config end" << std::endl;
    
    struct tm * now = print_current_data_and_time("Start algo.");
    int hour_start = now->tm_hour;
    int min_start = now->tm_min;
    int sec_start = now->tm_sec;

    std::ifstream in(filename);
    
    std::string id;
    PoPId pid;
    size_t access_time, size;
    in >> access_time >> pid >> id >> size;

    /* create map with previous period ends */
    /* initialize periods statistics for each PoP */
    PIDsPeriodEnds prev_period_ends;
    for (size_t i = 0; i < pids.size(); ++i) {
        PoPId pop_id = pids[i];
        prev_period_ends[pop_id] = access_time;
        pids_period_statistics[pop_id].back().start = access_time;
    }

    while (true) {
        if (in.eof()) {break;}

        if ((access_time - prev_period_ends[pid]) >= period_size) {
            /* now start for pre_push and size_filter are the same */
            if (pids_period_statistics[pid].size() >= start_pre_push) {
                pids_size_filters[pid].update_threshold
                                       (pids_history_managers[pid], contentSizes);
                print_current_data_and_time(std::string("[SizeFilter] new threshold -> ") +
                    ToString<float>(pids_size_filters[pid].get_threshold()));

                make_pre_push(pids_caches[pid],
                              pre_push,
                              pids_history_managers[pid],
                              pids_size_filters[pid],
                              config);
            }
            pids_period_statistics[pid].back().end = access_time;
            pids_period_statistics[pid].push_back(PeriodStat());
            pids_period_statistics[pid].back().start = access_time;
            prev_period_ends[pid] = access_time;
            pids_history_managers[pid].start_new_period();
        }

        // pids_caches[pid].addCidSize(id, size);
        pids_history_managers[pid].update_object_history
                                   (id, pids_period_statistics[pid].size());
        
        if (pids_caches[pid].find(id, access_time) == nullptr) {
            if (pids_size_filters[pid].admit_object(id, (float)size, 
                                        pids_history_managers[pid]) == true) {
                pids_caches[pid].put(id, id, access_time);
            }
        } else {
            pids_total_statistics[pid].hit += 1;
            pids_total_statistics[pid].hit_bytes += size;
            pids_period_statistics[pid].back().hit += 1;
            pids_period_statistics[pid].back().hit_bytes += size;
        }

        pids_total_statistics[pid].requests += 1;
        pids_total_statistics[pid].requests_bytes += size;
        pids_period_statistics[pid].back().requests += 1;
        pids_period_statistics[pid].back().requests_bytes += size;

        if (pids_total_statistics[pid].requests % 1000 == 0) {
            std::cerr <<  "PID: " << 
                          pid << 
                          " Process " << 
                          pids_total_statistics[pid].requests 
                          << std::endl;
        }

        in >> access_time >> pid >> id >> size;
    }

    now = print_current_data_and_time("Algorithm was finished.");
    int time =  now->tm_hour * 3600 + now->tm_min * 60 + now->tm_sec - 
                hour_start * 3600 - min_start * 60 - sec_start;
    std::cout   << "Algorithm work time -> " 
                <<  time / 3600  << "  hours " 
                << (time % 3600) / 60  << " mins " 
                << (time % 3600) % 60 << " secs" << std::endl;

    print_algorithm_results<Cache>(pids_total_statistics, 
                                   pids_period_statistics,
                                   pids_caches,
                                   pids, 
                                   pids_sizes);
    
    return 0;
}

int main(int argc, const char* argv[]) {
    if (argc != 7) {
        std::cerr << "Error in input parameters" << std::endl;
        return -1;
    }

    Config config = Config(std::string(argv[6])); // OK
    // Config config(std::string(argv[6])); // error

    std::string cacheType = argv[1];
    std::string::size_type sz = 0;
    size_t cacheSize = std::stoll(std::string(argv[2]), &sz, 0);

    // Parameters for PoPCaching and LRU_K
    size_t learn_limit = std::stoll(std::string(argv[3]), &sz, 0);
    size_t period = std::stoll(std::string(argv[4]), &sz, 0);

    std::string filename = argv[5];

    if (cacheType == "mid") {
        return test<MidPointLRUCache<std::string, std::string>>(cacheSize, filename, config);
    }

    if (cacheType == "lru") {
        return test<LRUCache<std::string, std::string>>(cacheSize, filename, config);
    }

    if (cacheType == "lru_k") {
        return test<LRU_K_Cache<std::string, std::string>>
                (cacheSize, filename, config);
    }    

    if (cacheType == "pop_caching") {
        return test<PoPCaching>(cacheSize, filename, config, learn_limit, period);
    }

    if (cacheType == "lfu") {
        return test<LFUCache<std::string, std::string>>(cacheSize, filename, config);
    }

    if (cacheType == "2q") {
        return test<TwoQCache<std::string, std::string>>(cacheSize, filename, config);
    }

    if (cacheType == "s4lru") {
        return test<SNLRUCache<std::string, std::string>>(cacheSize, filename, config);
    }

    if (cacheType == "fifo") {
        return test<FifoCache<std::string, std::string>>(cacheSize, filename, config);
    }

    if (cacheType == "mq") {
        return test<MQCache<std::string, std::string>>(cacheSize, filename, config);
    }

    if (cacheType == "arc") {
        return test<ARCCache<std::string, std::string>>(cacheSize, filename, config);
    }

    std::cout << "Unknown cache type " << cacheType << "\n";

    return 1;
}
