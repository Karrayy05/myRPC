#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "libmysyslog.h"

#define CONFIG_FILE "/etc/mysyslog/mysyslog.cfg"

volatile sig_atomic_t stop;

void handle_signal(int signal) {
	stop = 1;
}

void read_config(char* path, int* format, int* driver) {
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	fscanf(file, "path=%s\nformat=%d\ndriver=%d\n", path, format, driver);
	fclose(file);
}

int main() {
	signal(SIGTERM, handle_signal);
	signal(SIGINT, handle_signal);

	char path[256];
	int format;
	int driver;
	read_config(CONFIG_FILE, &format, &driver);

	while (!stop) {
		mysyslog("Daemon log entry", INFO, driver, format, path);
		sleep(5);
	}

	return 0;
}
