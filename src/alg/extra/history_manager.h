#pragma once

#include "defs.h"
#include "config.h"

#include <vector>
#include <unordered_map>


class HistoryManager {
	typedef std::unordered_map<std::string, std::vector<int>> ObjectsHistory;
public:
	HistoryManager(Config &config);
	void start_new_period();
	void update_object_history(std::string &object_id, const int &current_period);
	std::vector<int> get_object_history(std::string &object_id);
	std::vector<std::string> get_hot_objects(const int &window, const float &rate);
	void print_history();
private:
	int period_size;
	ObjectsHistory objects_history;
};
