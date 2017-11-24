#include "pre_push.h"

#include <fstream>
#include <iostream>
#include <unordered_set>


PrePush::PrePush(Config &config) {
	read_mother_child(config.get_str_by_name("PRE_PUSH_MOTHER_CHILD"));
	read_child_mother(config.get_str_by_name("PRE_PUSH_CHILD_MOTHER"));
	window 			 = config.get_int_by_name("PRE_PUSH_HISTORY_WINDOW");
	hist_hot_objects = config.get_float_by_name("PRE_PUSH_HISTORY_HOT_CONTENT");
}

VecStr PrePush::get_pre_push_list(VecStr &cache_hot_objects, 
								HistoryManager &history_manager) {
	std::unordered_set<std::string> result;
	VecStr history_hot_objects = history_manager.get_hot_objects(window,
													hist_hot_objects);
	/* find objects whose are in mother-child relationship */
	/* history hot objects */
	for (auto &object : history_hot_objects) {
		std::string mother_id = child_mother[object];
		result.insert(	mother_child[mother_id].begin(), 
						mother_child[mother_id].end());
	}

	/* cache hot objects */
	for (auto &object : cache_hot_objects) {
		std::string mother_id = child_mother[object];

		result.insert(	mother_child[mother_id].begin(),
						mother_child[mother_id].end());
	}

	/* add history hot objects */
	result.insert(	history_hot_objects.begin(),
					history_hot_objects.end());

	return VecStr(result.begin(), result.end());
}

void PrePush::read_mother_child(const std::string &mother_child_file) {
	std::fstream input(mother_child_file);
	std::string mother_id;
	int mother_count = 0;
	int child_count = 0;
	input >> mother_count;
	for(int i = 0; i < mother_count; ++i) {	
		input >> mother_id >> child_count;
		mother_child[mother_id] = VecStr(child_count);
		for (int j = 0; j < child_count; ++j)
			input >> mother_child[mother_id][j];
	}
	input.close();
}

void PrePush::read_child_mother(const std::string &child_mother_file) {
	std::fstream input(child_mother_file);
	std::string child_id;
	std::string mother_id;
	int child_count = 0;
	input >> child_count;
	for (int i = 0; i < child_count; ++i) {
		input >> child_id >> mother_id;
		child_mother[child_id] = mother_id;
	}
	input.close();
}

void PrePush::print_mother_child() {
	std::cout << "PrePush::mother_child"<< std::endl;
	for (auto &relation : mother_child) {
		std::cout << relation.first << ' ' << relation.second.size();
		for (auto &child : relation.second)
			std::cout << ' ' << child;
		std::cout << std::endl;
	}

}

void PrePush::print_child_mother() {
	std::cout << "PrePush::child_mother"<< std::endl;
	for (auto &relation : child_mother)
		std::cout << relation.first << ' ' << relation.second << std::endl;
}
