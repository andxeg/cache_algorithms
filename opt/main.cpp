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


template <typename T>
std::string ToString(T val)
{
    std::stringstream stream;
    stream << val;
    return stream.str();
}



class OptCache {
public:
    OptCache(size_t size, const std::string &fileName) :
            cacheSize(size),
            requestsFileName(fileName),
            requestCount(0),
            missCount(0),
            cyclesCount(0) {
        std::ifstream in(fileName);

        size_t count = 0;

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
                positionsQueue.push(posHolder);
                contentSizes[id] = size;
            }

            ++pos;

            // if (pos % 100000 == 0) {
            //    std::cout << "Get positions " << pos << "\n";
            // }
        }
    }

    size_t warmUp(const std::string &id) {
        if (lookup.find(id) == lookup.end()) {

            PositionHolder posHolder = IdPositionHolderMap[id];
            currentPositionsQueue.push(posHolder);
            lookup.insert(id);
        }

        std::cout << "Current cycle -> " << cyclesCount << std::endl;

        ++cyclesCount;


        std::cout << "WarmUp. Lookup size after adding -> " << lookup.size() << std::endl;
        std::cout << "Current cache size -> " << getCacheSize() << std::endl;

        return lookup.size();
    }

    void process(const std::string& id) {
        ++requestCount;


        if (lookup.find(id) == lookup.end()) {
            //Cache miss
            ++missCount;
            
            if (contentSizes[id] < cacheSize) {
                size_t idSize = contentSizes[id];

                std::cout << "Adding element with id -> " << id << std::endl;
                
                while ((idSize + getCacheSize()) > cacheSize) {
                    // std::cout << "freeUpSpace" << std::endl;
                    freeUpSpace();
                }


                PositionHolder posHolder = IdPositionHolderMap[id];
                currentPositionsQueue.push(posHolder);
                lookup.insert(id);

                std::cout << "Process. Lookup size after adding -> " << lookup.size() << std::endl;
                std::cout << "Current cache size -> " << getCacheSize() << std::endl;
            }

        }

        std::cout << "Current cycle -> " << cyclesCount << std::endl;

        ++cyclesCount;
    }

    float hitRate() const {
//        std::cout << requestCount << " " << missCount << "\n";
        return requestCount != 0 ? 100 * (requestCount - missCount) / float(requestCount) : -1;
    }

    void dump() const {
        std::cout << "Positions\n";
        for (auto& positions : itemPositions) {
            std::cout << positions.first << "\n";
            std::copy(positions.second.begin(), positions.second.end(), std::ostream_iterator<size_t>(std::cout, " "));
            std::cout << "\n";
        }
    }

    size_t getCacheSize() {
        size_t result = 0;

        for (auto& element : lookup) {
            result += contentSizes[element];
        }

        return result;
    }

    ContentSizes getContentSizes() {
        return contentSizes;
    }

private:
    void freeUpSpace() {
        std::string itemToRemove;
        size_t maxPos = 0;

        if (positionsQueue.empty()) {
            itemToRemove = *lookup.begin();
        } else {
            const PositionHolder& maxPosition = currentPositionsQueue.top();
            itemToRemove = maxPosition.it->first;
            auto& positionList = maxPosition.it->second;

            std::cout << "itemToRemove -> " << itemToRemove << std::endl;

            // print current cache requests
            for (const auto& element: lookup) {
                std::cout << element << std::endl;
            }
            //

            std::cout << "positionList.size() before pop -> " << positionList.size() << std::endl;

            
            // !!!
            if (!positionList.empty()) {
                /* OLD CODE
                std::cout << "positionList is not empty" << std::endl;
                // Delete previous positionHolder from IdPositionHolderMap
                // and add new with new max position.
                // Simple replace old positionHolder.

                auto it = IdPositionHolderMap.find(itemToRemove);
                if (it != IdPositionHolderMap.end()) {
                    PositionHolder oldPosHolder = it->second;
                    std::cout << "In if Delete old posHolder for -> " << oldPosHolder.it->first << std::endl;
                    // delete oldPosHolder from priority queue
                    std::cout << "currentPositionsQueue.size() before removing in IF-> " << currentPositionsQueue.size() << std::endl;
                    
                    currentPositionsQueue.remove(oldPosHolder);
                    
                    std::cout << "Top after removing -> " << currentPositionsQueue.top().it->first << std::endl;
                    PositionHolder newPosHolder = PositionHolder(positionList.front(), maxPosition.it);
                    positionList.pop_front();

                    IdPositionHolderMap.erase(it);
                    IdPositionHolderMap[itemToRemove] = newPosHolder;
                    std::cout << "item to remove" << itemToRemove << std::endl;
                }
                */

                maxPosition;
                






            }
        }


        auto& positionList = itemPositions[itemToRemove];
        std::cout << "positionList.size() after pop -> " << positionList.size() << std::endl;
        
        std::cout << "lookup.size() before erase -> " << lookup.size() << std::endl;
        lookup.erase(itemToRemove);
        std::cout << "lookup.size() after erase -> " << lookup.size() << std::endl;

        std::cout << "currentPositionsQueue.size() before removing -> " << currentPositionsQueue.size() << std::endl;

        PositionHolder posHolder = IdPositionHolderMap[itemToRemove];

        std::cout << "Delete posHolder for -> " << posHolder.it->first << std::endl;
        currentPositionsQueue.remove(posHolder);
        std::cout << "Top after removing -> " << currentPositionsQueue.top().it->first << std::endl;
        std::cout << "currentPositionsQueue.size() after removing -> " << currentPositionsQueue.size() << std::endl;
    }

