#include <stdarg.h>

#include "json_logger.hpp"

void json_logger::progress_tile(double progress) {
	fprintf(stderr, "{\"progress\":%3.1f}\n", progress);
}
