#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>

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
            in >> id;

            if (in.eof()) {
                break;
            }

            itemPositions[id].push_back(pos);

            auto it = itemPositions.find(id);
            if (it->second.size() == 1) {
                positionsQueue.push(PositionHolder(pos, it));
            }

            ++pos;

            if (pos % 100000 == 0) {
//                std::cout << "Get positions " << pos << "\n";
            }
        }
    }

    size_t warmUp(const std::string &id) {
        if (lookup.find(id) == lookup.end()) {
            lookup.insert(id);
        }

        ++cyclesCount;

        return lookup.size();
    }

    void process(const std::string& id) {
        ++requestCount;
        if (lookup.find(id) == lookup.end()) {
            //Cache miss
            ++missCount;
            replace(id);
        }

        ++cyclesCount;
    }

    float hitRate() const {
//        std::cout << requestCount << " " << missCount << "\n";
        return requestCount != 0 ? 100 * (requestCount - missCount) / float(requestCount) : 100;
    }

    void dump() const {
        std::cout << "Positions\n";
        for (auto& positions : itemPositions) {
            std::cout << positions.first << "\n";
            std::copy(positions.second.begin(), positions.second.end(), std::ostream_iterator<size_t>(std::cout, " "));
            std::cout << "\n";
        }
    }

private:
    void replace(const std::string &id) {
        std::string itemToRemove;
        size_t maxPos = 0;

        if (positionsQueue.empty()) {
            itemToRemove = *lookup.begin();
        } else {
            const PositionHolder& maxPosition = positionsQueue.top();
            itemToRemove = maxPosition.it->first;
            auto& positionList = maxPosition.it->second;

            if (!positionList.empty()) {
                positionsQueue.push(PositionHolder(positionList.front(), maxPosition.it));
                positionList.pop_front();
            }
        }

        lookup.erase(itemToRemove);
        lookup.insert(id);
    }

private:
    size_t cacheSize;
    std::string requestsFileName;

    typedef std::unordered_map<std::string, std::deque<size_t> > ItemPositions;
    ItemPositions itemPositions;

    size_t requestCount;
    size_t missCount;

    typedef std::unordered_set<std::string> Lookup;
    Lookup lookup;

    size_t cyclesCount;

    struct PositionHolder {
        size_t position;
        ItemPositions::iterator it;

        PositionHolder(size_t p, ItemPositions::iterator i) :
                position(p),
                it(i) {}

        bool operator < (const PositionHolder& other) const {
            return position < other.position;
        }
    };

    std::priority_queue<PositionHolder> positionsQueue;
};

int main(int argc, const char* argv[]) {
    size_t cacheSize = atoi(argv[1]);
    std::string fileName = argv[2];

    OptCache cache(cacheSize, fileName);

    //cache.dump();

    size_t count = 0;

    std::unordered_set<std::string> warmUpItems;

    std::ifstream in(fileName);
    while (true) {
        std::string id;
        in >> id;

        if (in.eof()) {
            break;
        }

        if (warmUpItems.size() < cacheSize) {
            cache.warmUp(id);
            warmUpItems.insert(id);
        } else {
            cache.process(id);
        }

        if (++count % 100000 == 0) {
//            std::cout << "Process " << count << "\n";
        }
    }

    std::cout << cacheSize << ' ' << cache.hitRate() << "\n";

    return 0;
}
