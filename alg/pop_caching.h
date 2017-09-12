#pragma once

#include <set>
#include <list>
#include <cmath>
#include <queue>
#include <vector>
#include <cstdlib>
#include <cassert>
#include <utility>
#include <iostream>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <unordered_set>


// algorithm parameters
// #define LEARN_LIMIT 1000 // !!!!
#define LEARN_LIMIT 1000
// #define PERIOD 10000 // !!!!!!!!!!!!!
#define PERIOD 10000
#define Z1 2
#define Z2 0.5

// other parameters
#define HOUR 3600
#define DAY 86400

#define EPS 0.00001
#define OVERFLOW_EPS 0.01



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
        bool result = (capacity >= z1 * pow(2, z2 * level));

        if (result == false)
            return false;

        // result = true;

        for (auto & bound : bounds) {
            if (fabs(bound.second.second - bound.second.first) < OVERFLOW_EPS) {
                result = false;
                break;
            }
        }

        return result;
    }

    size_t get_estimation() {
        if (capacity == 0 && sum_of_request_rate == 0)
            return 1;

        if (capacity == 0 && sum_of_request_rate != 0)
            return 1;

        float result = static_cast<float>(sum_of_request_rate);
        // std::cout << "result ->" << result << std::endl;

        result = std::ceil(result / capacity);

        // std::cout << "capacity ->" << capacity << std::endl;
        return result;
    }

    bool contain(ContextVector & x) {
        for (auto & bound : bounds) {
            std::string direction = bound.first;
            double left = bound.second.first;
            double right = bound.second.second;

            if ((x[direction] >= left && x[direction] <= right) == false) {
                return false;
            }
        }

        return true;
    }

    bool operator == (const HyperCube & other) const {
        bool result;
        result =    (level == other.level) && 
                    (capacity == other.capacity) &&
                    (sum_of_request_rate == other.sum_of_request_rate);

        if (result == false)
            return result;
        
        bool flag = true;

        Bounds other_bounds = other.get_bounds();

        for (auto & bound : bounds) {
            std::string direction = bound.first;
            double left = bound.second.first;
            double right = bound.second.second;
            double left_other = other_bounds[direction].first;
            double right_other = other_bounds[direction].second;
            if (fabs(left - left_other) > EPS || fabs(right - right_other) > EPS) {
                flag = false;
                break;
            }
        }

        if (flag == false)
            return false;

        return true;
    }

    friend std::ostream & operator << (std::ostream & out, const HyperCube & h) {
        Bounds bounds = h.get_bounds();
        if (bounds.size() == 0) {
            std::cout << "Hypercube is empty" << std::endl;
            return out;
        }

        std::cout << "hypecube" << std::endl;
        for (auto & bound : bounds) {
            out << bound.first << ' ' << '<' <<
                bound.second.first << ' ' << bound.second.second << "> ";
        }

        return out;
    }

    void add_capacity(const size_t & delta) {
        capacity += delta;
    }

    void add_sum_of_requests(const size_t & delta) {
        sum_of_request_rate += delta;
    }

    size_t get_capacity() {
        return capacity;
    }

    size_t get_sum_of_request() {
        return sum_of_request_rate;
    }

    size_t get_level() {
        return level;
    }

    Bounds get_bounds() const {
        return bounds;
    }

private:
    size_t level; 
    
    // count of requests in this hypercube
    size_t capacity; 

    size_t sum_of_request_rate;

    Bounds bounds;
};


void print_context_vector(const ContextVector & x) {
    std::cout << "context vector" << std::endl;
    for (auto & elem : x) {
        std::cout << elem.first << ' ' << elem.second << ' ';
    }
    std::cout << std::endl;
    std::cout << std::endl;
}


class ContextSpace {
    typedef std::unordered_map<std::string, size_t> CidLongLong;
public:
    ContextSpace() {}

