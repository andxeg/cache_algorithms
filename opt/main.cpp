#include <list>
#include <set>
#include <queue>
#include <ctime>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include "timestamps.h"

typedef std::unordered_map<std::string, size_t> ContentSizes;

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


class OptCache {
    typedef std::unordered_set<std::string> Lookup;
    typedef std::unordered_map<std::string, std::deque<size_t> > ItemPositions;
public:
    OptCache(size_t size, const std::string &fileName) :
            cacheSize(size),
            missCount(0),
            cyclesCount(0),
            currentCacheSize(0),
            requestsFileName(fileName)
    {

        std::ifstream in(fileName);
        size_t pos = 0;

        while (true) {
            std::string id;
            size_t size;

            in >> id;
            in >> size;

            if (in.eof()) {
                break;
            }

            itemPositions[id].push_back(pos);
            auto it = itemPositions.find(id);
            if (it->second.size() == 1) {
                PositionHolder posHolder = PositionHolder(pos, it);
                IdPositionHolderMap[id] = posHolder;
                contentSizes[id] = size;
            }
            ++pos;
        }

        pos += 10;
        for (auto & item : itemPositions) {
            auto & positionList = item.second;
            positionList.push_back(pos);
        }
    }

    void modifyPositionHolder(const std::string & id) {
        auto iter = itemPositions.find(id);
        auto & positionList = iter->second;
        if (positionList.size() >= 2) {
            positionList.pop_front();
            auto position = positionList.front();
            if (lookup.find(id) != lookup.end()) {
                PositionHolder oldPosHolder = IdPositionHolderMap[id];
                if (!currentPositionsQueue.remove(oldPosHolder))
                    std::cout << "Error modify" << std::endl;
            }
            PositionHolder newPosHolder = PositionHolder(position, iter);
            IdPositionHolderMap[id] = newPosHolder;
        }
    }

    bool find(const std::string & id) {
        if (lookup.find(id) == lookup.end())
            return false;

        // modify position holder for id
        modifyPositionHolder(id);

        PositionHolder newPosHolder = IdPositionHolderMap[id];
        currentPositionsQueue.push(newPosHolder);

        ++cyclesCount;

        return true;
    }

    void process(const std::string & id) {        
        if (lookup.find(id) == lookup.end()) {
            ++missCount;
            
            size_t idSize = contentSizes[id];

            if (idSize < cacheSize) {

                // if current content will be requested only one time
                // then don't cache it
                auto iter = itemPositions.find(id);
                auto & positionList = iter->second;
                if (positionList.size() == 2) {
                    modifyPositionHolder(id);                    
                    ++cyclesCount;
                    return;
                }
                
                while ((idSize + getCacheSize()) > cacheSize) {
                    freeUpSpace();
                }

                modifyPositionHolder(id);
                
                PositionHolder posHolder = IdPositionHolderMap[id];
                currentPositionsQueue.push(posHolder);

                lookup.insert(id);
                currentCacheSize += contentSizes[id];
            }

        }

        ++cyclesCount;
    }

    float hitRate() const {
        return cyclesCount != 0 ? 100 * (cyclesCount - missCount) / float(cyclesCount) : -1;
    }

    size_t getCacheSize() {
        return currentCacheSize;
    }

    ContentSizes getContentSizes() {
        return contentSizes;
    }

    size_t size() {
        return lookup.size();
    }

    size_t getCyclesCount() {
        return cyclesCount;
    }

    size_t getMissCount() {
        return missCount;
    }

private:
    void freeUpSpace() {
        std::string itemToRemove;
        const PositionHolder& maxPosition = currentPositionsQueue.top();
        itemToRemove = maxPosition.it->first;
        currentPositionsQueue.remove(maxPosition);
        currentCacheSize -= contentSizes[itemToRemove];
        lookup.erase(itemToRemove);
    }

private:
    size_t cacheSize;
    size_t missCount;
    size_t cyclesCount;
    size_t currentCacheSize;
    std::string requestsFileName;
    ContentSizes contentSizes;

    ItemPositions itemPositions;
    Lookup lookup;

    struct PositionHolder {
        size_t position;
        ItemPositions::iterator it;

        PositionHolder(){}

        PositionHolder(size_t p, ItemPositions::iterator i) :
                position(p),
                it(i) {}

        bool operator < (const PositionHolder& other) const {
            return position < other.position;
        }

        friend std::ostream & operator << (std::ostream & out, const PositionHolder& posHolder) {
            out << "position -> " << posHolder.position;
            return out; 
        }

        bool operator == (const PositionHolder& other) const {
            // return (this->position == other.position) && (this->it == other.it);
            return (this->position == other.position) && (this->it->first == other.it->first);
            
        }
    };

    std::priority_queue<PositionHolder> positionsQueue;
    custom_priority_queue<PositionHolder> currentPositionsQueue;
    std::unordered_map<std::string, PositionHolder> IdPositionHolderMap;
};


int main(int argc, const char* argv[]) {
    std::string::size_type sz = 0;
    size_t cacheSize = std::stoll(std::string(argv[1]), &sz, 0);
    std::string fileName = argv[2];

    std::string message = "Start algo.\0";
    struct tm * now = print_current_data_and_time(message);
    int hour_start = now->tm_hour;
    int min_start = now->tm_min;
    int sec_start = now->tm_sec;

    OptCache cache(cacheSize, fileName);

    message = "After cache initialization.\0";
    now = print_current_data_and_time(message);

    ContentSizes contentSizes = cache.getContentSizes();

    size_t count = 0;
    std::ifstream in(fileName);

    while (true) {
        std::string id;
        size_t size;

        in >> id;
        in >> size;

        if (in.eof()) {
            break;
        }
        
        bool value = cache.find(id);

        if (value == false)
            cache.process(id);

        if (++count % 1000 == 0) {
           std::cout << "Process " << count << "\n";
        }
    }


    message = "Algorithm was finished.\0";
    now = print_current_data_and_time(message);
    int time = now->tm_hour * 3600 + now->tm_min * 60 + now->tm_sec - hour_start * 3600 - min_start * 60 - sec_start;
    std::cout << "Algorithm work time -> " <<  
                time / 3600  << "  hours " << 
                (time % 3600) / 60  << " mins " << 
                (time % 3600) % 60 << " secs" << std::endl;

    std::cout << "\nAlgorithm results:\n";
    std::cout << "Cache size -> " << cacheSize << " Kbyte" << std::endl;
    std::cout << "Hit-rate -> " << cache.hitRate() << std::endl;

    std::cout << std::endl;

    std::cout << "More precisely:" << std::endl;
    std::cout << "Cycle -> " << cache.getCyclesCount() << std::endl;
    std::cout << "Miss Count -> " << cache.getMissCount() << std::endl;
    std::cout << "Cache size -> " << cache.getCacheSize() << std::endl;
    std::cout << "Cache size -> " << cache.size() << std::endl;

    return 0;
}
