#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <json-c/json.h>
#include "getopt.h"
#include "netdb.h"
#include "libmysyslog.h"
#include "libmysyslog-json.h"

#define CONF_FILE   "/etc/myRPC/myRPC.conf"
#define USERS_FILE  "/etc/myRPC/users.conf"
#define LOG_PATH    "/var/log/myRPC-server.log"
#define TMP_OUT_FMT "/tmp/myRPC_%d.stdout"
#define TMP_ERR_FMT "/tmp/myRPC_%d.stderr"

// Читает порт и тип сокета из конфига
void read_config(int *port, int *is_stream) {
	FILE *f = fopen(CONF_FILE, "r");
	if (!f) { perror("fopen config"); exit(1); }
	char line[128];
	while (fgets(line, sizeof(line), f)) {
		if (strncmp(line, "port", 4)==0)
			*port = atoi(strchr(line,'=')+1);
		if (strncmp(line, "socket_type", 11)==0)
			*is_stream = strstr(line,"stream") ? 1 : 0;
	}
	fclose(f);
}

// Проверяет, есть ли login в USERS_FILE
int user_allowed(const char *login) {
	FILE *f = fopen(USERS_FILE,"r");
	if (!f) return 0;
	char buf[64];
	while (fgets(buf,sizeof(buf),f)) {
		buf[strcspn(buf,"\r\n")] = 0;
		if (strcmp(buf,login)==0) { fclose(f); return 1; }
	}
	fclose(f);
	return 0;
}

// Исполняет команду и собирает вывод в строку
char *run_command(const char *cmd, int *code) {
	char outfile[64], errfile[64];
	pid_t pid = getpid();
	snprintf(outfile,sizeof(outfile), TMP_OUT_FMT, pid);
	snprintf(errfile,sizeof(errfile), TMP_ERR_FMT, pid);

	char full[512];
	snprintf(full,sizeof(full),
		"%s > %s 2> %s", cmd, outfile, errfile);
	*code = system(full)>>8;  // код возврата

	// читаем stdout+stderr
	FILE *fo = fopen(outfile,"r"), *fe = fopen(errfile,"r");
	fseek(fo,0,SEEK_END); long no = ftell(fo); rewind(fo);
	fseek(fe,0,SEEK_END); long ne = ftell(fe); rewind(fe);

	char *res = malloc(no+ne+2);
	fread(res,1,no,fo);
	fread(res+no,1,ne,fe);
	res[no+ne] = 0;

	fclose(fo); fclose(fe);
	return res;
}

int main() {
	int port = 0, is_stream = 1;
	read_config(&port,&is_stream);
	json_object *jroot, *jlogin, *jcmd, *jresp;

	// логируем старт
	json_log("Starting myRPC-server", INFO, LOG_PATH);

	int sock = socket(AF_INET,
		is_stream?SOCK_STREAM:SOCK_DGRAM, 0);
	if (sock<0) { perror("socket"); exit(1); }

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(port)
	};
	bind(sock,(struct sockaddr*)&addr,sizeof(addr));
	if (is_stream) listen(sock, 5);

	while (1) {
		int client;
		struct sockaddr_in cli_addr;
		socklen_t clilen = sizeof(cli_addr);

		if (is_stream) {
			client = accept(sock,
				(struct sockaddr*)&cli_addr,&clilen);
		} else {
			char buf[1];
			recvfrom(sock, buf,1, MSG_PEEK,
				(struct sockaddr*)&cli_addr,&clilen);
			client = sock;
		}
		// читаем запрос
		char buf[4096];
		ssize_t len = (is_stream)
			? recv(client, buf,sizeof(buf)-1,0)
			: recvfrom(client, buf,sizeof(buf)-1,0,
				(struct sockaddr*)&cli_addr,&clilen);
		if (len<1) { if (is_stream) close(client); continue; }
		buf[len]=0;
		json_log(buf, INFO, LOG_PATH);

		// парсим JSON
		jroot = json_tokener_parse(buf);
		json_object_object_get_ex(jroot,"login",&jlogin);
		json_object_object_get_ex(jroot,"command",&jcmd);
		const char *login = json_object_get_string(jlogin);
		const char *cmd   = json_object_get_string(jcmd);

		// проверяем пользователя
		int allowed = user_allowed(login);
		int code = 1;
		char *result = NULL;
		if (allowed) {
			code = 0;
			result = run_command(cmd,&code);
		} else {
			result = strdup("User not allowed");
		}
		// формируем ответ
		jresp = json_object_new_object();
		json_object_object_add(jresp,
			"code", json_object_new_int(code));
		json_object_object_add(jresp,
			"result", json_object_new_string(result));
		const char *out = json_object_to_json_string(jresp);
		send(client, out, strlen(out), 0);

		json_log(out, INFO, LOG_PATH);

		free(result);
		json_object_put(jroot);
		json_object_put(jresp);
		if (is_stream) close(client);
	}

    	close(sock);
    	return 0;
}
