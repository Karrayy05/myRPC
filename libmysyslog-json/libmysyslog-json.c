#include "libmysyslog.h"

int json_log(const char* msg, int level, const char* path) {
	return mysyslog(msg, level, 1, 1, path);
}
