#pragma once 

#include "defs.h"
#include "config.h"
#include "history_manager.h"


class SizeFilter {
public:
	SizeFilter() {};
	SizeFilter(Config &);
	bool admit_object(std::string &id, const float &size, HistoryManager &history_manager);
	void update_threshold(HistoryManager &history_manager, ContentSizes &content_sizes);
	float get_threshold();
private:
	int type;
	int window;
	bool reversed_size;
	bool enable;
	float threshold;
};
