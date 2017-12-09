#pragma once

#include "defs.h"

#include <list>
#include <limits>
#include <cstdlib>
#include <utility>
#include <vector>
#include <iostream>
#include <functional>
#include <unordered_map>



// https://github.com/igorcanadi/lru-k/blob/master/LRUK.py
// http://www.vldb.org/conf/1994/P439.PDF
// http://www.cs.cmu.edu/~christos/courses/721.S03/LECTURES-PDF/0241-LRUK.PDF
// http://moscow.sci-hub.cc/3a7e3c54009cd1f205654dd3f4f38891/oneil1993.pdf
// http://www.ece.eng.wayne.edu/~sjiang/ECE7995-fall-11/lecture-5.pdf


/*
    Cache has two strategies: LRU-K and LRU (for ambiguous cases)
*/

template <typename Key, typename Value>
class LRU_K_Cache {
    typedef std::list<std::pair<Key, Value>> LruList;
    typedef std::unordered_map<std::string, size_t> ContentSizes;
public:
    LRU_K_Cache() {};
    explicit LRU_K_Cache(size_t size, const size_t & learn_limit = 100, 
                            const size_t & period = 1000, const size_t & history_len = 2) :
            cacheSize(size < 1 ? 1 : size),
            currentCacheSize(0)
    {
        // TODO
        // Add config file and take all algorithm's parameters to it
        // learn_period and period from pop_caching
        // here learn_period and period have another semantic

        correlated_reference_period = learn_limit;
        retained_information_period = period;
        this->history_len = history_len;
    }

    void prepare_cache() {
        return;
    }

    Value* find(const Key &key, const size_t & current_time = 0) {
        auto it = lookup.find(key);

        if (it == lookup.end()) {
            return nullptr;
        }

        // update LRU cache
        Value * value = &promote(it->second)->second;

        // update history information
        size_t & last_request = cid_last_request[key];
        std::vector<size_t> & history = cid_history[key];

        if ((current_time - last_request) > correlated_reference_period) {
            /* a new, uncorreleated reference */
            size_t correl_period_of_refd_page = last_request - history[0];
            for (size_t i = (history.size() - 1); i >= 1; --i) {
                history[i] = history[i-1] + correl_period_of_refd_page;
            }
            history[0] = current_time;
            last_request = current_time;
        } else {
            /* a correleated reference */
            last_request = current_time;
        }

        return value;
    }

    Value* put(const Key &key, const Value &value, const size_t & current_time = 0) {
        Value *result = find(key);
        if (result) {
            return result;
        }

        size_t cidSize = contentSizes[key];
        if (cidSize > cacheSize)
            return nullptr;
        
        while ((getCacheSize() + cidSize) >  cacheSize) {
            std::vector<Key> victims = find_victims(key, current_time);
            // null, one or two cids in vector victims now
            // if two cids then HIST(victims[0], k) < HIST(victims[1], k)

            if (victims.size() == 0) {

                // if all elements in correlation period
                // delete elements by LRU-1 strategy
                makeSizeInvariant(cacheSize - cidSize, current_time);

            } else if (victims.size() == 1) {

                Key victim = victims[0];
                Value val = lookup[victim]->second;
                if (evictionCallback) {
                    evictionCallback(victim, val, current_time);
                }

                erase(victim);

            } else if (victims.size() == 2) {

                Key cid1 = victims[0];
                Key cid2 = victims[1];                
                size_t min1 = cid_history[cid1].back();
                size_t min2 = cid_history[cid2].back();
                
                // 0 <= min1 <= min2 then only 3 variants
                if (min1 != 0) {

                    Value val = lookup[cid1]->second;
                    if (evictionCallback) {
                        evictionCallback(cid1, val, current_time);
                    }

                    val = lookup[cid2]->second;
                    if (evictionCallback) {
                        evictionCallback(cid2, val, current_time);
                    }

                    erase(cid1);
                    erase(cid2);
                } else if (min1 == 0 && min2 != 0) {

                    Value val = lookup[cid1]->second;
                    if (evictionCallback) {
                        evictionCallback(cid1, val, current_time);
                    }

                    val = lookup[cid2]->second;
                    if (evictionCallback) {
                        evictionCallback(cid2, val, current_time);
                    }

                    erase(cid1);
                    erase(cid2);
                } else { // min1 == min2 == 0
                    delete_one_element(victims, current_time);
                }
            }
        }

        typename LruList::iterator addedIt = addCidToCache(key, value, current_time);
        return &addedIt->second;
    }

