#pragma once 

#include "defs.h"
#include "config.h"
#include "history_manager.h"

class SizeFilter {
public:
	SizeFilter(Config &);
	bool admit_object(std::string &, size_t &, HistoryManager &);
	void update_threshold(HistoryManager &, ContentSizes &);
private:
	bool enable;
	int window;
	int threshold;
};