// private:
public:
    size_t cacheSize;
    std::string requestsFileName;

    typedef std::unordered_map<std::string, std::deque<size_t> > ItemPositions;
    ItemPositions itemPositions;

    ContentSizes contentSizes;

    size_t requestCount;
    size_t missCount;

    typedef std::unordered_set<std::string> Lookup;
    Lookup lookup;

    size_t cyclesCount;

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


bool canAppendIdToWarmUpItems(  std::unordered_set<std::string> & warmUpItems,
                                std::string id,
                                size_t & cacheSize,
                                ContentSizes & contentSizes)
{
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



struct tm * print_current_data_and_time(const std::string & message) {
    time_t t = time(0);
    struct tm * now = localtime(&t);
    std::string result = ToString<int>(now->tm_year + 1900) + std::string("-") +
                         ToString<int>(now->tm_mon + 1) + std::string("-") +
                         ToString<int>(now->tm_mday ) + std::string(".") +
                         ToString<int>(now->tm_hour ) + std::string(":") +
                         ToString<int>(now->tm_min ) + std::string(":") +
                         ToString<int>(now->tm_sec );
    std::cout << message << " Current time -> " << result << std::endl;

    return now;
}


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

    //cache.dump();

    size_t count = 0;

    std::unordered_set<std::string> warmUpItems;

    std::ifstream in(fileName);
    while (true) {
        std::string id;
        size_t size;

        in >> id;
        in >> size;

        if (in.eof()) {
            break;
        }

        if (canAppendIdToWarmUpItems(warmUpItems, id, cacheSize, contentSizes)) {
            cache.warmUp(id);
            warmUpItems.insert(id);
        } else {
            // std::cout << "proccess" << std::endl;
            cache.process(id);
        }

        if (++count % 100000 == 0) {
        //    std::cout << "Process " << count << "\n";
        }
    }


    message = "Algorithm was finished.\0";
    now = print_current_data_and_time(message);
    int time = now->tm_hour * 3600 + now->tm_min * 60 + now->tm_sec - hour_start * 3600 - min_start * 60 - sec_start;
    std::cout << "Algorithm work time -> " <<  time / 3600  << "  hours " << (time % 3600) / 60  << " mins " << (time % 3600) % 60 << " secs" << std::endl;

    std::cout << "\nAlgorithm results:\n";
    std::cout << "Cache size -> " << cacheSize << " Kbyte" << std::endl;
    std::cout << "Hit-rate -> " << cache.hitRate() << std::endl;
    std::cout << "Cycle -> " << cache.cyclesCount << std::endl;
    std::cout << "Requests -> " << cache.requestCount << std::endl;
    std::cout << "Miss Count -> " << cache.missCount << std::endl;


    // for (auto& element : cache.IdPositionHolderMap) 
    //     std::cout <<  element.first << ' ' << element.second << std::endl;

    return 0;
}
