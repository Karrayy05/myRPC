#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <json-c/json.h>
#include "libmysyslog.h"

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
		if (strncmp(line,"port",4)==0)
			*port = atoi(strchr(line,'=')+1);
		if (strncmp(line,"socket_type",11)==0)
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

// Исполняет команду и собирает вывод
char *run_command(const char *cmd, int *code) {
	char out[64], err[64], full[512];
	pid_t pid = getpid();
	snprintf(out,sizeof(out),TMP_OUT_FMT,pid);
	snprintf(err,sizeof(err),TMP_ERR_FMT,pid);
	snprintf(full,sizeof(full),"%s > %s 2> %s", cmd,out,err);
	*code = system(full)>>8;

	FILE *fo = fopen(out,"r"), *fe = fopen(err,"r");
	fseek(fo,0,SEEK_END); long no=ftell(fo); rewind(fo);
	fseek(fe,0,SEEK_END); long ne=ftell(fe); rewind(fe);

	char *res = malloc(no+ne+1);
	fread(res,1,no,fo);
	fread(res+no,1,ne,fe);
	res[no+ne] = '\0';
	fclose(fo); fclose(fe);
	return res;
}

int main() {
	int port=0, is_stream=1;
	read_config(&port,&is_stream);

	json_log("Starting myRPC-server", INFO, LOG_PATH);

	int sock = socket(AF_INET,
        	is_stream?SOCK_STREAM:SOCK_DGRAM,0);
	if (sock<0) { perror("socket"); exit(1); }

	struct sockaddr_in addr = {
		.sin_family=AF_INET,
		.sin_addr.s_addr=INADDR_ANY,
		.sin_port=htons(port)
	};
	if (bind(sock,(struct sockaddr*)&addr,sizeof(addr))<0) {
		perror("bind"); exit(1);
	}
	if (is_stream) listen(sock,5);

	while (1) {
		int client;
		struct sockaddr_in cli;
		socklen_t len = sizeof(cli);

		if (is_stream) {
			client = accept(sock,(struct sockaddr*)&cli,&len);
		} else {
			char tmp;
			recvfrom(sock,&tmp,1,MSG_PEEK,
				(struct sockaddr*)&cli,&len);
			client = sock;
		}

		char buf[4096];
		ssize_t r = is_stream
			? recv(client,buf,sizeof(buf)-1,0)
			: recvfrom(client,buf,sizeof(buf)-1,0,
				(struct sockaddr*)&cli,&len);
		if (r<1) { if (is_stream) close(client); continue; }
		buf[r]=0;
		json_log(buf, INFO, LOG_PATH);

		// парсим JSON
		json_object *jroot = json_tokener_parse(buf);
		json_object *jlogin, *jcmd;
		json_object_object_get_ex(jroot,"login",&jlogin);
		json_object_object_get_ex(jroot,"command",&jcmd);
		const char *login = json_object_get_string(jlogin);
		const char *cmd   = json_object_get_string(jcmd);

		int code=1;
		char *result;
		if (user_allowed(login)) {
			result = run_command(cmd,&code);
		} else {
			result = strdup("User not allowed");
		}

		// формируем и отправляем ответ
		json_object *jresp = json_object_new_object();
		json_object_object_add(jresp,"code",
			json_object_new_int(code));
		json_object_object_add(jresp,"result",
			json_object_new_string(result));
		const char *out = json_object_to_json_string(jresp);

		send(client,out,strlen(out),0);
		json_log(out, INFO, LOG_PATH);

		free(result);
		json_object_put(jroot);
		json_object_put(jresp);
		if (is_stream) close(client);
	}

	close(sock);
	return 0;
}
