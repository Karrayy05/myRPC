#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>         // getlogin, close
#include <getopt.h>
#include <arpa/inet.h>      // sockaddr_in, htons, inet_pton
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "libmysyslog.h"
#include "libmysyslog-json.h"

#define LOG_PATH "/var/log/myRPC-client.log"

static void usage(const char *prog) {
	fprintf(stderr,
		"Usage: %s -h HOST -p PORT (-s | -d) -c COMMAND\n"
		"  -h, --host     IP-адрес сервера\n"
		"  -p, --port     порт сервера\n"
		"  -s, --stream   TCP-сокет\n"
		"  -d, --dgram    UDP-сокет\n"
		"  -c, --command  bash-команда в кавычках\n",
		prog);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	char *host    = NULL;
	int   port    = 0;
	int   is_tcp  = -1;        // 1 — stream, 0 — dgram
	char *command = NULL;
	int   opt;

	// Опции
	static struct option long_opts[] = {
		{"host",    required_argument, 0, 'h'},
		{"port",    required_argument, 0, 'p'},
		{"stream",  no_argument,       0, 's'},
		{"dgram",   no_argument,       0, 'd'},
		{"command", required_argument, 0, 'c'},
		{0,0,0,0}
	};

	while ((opt = getopt_long(argc, argv, "h:p:sdc:", long_opts, NULL)) != -1) {
		switch (opt) {
			case 'h': host    = optarg;         break;
			case 'p': port    = atoi(optarg);   break;
			case 's': is_tcp  = 1;              break;
			case 'd': is_tcp  = 0;              break;
			case 'c': command = strdup(optarg); break;
			default: usage(argv[0]);
		}
	}
	if (!host || port <= 0 || is_tcp < 0 || !command)
		usage(argv[0]);

	// Логируем запуск клиента
	json_log("Starting myRPC-client", INFO, LOG_PATH);

	// Получаем имя пользователя
	char *user = getlogin();
	if (!user) user = "unknown";

	// Формируем JSON-запрос
	char req[1024];
	snprintf(req, sizeof(req),
		"{\"login\":\"%s\",\"command\":\"%s\"}", user, command);

	// Создаём сокет
	int sock = socket(AF_INET, is_tcp? SOCK_STREAM : SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("socket");
		return 1;
	}

	struct sockaddr_in srv = {
		.sin_family = AF_INET,
		.sin_port   = htons(port)
	};
	if (inet_pton(AF_INET, host, &srv.sin_addr) != 1) {
		fprintf(stderr, "Invalid host: %s\n", host);
		return 1;
	}

	// Отправляем запрос
	if (is_tcp) {
		if (connect(sock, (struct sockaddr*)&srv, sizeof(srv)) < 0) {
			perror("connect");
			return 1;
		}
		send(sock, req, strlen(req), 0);
	} else {
		sendto(sock, req, strlen(req), 0,
			(struct sockaddr*)&srv, sizeof(srv));
	}
	json_log(req, INFO, LOG_PATH);

	// Принимаем ответ
	char buf[4096];
	ssize_t r;
	if (is_tcp) {
		while ((r = recv(sock, buf, sizeof(buf)-1, 0)) > 0) {
			buf[r] = '\0';
			printf("%s", buf);
		}
	} else {
		socklen_t len = sizeof(srv);
		r = recvfrom(sock, buf, sizeof(buf)-1, 0,
			(struct sockaddr*)&srv, &len);
		if (r > 0) {
			buf[r] = '\0';
			printf("%s", buf);
		}
	}
	json_log(buf, INFO, LOG_PATH);

	close(sock);
	free(command);
	return 0;
}