    ContextSpace(std::vector<std::string> time_features,
                const size_t & c1, const size_t & c2) :
        z1(c1), z2(c2)
    {
        Bounds bounds;
        directions = time_features;
        for (auto & feature : time_features) {
            std::pair<double, double> edge;
            edge.first = 0.0;
            edge.second = 1.0;
            bounds[feature] = edge;
        }

        HyperCube hypercube(0, 1, 0, bounds);
        hypercubes.push_back(hypercube);
    }

    HyperCube * find_hypercube(ContextVector & x) {
        // std::cout << "find_hypercube1" << std::endl;
        // print_context_space();

        // print_context_vector(x);
        for (auto & hypercube : hypercubes) {
            if (hypercube.contain(x))
                return &hypercube;
        }

        // std::cout << "find_hypercube2" << std::endl;
        
        print_context_space();
        std::cout << "hypercubes.size() -> " << hypercubes.size() << std::endl;
        print_context_vector(x);

        // std::cout << "Error in find hypercube" << std::endl;
        // Bounds bounds;
        // for (auto & direction : directions) {
        //     std::pair<double, double> edge;
        //     edge.first = -2.0;
        //     edge.second = -1.0;
        //     bounds[direction] = edge;
        // }

        // HyperCube hypercube(0, 1, 0, bounds);
        // hypercubes.push_back(hypercube);
        // return &hypercubes.back();
        return nullptr;
    }

    void init_splitter(std::string & splitter) {
        size_t dimension = directions.size();
        for (size_t i = 0; i < dimension; ++i) {
            splitter.push_back('0');
        }
    }

    bool splitter_exhausted(std::string & splitter) {
        bool overflow = true;
        for (size_t i = 0; i < splitter.length(); ++i) {
            if (overflow == false)
                break;

            if (splitter[i] == '0') {
                splitter[i] = '1';
                overflow = false;
            } else {
                // splitter[i] == 1
                splitter[i] = '0';
            }
        }

        if (overflow)
            return true;

        return false;
    }

    void create_new_hypercube(const size_t & level, const size_t & capacity, 
                const size_t & sum, Bounds & bounds, std::string & splitter)
    {
        // std::cout << "create_new_hypercube" << std::endl;

        Bounds new_bounds;

        // std::cout << level << ' ' << capacity << ' ' << sum << std::endl;

        for (size_t i = 0; i < splitter.length(); ++i) {
            std::string direction = directions[i];
            double left = 0.0;
            double right = 0.0;
            std::pair<double, double> pair;
            
            double delta = bounds[direction].second - bounds[direction].first;
            // may be check if delta < 0
            // it may be if delta is very small

            if (splitter[i] == '1') {

                left =  bounds[direction].first + delta / 2;
                right = bounds[direction].second;
            } else {
                left = bounds[direction].first;
                right = bounds[direction].first + delta/ 2;
            }

            pair = std::make_pair(left, right);
            new_bounds[direction] = pair;
        }

        HyperCube new_hypercube = HyperCube(level + 1, capacity, sum, new_bounds);
        // std::cout << new_hypercube << std::endl;
        hypercubes.push_back(new_hypercube);
    }

    void split(HyperCube * hypercube) {
        if (hypercube->overflow(z1, z2) == false)
            return;

        if (hypercubes.size() >= 10000)
            return;

        // std::cout << "splitting" << std::endl;
        // std::cout << *hypercube << std::endl;

        std::string splitter;
        init_splitter(splitter);

        Bounds bounds = hypercube->get_bounds();
        size_t level = hypercube->get_level();
        size_t capacity = hypercube->get_capacity();
        size_t sum = hypercube->get_sum_of_request();

        auto it = find(hypercubes.begin(), hypercubes.end(), *hypercube);
        hypercubes.erase(it);


        create_new_hypercube(level, capacity, sum, bounds, splitter);

        while (!splitter_exhausted(splitter)) {
            create_new_hypercube(level, capacity, sum, bounds, splitter);
        }

        // print_context_space();
    }

    void print_context_space() {
        if (hypercubes.empty()) {
            std::cout << "Context Space is empty" << std::endl;
            return;
        }

        std::cout << "context space" << std::endl;
        for (auto & hypercube : hypercubes) {
            std::cout << hypercube << std::endl;
        }
        std::cout << std::endl;
    }

