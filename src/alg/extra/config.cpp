#include "config.h"

#include <string>
#include <fstream>
#include <iostream>
#include "timestamps.h"


Config::Config(const std::string &filename) {
	std::fstream input(filename, std::fstream::in);
	if (!input.is_open()) {
		std::cerr << "[ERROR] Error while opening file " 
					<< filename << std::endl;
		return;
	}

	std::string line;
	while (true) {
		input >> line;
		if (input.eof()  && line == "") break;

		std::size_t pos = line.find('=');
		if (pos == std::string::npos) {
			std::cerr << "[ERROR] Error while read file " 
					<< filename << std::endl;	
			return;
		}

		config_map[line.substr(0, pos)] = line.substr(pos+1);
		line = std::string("");
	}
	input.close();
}

int Config::get_int_by_name(const std::string &name) {
	auto it = config_map.find(name);
	if (it == config_map.end()) {
		std::cerr << "[ERROR] There is not " << name << " in config file" << std::endl;
		return 0;
	}
	return convertFromStringTo<int>(it->second);
}

float Config::get_float_by_name(const std::string &name) {
	auto it = config_map.find(name);
	if (it == config_map.end()) {
		std::cerr << "[ERROR] There is not " << name << " in config file" << std::endl;
		return 0.0;
	}
	return convertFromStringTo<float>(it->second);
}

std::string Config::get_str_by_name(const std::string &name) {
	auto it = config_map.find(name);
	if (it == config_map.end()) {
		std::cerr << "[ERROR] There is not " << name << " in config file" << std::endl;
		return "";
	}
	return config_map[name];
}

void Config::print() {
	for (auto& elem : config_map) {
		std::cout << elem.first << ' ' << elem.second << std::endl;
	}
}
