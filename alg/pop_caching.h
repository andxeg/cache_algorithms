#pragma once

#include <set>
#include <list>
#include <cmath>
#include <queue>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <functional>
#include <unordered_map>
#include <unordered_set>


// algorithm parameters
#define LEARN_LIMIT 1000
#define PERIOD 10000
#define Z1 2
#define Z2 0.5

// other parameters
#define HOUR 3600
#define DAY 86400


template<typename T>
class custom_priority_queue : public std::priority_queue<T, std::vector<T>>
{
public:
    bool remove(const T& value) {
        auto it = std::find(this->c.begin(), this->c.end(), value);
        if (it != this->c.end()) {
            this->c.erase(it);
            std::make_heap(this->c.begin(), this->c.end(), this->comp);
            return true;
        } else {
            return false;
        }
    }
};


// context vector is the normalized access stat vector
typedef std::unordered_map<std::string, double> ContextVector;
typedef std::unordered_map<std::string, std::pair<double, double>> Bounds;

class HyperCube {
public:
    HyperCube(  const size_t & l, const size_t & c, 
                const size_t sum, const Bounds & b)
    {
        level = l;
        capacity = c;
        sum_of_request_rate = sum;
        bounds = b;
    }
    
    bool overflow(const double & z1, const double & z2) {
        return capacity >= z1 * pow(2, z2 * level);
    }
private:
    size_t level; 
    
    // count of requests in this hypercube
    size_t capacity; 

    size_t sum_of_request_rate;

    Bounds bounds;
};


class ContextSpace {
public:
    ContextSpace() {}

    ContextSpace(std::vector<std::string> time_features,
                const size_t & c1, const size_t & c2) :
        z1(c1), z2(c2)
    {
        Bounds bounds;
        for (auto & feature : time_features) {
            std::pair<double, double> edge;
            edge.first = 0.0;
            edge.second = 1.0;
            bounds[feature] = edge;
        }

        HyperCube hypercube(0, 1, 0, bounds);
        hypercubes.push_back(hypercube);
    }

    HyperCube find_hypercube(ContextVector & x) {
        // TODO
        Bounds bounds;
        HyperCube hypercube(0, 1, 0, bounds);
        return hypercube;
    }

    void split(HyperCube hypercube) {
        // TODO
    }

private:
    size_t z1;
    size_t z2;
    std::vector<HyperCube> hypercubes;
};


class Features {
    typedef std::unordered_map<std::string, size_t> CidLongLong;
public:
    Features() {}
    explicit Features(std::vector<std::string> time_features, 
                const size_t & first_access, const CidLongLong & periods)
    {
        total_requests = 0;
        this->first_access = first_access;
        for (auto & feature : time_features) {
            access_stat[feature] = 0;
            starts[feature] = first_access;
        }

        this->periods = periods;
    }

    void update_features(const size_t & current_time) {
        // TODO
    }

    bool popularity_revealed(const size_t & current_time, 
                                const size_t & time_limit) 
    {
        return (current_time - first_access) >= time_limit;
    }

    long long get_total_requests() {
        return total_requests;
    }

    ContextVector get_context_vector() {
        // TODO
        ContextVector contextVector;
        return contextVector;
    }


private:
    // time of the first request
    long long first_access;

    // total count of the requests
    long long total_requests; 

    //access statistics
    CidLongLong access_stat; 

    // starts of period from access statistics
    CidLongLong starts; 

    // periods for acccess_stat
    CidLongLong periods;
};



class PoPCaching {
    typedef std::unordered_map<std::string, size_t> ContentSizes;
    typedef std::unordered_map<std::string, size_t> CidLongLong;
    typedef std::unordered_map<std::string, std::string> Cache;
public:
    explicit PoPCaching(size_t size) : 
                cacheSize(size < 1 ? 1 : size),
                currentCacheSize(0),
                cyclesCount(0)
    {                        
        learn_limit = LEARN_LIMIT;
        period = PERIOD;
        z1 = Z1;
        z2 = Z2;

        std::vector<std::string> time_features;
        time_features.push_back("5 hours");
        time_features.push_back("30 hours");
        time_features.push_back("5 days");
        time_features.push_back("10 days");

        this->time_features = time_features;

        CidLongLong periods;
        periods["5 hours"] = 5 * HOUR;
        periods["30 hours"] = 30 * HOUR;
        periods["5 days"] = 5 * DAY;
        periods["10 days"] = 10 * DAY;

        this->periods = periods;

        ContextSpace contextSpace(time_features, z1, z2);

    }

    std::string * find(const std::string & cid, const size_t & current_time) {
        ++cyclesCount;

        if (content_features.find(cid) == content_features.end()) {
            Features features = Features(time_features, current_time, periods);
            content_features[cid] = features;
        }

        content_features[cid].update_features(current_time);

        update_evaluations();
        learn_popularity(cid);

        auto it = lookup.find(cid);
        if (it == lookup.end())
            return nullptr;

        return &it->second;
    }

    std::string * put(const std::string & cid, const std::string & value) {
        // TODO

        return nullptr;
    }

    size_t elementsCount() const {
        return lookup.size();
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

    void update_evaluations() {
        if (cyclesCount % period != 0)
            return;

        // Re-estimate the request rate for all content in 'estimations'
        // Rebuild the priority queue 'estimations'

        for (auto & element : lookup) {
            auto & cid = element.first;
            EstimationHolder old_holder = cidEstimationHolderMap[cid];
            if (!estimations.remove(old_holder)) {
                std::cout << "Error in removing holder from queue" << std::endl;
            }

            size_t new_estimation = estimate_popularity(cid);
            EstimationHolder new_holder = EstimationHolder(cid, new_estimation);
            cidEstimationHolderMap[cid] = new_holder;
            estimations.push(new_holder);
        }
    }

    void learn_popularity(const std::string & cid) {
        // TODO
    }


    size_t estimate_popularity(const std::string & cid) {
        // TODO

        return 0;
    }

private:
    size_t cacheSize;
    size_t currentCacheSize;
    size_t cyclesCount;

    // algorithm parameters
    size_t learn_limit;
    size_t period;
    size_t z1;
    size_t z2;

    std::vector<std::string> time_features;
    CidLongLong periods;

    ContentSizes contentSizes;
    ContextSpace contextSpace;
    
    // keep cid -> value
    Cache lookup;

    std::unordered_map<std::string, Features> content_features;

    struct EstimationHolder {
        std::string cid;
        size_t estimation;
        EstimationHolder(){}
        EstimationHolder(const std::string & cid, size_t estimation) : 
            cid(cid),
            estimation(estimation) {}

        bool operator < (const EstimationHolder & other) const {
            return estimation > other.estimation;
        }

        bool operator == (const EstimationHolder & other) const {
            return (cid == other.cid) && (estimation == other.estimation);
        }

        friend std::ostream & operator << (std::ostream & out, 
            const EstimationHolder & holder)
        {
            out << "cid -> " << holder.cid << ' ' 
                << "estimation -> " << holder.estimation;
            return out;
        }

    };

    // estimations for cids in cache
    custom_priority_queue<EstimationHolder> estimations;

    // map between cid and its EstimationHolder
    // This map is needed for removing elements from queue estimations
    std::unordered_map<std::string, EstimationHolder> cidEstimationHolderMap;
};