    size_t size() {
        return hypercubes.size();
    }

private:
    size_t z1;
    size_t z2;
    std::vector<std::string> directions;
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
        // std::cout << "update_features1" << std::endl;
        total_requests += 1;
        for (auto & element : access_stat) {
            std::string feature = element.first;
            // size_t count = element.second;
            size_t start = starts[feature];
            size_t period = periods[feature];
            if ((start + period) <= current_time) {
                // starts[feature] = period * (current_time / period);
                starts[feature] = current_time;
                access_stat[feature] = 0;
                // std::cout << "update_features2" << std::endl;
            } else {
                access_stat[feature] += 1;
                // std::cout << "update_features3" << std::endl;
            }
        }

    }

    bool popularity_revealed(const size_t & current_time, 
                                const size_t & time_limit) 
    {
        return (current_time - first_access) >= time_limit;
    }

    size_t get_total_requests() {
        return total_requests;
    }

    ContextVector get_context_vector() {
        ContextVector contextVector;

        size_t null = 0;
        double norm = 0;


        for (auto & element : access_stat) {
            if (element.second == 0)
                ++null;
            norm += element.second * element.second;
        }

        norm = pow(norm, 0.5);

        if (norm <= EPS || null == access_stat.size())
            norm = 1.0;

        for (auto & element : access_stat) {
            contextVector[element.first] = element.second / norm;
        }

        return contextVector;
    }


