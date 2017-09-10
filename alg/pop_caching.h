#pragma once

#include <list>
#include <cmath>
#include <queue>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <functional>
#include <unordered_map>


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
std::unordered_map<std::string, std::pair<double, double>> Bounds;

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
}


class ContextSpace() {
public:
    ContextSpace(std::vector<std::string> time_features) {
        Bounds bounds;
        for (auto & feature : time_features) {
            std::pair<double, double> edge;
            edge.first = 0.0;
            edge.second = 1.0;
            bounds[feature] = bounds;
        }

        HyperCube hypercube(0, 1, 0, bounds);
        hypercubes.push_back(hypercube);
    }

    HyperCube find_hypercube(ContextVector & x) {
        // TODO
    }

    void split(HyperCube hypercube) {
        // TODO
    }

private:
    std::vector<HyperCube> hypercubes;
};


class Features {
    typedef std::unordered_map<std::string, size_t> ContentSizes;
    typedef std::unordered_map<std::string, size_t> CidLongLong;
public:
    explicit Features(std::vector<std::string> time_features, 
                        const size_t & first_access)
    {
        total_requests = 0;
        first_access = first_access;
        for (auto & feature : time_features) {
            access_stat[feature] = 0;
        }
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
public:
    explicit PoPCaching(size_t size) :
                cacheSize(size < 1 ? 1 : size),
                currentCacheSize(0) {}

    std::string find(const std::string & key) {
        // TODO
    }

    std::string put(const std::string & key, const std::string & value) {
        // TODO
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

private:
    size_t cacheSize;
    size_t currentCacheSize;
    ContentSizes contentSizes;
    std::unordered_set<std::string> lookup;
    std::unordered_map<std::string, Features> content_features;

    struct EstimationHolder {
        std::string cid;
        size_t estimation;
        EstimationHolder(){}
        EstimatoinHolder(const std::) : {}
    };


    std::custom_priority_queue<EstimationHolder> estimations;
    std::unordered_map<std::string, EstimationHolder> cidEstimationHolderMap;
};
