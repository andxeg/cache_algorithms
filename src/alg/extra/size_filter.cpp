#include "size_filter.h"

#include <time.h>
#include <math.h>
#include <stdlib.h>


SizeFilter::SizeFilter(Config & config) {
	enable = (config.get_int_by_name("SIZE_FILTER_ENABLE") == 1);
	window = config.get_int_by_name("SIZE_FILTER_WINDOW");
	threshold = config.get_int_by_name("SIZE_FILTER_INITIAL_TRESHOLD");
}

bool SizeFilter::admit_object(	std::string & id, size_t & size, 
								HistoryManager & history_manager) {
	
	if (enable == false) return true;

	/* filter by size */
	srand((unsigned)(time(0)));
	float p = rand()/(float(RAND_MAX)+1);
	return (p <= exp(-size/(float)threshold) ) ? true : false;
}

void SizeFilter::update_threshold(HistoryManager &history_manager, 
									ContentSizes & content_sizes) {
	// TODO
	return;
}