    bool erase(const Key &key) {
        auto it = lookup.find(key);

        if (it == lookup.end()) {
            return false;
        }

        size_t cidSize = contentSizes[key];
        currentCacheSize -= cidSize;

        lruList.erase(it->second);
        lookup.erase(it);

        return true;
    }

    void setEvictionCallback(std::function<void(const Key &,const Value &, const size_t & current_time)> callback) {
        evictionCallback = callback;
    }

    size_t size() const {
        // limit of cache size
        return cacheSize;

        // return lookup.size(); previous code
    }

    size_t elementsCount() const {
        return lookup.size();
    }

    void setCacheSize(size_t size) {
        cacheSize = size;
        makeSizeInvariant(cacheSize);
    }

    const std::pair<Key, Value> *mruItem() const {
        return &lruList.back();
    }

    const std::pair<Key, Value> *lruItem() const {
        return &lruList.front();
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

    VecStr get_hot_content(const float &cache_hot_content) {
        return VecStr();
    }

private:
    void makeSizeInvariant(size_t size, const size_t & current_time = 0) {
        while (getCacheSize() > size) {
            if (evictionCallback) {
                evictionCallback(lruList.front().first, lruList.front().second, current_time);
            }

            size_t cidSize = contentSizes[lruList.front().first];
            currentCacheSize -= cidSize;

            lookup.erase(lruList.front().first);

            lruList.pop_front();
        }
    }

    typename LruList::iterator addCidToCache(const Key & key, const Value & value, const size_t & current_time) {
        size_t cidSize = contentSizes[key];
        lruList.push_back(std::make_pair(key, value));
        auto addedIt = --lruList.end();
        lookup[key] = addedIt;
        currentCacheSize += cidSize;

        // update history for old cid
        // or add history for new cid 

        auto it = cid_history.find(key);
        if (it == cid_history.end()) {
            std::vector<size_t> v(history_len, 0);
            cid_history[key] = v;
        } else {
            for (size_t i = (history_len - 1); i >= 1; --i) {
                cid_history[key][i] = cid_history[key][i-1];
            }
        }

        cid_history[key][0] = current_time;
        cid_last_request[key] = current_time;
        return addedIt;
    }

    std::vector<Key> find_victims(const Key & key, const size_t & current_time) {
        // return null, one or two cids with minimal HIST(cid, k)
        std::vector<Key> victims;
        
        size_t min1, min2;
        min1 = min2 = std::numeric_limits<size_t>::max();
        Key cid1, cid2;
        cid1 = cid2 = "";

        // 0 <= min1 <= min2
        for (auto & element : lookup) {
            Key cid = element.first;
            size_t last_request = cid_last_request[cid];
            if ((current_time - last_request) <= correlated_reference_period)
                continue;

            // cid is out of corr_ref_period
            size_t oldest_request = cid_history[cid].back();
            if (min1 > oldest_request) {
                min2 = min1;
                cid2 = cid1;
                
                min1 = oldest_request;
                cid1 = cid;
                continue;
            }

            if (min2 > oldest_request) {
                min2 = oldest_request;
                cid2 = cid;
            }
        }   

        if (cid1 != "")
            victims.push_back(cid1);
        
        if (cid2 != "")
            victims.push_back(cid2);

        return victims;
    }

    void delete_one_element(std::vector<Key> & elements, const size_t & current_time) {
        // delete one element from elements by LRU-1 strategy
        // elements.size() >= 2
        Key cid = elements[0];
        size_t min_last = cid_last_request[cid];
        for (auto & element : elements) {
            size_t last_request = cid_last_request[element];
            if (last_request < min_last) {
                cid = element;
                min_last = last_request;
            }
        }

        Key key = cid;
        Value value = lookup[cid]->second;
        if (evictionCallback) {
            evictionCallback(key, value, current_time);
        }

        erase(cid);
    }

    typename LruList::iterator promote(typename LruList::iterator it) {
        lruList.push_back(*it);

        auto addedIt = --lruList.end();
        lookup[it->first] = addedIt;

        lruList.erase(it);

        return addedIt;
    }

// private:
public:
    LruList lruList;
    std::unordered_map<Key, typename LruList::iterator> lookup;
    std::function<void(const Key &,const Value &, const size_t & current_time)> evictionCallback;


    size_t cacheSize;
    size_t currentCacheSize;
    size_t correlated_reference_period;
    size_t retained_information_period;
    
    // history_len is a paramter K in LRU-K
    size_t history_len;
    
    // time of last request ot the content
    std::unordered_map<Key, size_t> cid_last_request;

    // history of requests to the cid
    // cid_history[0] - last request
    // cid_history[1] - penultimate request
    // and etc.
    std::unordered_map<Key, std::vector<size_t>> cid_history;

    ContentSizes contentSizes;
};
