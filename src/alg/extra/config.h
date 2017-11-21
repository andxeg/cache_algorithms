#pragma once

#include <unordered_map>


class Config {
	typedef std::unordered_map<std::string, std::string> ConfigMap;
public:
	Config(const std::string &filename);
	int get_int_by_name(const std::string &name);
	float get_float_by_name(const std::string &name);
	std::string get_str_by_name(const std::string &name);
	void print();
	
private:
	ConfigMap config_map;
};

