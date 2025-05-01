#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <json-c/json.h>
#include "libmysyslog.h"
#include "libmysyslog-json.h"

#define CONF_FILE   "/etc/myRPC/myRPC.conf"
#define USERS_FILE  "/etc/myRPC/users.conf"
#define LOG_PATH    "/var/log/myRPC-server.log"
#define BUF_SIZE    4096

// Читаем порт и тип сокета
void read_config(int *port, int *is_stream) {
	FILE *f = fopen(CONF_FILE, "r");
	if (!f) { perror("fopen config"); exit(1); }
	char key[32], val[32];
	while (fscanf(f, "%31[^=]=%31s\n", key, val) == 2) {
		if (strcmp(key, "port")==0) 
			*port = atoi(val);
		if (strcmp(key, "socket_type")==0)
			*is_stream = (strcmp(val,"stream")==0);
	}
	fclose(f);
}

// Проверяем разрешён ли логин
int user_allowed(const char *login) {
	FILE *f = fopen(USERS_FILE, "r");
	if (!f) return 0;
	char buf[64];
	while (fgets(buf,sizeof(buf),f)) {
        	buf[strcspn(buf,"\r\n")] = 0;
		if (strcmp(buf, login)==0) { fclose(f); return 1; }
	}
	fclose(f);
	return 0;
}

// Запускаем команду через popen и возвращаем stdout+stderr
char *run_command(const char *cmd, int *code) {
	char full[512];
	snprintf(full, sizeof(full), "%s 2>&1", cmd);

	FILE *fp = popen(full, "r");
	if (!fp) {
		*code = 1;
		return strdup("popen failed");
	}

	size_t cap = 1024, len = 0;
	char *out = malloc(cap);
	int c;
	while ((c = fgetc(fp)) != EOF) {
		if (len+1 >= cap) out = realloc(out, cap *= 2);
		out[len++] = c;
	}
	out[len] = '\0';

	int status = pclose(fp);
	*code = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
	return out;
}

// Обрабатываем один запрос от client_fd
void handle_client(int client_fd) {
	char buf[BUF_SIZE];
	ssize_t r = recv(client_fd, buf, BUF_SIZE-1, 0);
	if (r <= 0) return;
	buf[r] = '\0';

	// Логируем входящий JSON
	json_log(buf, INFO, LOG_PATH);

	// Парсим JSON
	json_object *jreq = json_tokener_parse(buf);
	const char *login = json_object_get_string(
		json_object_object_get(jreq, "login"));
	const char *command = json_object_get_string(
		json_object_object_get(jreq, "command"));

	int code = 1;
	char *result;
	if (user_allowed(login)) {
		result = run_command(command, &code);
	} else {
		result = strdup("User not allowed");
	}

	// Формируем JSON-ответ
	json_object *jresp = json_object_new_object();
	json_object_object_add(jresp, "code",
		json_object_new_int(code));
	json_object_object_add(jresp, "result",
		json_object_new_string(result));
	const char *out = json_object_to_json_string(jresp);

	// Отправляем и логируем ответ
	send(client_fd, out, strlen(out), 0);
	json_log(out, INFO, LOG_PATH);

	free(result);
	json_object_put(jreq);
	json_object_put(jresp);
}

int main(void) {
	int port = 0, is_stream = 1;
	read_config(&port, &is_stream);
	json_log("Starting myRPC-server", INFO, LOG_PATH);

	int sock = socket(AF_INET,
		is_stream ? SOCK_STREAM : SOCK_DGRAM, 0);
	if (sock < 0) { perror("socket"); exit(1); }

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(port)
	};
	if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("bind"); exit(1);
	}
	if (is_stream) listen(sock, 5);

	while (1) {
		int client;
		struct sockaddr_in cli;
		socklen_t len = sizeof(cli);

		if (is_stream) {
			client = accept(sock,
				(struct sockaddr*)&cli, &len);
		} else {
			char tmp;
			recvfrom(sock, &tmp, 1, MSG_PEEK,
				(struct sockaddr*)&cli, &len);
			client = sock;
		}

		handle_client(client);
			if (is_stream) close(client);
	}

	close(sock);
	return 0;
}
