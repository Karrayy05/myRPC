#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>           // getlogin, close
#include <getopt.h>
#include <arpa/inet.h>        // sockaddr_in, htons, inet_pton
#include <sys/socket.h>       // socket, connect, send, recv
#include <json-c/json.h>
#include "libmysyslog.h"
#include "libmysyslog-json.h"

#define LOG_PATH "/var/log/myRPC-client.log"
#define BUF_SIZE 4096

static void usage(const char *p) {
	fprintf(stderr,
		"Usage: %s -h HOST -p PORT (-s | -d) -c COMMAND\n"
		"  -h, --host     сервер\n"
		"  -p, --port     порт\n"
		"  -s, --stream   TCP\n"
		"  -d, --dgram    UDP\n"
		"  -c, --command  bash-команда\n",
		p);
	exit(1);
}

int main(int argc, char **argv) {
	char *host = NULL, *cmd = NULL;
	int port = 0, is_tcp = -1, opt;

	static struct option longopts[] = {
		{"host",    required_argument, 0, 'h'},
		{"port",    required_argument, 0, 'p'},
		{"stream",  no_argument,       0, 's'},
		{"dgram",   no_argument,       0, 'd'},
		{"command", required_argument, 0, 'c'},
		{0,0,0,0}
	};
	while ((opt = getopt_long(argc, argv, "h:p:sdc:", longopts, NULL)) != -1) {
		switch (opt) {
			case 'h': host = optarg; break;
			case 'p': port = atoi(optarg); break;
			case 's': is_tcp = 1; break;
			case 'd': is_tcp = 0; break;
			case 'c': cmd = strdup(optarg); break;
			default: usage(argv[0]);
		}
	}
	if (!host || !port  || is_tcp<0 || !cmd) usage(argv[0]);

	// логируем старт
	json_log("Starting myRPC-client", INFO, LOG_PATH);

	// создаём JSON-запрос
	const char *user = getlogin() ? getlogin() : "unknown";
	json_object *jreq = json_object_new_object();
	json_object_object_add(jreq, "login",   json_object_new_string(user));
	json_object_object_add(jreq, "command", json_object_new_string(cmd));
	const char *req = json_object_to_json_string(jreq);
	json_log(req, INFO, LOG_PATH);

	// готовим сокет и адрес
	int sock = socket(AF_INET, is_tcp?SOCK_STREAM:SOCK_DGRAM, 0);
	if (sock<0) { perror("socket"); exit(1); }

	struct sockaddr_in srv = {
		.sin_family = AF_INET,
		.sin_port   = htons(port)
	};
	if (inet_pton(AF_INET, host, &srv.sin_addr) != 1) {
		fprintf(stderr, "Invalid host\n"); exit(1);
	}

	// отправка
	if (is_tcp) {
		if (connect(sock, (void*)&srv, sizeof(srv))<0) { perror("connect"); exit(1); }
		send(sock, req, strlen(req), 0);
	} else {
        	sendto(sock, req, strlen(req), 0, (void*)&srv, sizeof(srv));
	}

	// приём (один вызов — достаточно, т.к. ответ в один пакет)
	char buf[BUF_SIZE];
	ssize_t n = is_tcp
		? recv(sock, buf, BUF_SIZE-1, 0)
		: recvfrom(sock, buf, BUF_SIZE-1, 0, NULL, NULL);
	if (n>0) {
		buf[n] = '\0';
		printf("%s\n", buf);
		json_log(buf, INFO, LOG_PATH);
	}

	// финалка
	close(sock);
	free(cmd);
	json_object_put(jreq);
	return 0;
}
