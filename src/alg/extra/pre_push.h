#pragma once

#include "defs.h"
#include "config.h"


class PrePush {
	typedef std::unordered_map<std::string, VecStr> MotherChildMap;
	typedef std::unordered_map<std::string, std::string> ChildMotherMap;
public:
	PrePush(Config &config);
	VecStr get_pre_push_list(VecStr &cache_hot_objects);
	void read_mother_child(const std::string &mother_child_file);
	void read_child_mother(const std::string &child_mother_file);
	void print_mother_child();
	void print_child_mother();
private:
	int window;
	float hist_hot_objects;
	MotherChildMap mother_child;
	ChildMotherMap child_mother;
};
