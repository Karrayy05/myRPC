#include "libmysyslog.h"

int text_log(const char* msg, int level, const char* path) {
	return mysyslog(msg, level, 0, 0, path);
}
