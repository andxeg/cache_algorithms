#pragma once 

#include <vector>
#include <unordered_map>

typedef std::vector<int> VecInt;
typedef std::vector<std::string> VecStr;
typedef std::unordered_map<std::string, VecInt> ObjectsHistory;
typedef std::unordered_map<std::string, std::string> ConfigMap;
typedef std::unordered_map<std::string, VecStr> MotherChildMap;
typedef std::unordered_map<std::string, std::string> ChildMotherMap;
typedef std::unordered_map<std::string, size_t> ContentSizes;
