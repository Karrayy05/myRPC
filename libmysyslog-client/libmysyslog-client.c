#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "libmysyslog.h"

int main(int argc, char *argv[]) {
	int opt;
	char *msg = NULL;
	int level = INFO;
	int driver = 0;
	int format = 0;
	char *path = NULL;

	while ((opt = getopt(argc, argv, "m:l:d:f:p:")) != -1) {
		switch (opt) {
			case 'm': msg = optarg; break;
			case 'l': level = atoi(optarg); break;
			case 'd': driver = atoi(optarg); break;
			case 'f': format = atoi(optarg); break;
			case 'p': path = optarg; break;
			default:
				fprintf(stderr, "Usage: %s -m message -l level -d driver -f format -p path\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	if (msg == NULL || path == NULL) {
		fprintf(stderr, "Message and path are required\n");
		exit(EXIT_FAILURE);
	}

	if (mysyslog(msg, level, driver, format, path) != 0) {
		fprintf(stderr, "Failed to write log\n");
		exit(EXIT_FAILURE);
	}

	return 0;
}
