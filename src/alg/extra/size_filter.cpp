#include "size_filter.h"

#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <iostream>


SizeFilter::SizeFilter(Config & config) {
	srand((unsigned)(time(0)));
	enable = (config.get_int_by_name("SIZE_FILTER_ENABLE") == 1);
	window = config.get_int_by_name("SIZE_FILTER_WINDOW");
	threshold = config.get_float_by_name("SIZE_FILTER_INITIAL_TRESHOLD");
}

bool SizeFilter::admit_object(	std::string &id, const float &size, 
								HistoryManager &history_manager) {
	
	if (enable == false) return true;

	/* filter by size */
	float p = rand()/(float(RAND_MAX)+1);
	std::cout << "probability -> " << p << ' '
	<< "exp(-size/threshold) -> " << exp(-size/threshold)
	<< " size -> " << size
	<< " threshold -> " << threshold
	<< std::endl;
	return (p <= exp(-size/threshold) ) ? true : false;
}

void SizeFilter::update_threshold(HistoryManager &history_manager, 
									ContentSizes & content_sizes) {
	/* update threshold */
	threshold = enable ? history_manager.get_average_size_in_window
							(window, content_sizes) : threshold;
}

float SizeFilter::get_threshold() {
	return threshold;
}
