#include "defs.h"
#include "history_manager.h"

#include <iostream>
#include <algorithm>

#define MIN(X, Y) ((X) > (Y) ? (Y) : (X))
#define MAX(X, Y) ((X) < (Y) ? (Y) : (X))


HistoryManager::HistoryManager(Config &config) {
	period_size = config.get_int_by_name("HISTORY_PERIOD_SIZE");
}

void HistoryManager::start_new_period() {
	for (auto &hist : objects_history)
		hist.second.push_back(0);
}

void HistoryManager::update_object_history(std::string &object_id, 
											const int &current_period) {
	auto it = objects_history.find(object_id);
	if (it == objects_history.end()) {
		objects_history[object_id] = std::vector<int>(current_period,0);
	}
	objects_history[object_id].back()++;
}

std::vector<int>
HistoryManager::get_object_history(std::string &object_id) {
	auto it = objects_history.find(object_id);
	if (it == objects_history.end()) {
		return std::vector<int>();
	}
	return objects_history[object_id];
}

std::vector<std::string> 
HistoryManager::get_hot_objects(const int &window, const float &rate) {
	/*
	1. fetch summary requests of the last window for each objects
	2. sort objects by this sum
	3. return only rate of whole objects
	*/ 

	std::vector<std::string> objects(objects_history.size());
	std::unordered_map<std::string, int> window_history;
	int i = 0;
	int sum;
	for (auto &hist : objects_history) {
		sum = 0;
		objects[i++] = hist.first;
		for (int j = 0; j < MIN(window, (int)hist.second.size()); ++j)
			sum += hist.second[hist.second.size()-1-j];
		window_history[hist.first] = sum;
	}

	stable_sort(objects.begin(), objects.end(),
		[&window_history] (const std::string &first, const std::string &second){
			return window_history[first] > window_history[second];
		}
	);

	int count = MAX((int)(rate*objects_history.size()), 1);
	std::vector<std::string> result(count);
	for (unsigned int i = 0; i < result.size(); ++i) {
		result[i] = objects[i];
	}

	return result;
}

float HistoryManager::get_average_size_in_window(const int &window, 
										ContentSizes & content_sizes) {
	size_t count    = 0;
	size_t sum_size = 0;
	for (auto &hist : objects_history) {
		for (int i = 0; i < MIN(window, (int)hist.second.size()); ++i) {
			if (hist.second[hist.second.size()-i-1] != 0) {
				++count;
				sum_size += content_sizes[hist.first];
				break;
			}
		}
	}
	return sum_size / (float)count;
}

void HistoryManager::print_history() {
	std::cout 	<< "History for " 
				<< objects_history.begin()->second.size() 
				<< " periods"<< std::endl;
	for (auto &hist : objects_history) {
		std::cout << hist.first;
		for (auto &val : hist.second) {
			std::cout << ' ' << val;
		}
		std::cout << std::endl;
	}
}