private:
    // time of the first request
    size_t first_access;

    // total count of the requests
    size_t total_requests; 

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
    
    ~PoPCaching() {
        // contextSpace.print_context_space();
    }

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
        // std::cout << "initial context space" << std::endl;
        this->contextSpace = contextSpace;
        this->contextSpace.print_context_space();
    }

    std::string * find(const std::string & cid, const size_t & current_time) {
        ++cyclesCount;
        // std::cout << contextSpace.size() << std::endl;

        // std::cout << "cache::find1" << std::endl;
        // std::cout << "lookup.size() -> " << lookup.size() << std::endl;

        if (lookup.size() != currentCacheSize) {
            std::cout << "lookup.size != currentCacheSize" << std::endl;
            std::cout << lookup.size() << ' ' << currentCacheSize  << std::endl;
            exit(127);
        }

        if (lookup.size() != estimations.size()) {
            std::cout << "lookup.size != estimations.size()" << std::endl;
            std::cout << lookup.size() << ' ' << estimations.size()  << std::endl;
            exit(127);
        }

        // for (auto & period : periods) {
        //     std::cout << period.first << ' ' << period.second << ' ';
        // }
        // std::cout << std::endl;

        if (content_features.find(cid) == content_features.end()) {
            Features features = Features(time_features, current_time, periods);
            content_features[cid] = features;
        }

        // std::cout << "cache::find2" << std::endl;

        content_features[cid].update_features(current_time);

        update_evaluations();
        learn_popularity(cid, current_time);

        auto it = lookup.find(cid);
        if (it == lookup.end())
            return nullptr;

        // std::cout << "cache::find3" << std::endl;

        return &it->second;
    }

    std::string * put(const std::string & cid, const std::string & value) {
        // std::cout << "cache::put1" << std::endl;
        // std::cout << "lookup.size() -> " << lookup.size() << std::endl;
        // std::cout << "estimations.size() -> " << estimations.size() << std::endl;
        size_t cid_size = contentSizes[cid];
        size_t popularity_estimation = estimate_popularity(cid);

        if (cid_size < cacheSize) {    
            // std::cout << "cache::put2" << std::endl;
            size_t sum_popularity = 0;
            std::unordered_map<std::string, size_t> evicted_elements;

            // evict old elements
            while ((getCacheSize() + cid_size) > cacheSize) {
                // std::cout << "cache::put3" << std::endl;
                EstimationHolder least_popular = estimations.top();
                std::string least_cid = least_popular.cid;
                size_t least_estimation = least_popular.estimation;
                evicted_elements[least_cid] = least_estimation;
                sum_popularity += least_estimation;

                if (estimations.remove(least_popular) == false) {
                   std::cout << "Error in removing holder from queue 1" << std::endl;
                   exit(127);
                }
                
                currentCacheSize -= contentSizes[least_cid];
                cidEstimationHolderMap.erase(least_cid); 
                size_t s1 = lookup.size();
                lookup.erase(least_cid);
                size_t s2 = lookup.size();
                if (++s2 != s1)
                    std::cout << "ERROR. lookup.erase " << std::endl;
            }

            // compare sum popularity with popularity of the new content
            size_t compare_coeff = 2;
            if (popularity_estimation >= compare_coeff * sum_popularity) {   
                EstimationHolder new_holder = EstimationHolder(cid, 
                                                popularity_estimation);

                estimations.push(new_holder);
                currentCacheSize += contentSizes[cid];
                cidEstimationHolderMap[cid] = new_holder;
                std::pair<std::string, std::string> pair = std::make_pair(cid, cid);
                lookup.insert(pair);
                // std::cout << "cache::put4" << std::endl;
                // std::cout << "lookup.size() -> " << lookup.size() << std::endl;
                return &(lookup.find(cid)->second);
            } 

            // if popularity of the new content is not sufficient 
            // then return to previous state
            for (auto & element : evicted_elements) {
                std::string element_cid = element.first;
                size_t estimation = element.second;
                EstimationHolder holder = EstimationHolder(element_cid, estimation);
                estimations.push(holder);
                currentCacheSize += contentSizes[element_cid];
                cidEstimationHolderMap[element_cid] = holder;

                std::pair<std::string, std::string> pair = 
                            std::make_pair(element_cid, element_cid);

                lookup.insert(pair);
            }
            // std::cout << "cache::put5" << std::endl;
        }

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
        if ((cyclesCount % period) != 0)
            return;

        // Re-estimate the request rate for all content in 'estimations'
        // Rebuild the priority queue 'estimations'

        // std::cout << "update_evaluations" << std::endl;

        for (auto & element : lookup) {
            auto & cid = element.first;
            EstimationHolder old_holder = cidEstimationHolderMap[cid];
            if (estimations.remove(old_holder) == false) {
                std::cout << "Error in removing holder from queue 2" << std::endl;
                exit(127);
            }

            size_t new_estimation = estimate_popularity(cid);
            EstimationHolder new_holder = EstimationHolder(cid, new_estimation);
            cidEstimationHolderMap[cid] = new_holder;
            estimations.push(new_holder);
        }
    }

    void learn_popularity(const std::string & cid,
                            const size_t & current_time)
    {
        Features features = content_features[cid];
        if (features.popularity_revealed(current_time, learn_limit) == false) 
            return;

        // std::cout << "learn_popularity" << std::endl;

        // NOTE: learn must call only one time or many time if time >= learn_limit

        ContextVector context_vector = features.get_context_vector();
        HyperCube * hypercube = contextSpace.find_hypercube(context_vector);

        
        if (hypercube == nullptr) {
            std::cout << "In learn popularity. hypercube == nullptr" << std::endl;
            exit(127);
        }
        

        size_t total_requests = features.get_total_requests();
        hypercube->add_capacity(1);
        hypercube->add_sum_of_requests(total_requests);
        contextSpace.split(hypercube);
    }

    size_t estimate_popularity(const std::string & cid) {
        Features features = content_features[cid];
        ContextVector context_vector = features.get_context_vector();
        HyperCube * hypercube = contextSpace.find_hypercube(context_vector);


        if (hypercube == nullptr) {
            print_context_vector(context_vector);
            std::cout << "In learn popularity. hypercube == nullptr" << std::endl;
            exit(127);
        }

        size_t estimation = hypercube->get_estimation();

        // std::cout << "estimation -> " << estimation << std::endl;
        return estimation;
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
    
    // keep cid -> and value
    Cache lookup;

    std::unordered_map<std::string, Features> content_features;

    struct EstimationHolder {
        std::string cid;
        size_t estimation;
        EstimationHolder(){}
        EstimationHolder(const std::string & cid, const size_t & estimation) : 
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
