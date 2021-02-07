#include <stdio.h>
#include <stdlib.h>
#include <minIni.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <mysql.h>
#include <kakao.h>
#include <mystdlib.h>
#include <customsocket.h>
#define SECOND_IN_MICRO 1000000//1 секунда в микросекундах
#define PATH_BOBSERVER_CONF "/srv/bobserver/conf.ini"

char PATHS [25][MAX_PATH];
enum paths
{
	PATH_COMPANION_FILES,
	PATH_COMPANION_PHOTOS,
	PATH_PARTY_FILES,
	PATH_PARTY_PHOTOS,
	PATH_USER_AVATARS,
	PATH_PARTY_AVATARS,
	PATH_SERVERLOG,
	PATH_AUTH_LOG,
	PATH_USER_LOGS
};
const char *text_paths[]=
{
	"path_companion_files",
	"path_companion_photos",
	"path_party_files",
	"path_party_photos",
	"path_user_avatars",
	"path_party_avatars",
	"path_serverlog",
	"path_auth_log",
	"path_user_logs",
	""
};
unsigned long int LIMITS[25];
enum limits
{
	FPS_ACCEPT_USERS//сколько человек в секунду может подсоединяться к серверу
};
const char * text_limits[]=
{
	"fps_accept_users",
	""
};
char START_SERVER[3][256];
enum start_server
{
	IP,
	PORT
};
const char *  text_start_server[]=
{
	"ip",
	"port",
	""
};
char MYSQL_CONNECT [25][256];
enum mysql_connect
{
	DOMAIN,
	USER,
	PASS,
	DB
};
const char *text_mysql_connect[]=
{
	"domain",
	"user",
	"pass",
	"db",
	""
};
#define sizearray(a)  (sizeof(a) / sizeof((a)[0]))
int main_sock;
FILE *server_log;
void get_bobserver_conf();
void *fork_user(void*a);
void signal_handler(int sig);
void create_server_files(void);
int check_unique_nickname(MYSQL *conn,const char* nickname);
int check_password(MYSQL *conn,const char *nickname,const char *password);
char* get_id(MYSQL *conn,const char* nickname);
void user_online(some_data *data,char *people_id);
int block_user(some_data *data,char *from_id,char *to_id);
void party_writing(some_data *data,char *party_id);
int user_writing(some_data *data,char *from_id,char *to_id);
int allow_send_data_to_companion(some_data *my_data,char *to_id);//при отправке каких либо данных(файлов или текста или изображения) проверяем можно ли отправлять их пользователю и если можно выполняем какие либо другие действия 
int allow_send_data_to_party(some_data *my_data,char *party_id);//при отправке каких либо данных(файлов или текста или изображения) проверяем можно ли отправлять их группе и если можно выполняем какие либо другие действия 
int main(int argc, char *argv[])
{
	signal(SIGINT,signal_handler);
	//create_server_files();
	get_bobserver_conf();
	server_log=fopen(PATHS[PATH_SERVERLOG],"a");
	if(server_log==NULL) {
		fprintf(stderr, "Не могу открыть serverlog\n" ); fflush(stderr);
		return 0;
	}
	pthread_t thread;
	struct sockaddr_in client;
	int client_sock;
	main_sock = socket(AF_INET,SOCK_STREAM,0);
	if(!main_sock) {
		perror("socket");
		return 0;
	}
	char ipbuffer[256];
	strcpy(ipbuffer,START_SERVER[IP]);//запуск сервера на ip вашей локальной сети, например 192.168.1.66
	//char hostname[512];
	//gethostname(hostname, sizeof(hostname)); //получаем имя хоста
	//struct hostent *host_entry=gethostbyname(hostname);//получаем ip хоста
	//struct in_addr iphost = *((struct in_addr*) host_entry->h_addr_list[0]);
	//strcpy(ipbuffer,inet_ntoa(iphost));

	char opt;
	while( (opt=getopt(argc,argv,"l"))!=-1 ) {
		switch(opt) {
			case 'l'://если в аргументах -l, то запускаем сервер на localhost
				strcpy(ipbuffer,"127.0.0.1");
				break;
			case '?':
				exit(1);
		}
	}

	fprintf(stdout,"My ip: %s\n",ipbuffer);
	bind_to_addr(main_sock,ipbuffer,atoi(START_SERVER[PORT]));
	if(listen(main_sock,10)==-1)
	{
		perror("listen");
		close(main_sock);
		return 0;
	}
	int size_client=sizeof(client);
	while(1)
	{
		usleep(LIMITS[FPS_ACCEPT_USERS]);
		if((client_sock=accept(main_sock,(struct sockaddr*)&client,(int*)&size_client))==-1)
		{
			perror("accept");
			close(main_sock);
			return 0;
		}
		//pthread_create(&thread,NULL,fork_user,(void*)&client_sock);/*создаем поток*/
		switch(fork())
		{
			case -1:
				perror("fork"); fflush(stderr);
				break;
			case 0:
				fork_user((void*)&client_sock);
				exit(1);
			default:
				fprintf(stdout,"Клиент ip: %s port: %d\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port)); fflush(stdout);
				break;
		}
		continue;
	}
	return 1;
}
void get_bobserver_conf()
{
	int ret=0;
	/*получаем пути к различным файлам или каталогам*/
	for(int i=0;1;++i) {
		if(!ini_gets("paths", text_paths[i], NULL, PATHS[i], sizeof(*PATHS), PATH_BOBSERVER_CONF)) break;
	}

	/*получаем ограничения*/
	for(int i=0;1;++i) {
		if((ret=ini_getl("limits", text_limits[i], -1, PATH_BOBSERVER_CONF))<0) break;
		if(i==FPS_ACCEPT_USERS) {
			LIMITS[i]=SECOND_IN_MICRO/ret;//SECOND_IN_MICRO/100 - 100 человек в секунду
		}
		else {
			LIMITS[i]=ret;
		}
	} 

	/*получаем данные для старта сервера kakaobob*/
	for(int i=0;1;++i) {
		if(!ini_gets("start_server", text_start_server[i], NULL, START_SERVER[i], sizeof(*START_SERVER), PATH_BOBSERVER_CONF)) break;
	}

	/*получаем данные для подключения к mysql серверу*/
	for(int i=0;1;++i) {
		if(!ini_gets("mysql_connect", text_mysql_connect[i], NULL, MYSQL_CONNECT[i], sizeof(*MYSQL_CONNECT), PATH_BOBSERVER_CONF)) break;
	}
}
void signal_handler(int sig)
{
	if(sig==SIGINT)
	{
		printf("SIGNAL_HANDLER: SIGINT\n");
		close(main_sock);
		fclose(server_log);
		exit(SIGINT);
	}
}
void *fork_user(void*a)
{ 
	int sock=*((int *) a);
	some_data my_data;
	FILE *log, *auth_log;
	MYSQL_ROW row,row2;
	MYSQL_RES *result,*result2;
	char recv_buf[MAX_BUF];
	char query_buf[MAX_QUERY_BUF];
	char log_path[256];
	char limit[MYSQL_INT];
	char signal[20];
	char dialog_type[20];
	unsigned char *hash;
	info my_info;//мои данные
	info people_info;//данные кого-то другого
	info message_info;//данные сообщения
	info party_info;
	MYSQL *conn;
	short int ret;
	unsigned long int i;
	_Bool flag=false;
	_Bool flag_admin=false;
	_Bool flag_party_user=false;
	/*ждем от пользователя отправки размера версии своего приложения*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		fprintf(stdout,"Клиент отключился\n" ); fflush(stdout);
		close(sock);
		return NULL;
	}
	else if(ret<0){
		fprintf(server_log,"ERROR RECV: size of version(%d)\n",ret ); fflush(log);
		return NULL;//ждем следующий сигнал
	}
	/*ждем от пользователя отправки версии своего приложения*/
	if( (ret=recvall(sock,recv_buf,ntohl(sz),MSG_WAITALL))==0 ) {
		fprintf(stdout,"Клиент отключился\n" ); fflush(stdout);
		close(sock);
		return NULL;
	}
	else if(ret<0){
		fprintf(server_log,"ERROR RECVALL: version(%d)\n",ret ); fflush(log);
		return NULL;
	}
	fprintf(stderr,"Version: %s\n",recv_buf );
	conn=mysql_init(0);
	if(conn==NULL)
	{
		fprintf(server_log,"Failed initilization: conn Error: %s\n",mysql_error(conn)); fflush(server_log);
		close(sock);
		return NULL;
	}
	if(mysql_real_connect(conn,MYSQL_CONNECT[DOMAIN],MYSQL_CONNECT[USER],MYSQL_CONNECT[PASS],MYSQL_CONNECT[DB],0,0,0)==NULL)
	{
		fprintf(server_log, "MYSQL_REAL_CONNECT: %s\n",mysql_error(conn)); fflush(server_log);
		close(sock);
		return NULL;
	}

	if(mysql_query(conn,"SELECT version FROM versions WHERE actual=\"y\"")!=0)
	{
		fprintf(server_log, "fork_user: %s\n",mysql_error(conn)); fflush(server_log);
		close(sock);
		return NULL;
	}
	result=mysql_store_result(conn);
	if(mysql_num_rows(result)>0)
	{	
		while((row=mysql_fetch_row(result)))
		{
			if(strcmp(recv_buf,row[0])==0)
			{
				sendall(sock,"y",sizeof("y"));//разрешаем открыть приложение
				printf("Y: version: %s\n",recv_buf); fflush(stdout);
				flag=false;
				break;
			}
		}
		if(flag==true)
		{
			sendall(sock,"n",sizeof("n"));//не разрешаем открыть приложение(нужно обновиться чтобы оно открылось)
			printf("N: version: %s\n",recv_buf); fflush(stdout);
			close(sock);
			mysql_close(conn);
			mysql_free_result(result);
			mysql_free_result(result2);
			return NULL;
		}
	}
	else
	{
		sendall(sock,"n",sizeof("n"));//не разрешаем открыть приложение(нужно обновиться чтобы оно открылось)
		printf("N: version: %s\n",recv_buf); fflush(stdout);
		close(sock);
		mysql_close(conn);
		mysql_free_result(result);
		mysql_free_result(result2);
		return NULL;
	}

	/*Получаем размер никнейма*/
	if((ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0) {//получаем id пользователя
		fprintf(stdout,"Клиент отключился\n" ); fflush(stdout);
		close(sock);
		mysql_close(conn);
		mysql_free_result(result);
		mysql_free_result(result2);
		return NULL;
	}
	else if(ret<0) {
		fprintf(server_log,"ERROR: RECV size of login(%d)\n",ret ); fflush(server_log);
		close(sock);
		mysql_close(conn);
		mysql_free_result(result);
		mysql_free_result(result2);
		return NULL;
	}
	/*Получаем никнейм*/
	if((ret=recvall(sock,my_info.nickname,ntohl(sz),MSG_WAITALL))==0) {//получаем id пользователя
		fprintf(stdout,"Клиент отключился\n" ); fflush(stdout);
		close(sock);
		mysql_close(conn);
		mysql_free_result(result);
		mysql_free_result(result2);
		return NULL;
	}
	else if(ret<0) {
		fprintf(server_log,"ERROR: RECVALL login(%d)\n",ret ); fflush(server_log);
		close(sock);
		mysql_close(conn);
		mysql_free_result(result);
		mysql_free_result(result2);
		return NULL;
	}

	/*Получаем размер пароля*/
	if((ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0) {//получаем id пользователя
		fprintf(stdout,"Клиент отключился\n" ); fflush(stdout);
		close(sock);
		mysql_close(conn);
		mysql_free_result(result);
		mysql_free_result(result2);
		return NULL;
	}
	else if(ret<0) {
		fprintf(server_log,"ERROR: RECV size of password(%d)\n",ret ); fflush(server_log);
		close(sock);
		mysql_close(conn);
		mysql_free_result(result);
		mysql_free_result(result2);
		return NULL;
	}
	/*Получаем пароль*/
	if((ret=recvall(sock,my_info.password,ntohl(sz),MSG_WAITALL))==0) {//получаем id пользователя
		fprintf(stdout,"Клиент отключился\n" ); fflush(stdout);
		close(sock);
		mysql_close(conn);
		mysql_free_result(result);
		mysql_free_result(result2);
		return NULL;
	}
	else if(ret<0) {
		fprintf(server_log,"ERROR: RECVALL password(%d)\n",ret ); fflush(server_log);
		close(sock);
		mysql_close(conn);
		mysql_free_result(result);
		mysql_free_result(result2);
		return NULL;
	}
	auth_log=fopen(PATHS[PATH_AUTH_LOG],"w+");
	//printf("passwithhash %s\n",my_info.password ); fflush(stdout);
	strcpy(my_info.password,my_sha256(my_info.password,strlen(my_info.password)));
	//printf("passwithhash %s\n",my_info.password ); fflush(stdout);
	sprintf(query_buf, "SELECT count(*) FROM users WHERE nickname=\"%s\" AND password=\"%s\" ",my_info.nickname,my_info.password );
	/*проверяем есть ли данный пользователь*/
	if(mysql_query(conn,query_buf)!=0)
	{//обрабатываем ошибку
		fprintf(auth_log,"ERROR: mysql query user pass and login(%d)\n",ret ); fflush(auth_log);
		close(sock);
		fclose(auth_log);
		mysql_close(conn);
		mysql_free_result(result);
		mysql_free_result(result2);
		return NULL;
	}
	result=mysql_store_result(conn);
	row=mysql_fetch_row(result);
	if(!atoi(row[0])) //если логин или пароль неверные
	{
		/*отправляем отрицательный ответ*/
		sz=htonl(0);
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(auth_log, "ERROR AUTH : SEND  answer no(%d)\n",ret ); fflush(auth_log);
			close(sock);
			fclose(auth_log);
			mysql_close(conn);
			mysql_free_result(result);
			mysql_free_result(result2);
			return NULL;//ждем следующий сигнал
		}

		while( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL)) )//принимаем сигнал
		{
			/*принимаем тип диалога*/
			if( (ret=recvall(sock,signal,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(auth_log,"ERROR %s: RECVALL signal(%d)\n",kakao_signal[DARK_ERROR],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			if(strcmp(signal,kakao_signal[GET_REG_FORM])==0)
			{

				/*принимаем размер никнейма*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(auth_log,"ERROR %s: RECV size of nickname(%d)\n",kakao_signal[GET_REG_FORM],ret ); fflush(auth_log);
					continue;//ждем следующий сигнал
				}
				/*принимаем никнейм*/
				if( (ret=recvall(sock,my_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(auth_log,"ERROR %s: RECVALL nickname(%d)\n",kakao_signal[GET_REG_FORM],ret ); fflush(auth_log);
					continue;//ждем следующий сигнал
				}
				if(check_unique_nickname(conn,my_info.nickname)==0)// если никнейм не уникален
				{
					/*отправляем размер ответа(уникален или не уникален никейм)*/
					sz=htonl(get_str_size("n"));
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(auth_log, "ERROR %s : SEND size of unique nickname answer no\"n\"(%d)\n",kakao_signal[GET_REG_FORM],ret ); fflush(auth_log);
						continue;//ждем следующий сигнал
					}
					/*отправляем ответ (уникален или не уникален никейм)*/
					if( !(ret=sendall(sock,"n",ntohl(sz))) ) {
						fprintf(auth_log, "ERROR %s : SENDALL unique nickname answer no\"n\"(%d)\n",kakao_signal[GET_REG_FORM],ret ); fflush(auth_log);
						continue;//ждем следующий сигнал
					}
					continue;//ждем следующий сигнал
				}
				else
				{
					/*отправляем размер ответа(уникален или не уникален никейм)*/
					sz=htonl(get_str_size("y"));
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(auth_log, "ERROR %s : SEND size of unique nickname answer yes\"y\"(%d)\n",kakao_signal[GET_REG_FORM],ret ); fflush(auth_log);
						continue;//ждем следующий сигнал
					}
					/*отправляем ответ (уникален или не уникален никейм)*/
					if( !(ret=sendall(sock,"y",ntohl(sz))) ) {
						fprintf(auth_log, "ERROR %s : SENDALL unique nickname answer yes\"y\"(%d)\n",kakao_signal[GET_REG_FORM],ret ); fflush(auth_log);
						continue;//ждем следующий сигнал
					}

					/*принимаем размер пароля*/
					if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
					else if(ret<0){
						fprintf(auth_log,"ERROR %s: RECV size of password(%d)\n",kakao_signal[GET_REG_FORM],ret ); fflush(auth_log);
						continue;//ждем следующий сигнал
					}
					/*принимаем пароль*/
					if( (ret=recvall(sock,my_info.password,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
					else if(ret<0){
						fprintf(auth_log,"ERROR %s: RECVALL password(%d)\n",kakao_signal[GET_REG_FORM],ret ); fflush(auth_log);
						continue;//ждем следующий сигнал
					}

				}
				mysql_query(conn,"START TRANSACTION");
				strcpy(my_info.password,my_sha256(my_info.password,strlen(my_info.password)));//хешируем пароль
				//если пароль введен соответствуя всем правилам, никнейм уникален и пользователь создан то заносим все данные в базу и завершаем регистрацию
				sprintf(query_buf,"INSERT INTO users(nickname,password,created_at,path_avatar) VALUES (\"%s\",\"%s\",NOW(),\"\")",my_info.nickname,my_info.password);
				if(mysql_query(conn,query_buf)!=0)// Записываем данные пользователя в базу данных
				{
					mysql_query(conn,"ROLLBACK");
					fprintf(auth_log, "ERROR: %s\t%s\n",kakao_signal[GET_REG_FORM], mysql_error(conn));//записываем ошибку в лог
					fflush(auth_log);
					continue;//ждем следующий сигнал
				}
				strcpy(my_info.id,get_id(conn,my_info.nickname));//записываем id пользователя
				sprintf(query_buf,"INSERT INTO errors(user_id) VALUES (\"%s\")",my_info.id);
				if(mysql_query(conn,query_buf)!=0)// создаем запись с пользователем чтобы отправлять ему ошибки
				{
					mysql_query(conn,"ROLLBACK");
					fprintf(auth_log, "ERROR: %s\t%s\n",kakao_signal[GET_REG_FORM], mysql_error(conn));//записываем ошибку в лог
					fflush(auth_log);
					continue;//ждем следующий сигнал	
				}
				sprintf(query_buf,"INSERT INTO companion_writes(user_id,to_user_id) VALUES (\"%s\",\"%s\")",my_info.id,my_info.id);
				if(mysql_query(conn,query_buf)!=0)// создаем запись с пользователем чтобы отправлять ему ошибки
				{
					mysql_query(conn,"ROLLBACK");
					fprintf(auth_log, "ERROR: %s\t%s\n",kakao_signal[GET_REG_FORM], mysql_error(conn));//записываем ошибку в лог
					fflush(auth_log);
					continue;//ждем следующий сигнал	
				}
				sprintf(query_buf,"INSERT INTO party_writes(user_id) VALUES (\"%s\")",my_info.id);
				if(mysql_query(conn,query_buf)!=0)// создаем запись с пользователем чтобы отправлять ему ошибки
				{
					mysql_query(conn,"ROLLBACK");
					fprintf(auth_log, "ERROR: %s\t%s\n",kakao_signal[GET_REG_FORM], mysql_error(conn));//записываем ошибку в лог
					fflush(auth_log);
					continue;//ждем следующий сигнал	
				}
				mysql_query(conn,"COMMIT");
				break;// если регистрация успешна

			}
			else if(strcmp(signal,kakao_signal[GET_LOG_FORM])==0)
			{
				/*принимаем размер никнейма*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(auth_log,"ERROR %s: RECV size of nickname(%d)\n",kakao_signal[GET_LOG_FORM],ret ); fflush(auth_log);
					continue;//ждем следующий сигнал
				}
				/*принимаем никнейм*/
				if( (ret=recvall(sock,my_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(auth_log,"ERROR %s: RECVALL nickname(%d)\n",kakao_signal[GET_LOG_FORM],ret ); fflush(auth_log);
					continue;//ждем следующий сигнал
				}

				/*принимаем размер пароля*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(auth_log,"ERROR %s: RECV size of password(%d)\n",kakao_signal[GET_LOG_FORM],ret ); fflush(auth_log);
					continue;//ждем следующий сигнал
				}
				/*принимаем пароль*/
				if( (ret=recvall(sock,my_info.password,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(auth_log,"ERROR %s: RECVALL password(%d)\n",kakao_signal[GET_LOG_FORM],ret ); fflush(auth_log);
					continue;//ждем следующий сигнал
				}
				strcpy(my_info.password,my_sha256(my_info.password,strlen(my_info.password)));//хешируем пароль
				if(check_password(conn,my_info.nickname,my_info.password))//если пароль правильный
				{
					/*отправляем размер ответа(правильный пароль или нет)*/
					sz=htonl(get_str_size("y"));
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(auth_log, "ERROR %s : SEND size of check password answer yes\"y\"(%d)\n",kakao_signal[GET_LOG_FORM],ret ); fflush(auth_log);
						continue;//ждем следующий сигнал
					}
					/*отправляем ответ(правильный пароль или нет)*/
					if( !(ret=sendall(sock,"y",ntohl(sz))) ) {
						fprintf(auth_log, "ERROR %s : SENDALL check password answer yes\"y\"(%d)\n",kakao_signal[GET_LOG_FORM],ret ); fflush(auth_log);
						continue;//ждем следующий сигнал
					}
					strcpy(my_info.id,get_id(conn,my_info.nickname));//записываем id пользователя
					break;//выходим из цикла и работаем дальше с пользователем
				}
				else//если пароль неправильный
				{
					/*отправляем размер ответа(правильный пароль или нет)*/
					sz=htonl(get_str_size("n"));
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(auth_log, "ERROR %s : SEND size of check password answer no\"n\"(%d)\n",kakao_signal[GET_LOG_FORM],ret ); fflush(auth_log);
						continue;//ждем следующий сигнал
					}
					/*отправляем ответ(правильный пароль или нет)*/
					if( !(ret=sendall(sock,"n",ntohl(sz))) ) {
						fprintf(auth_log, "ERROR %s : SENDALL check password answer no\"n\"(%d)\n",kakao_signal[GET_LOG_FORM],ret ); fflush(auth_log);
						continue;//ждем следующий сигнал
					}
					continue;//ждем следующий сигнал
				}
			}
		}
		if(ret==0) {
			fprintf(stdout,"Клиент отключился\n" ); fflush(stdout);
			close(sock);
			fclose(auth_log);
			mysql_close(conn);
			mysql_free_result(result);
			mysql_free_result(result2);
			return NULL;
		}
		else if(ret<0) {
			fprintf(server_log,"У клиента произошла ошибка при авторизации\n" ); fflush(server_log);
			close(sock);
			fclose(auth_log);
			mysql_close(conn);
			mysql_free_result(result);
			mysql_free_result(result2);
			return NULL;
		}
	}
	else//отправляем положительный ответ
	{
		/*отправляем положительный ответ*/
		sz=htonl(1);
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(auth_log, "ERROR AUTH : SEND  answer yes(%d)\n",ret ); fflush(auth_log);
			return NULL;//ждем следующий сигнал
		}
		strcpy(my_info.id,get_id(conn,my_info.nickname));//записываем id пользователя
	}
	sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
	mysql_query(conn,query_buf);//удаляем  текст ошибки из бд
	sprintf(log_path,"%s/id: %s",PATHS[PATH_USER_LOGS],my_info.id);
	log=fopen(log_path,"w+");
	/*отправляем размер  его id*/
	sz=htonl(get_str_size(my_info.id));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(log, "ERROR: SEND size of my id(%d)\n",ret ); fflush(log);
	}
	/*отправляем его id*/
	if( !(ret=sendall(sock,my_info.id,ntohl(sz))) ) {
		fprintf(log, "ERROR: SENDALL my id(%d)\n",ret ); fflush(log);
	}
	my_data.log=log;
	my_data.conn=conn;
	my_data.query_buf=query_buf;
	my_data.id=my_info.id;
	my_data.sock=sock;
	my_data.recv_buf=recv_buf;
	/*обрабатываем сигналы*/
	while(recv(sock,&sz,sizeof(sz),MSG_WAITALL))//принимаем сигнал
	{
		/*принимаем тип диалога*/
		if( (ret=recvall(sock,signal,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
		else if(ret<0){
			fprintf(log,"ERROR %s: RECVALL signal(%d)\n",kakao_signal[DARK_ERROR],ret ); fflush(log);
			continue;//ждем следующий сигнал
		}
		sprintf(query_buf, "UPDATE users SET when_online=NOW() WHERE id=\"%s\"", my_info.id);//устанавливаем что пользователь онлайн
		if(mysql_query(conn,query_buf)!=0)//устанавливаем что пользователь онлайн
		{//обрабатываем ошибку
			fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DARK_ERROR], mysql_error(conn));//записываем ошибку в лог
			fflush(log);
			sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DARK_ERROR],my_info.id );
			mysql_query(conn,query_buf);//записываем текст ошибки в бд
			/*принимаем ответ пользователя чтобы удалить запись из бд*/
			if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break; //если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL mysql_error: %s(%d)\n",kakao_signal[DARK_ERROR],mysql_error(conn),ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			if(strcmp(recv_buf,"y")==0){
				sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
				mysql_query(conn,query_buf);//записываем текст ошибки в бд				
			}
			continue;//ждем следующий сигнал
		}

		if(strcmp(signal,kakao_signal[SEND_MESSAGE])==0)//если пользователь хочет отправить сообщение
		{

			/*принимаем размер типа диалога*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of type of dilaog(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем тип диалога*/
			if( (ret=recvall(sock,dialog_type,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL type of dialog(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			/*принимаем размер сообщения*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of message(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем сообщение*/
			if( (ret=recvall(sock,recv_buf,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL message(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			if(strcmp(dialog_type,text_dialog_type[COMPANION])==0) 
			{
				/*принимаем размер id пользователя*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of people id(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем id пользователя*/
				if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL people id(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				if(allow_send_data_to_companion(&my_data,people_info.id)) {//если разрешено отправить данные
					/*отправляем положительный ответ(можно отправлять данные)*/
					sz=htonl(1);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND answer true(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					sprintf(query_buf, "INSERT INTO messages (to_id,from_id,text) VALUES (\"%s\",\"%s\",\"%s\")",people_info.id,my_info.id,recv_buf );
					if(mysql_query(conn,query_buf)!=0)// Отправляем написанное нами сообщение
					{//обрабатываем ошибку
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SEND_MESSAGE], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SEND_MESSAGE],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break; //если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql_error: %s(%d)\n",kakao_signal[SEND_MESSAGE],mysql_error(conn),ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0)
						{
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
						continue;//ждем следующий сигнал
					}
				} 
				else {
					/*отправляем отрицательный ответ(нельзя отправлять данные)*/
					sz=htonl(0);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND answer false(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					continue;
				}
			}
			else if(strcmp(dialog_type,text_dialog_type[PARTY])==0) 
			{
				/*принимаем размер party_id*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of party_id(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем party_id*/
				if( (ret=recvall(sock,party_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL party_id(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(allow_send_data_to_party(&my_data,party_info.id))
				{
					/*отправляем положительный ответ(можно отправлять данные)*/
					sz=htonl(1);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND answer true(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					sprintf(query_buf, "INSERT INTO messages (party_id,from_id,text) VALUES (\"%s\",\"%s\",\"%s\")",party_info.id,my_info.id,recv_buf );
					if(mysql_query(conn,query_buf)!=0)// Отправляем написанное нами сообщение
					{//обрабатываем ошибку
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SEND_MESSAGE], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SEND_MESSAGE],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break; //если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql_error: %s(%d)\n",kakao_signal[SEND_MESSAGE],mysql_error(conn),ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0)
						{
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
						continue;//ждем следующий сигнал
					}
				}
				else 
				{
					/*отправляем отрицательный ответ(нельзя отправлять данные)*/
					sz=htonl(0);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND answer false(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					continue;
				}
			}
		}
		else if(strcmp(signal,kakao_signal[SHOW_MESSAGES])==0)
		{
			/*принимаем размер типа диалога*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of type of dilaog(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем тип диалога*/
			if( (ret=recvall(sock,dialog_type,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL type of dialog(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			if(strcmp(dialog_type,text_dialog_type[COMPANION])==0) 
			{
				/*принимаем размер to_id*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of to_id(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем to_id*/
				if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL to_id(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			}
			else if(strcmp(dialog_type,text_dialog_type[PARTY])==0) 
			{
				/*принимаем размер party_id*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of party_id(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем party_id*/
				if( (ret=recvall(sock,party_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL party_id(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				
				if(!allow_send_data_to_party(&my_data,party_info.id)) //если пользоваетль пытается нас обмануть
					continue;//ждем следующий сигнал
			}
			/*принимаем размер limit*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of limit(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем limit*/
			if( (ret=recvall(sock,limit,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL limit(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			if(strcmp(dialog_type,text_dialog_type[COMPANION])==0) 
			{
				sprintf(query_buf, "SELECT * FROM(SELECT text,from_id,id,created_at,(SELECT CASE WHEN count(*) THEN '1' ELSE '0' END AS saw FROM who_saw_message WHERE message_id=id) AS saw,important,(SELECT CASE WHEN file_id>0 THEN file_id ELSE '0' END) AS file_id,(SELECT CASE WHEN photo_id>0 THEN photo_id ELSE '0' END) AS photo_id FROM messages WHERE ( (from_id = \"%s\" AND to_id = \"%s\") OR (to_id = \"%s\" AND from_id = \"%s\") ) ORDER BY created_at DESC LIMIT %s) AS t1 ORDER BY created_at", my_info.id,people_info.id,my_info.id,people_info.id,limit);
				if(mysql_query(conn,query_buf)!=0)// Получаем все сообщения c данным собеседником
				{//обрабатываем ошибку
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_MESSAGES], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_MESSAGES],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query accept all messages with companion(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
			}
			else if(strcmp(dialog_type,text_dialog_type[PARTY])==0) 
			{
				//sprintf(recv_buf,"%ld",i);//переводим разность сообщений в строку
				sprintf(query_buf, "SELECT * FROM(SELECT t1.text,t1.from_id,t1.id,t1.created_at,(SELECT CASE WHEN count(*) THEN '1' ELSE '0' END AS saw FROM who_saw_message WHERE message_id=t1.id) AS saw,important,(SELECT CASE WHEN t1.file_id>0 THEN t1.file_id ELSE '0' END) AS file_id,(SELECT CASE WHEN t1.photo_id>0 THEN t1.photo_id ELSE '0' END) AS photo_id,t2.nickname FROM messages AS t1 INNER JOIN users AS t2 ON t1.party_id = \"%s\" AND t1.from_id=t2.id ORDER BY t1.created_at DESC LIMIT %s) AS t1 ORDER BY created_at", party_info.id,limit);
				if(mysql_query(conn,query_buf)!=0)// Получаем последние n сообщений c данной группой и никнейм отправителя
				{//обрабатываем ошибку
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_MESSAGES], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_MESSAGES],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query accept all messages with party(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
			}
			result=mysql_store_result(conn);// Извлечение результатов запроса

			if((i=mysql_num_rows(result)) > 0)// Если количество возвращенных записей больше чем 0(есть сообщения)
			{
				while((row=mysql_fetch_row(result)))// Извлекаем по одной записи(при каждом обращении извлекается след. запись)
				{
					/*отправляем размер id сообщения*/
					sz=htonl(get_str_size(row[2]));
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND size of id of message(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					/*отправляем id сообщения*/
					if( !(ret=sendall(sock,row[2],ntohl(sz))) ) {
						fprintf(log, "ERROR %s : SENDALL id of message(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					/*отправляем размер когда сообщение отправлено*/
					sz=htonl(get_str_size(row[3]));
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND size of created_at message(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					/*отправляем когда сообщение отправлено*/
					if( !(ret=sendall(sock,row[3],ntohl(sz))) ) {
						fprintf(log, "ERROR %s : SENDALL created_at message(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					if(atoi(row[6])) {
						/*извлекаем путь к файлу*/
						sprintf(query_buf, "SELECT filename,size FROM files WHERE id=\"%s\"",row[6] );
						if(mysql_query(conn,query_buf)!=0)
						{//обрабатываем ошибку
							fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_MESSAGES], mysql_error(conn));//записываем ошибку в лог
							fflush(log);
							sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_MESSAGES],my_info.id );

							mysql_query(conn,query_buf);//записываем текст ошибки в бд

							/*принимаем ответ пользователя чтобы удалить запись из бд*/
							if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
							else if(ret<0) {
								fprintf(log,"ERROR %s: RECVALL mysql query select file?(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							if(strcmp(recv_buf,"y")==0) {
								sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
								mysql_query(conn,query_buf);//записываем текст ошибки в бд
							}
							continue;//ждем следующий сигнал
						}
						result2=mysql_store_result(conn);// Извлечение результатов запроса
						row2=mysql_fetch_row(result2);

						/*отправляем тип сообщения*/
						sz=htonl(BOB_FILE);
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND mes type file(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*отправляем размер имени файла*/
						sz=htonl(get_str_size(row2[0]));
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND size of filename(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*отправляем имя файла*/
						if( !(ret=sendall(sock,row2[0],ntohl(sz))) ) {
							fprintf(log, "ERROR %s : SENDALL filename(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*отправляем размер размера файла*/
						sz=htonl(get_str_size(row2[1]));
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND size of size of file(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*отправляем размер файла*/
						if( !(ret=sendall(sock,row2[1],ntohl(sz))) ) {
							fprintf(log, "ERROR %s : SENDALL size of file(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						mysql_free_result(result2);
					}
					else if(atoi(row[7])) {
						/*извлекаем путь к фото*/
						sprintf(query_buf, "SELECT filename,size,path FROM photos WHERE id=\"%s\"",row[7] );
						if(mysql_query(conn,query_buf)!=0)
						{//обрабатываем ошибку
							fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_MESSAGES], mysql_error(conn));//записываем ошибку в лог
							fflush(log);
							sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_MESSAGES],my_info.id );

							mysql_query(conn,query_buf);//записываем текст ошибки в бд

							/*принимаем ответ пользователя чтобы удалить запись из бд*/
							if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
							else if(ret<0) {
								fprintf(log,"ERROR %s: RECVALL mysql query select file?(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							if(strcmp(recv_buf,"y")==0) {
								sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
								mysql_query(conn,query_buf);//записываем текст ошибки в бд
							}
							continue;//ждем следующий сигнал
						}
						result2=mysql_store_result(conn);// Извлечение результатов запроса
						row2=mysql_fetch_row(result2);

						/*отправляем тип сообщения*/
						sz=htonl(PHOTO);
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND mes type file(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*отправляем размер имени фото*/
						sz=htonl(get_str_size(row2[0]));
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND size of filename(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*отправляем имя фото*/
						if( !(ret=sendall(sock,row2[0],ntohl(sz))) ) {
							fprintf(log, "ERROR %s : SENDALL filename(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*отправляем размер размера фото*/
						sz=htonl(get_str_size(row2[1]));
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND size of size of file(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*отправляем размер фото*/
						if( !(ret=sendall(sock,row2[1],ntohl(sz))) ) {
							fprintf(log, "ERROR %s : SENDALL size of file(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*принимаем ответ*/
						if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
						else if(ret<0){
							fprintf(log,"ERROR %s: RECV answer(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						if(ntohl(sz)) {//если ответ положительный, то отправляем фото
							if(!send_file(sock,row2[2])) {//если произошла ошибка
								/*отправляем ноль*/
								sz=htonl(0);
								if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
									fprintf(log, "ERROR %s : SEND zero(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}
							}
						}
						mysql_free_result(result2);
					}
					else
					{
						/*отправляем тип сообщения*/
						sz=htonl(TEXT);
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND mes type file(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*отправляем размер текста*/
						sz=htonl(get_str_size(row[0]));
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND size of text message(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*отправляем текст*/
						if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
							fprintf(log, "ERROR %s : SENDALL text message(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
					}

					if(strcmp(row[5],"0")==0)//если сообщение не важное
					{
						if(strcmp(row[1],my_info.id)==0)//если сообщение отправлено от меня
						{
							/*отправляем размер положения сообщения*/
							sz=htonl(get_str_size(text_mes_hpos[RIGHT]));
							if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
								fprintf(log, "ERROR %s : SEND size of RIGHT(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}
							/*отправляем положение сообщения*/
							if( !(ret=sendall(sock,text_mes_hpos[RIGHT],ntohl(sz))) ) {
								fprintf(log, "ERROR %s : SENDALL RIGHT(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}
							/*отправляем прочитано ли сообщение*/
							sz=htonl(atoi(row[4]));
							if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
								fprintf(log, "ERROR %s : SEND saw message(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}
						}
						/*иначе если сообщение отправлено от собеседника*/
						else 
						{
							/*отправляем размер положения сообщения*/
							sz=htonl(get_str_size(text_mes_hpos[LEFT]));
							if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
								fprintf(log, "ERROR %s : SEND size of LEFT(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}
							/*отправляем положение сообщения*/
							if( !(ret=sendall(sock,text_mes_hpos[LEFT],ntohl(sz))) ) {
								fprintf(log, "ERROR %s : SENDALL LEFT(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}
							if(strcmp(dialog_type,text_dialog_type[PARTY])==0) 
							{
								if(strcmp(row[1],my_info.id)!=0)//если сообщение отправлено не от меня
								{
									/*отправляем размер никнейма отправителя*/
									sz=htonl(get_str_size(row[8]));
									if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
										fprintf(log, "ERROR %s : SEND size of sender nickname(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
										continue;//ждем следующий сигнал
									}
									/*отправляем никнейм отправителя*/
									if( !(ret=sendall(sock,row[8],ntohl(sz))) ) {
										fprintf(log, "ERROR %s : SENDALL sender nickname(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
										continue;//ждем следующий сигнал
									}
								}
							}
						}
					}
					else//если сообщение важное
					{
						/*отправляем размер положения сообщения*/
						sz=htonl(get_str_size(text_mes_hpos[CENTER]));
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND size of CENTER(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*отправляем положение сообщения*/
						if( !(ret=sendall(sock,text_mes_hpos[CENTER],ntohl(sz))) ) {
							fprintf(log, "ERROR %s : SENDALL CENTER(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
					}
				}
				
				mysql_free_result(result);
				/*отправляем нулевой размер чтобы клиент понял где конец сообщений*/
				sz=htonl(0);
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND size of end message(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
					continue;
				}
		
			} 
			else//иначе отправляем нулевое количество сообщений
			{
				mysql_free_result(result);
				/*отправляем нулевой размер чтобы клиент понял где конец сообщений*/
				sz=htonl(0);
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND size of end message%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			}
		}
		else if(strcmp(signal,kakao_signal[SHOW_MY_SUBS])==0)
		{
			/*принимаем размер никнейма из поиска*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of SearchEntry nickname(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем никнейм из поиска*/
			if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL SearchEntry nickname(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем размер limit*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of limit(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем limit*/
			if( (ret=recvall(sock,limit,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL limit(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*получаем данные моих подписчиков или друзей,учитывая тот никнейм, который пользователь ввел в поиске*/
			sprintf(query_buf, "SELECT t1.nickname ,t2.user_id FROM users AS t1 INNER JOIN friends AS t2 ON t1.id = t2.user_id AND t2.friend_id = \"%s\" AND t1.nickname LIKE \"%s%%\" AND NOT EXISTS (SELECT user_id FROM friends WHERE user_id=\"%s\" AND friend_id=t1.id) ORDER BY t1.when_online DESC LIMIT %s", my_info.id,people_info.nickname,my_info.id,limit);
			if(mysql_query(conn,query_buf)!=0)
			{
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_MY_SUBS], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_MY_SUBS],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query my_subs(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			result=mysql_store_result(conn);// Извлечение результатов запроса
			if(mysql_num_rows(result) > 0)
			{
				while((row=mysql_fetch_row(result)))
				{
					/*отправляем размер id подписчика*/
					sz=htonl(get_str_size(row[1]));
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND size of sub id(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					/*отправляем id*/
					if( !(ret=sendall(sock,row[1],ntohl(sz))) ) {
						fprintf(log, "ERROR %s : SENDALL sub id(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					/*отправляем размер никнейма подписчика*/
					sz=htonl(get_str_size(row[0]));
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND size of sub nickname(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					/*отправляем никнейм подписчика*/
					if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
						fprintf(log, "ERROR %s : SENDALL sub nickname(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					user_online(&my_data,row[1]);//отправляем онлайн ли юзер
				}
			}
			mysql_free_result(result);
			/*отправляем нулевой размер чтобы клиент понял где конец, чтобы выйти из ожидания*/
			sz=htonl(0);
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of end message(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
		}
		else if(strcmp(signal,kakao_signal[SHOW_FRIEND_LIST])==0)
		{
			/*принимаем размер никнейма*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of SearchEntry nickname(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем никнейм*/
			if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL SearchEntry nickname(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем размер limit*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of limit(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем limit*/
			if( (ret=recvall(sock,limit,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL limit(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*получаем данные друзей с никнеймом, ориентируясь на тот,который пользователь ввел в поиске*/
			sprintf(query_buf, "SELECT t1.nickname ,t2.friend_id FROM users AS t1 INNER JOIN friends AS t2 ON t1.id = t2.friend_id AND t2.user_id = \"%s\" AND t1.nickname LIKE \"%s%%\" AND EXISTS (SELECT user_id FROM friends WHERE friend_id = \"%s\" AND user_id=t1.id) ORDER BY t1.when_online DESC LIMIT %s", my_info.id,people_info.nickname,my_info.id,limit);
			if(mysql_query(conn,query_buf)!=0)
			{
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_FRIEND_LIST], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_FRIEND_LIST],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query my_friends(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			result=mysql_store_result(conn);// Извлечение результатов запроса
			if(mysql_num_rows(result) > 0)
			{
				while((row=mysql_fetch_row(result)))
				{

					/*отправляем размер id друга*/
					sz=htonl(get_str_size(row[1]));
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND size of friend id(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					/*отправляем id друга*/
					if( !(ret=sendall(sock,row[1],ntohl(sz))) ) {
						fprintf(log, "ERROR %s : SENDALL friend id(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					/*отправляем размер никнейма друга*/
					sz=htonl(get_str_size(row[0]));
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND size of friend nickname(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					/*отправляем никнейм друга*/
					if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
						fprintf(log, "ERROR %s : SENDALL friend nickname(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					user_online(&my_data,row[1]);//отправляем онлайн ли юзер
				}
			}
			mysql_free_result(result);
			/*отправляем нулевой размер чтобы клиент понял где конец, чтобы выйти из ожидания*/
			sz=htonl(0);
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of end message(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(log);
			}

		}
		else if(strcmp(signal,kakao_signal[SHOW_PARTY_MESSAGES_LIST])==0)
		{
			MYSQL_ROW row3;
			MYSQL_RES *result3;
			/*принимаем размер limit*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of limit(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем limit*/
			if( (ret=recvall(sock,limit,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL limit(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*получаем данные группы*/
			sprintf(query_buf, "SELECT t1.name, t1.id FROM parties AS t1 INNER JOIN party_users AS t2 ON t2.user_id = \"%s\" AND t2.party_id=t1.id ORDER BY (SELECT created_at FROM messages WHERE party_id=t1.id ORDER BY created_at DESC LIMIT 1) DESC LIMIT %s", my_info.id,limit);
			if(mysql_query(conn,query_buf)!=0)
			{// Получаем собеседников
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_PARTY_MESSAGES_LIST],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query data of parties(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			result=mysql_store_result(conn);// Извлечение результатов запроса
			if(mysql_num_rows(result) > 0 )
			{
				while( (row=mysql_fetch_row(result)) )
				{
					sprintf(query_buf, "SELECT count(*) FROM messages AS t1 INNER JOIN party_users AS t2 WHERE t1.party_id = \"%s\" AND from_id!=\"%s\" AND t1.party_id=t2.party_id AND t2.user_id=\"%s\" AND t1.id NOT IN (SELECT message_id FROM who_saw_message)",row[1],my_info.id,my_info.id);
					if(mysql_query(conn,query_buf)!=0)
					{// Получаем количество непрочитанных мной сообщений в группе
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_PARTY_MESSAGES_LIST],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql query quantity unread messages by me(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0) {
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
						continue;//ждем следующий сигнал
					}
					result3=mysql_store_result(conn);
					row3=mysql_fetch_row(result3);

					sprintf(query_buf, "SELECT t1.text,t2.nickname,(SELECT CASE WHEN t1.file_id>0 THEN t1.file_id ELSE '0' END) AS file_id,(SELECT CASE WHEN t1.photo_id>0 THEN t1.photo_id ELSE '0' END) AS photo_id,t1.id FROM messages AS t1 INNER JOIN users AS t2 ON t1.party_id=\"%s\" AND t1.from_id=t2.id ORDER BY t1.created_at DESC LIMIT 1 ",row[1]);
					if(mysql_query(conn,query_buf)!=0)
					{// Получаем последнее сообщение с группой и имя отправителя
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_PARTY_MESSAGES_LIST],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql query last message party(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0) {
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
						continue;//ждем следующий сигнал
					}
					result2=mysql_store_result(conn);
					if(mysql_num_rows(result2)>0)
					{
						while( (row2=mysql_fetch_row(result2)) )
						{
							/*отправляем размер id группы*/
							sz=htonl(get_str_size(row[1]));
							if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND size of party id(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}
							/*отправляем id группы*/
							if( !(ret=sendall(sock,row[1],ntohl(sz))) ) {
								fprintf(log, "ERROR %s : SENDALL party id(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							/*отправляем размер имени группы*/
							sz=htonl(get_str_size(row[0]));
							if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
								fprintf(log, "ERROR %s : SEND size of party name(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}
							/*отправляем имя группы*/
							if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
								fprintf(log, "ERROR %s : sendall party name(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							party_writing(&my_data,row[1]);//отправляем тех кто печатает сообщение

							if(atoi(row2[2])) {//если сообщение содержит файл
								MYSQL_RES *result4;
								MYSQL_ROW row4;
								/*извлекаем путь к фото*/
								sprintf(query_buf, "SELECT filename FROM files WHERE id=\"%s\"",row2[2] );
								if(mysql_query(conn,query_buf)!=0)
								{//обрабатываем ошибку
									fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST], mysql_error(conn));//записываем ошибку в лог
									fflush(log);
									sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_PARTY_MESSAGES_LIST],my_info.id );

									mysql_query(conn,query_buf);//записываем текст ошибки в бд

									/*принимаем ответ пользователя чтобы удалить запись из бд*/
									if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
									else if(ret<0) {
										fprintf(log,"ERROR %s: RECVALL mysql query select file?(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
										continue;//ждем следующий сигнал
									}

									if(strcmp(recv_buf,"y")==0) {
										sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
										mysql_query(conn,query_buf);//записываем текст ошибки в бд
									}
									continue;//ждем следующий сигнал
								}
								result4=mysql_store_result(conn);// Извлечение результатов запроса
								row4=mysql_fetch_row(result4);
								/*отправляем тип сообщения*/
								sz=htonl(BOB_FILE);
								if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
									fprintf(log, "ERROR %s : SEND mes type file(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}
								/*отправляем размер имени файла*/
								sz=htonl(get_str_size(row4[0]));
								if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
									fprintf(log, "ERROR %s : SEND size of filename(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}
								/*отправляем имя файла*/
								if( !(ret=sendall(sock,row4[0],ntohl(sz))) ) {
									fprintf(log, "ERROR %s : SENDALL filename(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}
								mysql_free_result(result4);
							}
							else if(atoi(row2[3])) {
								MYSQL_RES *result4;
								MYSQL_ROW row4;
								/*извлекаем путь к фото*/
								sprintf(query_buf, "SELECT filename FROM photos WHERE id=%s",row2[3] );
								if(mysql_query(conn,query_buf)!=0)
								{//обрабатываем ошибку
									fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST], mysql_error(conn));//записываем ошибку в лог
									fflush(log);
									sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_PARTY_MESSAGES_LIST],my_info.id );

									mysql_query(conn,query_buf);//записываем текст ошибки в бд

									/*принимаем ответ пользователя чтобы удалить запись из бд*/
									if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
									else if(ret<0) {
										fprintf(log,"ERROR %s: RECVALL mysql query select photo?(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
										continue;//ждем следующий сигнал
									}

									if(strcmp(recv_buf,"y")==0) {
										sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
										mysql_query(conn,query_buf);//записываем текст ошибки в бд
									}
									continue;//ждем следующий сигнал
								}
								result4=mysql_store_result(conn);// Извлечение результатов запроса
								row4=mysql_fetch_row(result4);
								/*отправляем тип сообщения*/
								sz=htonl(PHOTO);
								if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
									fprintf(log, "ERROR %s : SEND mes type photo(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}
								/*отправляем размер имени файла*/
								sz=htonl(get_str_size(row4[0]));
								if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
									fprintf(log, "ERROR %s : SEND size of name of photo(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}
								/*отправляем имя файла*/
								if( !(ret=sendall(sock,row4[0],ntohl(sz))) ) {
									fprintf(log, "ERROR %s : SENDALL name of photo(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}
								mysql_free_result(result4);
							}
							else {
								/*отправляем тип сообщения*/
								sz=htonl(TEXT);
								if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
									fprintf(log, "ERROR %s : SEND mes type text(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}
								/*отправляем размер последнего сообщения*/
								sz=htonl(get_str_size(row2[0]));
								if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
									fprintf(log, "ERROR %s : SEND size of last message with my companion(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}
								/*отправляем последнее сообщение*/
								if( !(ret=sendall(sock,row2[0],ntohl(sz))) ) {
									fprintf(log, "ERROR %s : SENDALL last message with my companion(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}
							}
							/*отправляем размер никнейма отправителя*/
							sz=htonl(get_str_size(row2[1]));
							if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
								fprintf(log, "ERROR %s : SEND size of sender nickname(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}
							/*отправляем никнейм отправителя*/
							if( !(ret=sendall(sock,row2[1],ntohl(sz))) ) {
								fprintf(log, "ERROR %s : SENDALL sender nickname(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}
							/*отправляем размер количества непрочитанных мной сообщений*/
							sz=htonl(get_str_size(row3[0]));
							if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
								fprintf(log, "ERROR %s : SEND size of quantity unread messages by me(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}
							/*отправляем количество непрочитанных мной сообщений*/
							if( !(ret=sendall(sock,row3[0],ntohl(sz))) ) {
								fprintf(log, "ERROR %s : SENDALL quantity unread messages by me(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}
							mysql_free_result(result3);
							continue;
						}
					}
					mysql_free_result(result2);
				}
			}
			mysql_free_result(result);
			/*отправляем нулевой размер чтобы клиент понял где конец, чтобы выйти из ожидания*/
			sz=htonl(0);
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of end message(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(log);
			}
		}
		else if(strcmp(signal,kakao_signal[SHOW_MESSAGES_LIST])==0)
		{
			MYSQL_ROW row3;
			MYSQL_RES *result3;
			/*принимаем размер limit*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of limit(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем limit*/
			if( (ret=recvall(sock,limit,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL limit(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*получаем данные собеседника*/
			sprintf(query_buf, "SELECT t1.nickname, t1.id FROM users AS t1 INNER JOIN dialog AS t2 ON (t1.id = t2.user1_id AND t2.user2_id = \"%s\") OR (t1.id = t2.user2_id AND t2.user1_id = \"%s\") ORDER BY (SELECT created_at FROM messages WHERE ((from_id= \"%s\" AND to_id = t1.id) OR (from_id= t1.id AND to_id = \"%s\")) ORDER BY created_at DESC LIMIT 1) DESC LIMIT %s", my_info.id,my_info.id,my_info.id,my_info.id,limit);
			if(mysql_query(conn,query_buf)!=0)
			{// Получаем собеседников
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_MESSAGES_LIST], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_MESSAGES_LIST],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query data of my companions(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			result=mysql_store_result(conn);// Извлечение результатов запроса
			if(mysql_num_rows(result) > 0 )
			{
				while( (row=mysql_fetch_row(result)) )
				{
					sprintf(query_buf, "SELECT count(*) FROM messages WHERE from_id= \"%s\" AND to_id = \"%s\" AND id NOT IN (SELECT message_id FROM who_saw_message)",row[1],my_info.id);
					if(mysql_query(conn,query_buf)!=0)
					{// Получаем количество непрочитанных мной сообщений
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_MESSAGES_LIST], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_MESSAGES_LIST],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql query quantity unread messages by me(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0) {
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
						continue;//ждем следующий сигнал
					}
					result3=mysql_store_result(conn);
					row3=mysql_fetch_row(result3);					
					sprintf(query_buf, "SELECT t1.text,t2.nickname,(SELECT CASE WHEN t1.file_id>0 THEN t1.file_id ELSE '0' END),(SELECT CASE WHEN t1.photo_id>0 THEN t1.photo_id ELSE '0' END) AS photo_id,t1.id FROM messages AS t1 INNER JOIN users AS t2 ON ((t1.from_id= \"%s\" AND t1.to_id = \"%s\") OR (t1.from_id= \"%s\" AND t1.to_id = \"%s\")) AND t1.from_id=t2.id ORDER BY t1.created_at DESC LIMIT 1 ", my_info.id,row[1],row[1],my_info.id);
					if(mysql_query(conn,query_buf)!=0)
					{// Получаем последнее сообщение с каждым собеседником
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_MESSAGES_LIST], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_MESSAGES_LIST],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql query last message with my companion(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0) {
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
						continue;//ждем следующий сигнал
					}
					result2=mysql_store_result(conn);
					if(mysql_num_rows(result2)>0)
					{
						while( (row2=mysql_fetch_row(result2)) )
						{
							/*отправляем размер id собеседника*/
							sz=htonl(get_str_size(row[1]));
							if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
								fprintf(log, "ERROR %s : SEND size of companion id(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}
							/*отправляем id собеседника*/
							if( !(ret=sendall(sock,row[1],ntohl(sz))) ) {
								fprintf(log, "ERROR %s : SENDALL companion id(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							/*отправляем размер никнейма собеседника*/
							sz=htonl(get_str_size(row[0]));
							if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
								fprintf(log, "ERROR %s : SEND size of companion nickname(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}
							/*отправляем никнейм друга*/
							if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
								fprintf(log, "ERROR %s : sendall companion nickname(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							i=user_writing(&my_data,row[1],my_info.id);//смотрим печатает ли юзер сообщение
							/*отправляем печатает ли юзер сообщение*/
							sz=htonl( i );
							if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
								fprintf(log, "ERROR %s : SEND writing?(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}
							/*если не печатает сообщение */ 
							if(!i) {
								user_online(&my_data,row[1]);//отправляем онлайн ли юзер
							}

							if(atoi(row2[2])) {//если сообщение содержит файл
								MYSQL_RES *result3;
								MYSQL_ROW row3;
								/*извлекаем путь к фото*/
								sprintf(query_buf, "SELECT filename FROM files WHERE id=\"%s\"",row2[2] );
								if(mysql_query(conn,query_buf)!=0)
								{//обрабатываем ошибку
									fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_MESSAGES_LIST], mysql_error(conn));//записываем ошибку в лог
									fflush(log);
									sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_MESSAGES_LIST],my_info.id );

									mysql_query(conn,query_buf);//записываем текст ошибки в бд

									/*принимаем ответ пользователя чтобы удалить запись из бд*/
									if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
									else if(ret<0) {
										fprintf(log,"ERROR %s: RECVALL mysql query select file?(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
										continue;//ждем следующий сигнал
									}

									if(strcmp(recv_buf,"y")==0) {
										sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
										mysql_query(conn,query_buf);//записываем текст ошибки в бд
									}
									continue;//ждем следующий сигнал
								}
								result3=mysql_store_result(conn);// Извлечение результатов запроса
								row3=mysql_fetch_row(result3);
								/*отправляем тип сообщения*/
								sz=htonl(BOB_FILE);
								if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
									fprintf(log, "ERROR %s : SEND mes type file(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}
								/*отправляем размер имени файла*/
								sz=htonl(get_str_size(row3[0]));
								if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
									fprintf(log, "ERROR %s : SEND size of filename(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}
								/*отправляем имя файла*/
								if( !(ret=sendall(sock,row3[0],ntohl(sz))) ) {
									fprintf(log, "ERROR %s : SENDALL filename(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}
								mysql_free_result(result3);
							}
							else if(atoi(row2[3])) {
								MYSQL_RES *result3;
								MYSQL_ROW row3;
								/*извлекаем путь к фото*/
								sprintf(query_buf, "SELECT filename FROM photos WHERE id=\"%s\"",row2[3] );
								if(mysql_query(conn,query_buf)!=0)
								{//обрабатываем ошибку
									fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_MESSAGES_LIST], mysql_error(conn));//записываем ошибку в лог
									fflush(log);
									sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_MESSAGES_LIST],my_info.id );

									mysql_query(conn,query_buf);//записываем текст ошибки в бд

									/*принимаем ответ пользователя чтобы удалить запись из бд*/
									if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
									else if(ret<0) {
										fprintf(log,"ERROR %s: RECVALL mysql query select photo?(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
										continue;//ждем следующий сигнал
									}

									if(strcmp(recv_buf,"y")==0) {
										sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
										mysql_query(conn,query_buf);//записываем текст ошибки в бд
									}
									continue;//ждем следующий сигнал
								}
								result3=mysql_store_result(conn);// Извлечение результатов запроса
								row3=mysql_fetch_row(result3);
								/*отправляем тип сообщения*/
								sz=htonl(PHOTO);
								if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
									fprintf(log, "ERROR %s : SEND mes type photo(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}
								/*отправляем размер имени файла*/
								sz=htonl(get_str_size(row3[0]));
								if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
									fprintf(log, "ERROR %s : SEND size of name of photo(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}
								/*отправляем имя файла*/
								if( !(ret=sendall(sock,row3[0],ntohl(sz))) ) {
									fprintf(log, "ERROR %s : SENDALL name of photo(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}
								mysql_free_result(result3);
							}
							else {
								/*отправляем тип сообщения*/
								sz=htonl(TEXT);
								if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
									fprintf(log, "ERROR %s : SEND mes type text(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}
								/*отправляем размер последнего сообщения*/
								sz=htonl(get_str_size(row2[0]));
								if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
									fprintf(log, "ERROR %s : SEND size of last message with my companion(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}
								/*отправляем последнее сообщение*/
								if( !(ret=sendall(sock,row2[0],ntohl(sz))) ) {
									fprintf(log, "ERROR %s : SENDALL last message with my companion(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}
							}
							/*отправляем размер никнейма отправителя*/
							sz=htonl(get_str_size(row2[1]));
							if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
								fprintf(log, "ERROR %s : SEND size of sender nickname(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}
							/*отправляем никнейм отправителя*/
							if( !(ret=sendall(sock,row2[1],ntohl(sz))) ) {
								fprintf(log, "ERROR %s : SENDALL sender nickname(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							/*отправляем размер количества непрочитанных мной сообщений*/
							sz=htonl(get_str_size(row3[0]));
							if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
								fprintf(log, "ERROR %s : SEND size of quantity unread messages by me(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}
							/*отправляем количество непрочитанных мной сообщений*/
							if( !(ret=sendall(sock,row3[0],ntohl(sz))) ) {
								fprintf(log, "ERROR %s : SENDALL quantity unread messages by me(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}
							mysql_free_result(result3);
						}
					}
					mysql_free_result(result2);
				}
			}
			mysql_free_result(result);
			/*отправляем нулевой размер чтобы клиент понял где конец, чтобы выйти из ожидания*/
			sz=htonl(0);
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of end message(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(log);
			}

		}
		else if(strcmp(signal,kakao_signal[SHOW_PEOPLE_LIST])==0)
		{
			/*принимаем размер никнейма*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of SearchEntry nickname(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем никнейм*/
			if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL SearchEntry nickname(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			/*принимаем размер limit*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of limit(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем limit*/
			if( (ret=recvall(sock,limit,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL limit(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			//извлекаем данные пользователей где никнейм равен тексту из search entry
			sprintf(query_buf, "SELECT nickname,id FROM users WHERE nickname LIKE \"%s%%\" AND id!=\"%s\" ORDER BY when_online DESC LIMIT %s",people_info.nickname,my_info.id,limit);
			if(mysql_query(conn,query_buf)!=0)
			{// Получаем все сообщения
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_PEOPLE_LIST], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_PEOPLE_LIST],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query show people list(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			result=mysql_store_result(conn);
			if(mysql_num_rows(result) > 0)
			{
				while((row=mysql_fetch_row(result)))
				{
					/*отправляем размер id пользователя*/
					sz=htonl(get_str_size(row[1]));
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND size of people id(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					/*отправляем id пользователя*/
					if( !(ret=sendall(sock,row[1],ntohl(sz))) ) {
						fprintf(log, "ERROR %s : SENDALL people id(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					/*отправляем размер никнейма пользователя*/
					sz=htonl(get_str_size(row[0]));
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND size of people nickname(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					/*отправляем никнейм пользователя*/
					if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
						fprintf(log, "ERROR %s : SENDALL people nickname(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					user_online(&my_data,row[1]);//отправляем онлайн ли юзер
				}
			}
			mysql_free_result(result);
			/*отправляем нулевой размер чтобы клиент понял где конец, чтобы выйти из ожидания*/
			sz=htonl(0);
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of end message(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(log);
			}
		}
		else if(strcmp(signal,kakao_signal[SHOW_PROFILE_WINDOW])==0)
		{
			/*принимаем id пользователя в профиле*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of profile id(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем id пользователя в профиле*/
			if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL profile id(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			user_online(&my_data,people_info.id);//отправляем онлайн ли юзер

			if(block_user(&my_data,my_info.id,people_info.id)) {//если мы заблокировали пользователя
				/*отправляем положительный ответ(мы заблокировали пользователя)*/
				sz=htonl(1);
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND answer we blocked the user(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			}
			else {
				/*отправляем отрицательный ответ(мы не заблокировали пользователя)*/
				sz=htonl(0);
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND answer we didnt block the user(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			}

			if(block_user(&my_data,people_info.id,my_info.id)) {//если мы заблокированы
				/*отправляем положительный ответ(мы заблокированы)*/
				sz=htonl(1);
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND answer we blocked(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				continue;//ждем следующий сигнал
			}
			else {
				/*отправляем отрицательный ответ(мы не заблокированы)*/
				sz=htonl(0);
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND answer we didnt block(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			}

			/*получаем путь к аватарке пользователя*/
			sprintf(query_buf, "SELECT path_avatar FROM users WHERE id=\"%s\"",people_info.id);
			if(mysql_query(conn,query_buf)!=0)
			{
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_PROFILE_WINDOW], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_PROFILE_WINDOW],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query friend or sub(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			result=mysql_store_result(conn);
			row=mysql_fetch_row(result);
			if(strcmp(row[0],"")==0) {//если аватарки нет 
				/*отправляем отрицательный ответ(аватарки нет)*/
				sz=htonl(0);
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND answer havent avatar -(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			} 
			else {
				/*отправляем положительный ответ(аватарка есть)*/
				sz=htonl(1);
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND answer have avatar +(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*отправляем размер имени фото*/
				sz=htonl(get_str_size(from_path_to_filename(row[0])));
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND size of name of the image(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*отправляем имя фото*/
				if( !(ret=sendall(sock,from_path_to_filename(row[0]),ntohl(sz))) ) {
					fprintf(log, "ERROR %s : SENDALL name of the image(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*оптправляем аватарку*/
				if(!send_file(sock,row[0])) {//если произошла ошибка
					/*отправляем ноль*/
					sz=htonl(0);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND zero(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
				}
			}
			mysql_free_result(result);

			sprintf(query_buf, "SELECT count(*) FROM friends WHERE (user_id=\"%s\" AND friend_id=\"%s\") OR (friend_id=\"%s\" AND user_id=\"%s\")",my_info.id,people_info.id,my_info.id,people_info.id);
			if(mysql_query(conn,query_buf)!=0)// Получаем список друзей или подписок наших или к нам
			{
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_PROFILE_WINDOW], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_PROFILE_WINDOW],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query friend or sub(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			result=mysql_store_result(conn);
			row=mysql_fetch_row(result);
			mysql_free_result(result);
			/*отправляем размер количества записей*/
			sz=htonl(get_str_size(row[0]));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of quantity entrys(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*отправляем  количество записей*/
			if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
				fprintf(log, "ERROR %s : SENDALL quantity entrys(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			if(strcmp(row[0],"1")==0)
			{
				sprintf(query_buf, "SELECT count(*) FROM friends WHERE (user_id=\"%s\" AND friend_id=\"%s\")",my_info.id,people_info.id);
				if(mysql_query(conn,query_buf)!=0)
				{// Получаем список кому мы отправили заявку(проверяем, отпавили ли мы данному пользователю заявку в друзья)
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_PROFILE_WINDOW], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_PROFILE_WINDOW],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query who is sub(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				result=mysql_store_result(conn);
				row=mysql_fetch_row(result);
				mysql_free_result(result);
				/*отправляем размер количества записей*/
				sz=htonl(get_str_size(row[0]));
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND size of quantity entrys who is sub(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*отправляем  количество записей*/
				if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
					fprintf(log, "ERROR %s : SENDALL quantity entrys who is sub(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			}
		}
		else if(strcmp(signal,kakao_signal[CHANGE_PASSWORD])==0)
		{
			char old_pass[MAX_PASSWORD];
			char new_pass[MAX_PASSWORD];
			/*принимаем размер старого пароля*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of old_pass(%d)\n",kakao_signal[CHANGE_PASSWORD],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем старый пароль*/
			if( (ret=recvall(sock,old_pass,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL old_pass(%d)\n",kakao_signal[CHANGE_PASSWORD],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			printf("old_pass %s\n",old_pass );

			/*принимаем размер нового пароля*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of new_pass(%d)\n",kakao_signal[CHANGE_PASSWORD],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем новый пароль*/
			if( (ret=recvall(sock,new_pass,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL new_pass(%d)\n",kakao_signal[CHANGE_PASSWORD],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			sprintf(query_buf, "SELECT count(*) FROM users WHERE id=\"%s\" AND password=\"%s\"",my_info.id,my_sha256(old_pass,strlen(old_pass)) );
			if(mysql_query(conn,query_buf)!=0)
			{
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[CHANGE_PASSWORD], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[CHANGE_PASSWORD],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query quantity of entrys(%d)\n",kakao_signal[CHANGE_PASSWORD],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}		
			result=mysql_store_result(conn);
			row=mysql_fetch_row(result);
			mysql_free_result(result);
			if(atoi(row[0]))//если старый пароль верный
			{
				sprintf(query_buf, "UPDATE users SET password = \"%s\" WHERE id = \"%s\"",my_sha256(new_pass,strlen(new_pass)),my_info.id);
				if(mysql_query(conn,query_buf)!=0)
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[CHANGE_PASSWORD], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[CHANGE_PASSWORD],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql change password(%d)\n",kakao_signal[CHANGE_PASSWORD],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
			}
			/*отправляем размер количества записей*/
			sz=htonl(get_str_size(row[0]));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of quantity entrys(%d)\n",kakao_signal[CHANGE_PASSWORD],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*отправляем  количество записей*/
			if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
				fprintf(log, "ERROR %s : SENDALL quantity entrys(%d)\n",kakao_signal[CHANGE_PASSWORD],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
		}
		else if(strcmp(signal,kakao_signal[CHANGE_NICKNAME])==0)
		{
			char new_nick[MAX_NICKNAME];
			/*принимаем размер нового никнейма*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of new_nick(%d)\n",kakao_signal[CHANGE_NICKNAME],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем новый никнейм*/
			if( (ret=recvall(sock,new_nick,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL new_nick(%d)\n",kakao_signal[CHANGE_NICKNAME],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			ret=check_unique_nickname(conn,new_nick);
			if(ret)
			{
				sprintf(query_buf, "UPDATE users SET nickname = \"%s\" WHERE id = \"%s\"",new_nick,my_info.id);
				if(mysql_query(conn,query_buf)!=0)
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[CHANGE_NICKNAME], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[CHANGE_NICKNAME],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql change nickname(%d)\n",kakao_signal[CHANGE_NICKNAME],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				/*отправляем размер уникален ли никнейм*/
				sz=htonl(get_str_size("1"));
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND size of unique_nickname 1(%d)\n",kakao_signal[CHANGE_NICKNAME],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*отправляем уникален ли никнейм*/
				if( !(ret=sendall(sock,"1",ntohl(sz))) ) {
					fprintf(log, "ERROR %s : SENDALL unique_nickname 1(%d)\n",kakao_signal[CHANGE_NICKNAME],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

			}
			else 
			{
				/*отправляем размер уникален ли никнейм*/
				sz=htonl(get_str_size("0"));
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND size of unique_nickname 0(%d)\n",kakao_signal[CHANGE_NICKNAME],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*отправляем уникален ли никнейм*/
				if( !(ret=sendall(sock,"0",ntohl(sz))) ) {
					fprintf(log, "ERROR %s : SENDALL unique_nickname 0(%d)\n",kakao_signal[CHANGE_NICKNAME],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			}

		}
		else if(strcmp(signal,kakao_signal[DELETE_FRIEND_OR_SUB])==0)
		{
			/*принимаем размер id*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of id(%d)\n",kakao_signal[DELETE_FRIEND_OR_SUB],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем id*/
			if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL id(%d)\n",kakao_signal[DELETE_FRIEND_OR_SUB],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			sprintf(query_buf, "DELETE FROM friends WHERE (user_id=\"%s\" AND friend_id=\"%s\") OR (user_id=\"%s\" AND friend_id=\"%s\")",my_info.id,people_info.id,people_info.id,my_info.id);
			if(mysql_query(conn,query_buf)!=0)
			{// удаляем друга
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_FRIEND_OR_SUB], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_FRIEND_OR_SUB],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query(%d)\n",kakao_signal[DELETE_FRIEND_OR_SUB],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
		}
		else if(strcmp(signal,kakao_signal[ADD_FRIEND_OR_SUB])==0)
		{
			/*принимаем размер id*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of id(%d)\n",kakao_signal[ADD_FRIEND_OR_SUB],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем id*/
			if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL id(%d)\n",kakao_signal[ADD_FRIEND_OR_SUB],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			if(!block_user(&my_data,people_info.id,my_info.id)) //если мы не заблокированы данным пользователем
			{
				sprintf(query_buf, "INSERT INTO friends(user_id,friend_id) VALUES(\"%s\",\"%s\")",my_info.id,people_info.id);
				if(mysql_query(conn,query_buf)!=0)
				{// добавляем друга
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[ADD_FRIEND_OR_SUB], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[ADD_FRIEND_OR_SUB],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query(%d)\n",kakao_signal[ADD_FRIEND_OR_SUB],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
			}
		}
		else if(strcmp(signal,kakao_signal[UPDATE_DIALOG_MESSAGES])==0)
		{
			/*принимаем размер типа диалога*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of type of dilaog(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем тип диалога*/
			if( (ret=recvall(sock,dialog_type,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL type of dialog(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}


			/*принимаем размер limit*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of limit dilaog(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем limit*/
			if( (ret=recvall(sock,limit,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL limit dialog(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			if(strcmp(dialog_type,text_dialog_type[COMPANION])==0) 
			{
				sprintf(query_buf, "SELECT diff FROM (SELECT MAX(t1.id)-MIN(t1.id) AS diff FROM ( SELECT id FROM messages WHERE ((from_id = %s AND to_id = %s) OR (to_id = %s AND from_id = %s)) ) AS t1) AS t2 WHERE diff IS NOT NULL", my_info.id,people_info.id,my_info.id,people_info.id);
				if(mysql_query(conn,query_buf)!=0)// Получаем разность сообщений
				{//обрабатываем ошибку
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[UPDATE_DIALOG_MESSAGES], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[UPDATE_DIALOG_MESSAGES],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query accept difference messages with companion(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				result=mysql_store_result(conn);
				sprintf(query_buf, "SELECT count(*) FROM messages AS t1 INNER JOIN who_saw_message AS t2 WHERE (t1.from_id=\"%s\" AND t1.to_id=\"%s\") AND t1.to_id=t2.who_saw_id AND t1.id=t2.message_id", my_info.id,people_info.id);
				if(mysql_query(conn,query_buf)!=0)// Получаем количество прочитанных моих сообщений
				{//обрабатываем ошибку
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[UPDATE_DIALOG_MESSAGES], mysql_error(conn));//записываем ошибку в лог
					fflush(log); 
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[UPDATE_DIALOG_MESSAGES],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query accept saw messages with companion(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				result2=mysql_store_result(conn);
				row2=mysql_fetch_row(result2);
				mysql_free_result(result2);
			}
			else if(strcmp(dialog_type,text_dialog_type[PARTY])==0) 
			{
				sprintf(query_buf, "SELECT diff FROM (SELECT MAX(t1.id)-MIN(t1.id) AS diff FROM (SELECT id  FROM messages WHERE party_id = \"%s\") AS t1) AS t2 WHERE diff IS NOT NULL", party_info.id);
				if(mysql_query(conn,query_buf)!=0)// Получаем разность сообщений
				{//обрабатываем ошибку
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[UPDATE_DIALOG_MESSAGES], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[UPDATE_DIALOG_MESSAGES],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query accept difference messages with party(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				result=mysql_store_result(conn);

				sprintf(query_buf, "SELECT count(DISTINCT t2.message_id) FROM messages AS t1 INNER JOIN who_saw_message AS t2 ON (t1.from_id=\"%s\" AND t1.party_id=\"%s\") AND t1.id=t2.message_id", my_info.id,party_info.id);
				if(mysql_query(conn,query_buf)!=0)// Получаем количество прочитанных моих сообщений
				{//обрабатываем ошибку
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[UPDATE_DIALOG_MESSAGES], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[UPDATE_DIALOG_MESSAGES],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query accept saw messages with party(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				result2=mysql_store_result(conn);
				row2=mysql_fetch_row(result2);
				mysql_free_result(result2);
			}
			if(mysql_num_rows(result)>0) {
				row=mysql_fetch_row(result);
				/*отправляем размер разности сообщений*/
				sz=htonl(get_str_size(row[0]));
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND size of messages difference(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*отправляем разность сообщений*/
				if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
					fprintf(log, "ERROR %s : SENDALL quantity messages difference(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			}
			else {
				/*отправляем размер разности сообщений -1(если сообщений нет)*/
				sz=htonl(get_str_size("-1"));
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND size of null messages difference(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*отправляем разность сообщений -1(если сообщений нет)*/
				if( !(ret=sendall(sock,"-1",ntohl(sz))) ) {
					fprintf(log, "ERROR %s : SENDALL null messages difference(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			}
			mysql_free_result(result);
			/*отправляем размер количества прочитанных моих сообщений*/
			sz=htonl(get_str_size(row2[0]));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of quantity saw my messages(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*отправляем количество прочитанных моих сообщений*/
			if( !(ret=sendall(sock,row2[0],ntohl(sz))) ) {
				fprintf(log, "ERROR %s : SENDALL quantity saw my messages(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
		}
		else if(strcmp(signal,kakao_signal[BOB_CREATE_PARTY])==0)
		{
			char partyname[MAX_PARTYNAME];
			/*принимаем размер имени группы*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of party name(%d)\n",kakao_signal[BOB_CREATE_PARTY],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем имя группы*/
			if( (ret=recvall(sock,partyname,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL party name(%d)\n",kakao_signal[BOB_CREATE_PARTY],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем размер описания группы*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of party name(%d)\n",kakao_signal[BOB_CREATE_PARTY],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем описание группы*/
			if( (ret=recvall(sock,recv_buf,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL party name(%d)\n",kakao_signal[BOB_CREATE_PARTY],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			
			mysql_query(conn,"START TRANSACTION");
			sprintf(query_buf, "INSERT INTO parties(founder_id,name,description,path_avatar) VALUES(\"%s\",\"%s\",\"%s\",\"\")", my_info.id,partyname,recv_buf);
			if(mysql_query(conn,query_buf)!=0)
			{
				mysql_query(conn,"ROLLBACK");
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[BOB_CREATE_PARTY], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[BOB_CREATE_PARTY],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query create party(%d)\n",kakao_signal[BOB_CREATE_PARTY],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			mysql_query(conn,"COMMIT");

			mysql_query(conn,"START TRANSACTION");
			sprintf(query_buf, "SELECT id from parties WHERE founder_id = \"%s\" ORDER BY created_at DESC limit 1", my_info.id);
			if(mysql_query(conn,query_buf)!=0)
			{
				mysql_query(conn,"ROLLBACK");
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[BOB_CREATE_PARTY], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[BOB_CREATE_PARTY],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query select parties(%d)\n",kakao_signal[BOB_CREATE_PARTY],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			result=mysql_store_result(conn);
			if(mysql_num_rows(result)>0) {
				row=mysql_fetch_row(result);
				mysql_free_result(result);
				strcpy(party_info.id,row[0]);
			}
			else {
				mysql_query(conn,"ROLLBACK");
				mysql_free_result(result);
				continue;
			}
			sprintf(query_buf, "INSERT INTO party_users(party_id,user_id) VALUES(\"%s\",\"%s\")", party_info.id,my_info.id);
			if(mysql_query(conn,query_buf)!=0)
			{
				mysql_query(conn,"ROLLBACK");
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[BOB_CREATE_PARTY], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[BOB_CREATE_PARTY],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query party_users(%d)\n",kakao_signal[BOB_CREATE_PARTY],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			sprintf(query_buf, "INSERT INTO party_admins(party_id,admin_id) VALUES(\"%s\",\"%s\")", party_info.id,my_info.id);
			if(mysql_query(conn,query_buf)!=0)
			{
				mysql_query(conn,"ROLLBACK");
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[BOB_CREATE_PARTY], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[BOB_CREATE_PARTY],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query party_admins(%d)\n",kakao_signal[BOB_CREATE_PARTY],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			sprintf(query_buf, "INSERT INTO messages (from_id,text,party_id,important,removable) VALUES (\"%s\",\"Я создал(a) группу!\",\"%s\",true,false)",my_info.id, party_info.id);
			if(mysql_query(conn,query_buf)!=0)//создаем запись в parties(создаем группу)
			{
				mysql_query(conn,"ROLLBACK");
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[BOB_CREATE_PARTY], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[BOB_CREATE_PARTY],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query insert into messages party_id(%d)\n",kakao_signal[BOB_CREATE_PARTY],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			mysql_query(conn,"COMMIT");
		}
		else if(strcmp(signal,kakao_signal[SHOW_PROFILE_PARTY])==0)
		{
			/*-->мы НЕ вытаскиваем из бд party_id, т.к сделали уже это в show_messages<--*/

			if(flag_admin) {//если я админ группы
				/*отправляем положительный ответ*/
				sz=htonl(1);
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND answer + is an admin?(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			}
			else {
				/*отправляем отрицательный ответ*/
				sz=htonl(0);
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND answer - is an admin?(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			}
			if(flag_admin==false)
			{
				if(flag_party_user) {//если я пользователь группы
					/*отправляем положительный ответ*/
					sz=htonl(1);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND answer + is an party user?(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
				}
				else {
					/*отправляем отрицательный ответ*/
					sz=htonl(0);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND answer - is an party user?(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					continue;
				}
			}

			if(flag_party_user)//если я пользователь группы
			{
				/*получаем путь к аватарке группы*/
				sprintf(query_buf, "SELECT path_avatar,description FROM parties WHERE id=\"%s\"",party_info.id);
				if(mysql_query(conn,query_buf)!=0)
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_PROFILE_PARTY], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_PROFILE_PARTY],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query select path(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				result=mysql_store_result(conn);
				row=mysql_fetch_row(result);
				mysql_free_result(result);
				/*отправляем размер описания группы*/
				sz=htonl(get_str_size(row[1]));
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND size of description of the party(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*отправляем описание группы*/
				if( !(ret=sendall(sock,row[1],ntohl(sz))) ) {
					fprintf(log, "ERROR %s : SENDALL description of the party(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(row[0],"")==0) {//если аватарки нет 
					/*отправляем отрицательный ответ(аватарки нет)*/
					sz=htonl(0);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND answer havent avatar -(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
				} 
				else {
					/*отправляем положительный ответ(аватарка есть)*/
					sz=htonl(1);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND answer have avatar +(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					/*отправляем размер имени фото*/
					sz=htonl(get_str_size(from_path_to_filename(row[0])));
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND size of name of the image(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					/*отправляем имя фото*/
					if( !(ret=sendall(sock,from_path_to_filename(row[0]),ntohl(sz))) ) {
						fprintf(log, "ERROR %s : SENDALL name of the image(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					/*оптправляем аватарку*/
					if(!send_file(sock,row[0])) {//если произошла ошибка
						/*отправляем ноль*/
						sz=htonl(0);
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND zero(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
					}
				}
				/*принимаем размер никнейма*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of SearchEntry nickname(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем никнейм*/
				if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL SearchEntry nickname(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				/*принимаем размер limit*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of limit(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем limit*/
				if( (ret=recvall(sock,limit,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL limit(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				//извлекаем никнеймы пользователей группы  где никнейм равен тексту из search entry
				sprintf(query_buf, "SELECT nickname,id FROM users AS t1 INNER JOIN party_users AS t2 ON t1.id=t2.user_id AND t2.party_id=\"%s\" AND t2.user_id!=\"%s\" AND t1.nickname LIKE \"%s%%\" ORDER BY t1.id DESC LIMIT %s",party_info.id,my_info.id,people_info.nickname,limit);
				if(mysql_query(conn,query_buf)!=0)
				{// Получаем все сообщения
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_PROFILE_PARTY], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_PROFILE_PARTY],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query show party people list(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				result=mysql_store_result(conn);
				if(mysql_num_rows(result) > 0)
				{
					while((row=mysql_fetch_row(result)))
					{
						/*отправляем размер id пользователя*/
						sz=htonl(get_str_size(row[1]));
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND size of people id(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*отправляем id пользователя*/
						if( !(ret=sendall(sock,row[1],ntohl(sz))) ) {
							fprintf(log, "ERROR %s : SENDALL people id(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						/*отправляем размер никнейма пользователя*/
						sz=htonl(get_str_size(row[0]));
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND size of people nickname(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*отправляем никнейм пользователя*/
						if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
							fprintf(log, "ERROR %s : SENDALL people nickname(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						user_online(&my_data,row[1]);//отправляем онлайн ли юзер

						//смотрим является админом ли данный пользователь
						sprintf(query_buf, "SELECT count(*) FROM party_admins WHERE party_id=\"%s\" AND admin_id=\"%s\"",party_info.id,row[1]);
						if(mysql_query(conn,query_buf)!=0)
						{// Получаем все сообщения
							fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_PROFILE_PARTY], mysql_error(conn));//записываем ошибку в лог
							fflush(log);
							sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_PROFILE_PARTY],my_info.id );

							mysql_query(conn,query_buf);//записываем текст ошибки в бд

							/*принимаем ответ пользователя чтобы удалить запись из бд*/
							if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
							else if(ret<0) {
								fprintf(log,"ERROR %s: RECVALL mysql query is user an admin?(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							if(strcmp(recv_buf,"y")==0) {
								sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
								mysql_query(conn,query_buf);//записываем текст ошибки в бд
							}
							continue;//ждем следующий сигнал
						}
						result2=mysql_store_result(conn);
						row2=mysql_fetch_row(result2);
						mysql_free_result(result2);
						if(atoi(row2[0])>0) {//если юзер админ группы
							/*отправляем положительный ответ*/
							sz=htonl(1);
							if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
								fprintf(log, "ERROR %s : SEND  answer(1) is user an admin(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}
						}
						else {
							/*отправляем отрицательный ответ*/
							sz=htonl(0);
							if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
								fprintf(log, "ERROR %s : SEND  answer(0) is user an admin(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}
						}
					}
				}
				mysql_free_result(result);
				/*отправляем нулевой размер чтобы клиент понял где конец, чтобы выйти из ожидания*/
				sz=htonl(0);
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND size of end message(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(log);
				}
			}

		}
		else if(strcmp(signal,kakao_signal[SHOW_PROFILE_PARTY_USERS])==0)
		{
			/*принимаем размер никнейма*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of SearchEntry nickname(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем никнейм*/
			if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL SearchEntry nickname(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			/*принимаем размер limit*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of limit(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем limit*/
			if( (ret=recvall(sock,limit,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL limit(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			//извлекаем никнеймы пользователей группы  где никнейм равен тексту из search entry
			sprintf(query_buf, "SELECT nickname,id FROM users AS t1 INNER JOIN party_users AS t2 ON t1.id=t2.user_id AND t2.party_id=\"%s\" AND t2.user_id!=\"%s\" AND t1.nickname LIKE \"%s%%\" ORDER BY t1.id DESC LIMIT %s",party_info.id,my_info.id,people_info.nickname,limit);
			if(mysql_query(conn,query_buf)!=0)
			{// Получаем все сообщения
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_PROFILE_PARTY_USERS], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_PROFILE_PARTY_USERS],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query show party people list(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			result=mysql_store_result(conn);
			if(mysql_num_rows(result) > 0)
			{
				while((row=mysql_fetch_row(result)))
				{
					/*отправляем размер id пользователя*/
					sz=htonl(get_str_size(row[1]));
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND size of people id(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					/*отправляем id пользователя*/
					if( !(ret=sendall(sock,row[1],ntohl(sz))) ) {
						fprintf(log, "ERROR %s : SENDALL people id(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					/*отправляем размер никнейма пользователя*/
					sz=htonl(get_str_size(row[0]));
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND size of people nickname(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					/*отправляем никнейм пользователя*/
					if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
						fprintf(log, "ERROR %s : SENDALL people nickname(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					user_online(&my_data,row[1]);//отправляем онлайн ли юзер

					//смотрим является админом ли данный пользователь
					sprintf(query_buf, "SELECT count(*) FROM party_admins WHERE party_id=\"%s\" AND admin_id=\"%s\"",party_info.id,row[1]);
					if(mysql_query(conn,query_buf)!=0)
					{// Получаем все сообщения
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_PROFILE_PARTY_USERS], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_PROFILE_PARTY_USERS],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql query is user an admin?(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0) {
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
						continue;//ждем следующий сигнал
					}
					result2=mysql_store_result(conn);
					row2=mysql_fetch_row(result2);
					mysql_free_result(result2);
					if(atoi(row2[0])>0) {//если юзер админ группы
						/*отправляем положительный ответ*/
						sz=htonl(1);
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND  answer(1) is user an admin(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
					}
					else {
						/*отправляем отрицательный ответ*/
						sz=htonl(0);
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND  answer(0) is user an admin(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
					}
				}
			}
			mysql_free_result(result);
			/*отправляем нулевой размер чтобы клиент понял где конец, чтобы выйти из ожидания*/
			sz=htonl(0);
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of end message(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(log);
			}
		}
		else if(strcmp(signal,kakao_signal[SHOW_PROFILE_PARTY_AVATAR])==0)
		{
			/*получаем путь к аватарке группы*/
			sprintf(query_buf, "SELECT path_avatar FROM parties WHERE id=\"%s\"",party_info.id);
			if(mysql_query(conn,query_buf)!=0)
			{
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_PROFILE_PARTY_AVATAR], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_PROFILE_PARTY_AVATAR],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query friend or sub(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_AVATAR],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			result=mysql_store_result(conn);
			row=mysql_fetch_row(result);
			mysql_free_result(result);
			if(strcmp(row[0],"")==0) {//если аватарки нет 
				/*отправляем отрицательный ответ(аватарки нет)*/
				sz=htonl(0);
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND answer havent avatar -(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_AVATAR],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			} 
			else {
				/*отправляем положительный ответ(аватарка есть)*/
				sz=htonl(1);
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND answer have avatar +(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_AVATAR],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*отправляем размер имени фото*/
				sz=htonl(get_str_size(from_path_to_filename(row[0])));
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND size of name of the image(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_AVATAR],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*отправляем имя фото*/
				if( !(ret=sendall(sock,from_path_to_filename(row[0]),ntohl(sz))) ) {
					fprintf(log, "ERROR %s : SENDALL name of the image(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_AVATAR],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*оптправляем аватарку*/
				if(!send_file(sock,row[0])) {//если произошла ошибка
					/*отправляем ноль*/
					sz=htonl(0);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND zero(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_AVATAR],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
				}
			}
		}
		else if(strcmp(signal,kakao_signal[CHANGE_PARTY_AVATAR])==0)
		{
			if(flag_admin)//если юзер админ группы
			{
				char filename[MAX_FILENAME];
				char path[MAX_PATH];
				char buf[(MAX_PATH*2)+256];
				char *extension;
				/*принимаем размер имени фото(аватарки)*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of name of photo(%d)\n",kakao_signal[CHANGE_PARTY_AVATAR],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем имя фото*/
				if( (ret=recvall(sock,filename,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL name of photo(%d)\n",kakao_signal[CHANGE_PARTY_AVATAR],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				snprintf(path,sizeof(path),"%s/%s",PATHS[PATH_PARTY_AVATARS],party_info.id);//создаем путь к дирректории с id пользователя
				sprintf(buf,"if ! [ -d \"%s\" ]; then mkdir -p %s; fi",path,path);//создаем команду для создания дирректории
				system(buf);//создаем каталог если это нужно

				if((extension=from_filename_to_extension(filename))!=NULL) {//если у фото есть расширение 
					snprintf(path,sizeof(path),"%s/%s/%ld.%s",PATHS[PATH_PARTY_AVATARS],party_info.id,time(NULL),extension);//PATHS[PATH_PARTY_AVATARS]/id_группы/текущее_время.расширение
				}
				else {
					snprintf(path,sizeof(path),"%s/%s/%ld",PATHS[PATH_PARTY_AVATARS],party_info.id,time(NULL));//PATHS[PATH_PARTY_AVATARS]/id_группы/текущее_время
				}
				sprintf(buf,"rm -R %s/%s/*",PATHS[PATH_PARTY_AVATARS],party_info.id);//удаляем прошлую аватарку
				system(buf);
				if(recv_file(sock,path,NULL))//принимаем новую аватарку
				{
					/*изменяем путь к нашей аватарке*/
					sprintf(query_buf, "UPDATE parties SET path_avatar=\"%s\" WHERE id=\"%s\"",path,party_info.id);
					if(mysql_query(conn,query_buf)!=0)
					{//обрабатываем ошибку
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[CHANGE_PARTY_AVATAR], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[CHANGE_PARTY_AVATAR],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql query change party avatar(%d)\n",kakao_signal[CHANGE_PARTY_AVATAR],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0) {
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
						continue;//ждем следующий сигнал
					}
				}
			}
		}
		else if(strcmp(signal,kakao_signal[DELETE_PARTY_AVATAR])==0)
		{
			if(flag_admin)//если юзер админ группы
			{
				/*удаляем аватарку группы*/
				sprintf(query_buf, "UPDATE parties SET path_avatar=\"\" WHERE id=\"%s\"",party_info.id );
				if(mysql_query(conn,query_buf)!=0)
				{//обрабатываем ошибку
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_PARTY_AVATAR], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_PARTY_AVATAR],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query delete party avatar(%d)\n",kakao_signal[DELETE_PARTY_AVATAR],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				sprintf(recv_buf,"rm -R %s/%s/*",PATHS[PATH_PARTY_AVATARS],party_info.id);//удаляем прошлую аватарку
				system(recv_buf);
			}
		}
		else if(strcmp(signal,kakao_signal[CHANGE_PARTY_NAME])==0)
		{
			if(flag_admin)//если юзер админ группы
			{
				char partyname[MAX_PARTYNAME];
				/*принимаем размер имени группы*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of party name(%d)\n",kakao_signal[CHANGE_PARTY_NAME],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем имя группы*/
				if( (ret=recvall(sock,partyname,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL party name(%d)\n",kakao_signal[CHANGE_PARTY_NAME],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				sprintf(query_buf, "UPDATE parties SET name = \"%s\" WHERE id= \"%s\"",partyname,party_info.id);
				if(mysql_query(conn,query_buf)!=0)//создаем запись в parties(создаем группу)
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[CHANGE_PARTY_NAME], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[CHANGE_PARTY_NAME],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query change party name?(%d)\n",kakao_signal[CHANGE_PARTY_NAME],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}

					/*отправляем отрицательный ответ(не получилось изменить имя)*/
					sz=htonl(0);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND answer - change party name?(%d)\n",kakao_signal[CHANGE_PARTY_NAME],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					continue;//ждем следующий сигнал
				}
				/*отправляем положительный ответ(получилось изменить имя)*/
				sz=htonl(1);
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND answer + change party name?(%d)\n",kakao_signal[CHANGE_PARTY_NAME],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			}
		}
		else if(strcmp(signal,kakao_signal[CHANGE_PARTY_DESCRIPTION])==0)
		{
			if(flag_admin)//если юзер админ группы
			{
				/*принимаем размер описания группы*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of party description(%d)\n",kakao_signal[CHANGE_PARTY_DESCRIPTION],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем описание группы*/
				if( (ret=recvall(sock,recv_buf,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL party description(%d)\n",kakao_signal[CHANGE_PARTY_DESCRIPTION],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				sprintf(query_buf, "UPDATE parties SET description = \"%s\" WHERE id= \"%s\"",recv_buf,party_info.id);
				if(mysql_query(conn,query_buf)!=0)//создаем запись в parties(создаем группу)
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[CHANGE_PARTY_DESCRIPTION], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[CHANGE_PARTY_DESCRIPTION],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query change party description(%d)\n",kakao_signal[CHANGE_PARTY_DESCRIPTION],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}

					/*отправляем отрицательный ответ(не получилось изменить имя)*/
					sz=htonl(0);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND answer - change party description?(%d)\n",kakao_signal[CHANGE_PARTY_DESCRIPTION],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					continue;//ждем следующий сигнал
				}
				/*отправляем положительный ответ(получилось изменить имя)*/
				sz=htonl(1);
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND answer + change party description?(%d)\n",kakao_signal[CHANGE_PARTY_DESCRIPTION],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

			}
		}
		else if(strcmp(signal,kakao_signal[SHOW_PARTY_APPOINT_ADMINS])==0)
		{
			if(flag_admin)//если юзер админ группы
			{
				/*принимаем размер никнейма*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of SearchEntry nickname(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем никнейм*/
				if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL SearchEntry nickname(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				/*принимаем размер limit*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of limit(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем limit*/
				if( (ret=recvall(sock,limit,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL limit(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				//извлекаем никнеймы пользователей группы , которые не являются админами и где никнейм равен тексту из search entry
				sprintf(query_buf, "SELECT t1.nickname,t1.id FROM users AS t1 INNER JOIN party_users AS t2 ON t1.id=t2.user_id AND t2.party_id=\"%s\" AND t2.user_id NOT IN(SELECT admin_id FROM party_admins WHERE party_id=%s) AND t1.nickname LIKE \"%s%%\" ORDER BY t1.id DESC LIMIT %s",party_info.id,party_info.id,people_info.nickname,limit);
				if(mysql_query(conn,query_buf)!=0)
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_PARTY_APPOINT_ADMINS],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query show party people list(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				result=mysql_store_result(conn);
				if(mysql_num_rows(result) > 0)
				{
					while((row=mysql_fetch_row(result)))
					{
						/*отправляем размер id пользователя*/
						sz=htonl(get_str_size(row[1]));
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND size of people id(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*отправляем id пользователя*/
						if( !(ret=sendall(sock,row[1],ntohl(sz))) ) {
							fprintf(log, "ERROR %s : SENDALL people id(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						/*отправляем размер никнейма пользователя*/
						sz=htonl(get_str_size(row[0]));
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND size of people nickname(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*отправляем никнейм пользователя*/
						if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
							fprintf(log, "ERROR %s : SENDALL people nickname(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
					}
				}
				mysql_free_result(result);
			}
			/*отправляем нулевой размер чтобы клиент понял где конец, чтобы выйти из ожидания*/
			sz=htonl(0);
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of end message(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(log);
			}
		}
		else if(strcmp(signal,kakao_signal[PARTY_ADD_ADMIN])==0)
		{
			if(flag_admin)//если юзер админ группы
			{
				/*принимаем размер id*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of people id(%d)\n",kakao_signal[PARTY_ADD_ADMIN],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем id*/
				if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL people id(%d)\n",kakao_signal[PARTY_ADD_ADMIN],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				//добавляем админа в группу
				sprintf(query_buf, "INSERT INTO party_admins(party_id,admin_id) VALUES(\"%s\",\"%s\")",party_info.id,people_info.id);
				if(mysql_query(conn,query_buf)!=0)
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[PARTY_ADD_ADMIN], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[PARTY_ADD_ADMIN],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query add party admin(%d)\n",kakao_signal[PARTY_ADD_ADMIN],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
			}
		}
		else if(strcmp(signal,kakao_signal[SHOW_PARTY_DELETE_ADMINS])==0)
		{
			if(flag_admin)//если юзер админ группы
			{
				/*принимаем размер никнейма*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of SearchEntry nickname(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем никнейм*/
				if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL SearchEntry nickname(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				/*принимаем размер limit*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of limit(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем limit*/
				if( (ret=recvall(sock,limit,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL limit(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				//извлекаем никнеймы пользователей группы , которые  являются админами и где никнейм равен тексту из search entry
				sprintf(query_buf, "SELECT t1.nickname,t1.id FROM users AS t1 INNER JOIN party_admins AS t2 ON t1.id=t2.admin_id AND t2.party_id=\"%s\" AND t2.admin_id!=\"%s\" AND t1.nickname LIKE \"%s%%\" ORDER BY t1.id DESC LIMIT %s",party_info.id,my_info.id,people_info.nickname,limit);
				if(mysql_query(conn,query_buf)!=0)
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_PARTY_DELETE_ADMINS],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query show party admins list(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				result=mysql_store_result(conn);
				if(mysql_num_rows(result) > 0)
				{
					while((row=mysql_fetch_row(result)))
					{
						/*отправляем размер id пользователя*/
						sz=htonl(get_str_size(row[1]));
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND size of people id(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*отправляем id пользователя*/
						if( !(ret=sendall(sock,row[1],ntohl(sz))) ) {
							fprintf(log, "ERROR %s : SENDALL people id(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						/*отправляем размер никнейма пользователя*/
						sz=htonl(get_str_size(row[0]));
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND size of people nickname(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*отправляем никнейм пользователя*/
						if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
							fprintf(log, "ERROR %s : SENDALL people nickname(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
					}
				}
				mysql_free_result(result);
			}
			/*отправляем нулевой размер чтобы клиент понял где конец, чтобы выйти из ожидания*/
			sz=htonl(0);
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of end message(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(log);
			}
		}
		else if(strcmp(signal,kakao_signal[PARTY_DELETE_ADMIN])==0)
		{
			if(flag_admin)//если юзер админ группы
			{
				/*принимаем размер id*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of people id(%d)\n",kakao_signal[PARTY_DELETE_ADMIN],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем id*/
				if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL people id(%d)\n",kakao_signal[PARTY_DELETE_ADMIN],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				//удаляем админа 
				sprintf(query_buf, "DELETE FROM party_admins WHERE party_id=\"%s\" AND admin_id=\"%s\"",party_info.id,people_info.id);
				if(mysql_query(conn,query_buf)!=0)
				{// Получаем все сообщения
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[PARTY_DELETE_ADMIN], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[PARTY_DELETE_ADMIN],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query delete party admin(%d)\n",kakao_signal[PARTY_DELETE_ADMIN],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
			}
		}
		else if(strcmp(signal,kakao_signal[SHOW_PARTY_ADD_FRIENDS])==0)
		{
			if(flag_party_user)//если юзер участник группы
			{
				/*принимаем размер никнейма*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of SearchEntry nickname(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем никнейм*/
				if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL SearchEntry nickname(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				/*принимаем размер limit*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of limit(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем limit*/
				if( (ret=recvall(sock,limit,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL limit(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*получаем данные друзей с никнеймом, ориентируясь на тот,который пользователь ввел в поиске*/
				sprintf(query_buf, "SELECT t1.nickname ,t2.friend_id FROM users AS t1 INNER JOIN friends AS t2 ON t1.id = t2.friend_id AND t2.user_id = \"%s\" AND t1.nickname LIKE \"%s%%\" AND EXISTS (SELECT user_id FROM friends WHERE friend_id = \"%s\" AND user_id=t1.id) AND t1.id NOT IN (SELECT user_id FROM party_users WHERE party_id=\"%s\") ORDER BY t1.id LIMIT %s", my_info.id,people_info.nickname,my_info.id,party_info.id,limit);
				if(mysql_query(conn,query_buf)!=0)
				{// Получаем все сообщения
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_PARTY_ADD_FRIENDS],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query show party friends list(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				result=mysql_store_result(conn);
				if(mysql_num_rows(result) > 0)
				{
					while((row=mysql_fetch_row(result)))
					{
						/*отправляем размер id пользователя*/
						sz=htonl(get_str_size(row[1]));
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND size of friend id(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*отправляем id пользователя*/
						if( !(ret=sendall(sock,row[1],ntohl(sz))) ) {
							fprintf(log, "ERROR %s : SENDALL friend id(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						/*отправляем размер никнейма пользователя*/
						sz=htonl(get_str_size(row[0]));
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND size of friend nickname(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
						/*отправляем никнейм пользователя*/
						if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
							fprintf(log, "ERROR %s : SENDALL friend nickname(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}
					}
				}
				mysql_free_result(result);
			}
			/*отправляем нулевой размер чтобы клиент понял где конец, чтобы выйти из ожидания*/
			sz=htonl(0);
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of end message(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(log);
			}
		}
		else if(strcmp(signal,kakao_signal[PARTY_ADD_FRIEND])==0)
		{
			if(flag_party_user)//если юзер участник группы
			{
				/*принимаем размер id*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of people id(%d)\n",kakao_signal[PARTY_ADD_FRIEND],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем id*/
				if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL people id(%d)\n",kakao_signal[PARTY_ADD_FRIEND],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				sprintf(query_buf, "INSERT INTO party_users(party_id,user_id) VALUES(\"%s\",\"%s\")",party_info.id,people_info.id);
				if(mysql_query(conn,query_buf)!=0)
				{// Получаем все сообщения
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[PARTY_ADD_FRIEND], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[PARTY_ADD_FRIEND],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query party add_friend(%d)\n",kakao_signal[PARTY_ADD_FRIEND],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
			}
		}
		else if(strcmp(signal,kakao_signal[PARTY_DELETE_USER])==0)
		{
			if(flag_admin)//если юзер админ группы
			{
				/*принимаем размер id*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of people id(%d)\n",kakao_signal[PARTY_DELETE_USER],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем id*/
				if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL people id(%d)\n",kakao_signal[PARTY_DELETE_USER],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				sprintf(query_buf, "DELETE FROM party_users WHERE party_id=\"%s\" AND user_id=\"%s\"",party_info.id,people_info.id);
				if(mysql_query(conn,query_buf)!=0)
				{// Получаем все сообщения
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[PARTY_DELETE_USER], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[PARTY_DELETE_USER],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query party delete user(%d)\n",kakao_signal[PARTY_DELETE_USER],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
			}
		}
		else if(strcmp(signal,kakao_signal[PARTY_EXIT])==0)
		{
			if(flag_party_user)//если юзер участник группы
			{
				/*смотрим количество участников группы*/
				sprintf(query_buf, "SELECT count(*) FROM party_users WHERE party_id=\"%s\"",party_info.id);
				if(mysql_query(conn,query_buf)!=0)
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[PARTY_EXIT], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[PARTY_EXIT],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query select quantity party users(%d)\n",kakao_signal[PARTY_EXIT],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					sz=htonl(0);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND size of 0 message(%d)\n",kakao_signal[PARTY_EXIT],ret ); fflush(log);
					}
					continue;//ждем следующий сигнал
				}
				result=mysql_store_result(conn);
				row=mysql_fetch_row(result);
				mysql_free_result(result);
				if(atoi(row[0])==1)//если в группе всего 1 человек
				{
					//mysql_query(conn,"START TRANSACTION");
					
					/*удаляем все файлы или фото сообщений связанные с группой*/
					sprintf(recv_buf,"rm -R %s/%s",PATHS[PATH_PARTY_FILES],party_info.id);//удаляем каталог с id нашей группы
					system(recv_buf);
					sprintf(recv_buf,"rm -R %s/%s",PATHS[PATH_PARTY_PHOTOS],party_info.id);//удаляем каталог с id нашей группы
					system(recv_buf);

					/*удаляем группу,а вместе и с ней удаляются все сообщения и данные в таблицах party_user и т.п*/
					sprintf(query_buf, "DELETE FROM parties WHERE id=\"%s\"",party_info.id);
					if(mysql_query(conn,query_buf)!=0)
					{
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[PARTY_EXIT], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						//mysql_query(conn,"ROLLBACK");
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[PARTY_EXIT],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql query delete party (%d)\n",kakao_signal[PARTY_EXIT],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0) {
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
						sz=htonl(0);
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND size of 0 message(%d)\n",kakao_signal[PARTY_EXIT],ret ); fflush(log);
						}
						continue;//ждем следующий сигнал
					}
					//mysql_query(conn,"COMMIT");
					sz=htonl(1);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND size of 1 message(%d)\n",kakao_signal[PARTY_EXIT],ret ); fflush(log);
					}
					continue;
				}
				else
				{
					if(flag_admin)//если юзер админ группы
					{
						sprintf(query_buf, "SELECT count(*) FROM party_admins WHERE party_id=\"%s\"",party_info.id);
						if(mysql_query(conn,query_buf)!=0)
						{
							fprintf(log, "ERROR: %s\t%s\n",kakao_signal[PARTY_EXIT], mysql_error(conn));//записываем ошибку в лог
							fflush(log);
							sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[PARTY_EXIT],my_info.id );

							mysql_query(conn,query_buf);//записываем текст ошибки в бд

							/*принимаем ответ пользователя чтобы удалить запись из бд*/
							if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
							else if(ret<0) {
								fprintf(log,"ERROR %s: RECVALL mysql query flag_admin select quantity admins(%d)\n",kakao_signal[PARTY_EXIT],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							if(strcmp(recv_buf,"y")==0) {
								sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
								mysql_query(conn,query_buf);//записываем текст ошибки в бд
							}
							continue;//ждем следующий сигнал
						}
						result=mysql_store_result(conn);
						row=mysql_fetch_row(result);
						mysql_free_result(result);
						if(atoi(row[0])>1)//если есть как минимум два админа
						{
							sprintf(query_buf, "DELETE FROM party_users WHERE party_id=\"%s\" AND user_id=\"%s\"",party_info.id,my_info.id);
							if(mysql_query(conn,query_buf)!=0)
							{
								fprintf(log, "ERROR: %s\t%s\n",kakao_signal[PARTY_EXIT], mysql_error(conn));//записываем ошибку в лог
								fflush(log);
								sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[PARTY_EXIT],my_info.id );

								mysql_query(conn,query_buf);//записываем текст ошибки в бд

								/*принимаем ответ пользователя чтобы удалить запись из бд*/
								if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
								else if(ret<0) {
									fprintf(log,"ERROR %s: RECVALL mysql query flag_admin (%d)\n",kakao_signal[PARTY_EXIT],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}

								if(strcmp(recv_buf,"y")==0) {
									sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
									mysql_query(conn,query_buf);//записываем текст ошибки в бд
								}
								continue;//ждем следующий сигнал
							}
							sz=htonl(1);
							if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
								fprintf(log, "ERROR %s : SEND size of 1 message(%d)\n",kakao_signal[PARTY_EXIT],ret ); fflush(log);
							}

						}
						else
						{
							/*отправляем нулевой размер чтобы клиент понял что нужно добавить еще одного админа прежде чем выйти*/
							sz=htonl(0);
							if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
								fprintf(log, "ERROR %s : SEND size of 0 message(%d)\n",kakao_signal[PARTY_EXIT],ret ); fflush(log);
							}
							continue;
						}
					}
					else
					{
						sprintf(query_buf, "DELETE FROM party_users WHERE party_id=\"%s\" AND user_id=\"%s\"",party_info.id,my_info.id);
						if(mysql_query(conn,query_buf)!=0)
						{
							fprintf(log, "ERROR: %s\t%s\n",kakao_signal[PARTY_EXIT], mysql_error(conn));//записываем ошибку в лог
							fflush(log);
							sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[PARTY_EXIT],my_info.id );

							mysql_query(conn,query_buf);//записываем текст ошибки в бд

							/*принимаем ответ пользователя чтобы удалить запись из бд*/
							if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
							else if(ret<0) {
								fprintf(log,"ERROR %s: RECVALL mysql query flag_party_user(%d)\n",kakao_signal[PARTY_EXIT],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							if(strcmp(recv_buf,"y")==0) {
								sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
								mysql_query(conn,query_buf);//записываем текст ошибки в бд
							}
							continue;//ждем следующий сигнал
						}
						sz=htonl(1);
						if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
							fprintf(log, "ERROR %s : SEND size of 1 message(%d)\n",kakao_signal[PARTY_EXIT],ret ); fflush(log);
						}
					}
				}
			}
			else//если юзер пытается нас обмануть(он не пользователь группы)
			{
				sz=htonl(0);
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND size of 0 message(%d)\n",kakao_signal[PARTY_EXIT],ret ); fflush(log);
				}
			}

		}
		else if(strcmp(signal,kakao_signal[DELETE_MESSAGE_EVERYWHERE])==0)
		{
			/*принимаем размер id сообщения*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
			fprintf(log,"ERROR %s: RECV size of message id(%d)\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем id сообщения*/
			if( (ret=recvall(sock,message_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL message id(%d)\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			sprintf(query_buf, "SELECT count(*) FROM messages WHERE id=\"%s\" AND from_id=\"%s\"",message_info.id,my_info.id);
			if(mysql_query(conn,query_buf)!=0)//проверяем что сообщение отправлено от меня(проверяем,не хочет ли пользователь обмануть нас)
			{
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MESSAGE_EVERYWHERE],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query Has a message been sent from me?(%d)\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
			}
			result=mysql_store_result(conn);
			row=mysql_fetch_row(result);
			mysql_free_result(result);
			if(atoi(row[0]))//если сообщение отправлено от меня
			{
				sprintf(query_buf, "SELECT (SELECT CASE WHEN file_id>0 THEN file_id ELSE '0' END) AS file_id,(SELECT CASE WHEN photo_id>0 THEN photo_id ELSE '0' END) AS photo_id FROM messages WHERE id=\"%s\" AND removable",message_info.id);
				if(mysql_query(conn,query_buf)!=0)//проверяем что сообщение отправлено от меня(проверяем,не хочет ли пользователь обмануть нас)
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MESSAGE_EVERYWHERE],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query What type of message?(%d)\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
				}
				result=mysql_store_result(conn);
				row=mysql_fetch_row(result);
				if(atoi(row[0])) {//если сообщение содержит файл
					sprintf(query_buf, "SELECT path FROM files WHERE id=\"%s\"",row[0]);
					if(mysql_query(conn,query_buf)!=0)//извлекаем путь к файлу
					{
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MESSAGE_EVERYWHERE],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql query path of the file(%d)\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0) {
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
					}
					result2=mysql_store_result(conn);
					row2=mysql_fetch_row(result2);

					sprintf(recv_buf,"rm %s",row2[0]);
					mysql_free_result(result2);
					//удаляем файл из системы
					if(system(recv_buf)!=0)//если ошибка
					{
						continue;
					}
				}
				else if(atoi(row[1])) {//если сообщение содержит фото
					sprintf(query_buf, "SELECT path FROM photos WHERE id=\"%s\"",row[1]);
					if(mysql_query(conn,query_buf)!=0)//извлекаем путь к фото
					{
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MESSAGE_EVERYWHERE],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql query path of the photo(%d)\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0) {
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
					}
					result2=mysql_store_result(conn);
					row2=mysql_fetch_row(result2);
					
					sprintf(recv_buf,"rm %s",row2[0]);
					mysql_free_result(result2);
					//удаляем фото из системы
					if(system(recv_buf)!=0)//если ошибка
					{
						continue;
					}
				}
				mysql_free_result(result);

				sprintf(query_buf, "DELETE FROM messages WHERE id=\"%s\"",message_info.id);
				if(mysql_query(conn,query_buf)!=0)//удаляем сообщение
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MESSAGE_EVERYWHERE],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query delete message(%d)\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
				}
				if(strcmp(dialog_type,text_dialog_type[COMPANION])==0)
				{
					//смотрим количество сообщений с собеседником
					sprintf(query_buf, "SELECT count(*) FROM messages WHERE (from_id=\"%s\" AND to_id=\"%s\") OR (to_id=\"%s\" AND from_id=\"%s\") ",people_info.id,my_info.id,people_info.id,my_info.id);
					if(mysql_query(conn,query_buf)!=0)
					{
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MESSAGE_EVERYWHERE],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql query quantity of messages with companion?(%d)\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0) {
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
						continue;//ждем следующий сигнал
					}
				}
				else if(strcmp(dialog_type,text_dialog_type[PARTY])==0)
				{
					//смотрим количество сообщений с группой
					sprintf(query_buf, "SELECT count(*) FROM messages WHERE (party_id=\"%s\")",party_info.id);
					if(mysql_query(conn,query_buf)!=0)
					{
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MESSAGE_EVERYWHERE],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql query quantity of messages in party?(%d)\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0) {
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
						continue;//ждем следующий сигнал
					}
				}

				result=mysql_store_result(conn);
				row=mysql_fetch_row(result);
				mysql_free_result(result);
				if(atoi(row[0])==0)
				{
					if(strcmp(dialog_type,text_dialog_type[COMPANION])==0)
					{
						//извлекаем id диалога
						sprintf(query_buf, "SELECT id FROM dialog WHERE (user1_id=\"%s\" AND user2_id=\"%s\") OR (user2_id=\"%s\" AND user1_id=\"%s\")",my_info.id,people_info.id,my_info.id,people_info.id);
						if(mysql_query(conn,query_buf)!=0)
						{
							fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE], mysql_error(conn));//записываем ошибку в лог
							fflush(log);
							sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MESSAGE_EVERYWHERE],my_info.id );

							mysql_query(conn,query_buf);//записываем текст ошибки в бд

							/*принимаем ответ пользователя чтобы удалить запись из бд*/
							if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
							else if(ret<0) {
								fprintf(log,"ERROR %s: RECVALL mysql query id of the dialog?(%d)\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							if(strcmp(recv_buf,"y")==0) {
								sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
								mysql_query(conn,query_buf);//записываем текст ошибки в бд
							}
							continue;//ждем следующий сигнал
						}
						result=mysql_store_result(conn);
						row=mysql_fetch_row(result);

						sprintf(recv_buf,"rm -R %s/%s",PATHS[PATH_COMPANION_FILES],row[0]);//удаляем каталог с id нашего диалога
						system(recv_buf);
						sprintf(recv_buf,"rm -R %s/%s",PATHS[PATH_COMPANION_PHOTOS],row[0]);//удаляем каталог с id нашего диалога
						system(recv_buf);
						mysql_free_result(result);
						/*удаляем запись что мы имеем диалог с юзером*/
						sprintf(query_buf, "DELETE FROM dialog WHERE (user1_id=\"%s\" AND user2_id=\"%s\") OR (user2_id=\"%s\" AND user1_id=\"%s\")",my_info.id,people_info.id,my_info.id,people_info.id);
						if(mysql_query(conn,query_buf)!=0)//удаляем его из друзей или подписок
						{
							fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE], mysql_error(conn));//записываем ошибку в лог
							fflush(log);
							sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MESSAGE_EVERYWHERE],my_info.id );

							mysql_query(conn,query_buf);//записываем текст ошибки в бд

							/*принимаем ответ пользователя чтобы удалить запись из бд*/
							if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
							else if(ret<0) {
								fprintf(log,"ERROR %s: RECVALL mysql query  delete record in dialog(%d)\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							if(strcmp(recv_buf,"y")==0) {
								sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
								mysql_query(conn,query_buf);//записываем текст ошибки в бд
							}
							continue;//ждем следующий сигнал
						}
					}
					else if(strcmp(dialog_type,text_dialog_type[PARTY])==0)
					{
						sprintf(recv_buf,"rm -R %s/%s",PATHS[PATH_PARTY_FILES],party_info.id);//удаляем каталог с id нашей группы
						system(recv_buf);
						sprintf(recv_buf,"rm -R %s/%s",PATHS[PATH_PARTY_PHOTOS],party_info.id);//удаляем каталог с id нашей группы
						system(recv_buf);
					}
				}
			}
		}
		else if(strcmp(signal,kakao_signal[EDIT_MESSAGE])==0)
		{
			/*принимаем размер id сообщения*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of message id(%d)\n",kakao_signal[EDIT_MESSAGE],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем id сообщения*/
			if( (ret=recvall(sock,message_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL message id(%d)\n",kakao_signal[EDIT_MESSAGE],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем размер сообщения*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of message(%d)\n",kakao_signal[EDIT_MESSAGE],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем сообщение*/
			if( (ret=recvall(sock,recv_buf,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL message(%d)\n",kakao_signal[EDIT_MESSAGE],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			sprintf(query_buf, "SELECT (SELECT CASE WHEN file_id>0 THEN file_id ELSE '0' END) AS file_id,(SELECT CASE WHEN photo_id>0 THEN photo_id ELSE '0' END) AS photo_id FROM messages WHERE id=\"%s\" AND from_id=\"%s\"",message_info.id,my_info.id);
			if(mysql_query(conn,query_buf)!=0)//проверяем что сообщение не содержит файл или изображение(проверяем,не хочет ли пользователь обмануть нас)
			{
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[EDIT_MESSAGE], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[EDIT_MESSAGE],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query check that the message does not contain a file or image(%d)\n",kakao_signal[EDIT_MESSAGE],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
			}
			result=mysql_store_result(conn);
			row=mysql_fetch_row(result);
			mysql_free_result(result);
			if(atoi(row[0]) && atoi(row[1]))
			{
				sprintf(query_buf, "SELECT count(*) FROM messages WHERE id=\"%s\" AND from_id=\"%s\"",message_info.id,my_info.id);
				if(mysql_query(conn,query_buf)!=0)//проверяем что сообщение отправлено от меня(проверяем,не хочет ли пользователь обмануть нас)
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[EDIT_MESSAGE], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[EDIT_MESSAGE],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query Has a message been sent from me?(%d)\n",kakao_signal[EDIT_MESSAGE],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
				}
				result=mysql_store_result(conn);
				row=mysql_fetch_row(result);
				mysql_free_result(result);
				if(atoi(row[0]))//если сообщение отправлено от меня
				{
					sprintf(query_buf, "UPDATE messages SET text=\"%s\" WHERE id=\"%s\"",recv_buf,message_info.id);
					if(mysql_query(conn,query_buf)!=0)//удаляем сообщение
					{
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[EDIT_MESSAGE], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[EDIT_MESSAGE],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql query edit message(%d)\n",kakao_signal[EDIT_MESSAGE],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0) {
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
					}
				}
			}
		}
		else if(strcmp(signal,kakao_signal[I_WRITE])==0)
		{
			/*принимаем размер размер типа сообщения*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of message type(%d)\n",kakao_signal[I_WRITE],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем тип сообщения*/
			if( (ret=recvall(sock,dialog_type,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL message type(%d)\n",kakao_signal[I_WRITE],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			if(strcmp(dialog_type,text_dialog_type[COMPANION])==0)
			{
				/*принимаем размер id пользователя */
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of people id(%d)\n",kakao_signal[I_WRITE],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем id пользователя*/
				if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL people id(%d)\n",kakao_signal[I_WRITE],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				sprintf(query_buf, "UPDATE companion_writes SET to_user_id=\"%s\",when_writed=NOW() WHERE user_id=\"%s\"",people_info.id,my_info.id);
				if(mysql_query(conn,query_buf)!=0)//обновляем запись что мы пишем сообщение
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[I_WRITE], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[I_WRITE],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query set write companion(%d)\n",kakao_signal[I_WRITE],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
				}
			}
			else if(strcmp(dialog_type,text_dialog_type[PARTY])==0)
			{
				/*принимаем размер id группы */
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of party id(%d)\n",kakao_signal[I_WRITE],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем id группы*/
				if( (ret=recvall(sock,party_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL party id(%d)\n",kakao_signal[I_WRITE],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				sprintf(query_buf, "UPDATE party_writes SET to_party_id=\"%s\",when_writed=NOW() WHERE user_id=\"%s\"",party_info.id,my_info.id);
				if(mysql_query(conn,query_buf)!=0)//обновляем запись что мы пишем сообщение
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[I_WRITE], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[I_WRITE],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query set write party(%d)\n",kakao_signal[I_WRITE],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
				}
			}
		}
		else if(strcmp(signal,kakao_signal[SHOW_DIALOG_WINDOW])==0)
		{
			/*принимаем размер типа диалога*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of type of dialog(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем тип диалога*/
			if( (ret=recvall(sock,dialog_type,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL type of dialog(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*мы не принимаем некоторые данные, т.к они уже были приняты в show_messages*/
			if(strcmp(dialog_type,text_dialog_type[COMPANION])==0)
			{
				/*принимаем размер id собеседника */
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of people id(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем id собеседника*/
				if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL people id(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
								   
				sprintf(query_buf, "insert into who_saw_message(message_id,who_saw_id) select id,\"%s\" from messages WHERE (from_id = \"%s\" AND to_id = \"%s\") AND id NOT IN(select message_id from who_saw_message)",my_info.id,people_info.id,my_info.id);
				if(mysql_query(conn,query_buf)!=0)// записываем что все сообщения данного пользователя прочитаны мной
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_DIALOG_WINDOW], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_DIALOG_WINDOW],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s  %s: RECVALL mysql query i saw your fucking messages(%d)\n",text_dialog_type[COMPANION],kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}

				i=user_writing(&my_data,people_info.id,my_info.id);//смотрим печатает ли юзер сообщение
				/*отправляем печатает ли юзер сообщение*/
				sz=htonl( i );
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND writing?(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*если не печатает сообщение */ 
				if(!i) {
					user_online(&my_data,people_info.id);//отправляем онлайн ли юзер
				}
				
			}
			else if(strcmp(dialog_type,text_dialog_type[PARTY])==0)
			{
				/*принимаем размер id группы */
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of party id(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем id группы*/
				if( (ret=recvall(sock,party_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL party id(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				//проверяем, админ ли я этой группы
				sprintf(query_buf, "SELECT count(*) FROM party_admins WHERE party_id=\"%s\" AND admin_id=\"%s\"",party_info.id,my_info.id);
				if(mysql_query(conn,query_buf)!=0)
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_DIALOG_WINDOW], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_DIALOG_WINDOW],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query is an admin?(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				result=mysql_store_result(conn);
				row=mysql_fetch_row(result);
				mysql_free_result(result);
				if(atoi(row[0])>0) {//если я админ группы
					flag_admin=true;
					flag_party_user=true;
				}
				else {
					flag_admin=false;
					//проверяем, пользователь ли я этой группы
					sprintf(query_buf, "SELECT count(*) FROM party_users WHERE party_id=\"%s\" AND user_id=\"%s\"",party_info.id,my_info.id);
					if(mysql_query(conn,query_buf)!=0)
					{
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_DIALOG_WINDOW], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_DIALOG_WINDOW],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql query is an party user?(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0) {
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
						continue;//ждем следующий сигнал
					}
					result=mysql_store_result(conn);
					row=mysql_fetch_row(result);
					if(atoi(row[0])>0) {//если я пользователь группы
						flag_party_user=true;
					} 
					else {
						flag_party_user=false;
					}
					mysql_free_result(result);
				}

				if(flag_party_user)//если я пользователь группы
				{
					sprintf(query_buf, "INSERT INTO who_saw_message(message_id,who_saw_id) select id,\"%s\" FROM messages WHERE party_id=\"%s\" AND from_id!=\"%s\" AND EXISTS(SELECT message_id FROM who_saw_message WHERE message_id=id AND who_saw_id=\"%s\")!=true",my_info.id,party_info.id,my_info.id,my_info.id);
					if(mysql_query(conn,query_buf)!=0)// записываем что все сообщения данного пользователя прочитаны мной
					{
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_DIALOG_WINDOW], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_DIALOG_WINDOW],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s  %s: RECVALL mysql query i saw your fucking messages(%d)\n",text_dialog_type[PARTY],kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0) {
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
						continue;//ждем следующий сигнал
					}

					party_writing(&my_data,party_info.id);//отправляем никнеймы участников беседы которые печатают сообщение
				}
				else 
				{
					/*отправляем нулевой размер */
					sz=htonl(0);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND size of 0 message(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(log);
					}
					continue;
				}
				continue;
				
			}
		}
		else if(strcmp(signal,kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG])==0)
		{
			if(strcmp(dialog_type,text_dialog_type[COMPANION])==0) 
			{
				sprintf(query_buf, "SELECT (SELECT CASE WHEN file_id>0 THEN file_id ELSE '0' END) AS file_id,(SELECT CASE WHEN photo_id>0 THEN photo_id ELSE '0' END) AS photo_id FROM messages WHERE (from_id=\"%s\" AND to_id=\"%s\") AND removable",my_info.id,people_info.id);
				if(mysql_query(conn,query_buf)!=0)//проверяем что сообщение отправлено от меня(проверяем,не хочет ли пользователь обмануть нас)
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MY_MESSAGES_FROM_DIALOG],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query What type of message?(%d)\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
				}
				result=mysql_store_result(conn);
				while((row=mysql_fetch_row(result)))
				{
					if(atoi(row[0])) //если сообщение содержит файл
					{
						sprintf(query_buf, "SELECT path FROM files WHERE id=\"%s\"",row[0]);
						if(mysql_query(conn,query_buf)!=0)//извлекаем путь к файлу
						{
							fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG], mysql_error(conn));//записываем ошибку в лог
							fflush(log);
							sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MY_MESSAGES_FROM_DIALOG],my_info.id );

							mysql_query(conn,query_buf);//записываем текст ошибки в бд

							/*принимаем ответ пользователя чтобы удалить запись из бд*/
							if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
							else if(ret<0) {
								fprintf(log,"ERROR %s: RECVALL mysql query path of the file(%d)\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							if(strcmp(recv_buf,"y")==0) {
								sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
								mysql_query(conn,query_buf);//записываем текст ошибки в бд
							}
						}
						result2=mysql_store_result(conn);
						row2=mysql_fetch_row(result2);

						sprintf(recv_buf,"rm %s",row2[0]);
						mysql_free_result(result2);
						//удаляем файл из системы
						if(system(recv_buf)!=0)//если ошибка
						{
							continue;
						}
					}
					else if(atoi(row[1])) //если сообщение содержит фото
					{
						sprintf(query_buf, "SELECT path FROM photos WHERE id=\"%s\"",row[1]);
						if(mysql_query(conn,query_buf)!=0)//извлекаем путь к фото
						{
							fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG], mysql_error(conn));//записываем ошибку в лог
							fflush(log);
							sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MY_MESSAGES_FROM_DIALOG],my_info.id );

							mysql_query(conn,query_buf);//записываем текст ошибки в бд

							/*принимаем ответ пользователя чтобы удалить запись из бд*/
							if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
							else if(ret<0) {
								fprintf(log,"ERROR %s: RECVALL mysql query path of the photo(%d)\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							if(strcmp(recv_buf,"y")==0) {
								sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
								mysql_query(conn,query_buf);//записываем текст ошибки в бд
							}
						}
						result2=mysql_store_result(conn);
						row2=mysql_fetch_row(result2);
							
						sprintf(recv_buf,"rm %s",row2[0]);
						mysql_free_result(result2);
						//удаляем фото из системы
						if(system(recv_buf)!=0)//если ошибка
						{
							continue;
						}
					}
				}
				mysql_free_result(result);

				/*удаляем все мои сообщения из базы*/
				sprintf(query_buf, "DELETE FROM messages WHERE (from_id=\"%s\" AND to_id=\"%s\") AND removable",my_info.id,people_info.id);
				if(mysql_query(conn,query_buf)!=0)//удаляем его из друзей или подписок
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MY_MESSAGES_FROM_DIALOG],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query  delete my messages with companion(%d)\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				//смотрим количество сообщений с собеседником
				sprintf(query_buf, "SELECT count(*) FROM messages WHERE (from_id=\"%s\" AND to_id=\"%s\") OR (to_id=\"%s\" AND from_id=\"%s\")",people_info.id,my_info.id,people_info.id,my_info.id);
				if(mysql_query(conn,query_buf)!=0)
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MY_MESSAGES_FROM_DIALOG],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query quantity of messages?(%d)\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				result=mysql_store_result(conn);
				row=mysql_fetch_row(result);
				mysql_free_result(result);
				if(atoi(row[0])==0)//если сообщений нет
				{
					//извлекаем id диалога
					sprintf(query_buf, "SELECT id FROM dialog WHERE (user1_id=\"%s\" AND user2_id=\"%s\") OR (user2_id=\"%s\" AND user1_id=\"%s\")",my_info.id,people_info.id,my_info.id,people_info.id);
					if(mysql_query(conn,query_buf)!=0)
					{
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MY_MESSAGES_FROM_DIALOG],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql query id of the dialog?(%d)\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0) {
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
						continue;//ждем следующий сигнал
					}
					result=mysql_store_result(conn);
					row=mysql_fetch_row(result);

					sprintf(recv_buf,"rm -R %s/%s",PATHS[PATH_COMPANION_FILES],row[0]);//удаляем каталог с id нашего диалога
					system(recv_buf);
					sprintf(recv_buf,"rm -R %s/%s",PATHS[PATH_COMPANION_PHOTOS],row[0]);//удаляем каталог с id нашего диалога
					system(recv_buf);
					mysql_free_result(result);
					/*удаляем запись что мы имеем диалог с юзером*/
					sprintf(query_buf, "DELETE FROM dialog WHERE (user1_id=\"%s\" AND user2_id=\"%s\") OR (user2_id=\"%s\" AND user1_id=\"%s\")",my_info.id,people_info.id,my_info.id,people_info.id);
					if(mysql_query(conn,query_buf)!=0)//удаляем его из друзей или подписок
					{
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MY_MESSAGES_FROM_DIALOG],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql query  delete record in dialog(%d)\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0) {
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
						continue;//ждем следующий сигнал
					}
				}
			}
			else if(strcmp(dialog_type,text_dialog_type[PARTY])==0) 
			{
				sprintf(query_buf, "SELECT (SELECT CASE WHEN file_id>0 THEN file_id ELSE '0' END) AS file_id,(SELECT CASE WHEN photo_id>0 THEN photo_id ELSE '0' END) AS photo_id FROM messages WHERE (from_id=\"%s\" AND party_id=\"%s\") AND removable",my_info.id,people_info.id);
				if(mysql_query(conn,query_buf)!=0)//проверяем что сообщение отправлено от меня(проверяем,не хочет ли пользователь обмануть нас)
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MY_MESSAGES_FROM_DIALOG],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query What type of message?(%d)\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
				}
				result=mysql_store_result(conn);
				while((row=mysql_fetch_row(result)))
				{
					if(atoi(row[0])) //если сообщение содержит файл
					{
						sprintf(query_buf, "SELECT path FROM files WHERE id=\"%s\"",row[0]);
						if(mysql_query(conn,query_buf)!=0)//извлекаем путь к файлу
						{
							fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG], mysql_error(conn));//записываем ошибку в лог
							fflush(log);
							sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MY_MESSAGES_FROM_DIALOG],my_info.id );

							mysql_query(conn,query_buf);//записываем текст ошибки в бд

							/*принимаем ответ пользователя чтобы удалить запись из бд*/
							if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
							else if(ret<0) {
								fprintf(log,"ERROR %s: RECVALL mysql query path of the file(%d)\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							if(strcmp(recv_buf,"y")==0) {
								sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
								mysql_query(conn,query_buf);//записываем текст ошибки в бд
							}
						}
						result2=mysql_store_result(conn);
						row2=mysql_fetch_row(result2);

						sprintf(recv_buf,"rm %s",row2[0]);
						mysql_free_result(result2);
						//удаляем файл из системы
						if(system(recv_buf)!=0)//если ошибка
						{
							continue;
						}
					}
					else if(atoi(row[1])) //если сообщение содержит фото
					{
						sprintf(query_buf, "SELECT path FROM photos WHERE id=\"%s\"",row[1]);
						if(mysql_query(conn,query_buf)!=0)//извлекаем путь к фото
						{
							fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG], mysql_error(conn));//записываем ошибку в лог
							fflush(log);
							sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MY_MESSAGES_FROM_DIALOG],my_info.id );

							mysql_query(conn,query_buf);//записываем текст ошибки в бд

							/*принимаем ответ пользователя чтобы удалить запись из бд*/
							if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
							else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql query path of the photo(%d)\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							if(strcmp(recv_buf,"y")==0) {
								sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
								mysql_query(conn,query_buf);//записываем текст ошибки в бд
							}
						}
						result2=mysql_store_result(conn);
						row2=mysql_fetch_row(result2);
							
						sprintf(recv_buf,"rm %s",row2[0]);
						mysql_free_result(result2);
						//удаляем фото из системы
						if(system(recv_buf)!=0)//если ошибка
						{
							continue;
						}
					}
				}
				mysql_free_result(result);
				/*удаляем все мои сообщения*/
				sprintf(query_buf, "DELETE FROM messages WHERE (from_id=\"%s\" AND party_id=\"%s\") AND removable",my_info.id,party_info.id);
				if(mysql_query(conn,query_buf)!=0)//удаляем его из друзей или подписок
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MY_MESSAGES_FROM_DIALOG],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query  delete my messages with party(%d)\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				//смотрим количество сообщений с группой
				sprintf(query_buf, "SELECT count(*) FROM messages WHERE party_id=\"%s\" AND removable",party_info.id);
				if(mysql_query(conn,query_buf)!=0)
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MY_MESSAGES_FROM_DIALOG],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query quantity of messages in party?(%d)\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				result=mysql_store_result(conn);
				row=mysql_fetch_row(result);
				mysql_free_result(result);
				if(atoi(row[0])==0)//если сообщений нет
				{
					sprintf(recv_buf,"rm -R %s/%s",PATHS[PATH_PARTY_FILES],party_info.id);//удаляем каталог с id нашей группы
					system(recv_buf);
					sprintf(recv_buf,"rm -R %s/%s",PATHS[PATH_PARTY_PHOTOS],party_info.id);//удаляем каталог с id нашей группы
					system(recv_buf);
				}
			}
		}
		else if(strcmp(signal,kakao_signal[BLOCK_USER])==0)
		{
			/*принимаем размер id пользователя*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of people id(%d)\n",kakao_signal[BLOCK_USER],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем id пользователя*/
			if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL people id(%d)\n",kakao_signal[BLOCK_USER],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			if(!block_user(&my_data,my_info.id,people_info.id)) {//если мы еще не заблокировали данного пользователя
				sprintf(query_buf, "INSERT INTO blocked_users(from_id,to_id) VALUES(\"%s\",\"%s\")",my_info.id,people_info.id);
				if(mysql_query(conn,query_buf)!=0)//блокируем его
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[BLOCK_USER], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[BLOCK_USER],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query  block user(%d)\n",kakao_signal[BLOCK_USER],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				sprintf(query_buf, "DELETE FROM friends WHERE (user_id=\"%s\" AND friend_id=\"%s\") OR (user_id=\"%s\" AND friend_id=\"%s\")",my_info.id,people_info.id,my_info.id,people_info.id);
				if(mysql_query(conn,query_buf)!=0)//удаляем его из друзей или подписок
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[BLOCK_USER], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[BLOCK_USER],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query  delete friend or sub or request(%d)\n",kakao_signal[BLOCK_USER],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
			}

		}
		else if(strcmp(signal,kakao_signal[UNBLOCK_USER])==0)
		{
			/*принимаем размер id пользователя*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of people id(%d)\n",kakao_signal[UNBLOCK_USER],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем id пользователя*/
			if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL people id(%d)\n",kakao_signal[UNBLOCK_USER],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			if(block_user(&my_data,my_info.id,people_info.id)>0) {//если мы и правда заблокировали данного пользователя то разблокируем его
				sprintf(query_buf, "DELETE FROM blocked_users WHERE from_id=\"%s\" AND to_id=\"%s\"",my_info.id,people_info.id);
				if(mysql_query(conn,query_buf)!=0)//блокируем его
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[UNBLOCK_USER], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[UNBLOCK_USER],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query unblock user(%d)\n",kakao_signal[UNBLOCK_USER],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
			}

		}
		else if(strcmp(signal,kakao_signal[UPDATE_MESSAGES_LIST])==0)
		{
			//общее количество сообщений связанных со мной или группой в которой я состою
			sprintf(query_buf, "SELECT count(*) FROM messages WHERE party_id IN (SELECT party_id FROM party_users WHERE user_id=\"%s\") OR from_id=\"%s\" OR to_id=\"%s\"",my_info.id,my_info.id,my_info.id);
			if(mysql_query(conn,query_buf)!=0)
			{
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[UPDATE_MESSAGES_LIST], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[UPDATE_MESSAGES_LIST],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query total number of messages(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			result=mysql_store_result(conn);
			row=mysql_fetch_row(result);
			mysql_free_result(result);
			/*отправляем размер общего количества сообщений*/
			sz=htonl(get_str_size(row[0]));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of total number of messages(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*отправляем общее количество сообщений*/
			if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
				fprintf(log, "ERROR %s : SENDALL total number of messages(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			/*принимаем ответ*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV answer(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			if(!ntohl(sz)) continue;//не извлекаем следующие данные



			//общее количество печатающих собеседников
			sprintf(query_buf, "SELECT count(*) FROM companion_writes WHERE to_user_id=\"%s\" AND (NOW()-when_writed)<=%d",my_info.id,WRITING_TIME);
			if(mysql_query(conn,query_buf)!=0)
			{
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[UPDATE_MESSAGES_LIST], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[UPDATE_MESSAGES_LIST],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query writing companions(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			result=mysql_store_result(conn);
			row=mysql_fetch_row(result);
			mysql_free_result(result);
			/*отправляем размер общего количества печатающих собеседников*/
			sz=htonl(get_str_size(row[0]));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of writing companions(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*отправляем общее количество печатающих собеседников*/
			if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
				fprintf(log, "ERROR %s : SENDALL writing companions(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			/*принимаем ответ*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV answer(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			if(!ntohl(sz)) continue;//не извлекаем следующие данные

			//количество собеседников в сети
			sprintf(query_buf, "SELECT count(*) FROM users AS t1 INNER JOIN dialog AS t2 ON ((t1.id=t2.user1_id AND t2.user2_id=\"%s\") OR (t1.id=t2.user2_id AND t2.user1_id=\"%s\")) AND (NOW()-t1.when_online)<=%d",my_info.id,my_info.id,ONLINE_TIME);
			if(mysql_query(conn,query_buf)!=0)
			{
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[UPDATE_MESSAGES_LIST], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[UPDATE_MESSAGES_LIST],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query total number of online companions(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			result=mysql_store_result(conn);
			row=mysql_fetch_row(result);
			mysql_free_result(result);
			/*отправляем размер общего количества собеседников в сети*/
			sz=htonl(get_str_size(row[0]));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of total number of online companions(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*отправляем общее количество собеседников в сети*/
			if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
				fprintf(log, "ERROR %s : SENDALL total number of online companions(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			/*принимаем ответ*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV answer(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			if(!ntohl(sz)) continue;//не извлекаем следующие данные



			//количество собеседников не в сети
			sprintf(query_buf, "SELECT count(*) FROM users AS t1 INNER JOIN dialog AS t2 ON ((t1.id=t2.user1_id AND t2.user2_id=\"%s\") OR (t1.id=t2.user2_id AND t2.user1_id=\"%s\")) AND (NOW()-t1.when_online)>%d",my_info.id,my_info.id,NOTONLINE_TIME);
			if(mysql_query(conn,query_buf)!=0)
			{
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[UPDATE_MESSAGES_LIST], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[UPDATE_MESSAGES_LIST],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query total number of not online companions(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			result=mysql_store_result(conn);
			row=mysql_fetch_row(result);
			mysql_free_result(result);
			/*отправляем размер общего количества собеседников не в сети*/
			sz=htonl(get_str_size(row[0]));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of total number of not online companions(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*отправляем общее количество собеседников не в сети*/
			if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
				fprintf(log, "ERROR %s : SENDALL total number of not online companions(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			/*принимаем ответ*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV answer(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			if(!ntohl(sz)) continue;//не извлекаем следующие данные
		}
		else if(strcmp(signal,kakao_signal[UPDATE_PARTY_MESSAGES_LIST])==0)
		{
			//общее количество сообщений связанных со мной или группой в которой я состою
			sprintf(query_buf, "SELECT count(*) FROM messages WHERE party_id IN (SELECT party_id FROM party_users WHERE user_id=\"%s\") OR from_id=\"%s\" OR to_id=\"%s\"",my_info.id,my_info.id,my_info.id);
			if(mysql_query(conn,query_buf)!=0)
			{
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[UPDATE_PARTY_MESSAGES_LIST],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query total number of messages(%d)\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			result=mysql_store_result(conn);
			row=mysql_fetch_row(result);
			mysql_free_result(result);
			/*отправляем размер общего количества сообщений*/
			sz=htonl(get_str_size(row[0]));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of total number of messages(%d)\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*отправляем общее количество сообщений*/
			if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
				fprintf(log, "ERROR %s : SENDALL total number of messages(%d)\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			/*принимаем ответ*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV answer(%d)\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			if(!ntohl(sz)) continue;//не извлекаем следующие данные



			//общее количество печатающих участников группы
			sprintf(query_buf, "SELECT count(*) FROM party_writes AS t1 INNER JOIN party_users AS t2 WHERE t2.user_id=\"%s\" AND t1.to_party_id=t2.party_id AND (NOW()-t1.when_writed<=%d)",my_info.id,WRITING_TIME);
			if(mysql_query(conn,query_buf)!=0)
			{
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[UPDATE_PARTY_MESSAGES_LIST],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query writing party companions(%d)\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			result=mysql_store_result(conn);
			row=mysql_fetch_row(result);
			mysql_free_result(result);
			/*отправляем размер общего количества печатающих участников группы*/
			sz=htonl(get_str_size(row[0]));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of writing party companions(%d)\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*отправляем общее количество печатающих участников группы*/
			if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
				fprintf(log, "ERROR %s : SENDALL writing party companions(%d)\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			/*принимаем ответ*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV answer(%d)\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			if(!ntohl(sz)) continue;//не извлекаем следующие данные
		}
		else if(strcmp(signal,kakao_signal[UNREAD_DIALOGS])==0)
		{
			//количество непрочитанных диалогов c собеседников и группой
			sprintf(query_buf, "SELECT (SELECT count(DISTINCT from_id) FROM messages WHERE to_id=\"%s\" AND id NOT IN (SELECT message_id FROM who_saw_message)) companion_unreads, (SELECT count(DISTINCT t1.party_id) FROM messages AS t1 INNER JOIN party_users AS t2 WHERE t1.party_id=t2.party_id AND t1.from_id!=\"%s\" AND t2.user_id=\"%s\" AND t1.id NOT IN (SELECT message_id from who_saw_message)) AS party_unreads",my_info.id,my_info.id,my_info.id);
			if(mysql_query(conn,query_buf)!=0)
			{
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[UNREAD_DIALOGS], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[UNREAD_DIALOGS],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query number of unread dialogs(%d)\n",kakao_signal[UNREAD_DIALOGS],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			result=mysql_store_result(conn);
			row=mysql_fetch_row(result);
			mysql_free_result(result);
			/*отправляем размер количества непрочитанных диалогов с собеседником*/
			sz=htonl(get_str_size(row[0]));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of number of unread dialogs with companion(%d)\n",kakao_signal[UNREAD_DIALOGS],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*отправляем количество непрочитанных диалогов с собеседником*/
			if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
				fprintf(log, "ERROR %s : SENDALL number of unread dialogs with companion(%d)\n",kakao_signal[UNREAD_DIALOGS],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*отправляем размер количества непрочитанных диалогов с группой*/
			sz=htonl(get_str_size(row[1]));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of number of unread dialogs with party(%d)\n",kakao_signal[UNREAD_DIALOGS],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*отправляем количество непрочитанных диалогов с группой*/
			if( !(ret=sendall(sock,row[1],ntohl(sz))) ) {
				fprintf(log, "ERROR %s : SENDALL number of unread dialogs with party(%d)\n",kakao_signal[UNREAD_DIALOGS],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

		}
		else if(strcmp(signal,kakao_signal[UPDATE_UNREAD_DIALOGS])==0)
		{
			if(strcmp(dialog_type,text_dialog_type[COMPANION])==0)
			{
				//количество непрочитанных диалогов c собеседников и группой несчитая диалога с моим собеседником
				sprintf(query_buf, "SELECT (SELECT count(DISTINCT from_id) FROM messages WHERE to_id=\"%s\" AND from_id!=\"%s\" AND id NOT IN (SELECT message_id FROM who_saw_message)) companion_unreads, (SELECT count(DISTINCT t1.party_id) FROM messages AS t1 INNER JOIN party_users AS t2 WHERE t1.party_id=t2.party_id AND t1.from_id!=\"%s\" AND t2.user_id=\"%s\" AND t1.id NOT IN (SELECT message_id from who_saw_message)) AS party_unreads",my_info.id,people_info.id,my_info.id,my_info.id);
				if(mysql_query(conn,query_buf)!=0)
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[UPDATE_UNREAD_DIALOGS], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[UPDATE_UNREAD_DIALOGS],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query number of unread dialogs(%d)\n",kakao_signal[UPDATE_UNREAD_DIALOGS],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
			} 
			else if(strcmp(dialog_type,text_dialog_type[PARTY])==0)
			{
				//количество непрочитанных диалогов c собеседников и группой несчитая диалога с выбранной группой
				sprintf(query_buf, "SELECT (SELECT count(DISTINCT from_id) FROM messages WHERE to_id=\"%s\" AND id NOT IN (SELECT message_id FROM who_saw_message)) companion_unreads, (SELECT count(DISTINCT t1.party_id) FROM messages AS t1 INNER JOIN party_users AS t2 WHERE t1.party_id=t2.party_id AND t1.from_id!=\"%s\" AND t2.user_id=\"%s\" AND t1.party_id!=\"%s\" AND t1.id NOT IN (SELECT message_id from who_saw_message)) AS party_unreads",my_info.id,my_info.id,my_info.id,party_info.id);
				if(mysql_query(conn,query_buf)!=0)
				{
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[UPDATE_UNREAD_DIALOGS], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[UPDATE_UNREAD_DIALOGS],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query number of unread dialogs(%d)\n",kakao_signal[UPDATE_UNREAD_DIALOGS],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
			}
			result=mysql_store_result(conn);
			row=mysql_fetch_row(result);
			mysql_free_result(result);
			/*отправляем размер количества непрочитанных диалогов с собеседником*/
			sz=htonl(get_str_size(row[0]));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of number of unread dialogs with companion(%d)\n",kakao_signal[UPDATE_UNREAD_DIALOGS],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*отправляем количество непрочитанных диалогов с собеседником*/
			if( !(ret=sendall(sock,row[0],ntohl(sz))) ) {
				fprintf(log, "ERROR %s : SENDALL number of unread dialogs with companion(%d)\n",kakao_signal[UPDATE_UNREAD_DIALOGS],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*отправляем размер количества непрочитанных диалогов с группой*/
			sz=htonl(get_str_size(row[1]));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of number of unread dialogs with party(%d)\n",kakao_signal[UPDATE_UNREAD_DIALOGS],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*отправляем количество непрочитанных диалогов с группой*/
			if( !(ret=sendall(sock,row[1],ntohl(sz))) ) {
				fprintf(log, "ERROR %s : SENDALL number of unread dialogs with party(%d)\n",kakao_signal[UPDATE_UNREAD_DIALOGS],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
		}
		else if(strcmp(signal,kakao_signal[BOB_SEND_FILE])==0)
		{
			char path[MAX_PATH];
			char filename[MAX_FILENAME];
			char dialog_type[20];
			/*принимаем размер диалога*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of dialog type(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем диалог*/
			if( (ret=recvall(sock,dialog_type,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL dialog type(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			if(strcmp(dialog_type,text_dialog_type[COMPANION])==0) {
				/*принимаем размер id пользователя*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of people id(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем id пользователя*/
				if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL people id(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			}
			else if(strcmp(dialog_type,text_dialog_type[PARTY])==0) {
				/*принимаем размер id группы*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of party id(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем id группы*/
				if( (ret=recvall(sock,party_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL party id(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			}
			/*принимаем размер имени файла */
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of filename(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем имя файла вида имя.расширение*/
			if( (ret=recvall(sock,filename,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL filename(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			if(strcmp(dialog_type,text_dialog_type[COMPANION])==0) 
			{
				if(allow_send_data_to_companion(&my_data,people_info.id))//если разрешено отправить данные
				{
					/*отправляем положительный ответ(можно отправлять данные)*/
					sz=htonl(1);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND answer true(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					//извлекаем id диалога(будет как каталог)
					sprintf(query_buf, "SELECT id FROM dialog WHERE (user1_id=\"%s\" AND user2_id=\"%s\") OR ( user1_id=\"%s\" AND user2_id=\"%s\")",my_info.id,people_info.id,people_info.id,my_info.id);
					if(mysql_query(conn,query_buf)!=0)
					{
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[BOB_SEND_FILE], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[BOB_SEND_FILE],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql query dialog id(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0) {
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
						continue;//ждем следующий сигнал
					}
					result=mysql_store_result(conn);
					if(mysql_num_rows(result)>0)
					{
						float file_size;//размер файла в мегабайтах
						char buf[(MAX_PATH*2)+256];
						char *extension;
						row=mysql_fetch_row(result);
						mysql_free_result(result);

						snprintf(path,sizeof(path),"%s/%s",PATHS[PATH_COMPANION_FILES],row[0]);//создаем путь к дирректории с id диалога
						sprintf(buf,"if ! [ -d \"%s\" ]; then mkdir -p %s; fi",path,path);//создаем команду для создания дирректории
						system(buf);//создаем каталог если это нужно

						if((extension=from_filename_to_extension(filename))!=NULL) {//если у файла есть расширение 
							snprintf(path,sizeof(path),"%s/%s/%ld.%s",PATHS[PATH_COMPANION_FILES],row[0],time(NULL),extension);//PATHS[PATH_COMPANION_FILES]/номер_диалога/текущее_время.расширение
						}
						else {
							snprintf(path,sizeof(path),"%s/%s/%ld",PATHS[PATH_COMPANION_FILES],row[0],time(NULL));//PATHS[PATH_COMPANION_FILES]/номер_диалога/текущее_время
						}
						if(recv_file(sock,path,&file_size)) //принимаем файл
						{
							mysql_query(conn,"START TRANSACTION");
							//вставляем файл в базу данных как сообщение
							sprintf(buf, "INSERT INTO files(filename,path,size,sender_id) VALUES(\"%s\",\"%s\",%f,\"%s\")",filename,path,file_size,my_info.id);
							if(mysql_query(conn,buf)!=0)
							{
								fprintf(log, "ERROR: %s\t%s\n",kakao_signal[BOB_SEND_FILE], mysql_error(conn));//записываем ошибку в лог
								fflush(log);
								mysql_query(conn,"ROLLBACK");
								sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[BOB_SEND_FILE],my_info.id );

								mysql_query(conn,query_buf);//записываем текст ошибки в бд

								/*принимаем ответ пользователя чтобы удалить запись из бд*/
								if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
								else if(ret<0) {
									fprintf(log,"ERROR %s: RECVALL mysql query insert into files(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}

								if(strcmp(recv_buf,"y")==0) {
									sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
									mysql_query(conn,query_buf);//записываем текст ошибки в бд
								}
								continue;//ждем следующий сигнал
							}
							//вставляем файл в базу данных как сообщение
							sprintf(buf, "INSERT INTO messages(text,from_id,to_id,file_id) VALUES(\"%s\",\"%s\",\"%s\",(SELECT id FROM files WHERE sender_id=\"%s\" ORDER BY sent_at DESC LIMIT 1))",filename,my_info.id,people_info.id,my_info.id);
							if(mysql_query(conn,buf)!=0)
							{
								fprintf(log, "ERROR: %s\t%s\n",kakao_signal[BOB_SEND_FILE], mysql_error(conn));//записываем ошибку в лог
								fflush(log);
								mysql_query(conn,"ROLLBACK");
								sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[BOB_SEND_FILE],my_info.id );

								mysql_query(conn,query_buf);//записываем текст ошибки в бд

								/*принимаем ответ пользователя чтобы удалить запись из бд*/
								if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
								else if(ret<0) {
									fprintf(log,"ERROR %s: RECVALL mysql query message file(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}

								if(strcmp(recv_buf,"y")==0) {
									sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
									mysql_query(conn,query_buf);//записываем текст ошибки в бд
								}
								continue;//ждем следующий сигнал
							}
							mysql_query(conn,"COMMIT");
						}
					}
				}
				else {
					/*отправляем отрицательный ответ(нельзя отправлять данные)*/
					sz=htonl(0);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND answer false(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					continue;
				}
			}
			else if(strcmp(dialog_type,text_dialog_type[PARTY])==0) 
			{
				if(allow_send_data_to_party(&my_data,party_info.id))//если разрешено отправить данные
				{
					/*отправляем положительный ответ(можно отправлять данные)*/
					sz=htonl(1);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND answer true(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					float file_size;//размер файла в мегабайтах
					char buf[(MAX_PATH*2)+256];
					char *extension;

					snprintf(path,sizeof(path),"%s/%s",PATHS[PATH_PARTY_FILES],party_info.id);//создаем путь к дирректории с id группы
					sprintf(buf,"if ! [ -d \"%s\" ]; then mkdir -p %s; fi",path,path);//создаем команду для создания дирректории
					system(buf);//создаем каталог если это нужно

					if((extension=from_filename_to_extension(filename))!=NULL) {//если у файла есть расширение 
						snprintf(path,sizeof(path),"%s/%s/%ld.%s",PATHS[PATH_PARTY_FILES],party_info.id,time(NULL),extension);//PATHS[PATH_PARTY_FILES]/номер_группы/текущее_время.расширение
					}
					else {
						snprintf(path,sizeof(path),"%s/%s/%ld",PATHS[PATH_PARTY_FILES],party_info.id,time(NULL));//PATHS[PATH_PARTY_FILES]/номер_группы/текущее_время
					}
					if(recv_file(sock,path,&file_size)) //принимаем файл
					{
						mysql_query(conn,"START TRANSACTION");
						//вставляем файл в базу данных как сообщение
						sprintf(buf, "INSERT INTO files(filename,path,size,sender_id) VALUES(\"%s\",\"%s\",%f,\"%s\")",filename,path,file_size,my_info.id);
						if(mysql_query(conn,buf)!=0)
						{
							fprintf(log, "ERROR: %s\t%s\n",kakao_signal[BOB_SEND_FILE], mysql_error(conn));//записываем ошибку в лог
							fflush(log);
							mysql_query(conn,"ROLLBACK");
							sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[BOB_SEND_FILE],my_info.id );

							mysql_query(conn,query_buf);//записываем текст ошибки в бд

							/*принимаем ответ пользователя чтобы удалить запись из бд*/
							if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
							else if(ret<0) {
								fprintf(log,"ERROR %s: RECVALL mysql query insert into files(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							if(strcmp(recv_buf,"y")==0) {
								sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
								mysql_query(conn,query_buf);//записываем текст ошибки в бд
							}
							continue;//ждем следующий сигнал
						}
						//вставляем файл в базу данных как сообщение
						sprintf(buf, "INSERT INTO messages(text,from_id,party_id,file_id) VALUES(\"\",\"%s\",\"%s\",(SELECT id FROM files WHERE sender_id=\"%s\" ORDER BY sent_at DESC LIMIT 1))",my_info.id,party_info.id,my_info.id);
						if(mysql_query(conn,buf)!=0)
						{
							fprintf(log, "ERROR: %s\t%s\n",kakao_signal[BOB_SEND_FILE], mysql_error(conn));//записываем ошибку в лог
							fflush(log);
							mysql_query(conn,"ROLLBACK");
							sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[BOB_SEND_FILE],my_info.id );

							mysql_query(conn,query_buf);//записываем текст ошибки в бд

							/*принимаем ответ пользователя чтобы удалить запись из бд*/
							if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
							else if(ret<0) {
								fprintf(log,"ERROR %s: RECVALL mysql query message file(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							if(strcmp(recv_buf,"y")==0) {
								sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
								mysql_query(conn,query_buf);//записываем текст ошибки в бд
							}
							continue;//ждем следующий сигнал
						}
						mysql_query(conn,"COMMIT");
					}
				}
				else {
					/*отправляем отрицательный ответ(нельзя отправлять данные)*/
					sz=htonl(0);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND answer false(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					continue;
				}
			}

		}
		else if(strcmp(signal,kakao_signal[BOB_SEND_PHOTO])==0)
		{
			char path[MAX_PATH];
			char filename[MAX_FILENAME];
			char dialog_type[20];
			/*принимаем размер диалога*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of dialog type(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем диалог*/
			if( (ret=recvall(sock,dialog_type,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL dialog type(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			if(strcmp(dialog_type,text_dialog_type[COMPANION])==0) {
				/*принимаем размер id пользователя*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of people id(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем id пользователя*/
				if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL people id(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			}
			else if(strcmp(dialog_type,text_dialog_type[PARTY])==0) {
				/*принимаем размер id группы*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECV size of party id(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*принимаем id группы*/
				if( (ret=recvall(sock,party_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
				else if(ret<0){
					fprintf(log,"ERROR %s: RECVALL party id(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			}
			/*принимаем размер имени файла */
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of name of photo(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем имя файла вида имя.расширение*/
			if( (ret=recvall(sock,filename,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL name of photo(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			if(strcmp(dialog_type,text_dialog_type[COMPANION])==0) 
			{
				if(allow_send_data_to_companion(&my_data,people_info.id))//если разрешено отправить данные
				{
					/*отправляем положительный ответ(можно отправлять данные)*/
					sz=htonl(1);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND answer true(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					//извлекаем id диалога(будет как каталог)
					sprintf(query_buf, "SELECT id FROM dialog WHERE (user1_id=\"%s\" AND user2_id=\"%s\") OR ( user1_id=\"%s\" AND user2_id=\"%s\")",my_info.id,people_info.id,people_info.id,my_info.id);
					if(mysql_query(conn,query_buf)!=0)
					{
						fprintf(log, "ERROR: %s\t%s\n",kakao_signal[BOB_SEND_PHOTO], mysql_error(conn));//записываем ошибку в лог
						fflush(log);
						sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[BOB_SEND_PHOTO],my_info.id );

						mysql_query(conn,query_buf);//записываем текст ошибки в бд

						/*принимаем ответ пользователя чтобы удалить запись из бд*/
						if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
						else if(ret<0) {
							fprintf(log,"ERROR %s: RECVALL mysql query dialog id(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(log);
							continue;//ждем следующий сигнал
						}

						if(strcmp(recv_buf,"y")==0) {
							sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
							mysql_query(conn,query_buf);//записываем текст ошибки в бд
						}
						continue;//ждем следующий сигнал
					}
					result=mysql_store_result(conn);
					if(mysql_num_rows(result)>0)
					{
						float photo_size;//размер файла в мегабайтах
						char buf[(MAX_PATH*2)+256];
						char *extension;
						row=mysql_fetch_row(result);
						mysql_free_result(result);

						snprintf(path,sizeof(path),"%s/%s",PATHS[PATH_COMPANION_PHOTOS],row[0]);//создаем путь к дирректории с id диалога пользователей
						sprintf(buf,"if ! [ -d \"%s\" ]; then mkdir -p %s; fi",path,path);//создаем команду для создания дирректории
						system(buf);//создаем каталог если это нужно

						if((extension=from_filename_to_extension(filename))!=NULL) {//если у фото есть расширение 
							snprintf(path,sizeof(path),"%s/%s/%ld.%s",PATHS[PATH_COMPANION_PHOTOS],row[0],time(NULL),extension);//PATHS[PATH_COMPANION_PHOTOS]/номер_диалога/текущее_время.расширение
						}
						else {
							snprintf(path,sizeof(path),"%s/%s/%ld",PATHS[PATH_COMPANION_PHOTOS],row[0],time(NULL));//PATHS[PATH_COMPANION_PHOTOS]/номер_диалога/текущее_время
						}
						if(recv_file(sock,path,&photo_size)) //принимаем файл
						{
							mysql_query(conn,"START TRANSACTION");
							//вставляем файл в базу данных как сообщение
							sprintf(buf, "INSERT INTO photos(filename,path,size,sender_id) VALUES(\"%s\",\"%s\",%f,\"%s\")",filename,path,photo_size,my_info.id);
							if(mysql_query(conn,buf)!=0)
							{
								fprintf(log, "ERROR: %s\t%s\n",kakao_signal[BOB_SEND_PHOTO], mysql_error(conn));//записываем ошибку в лог
								fflush(log);
								mysql_query(conn,"ROLLBACK");
								sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[BOB_SEND_PHOTO],my_info.id );

								mysql_query(conn,query_buf);//записываем текст ошибки в бд

								/*принимаем ответ пользователя чтобы удалить запись из бд*/
								if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
								else if(ret<0) {
									fprintf(log,"ERROR %s: RECVALL mysql query insert into photos(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}

								if(strcmp(recv_buf,"y")==0) {
									sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
									mysql_query(conn,query_buf);//записываем текст ошибки в бд
								}
								continue;//ждем следующий сигнал
							}
							//вставляем файл в базу данных как сообщение
							sprintf(buf, "INSERT INTO messages(text,from_id,to_id,photo_id) VALUES(\"%s\",\"%s\",\"%s\",(SELECT id FROM photos WHERE sender_id=\"%s\" ORDER BY sent_at DESC LIMIT 1))",filename,my_info.id,people_info.id,my_info.id);
							if(mysql_query(conn,buf)!=0)
							{
								fprintf(log, "ERROR: %s\t%s\n",kakao_signal[BOB_SEND_PHOTO], mysql_error(conn));//записываем ошибку в лог
								fflush(log);
								mysql_query(conn,"ROLLBACK");
								sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[BOB_SEND_PHOTO],my_info.id );

								mysql_query(conn,query_buf);//записываем текст ошибки в бд

								/*принимаем ответ пользователя чтобы удалить запись из бд*/
								if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
								else if(ret<0) {
									fprintf(log,"ERROR %s: RECVALL mysql query message photo(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(log);
									continue;//ждем следующий сигнал
								}

								if(strcmp(recv_buf,"y")==0) {
									sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
									mysql_query(conn,query_buf);//записываем текст ошибки в бд
								}
								continue;//ждем следующий сигнал
							}
							mysql_query(conn,"COMMIT");
						}
					}
				}
				else {
					/*отправляем отрицательный ответ(нельзя отправлять данные)*/
					sz=htonl(0);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND answer false(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					continue;
				}
			}
			else if(strcmp(dialog_type,text_dialog_type[PARTY])==0) 
			{
				if(allow_send_data_to_party(&my_data,party_info.id))//если разрешено отправить данные
				{
					/*отправляем положительный ответ(можно отправлять данные)*/
					sz=htonl(1);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND answer true(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					float photo_size;//размер файла в мегабайтах
					char buf[(MAX_PATH*2)+256];
					char *extension;

					snprintf(path,sizeof(path),"%s/%s",PATHS[PATH_PARTY_PHOTOS],party_info.id);//создаем путь к дирректории с id группы
					sprintf(buf,"if ! [ -d \"%s\" ]; then mkdir -p %s; fi",path,path);//создаем команду для создания дирректории
					system(buf);//создаем каталог если это нужно

					if((extension=from_filename_to_extension(filename))!=NULL) {//если у фото есть расширение 
						snprintf(path,sizeof(path),"%s/%s/%ld.%s",PATHS[PATH_PARTY_PHOTOS],party_info.id,time(NULL),extension);//PATHS[PATH_PARTY_PHOTOS]/номер_группы/текущее_время.расширение
					}
					else {
						snprintf(path,sizeof(path),"%s/%s/%ld",PATHS[PATH_PARTY_PHOTOS],party_info.id,time(NULL));//PATHS[PATH_PARTY_PHOTOS]/номер_диалога/текущее_время
					}
					if(recv_file(sock,path,&photo_size)) //принимаем файл
					{
						mysql_query(conn,"START TRANSACTION");
						//вставляем файл в базу данных как сообщение
						sprintf(buf, "INSERT INTO photos(filename,path,size,sender_id) VALUES(\"%s\",\"%s\",%f,\"%s\")",filename,path,photo_size,my_info.id);
						if(mysql_query(conn,buf)!=0)
						{
							fprintf(log, "ERROR: %s\t%s\n",kakao_signal[BOB_SEND_PHOTO], mysql_error(conn));//записываем ошибку в лог
							fflush(log);
							mysql_query(conn,"ROLLBACK");
							sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[BOB_SEND_PHOTO],my_info.id );

							mysql_query(conn,query_buf);//записываем текст ошибки в бд

							/*принимаем ответ пользователя чтобы удалить запись из бд*/
							if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
							else if(ret<0) {
								fprintf(log,"ERROR %s: RECVALL mysql query insert into photos(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							if(strcmp(recv_buf,"y")==0) {
								sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
								mysql_query(conn,query_buf);//записываем текст ошибки в бд
							}
							continue;//ждем следующий сигнал
						}
						//вставляем файл в базу данных как сообщение
						sprintf(buf, "INSERT INTO messages(text,from_id,party_id,photo_id) VALUES(\"\",\"%s\",\"%s\",(SELECT id FROM photos WHERE sender_id=\"%s\" ORDER BY sent_at DESC LIMIT 1))",my_info.id,party_info.id,my_info.id);
						if(mysql_query(conn,buf)!=0)
						{
							fprintf(log, "ERROR: %s\t%s\n",kakao_signal[BOB_SEND_PHOTO], mysql_error(conn));//записываем ошибку в лог
							fflush(log);
							mysql_query(conn,"ROLLBACK");
							sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[BOB_SEND_PHOTO],my_info.id );

							mysql_query(conn,query_buf);//записываем текст ошибки в бд

							/*принимаем ответ пользователя чтобы удалить запись из бд*/
							if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
							else if(ret<0) {
								fprintf(log,"ERROR %s: RECVALL mysql query message photo(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(log);
								continue;//ждем следующий сигнал
							}

							if(strcmp(recv_buf,"y")==0) {
								sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
								mysql_query(conn,query_buf);//записываем текст ошибки в бд
							}
							continue;//ждем следующий сигнал
						}
						mysql_query(conn,"COMMIT");
					}
				}
				else {
					/*отправляем отрицательный ответ(нельзя отправлять данные)*/
					sz=htonl(0);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND answer false(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
					continue;
				}
			}
		}
		else if(strcmp(signal,kakao_signal[DOWNLOAD])==0)
		{
			int mes_type;
			/*принимаем размер типа диалога*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of type of dilaog(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем тип диалога*/
			if( (ret=recvall(sock,dialog_type,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL type of dialog(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			/*принимаем размер id сообщения*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of message id(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем id сообщения*/
			if( (ret=recvall(sock,message_info.id,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL message id(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			/*принимаем тип сообщения*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV message type(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			mes_type=ntohl(sz);

			if(strcmp(dialog_type,text_dialog_type[COMPANION])==0) 
			{
				/*убеждаемся что сообщение как-то связано с нами(вдруг пользователь отправит id чужого сообщения)*/
				sprintf(query_buf, "SELECT count(*) FROM messages WHERE id=\"%s\" AND (from_id=\"%s\" OR to_id=\"%s\")",message_info.id,my_info.id,my_info.id );
				if(mysql_query(conn,query_buf)!=0)// получаем запись состоит ли пользователь в группе
				{//обрабатываем ошибку
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DOWNLOAD], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DOWNLOAD],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query message related to us?(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				result=mysql_store_result(conn);// Извлечение результатов запроса
				row=mysql_fetch_row(result);
				mysql_free_result(result);
				if(atoi(row[0])<1) //если пользоваетль пытается нас обмануть
					continue;//ждем следующий сигнал
			}
			else if(strcmp(dialog_type,text_dialog_type[PARTY])==0) 
			{
				/*убеждаемся что сообщение как-то связано с нами(вдруг пользователь отправит id чужого сообщения)*/
				sprintf(query_buf, "SELECT count(*) FROM messages AS t1 INNER JOIN party_users AS t2 ON t1.id=\"%s\" AND (t1.party_id=t2.party_id AND t2.user_id=\"%s\")",message_info.id,my_info.id );
				if(mysql_query(conn,query_buf)!=0)// получаем запись состоит ли пользователь в группе
				{//обрабатываем ошибку
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DOWNLOAD], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DOWNLOAD],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query message related to us?(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				result=mysql_store_result(conn);// Извлечение результатов запроса
				row=mysql_fetch_row(result);
				mysql_free_result(result);
				if(atoi(row[0])<1) //если пользоваетль пытается нас обмануть
					continue;//ждем следующий сигнал
			}
			/*если проверка пройдена*/
			if(mes_type==BOB_FILE)//если пользователю нужно скачать файл
			{
				/*извлекаем путь к файлу и его имя*/
				sprintf(query_buf, "SELECT t2.path,t2.filename FROM messages AS t1 INNER JOIN files AS t2 WHERE t1.id=\"%s\" AND t1.file_id=t2.id",message_info.id );
				if(mysql_query(conn,query_buf)!=0)
				{//обрабатываем ошибку
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DOWNLOAD], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DOWNLOAD],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query path of file(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				result=mysql_store_result(conn);// Извлечение результатов запроса
				row=mysql_fetch_row(result);
				mysql_free_result(result);
			}
			else if(mes_type==PHOTO)//если пользователю нужно скачать фото
			{
				/*извлекаем путь к фото*/
				sprintf(query_buf, "SELECT t2.path,t2.filename FROM messages AS t1 INNER JOIN photos AS t2 WHERE t1.id=\"%s\" AND t1.photo_id=t2.id",message_info.id );
				if(mysql_query(conn,query_buf)!=0)
				{//обрабатываем ошибку
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DOWNLOAD], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DOWNLOAD],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query path of photo(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
				result=mysql_store_result(conn);// Извлечение результатов запроса
				row=mysql_fetch_row(result);
				mysql_free_result(result);
			}

			/*отправляем размер имени файла*/
			sz=htonl(get_str_size(row[1]));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(log, "ERROR %s : SEND size of name of photo(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*отправляем имя файла*/
			if( !(ret=sendall(sock,row[1],ntohl(sz))) ) {
				fprintf(log, "ERROR %s : SENDALL name of photo(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			if(!send_file(sock,row[0])) {//если произошла ошибка
				/*отправляем ноль*/
				sz=htonl(0);
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND zero(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			}

		}
		else if(strcmp(signal,kakao_signal[SHOW_SETTING_PROFILE])==0)
		{
			/*извлекаем путь к нашей аватарке*/
			sprintf(query_buf, "SELECT path_avatar FROM users WHERE id=\"%s\"",my_info.id );
			if(mysql_query(conn,query_buf)!=0)
			{//обрабатываем ошибку
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[SHOW_SETTING_PROFILE], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[SHOW_SETTING_PROFILE],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query path of the my avatar(%d)\n",kakao_signal[SHOW_SETTING_PROFILE],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			result=mysql_store_result(conn);// Извлечение результатов запроса
			row=mysql_fetch_row(result);
			if(strcmp(row[0],"")!=0) {//если у нас есть аватарка
				/*отправляем ответ что у нас есть аватарка*/
				sz=htonl(1);
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND we have avatar (%d)\n",kakao_signal[SHOW_SETTING_PROFILE],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*отправляем размер имени фото*/
				sz=htonl( get_str_size(from_path_to_filename(row[0])) );
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND size of name of the photo(%d)\n",kakao_signal[SHOW_SETTING_PROFILE],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*отправляем имя фото*/
				if( !(ret=sendall(sock,from_path_to_filename(row[0]),ntohl(sz))) ) {
					fprintf(log, "ERROR %s : SENDALL name of the photo(%d)\n",kakao_signal[SHOW_SETTING_PROFILE],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
				/*отправляем нашу аватарку*/
				if(!send_file(sock,row[0])) {//если произошла ошибка
					/*отправляем ноль*/
					sz=htonl(0);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(log, "ERROR %s : SEND zero(%d)\n",kakao_signal[SHOW_SETTING_PROFILE],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}
				}
			} 
			else {
				/*отправляем ответ что у нас нет аватарки*/
				sz=htonl(0);
				if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
					fprintf(log, "ERROR %s : SEND we have not avatar (%d)\n",kakao_signal[SHOW_SETTING_PROFILE],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}
			}
			mysql_free_result(result);
		}
		else if(strcmp(signal,kakao_signal[DELETE_MY_AVATAR])==0)
		{
			/*удаляем нашу аватарку*/
			sprintf(query_buf, "UPDATE users SET path_avatar=\"\" WHERE id=\"%s\"",my_info.id );
			if(mysql_query(conn,query_buf)!=0)
			{//обрабатываем ошибку
				fprintf(log, "ERROR: %s\t%s\n",kakao_signal[DELETE_MY_AVATAR], mysql_error(conn));//записываем ошибку в лог
				fflush(log);
				sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[DELETE_MY_AVATAR],my_info.id );

				mysql_query(conn,query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
				else if(ret<0) {
					fprintf(log,"ERROR %s: RECVALL mysql query delete my avatar(%d)\n",kakao_signal[DELETE_MY_AVATAR],ret ); fflush(log);
					continue;//ждем следующий сигнал
				}

				if(strcmp(recv_buf,"y")==0) {
					sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
					mysql_query(conn,query_buf);//записываем текст ошибки в бд
				}
				continue;//ждем следующий сигнал
			}
			sprintf(recv_buf,"rm -R %s/%s/*",PATHS[PATH_USER_AVATARS],my_info.id);//удаляем прошлую аватарку
			system(recv_buf);
		}
		else if(strcmp(signal,kakao_signal[CHANGE_MY_AVATAR])==0)
		{
			char filename[MAX_FILENAME];
			char path[MAX_PATH];
			char buf[(MAX_PATH*2)+256];
			char *extension;
			/*принимаем размер имени фото(аватарки)*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECV size of name of photo(%d)\n",kakao_signal[CHANGE_MY_AVATAR],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}
			/*принимаем имя фото*/
			if( (ret=recvall(sock,filename,ntohl(sz),MSG_WAITALL))==0 ) break;//если клиент отключился
			else if(ret<0){
				fprintf(log,"ERROR %s: RECVALL name of photo(%d)\n",kakao_signal[CHANGE_MY_AVATAR],ret ); fflush(log);
				continue;//ждем следующий сигнал
			}

			snprintf(path,sizeof(path),"%s/%s",PATHS[PATH_USER_AVATARS],my_info.id);//создаем путь к дирректории с id пользователя
			sprintf(buf,"if ! [ -d \"%s\" ]; then mkdir -p %s; fi",path,path);//создаем команду для создания дирректории
			system(buf);//создаем каталог если это нужно

			if((extension=from_filename_to_extension(filename))!=NULL) {//если у фото есть расширение 
				snprintf(path,sizeof(path),"%s/%s/%ld.%s",PATHS[PATH_USER_AVATARS],my_info.id,time(NULL),extension);//PATHS[PATH_USER_AVATARS]/id_пользователя/текущее_время.расширение
			}
			else {
				snprintf(path,sizeof(path),"%s/%s/%ld",PATHS[PATH_USER_AVATARS],my_info.id,time(NULL));//PATHS[PATH_USER_AVATARS]/id_пользователя/текущее_время
			}
			sprintf(buf,"rm -R %s/%s/*",PATHS[PATH_USER_AVATARS],my_info.id);//удаляем прошлую аватарку
			system(buf);
			if(recv_file(sock,path,NULL))//принимаем новую аватарку
			{
				/*изменяем путь к нашей аватарке*/
				sprintf(query_buf, "UPDATE users SET path_avatar=\"%s\" WHERE id=\"%s\"",path,my_info.id);
				if(mysql_query(conn,query_buf)!=0)
				{//обрабатываем ошибку
					fprintf(log, "ERROR: %s\t%s\n",kakao_signal[CHANGE_MY_AVATAR], mysql_error(conn));//записываем ошибку в лог
					fflush(log);
					sprintf(query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[CHANGE_MY_AVATAR],my_info.id );

					mysql_query(conn,query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(sock,recv_buf,sizeof(recv_buf),0))==0 ) break;//если клиент отключился
					else if(ret<0) {
						fprintf(log,"ERROR %s: RECVALL mysql query change my avatar(%d)\n",kakao_signal[CHANGE_MY_AVATAR],ret ); fflush(log);
						continue;//ждем следующий сигнал
					}

					if(strcmp(recv_buf,"y")==0) {
						sprintf(query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_info.id );
						mysql_query(conn,query_buf);//записываем текст ошибки в бд
					}
					continue;//ждем следующий сигнал
				}
			}
		}

	}

	/*Клиент отключился*/
	fprintf(stdout,"Клиент отключился\n" ); fflush(stdout);
	fflush(log);
	fclose(log);
	fclose(auth_log);
	mysql_close(conn);
	close(sock);
	return NULL;

}
int allow_send_data_to_companion(some_data *my_data,char *to_id)//при отправке каких либо данных(файлов или текста или изображения) проверяем можно ли отправлять их пользователю и если можно выполняем какие либо другие действия 
{
	uint32_t sz;
	MYSQL_RES *result;
	MYSQL_ROW row;
	short int ret;
	if(!block_user(my_data,to_id,my_data->id)) {//если мы не заблокированы
		mysql_query(my_data->conn,"START TRANSACTION");
		sprintf(my_data->query_buf, "SELECT id FROM messages WHERE (from_id= \"%s\" AND to_id = \"%s\") OR (from_id= \"%s\" AND to_id = \"%s\")", my_data->id,to_id,to_id,my_data->id);
		if(mysql_query(my_data->conn,my_data->query_buf)!=0)//получаем запись с сообщениями с данным собеседником
		{//обрабатываем ошибку
			fprintf(my_data->log, "ERROR: %s\t%s\n",kakao_signal[ALLOW_SEND_DATA_TO_COMPANION], mysql_error(my_data->conn));//записываем ошибку в лог
			fflush(my_data->log);
			mysql_query(my_data->conn,"ROLLBACK");
			sprintf(my_data->query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[ALLOW_SEND_DATA_TO_COMPANION],my_data->id );
			mysql_query(my_data->conn,my_data->query_buf);//записываем текст ошибки в бд
			/*принимаем ответ пользователя чтобы удалить запись из бд*/
			if( (ret=recvall(my_data->sock,my_data->recv_buf,sizeof(my_data->recv_buf),0))==0 ) {
				strcpy(to_id,"");
				return 0; //если клиент отключился
			}
			else if(ret<0){
				fprintf(my_data->log,"ERROR %s: RECVALL mysql_error: %s(%d)\n",kakao_signal[ALLOW_SEND_DATA_TO_COMPANION],mysql_error(my_data->conn),ret ); fflush(my_data->log);
				strcpy(to_id,"");
				return 0;
			}

			if(strcmp(my_data->recv_buf,"y")==0){
				sprintf(my_data->query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_data->id );
				mysql_query(my_data->conn,my_data->query_buf);//записываем текст ошибки в бд				
			}
			strcpy(to_id,"");
			return 0;
		}
		result=mysql_store_result(my_data->conn);

		if(mysql_num_rows(result) == 0)//если нет сообщений с данным пользователем(если мы отправляем первое сообщение)
		{
			mysql_free_result(result);
			//смотрим есть ли у нас диалог с юзером
			sprintf(my_data->query_buf, "SELECT count(*) FROM dialog WHERE (user1_id=\"%s\" AND user2_id=\"%s\") OR (user2_id=\"%s\" AND user1_id=\"%s\")",to_id,my_data->id,to_id,my_data->id);
			if(mysql_query(my_data->conn,my_data->query_buf)!=0)
			{
				fprintf(my_data->log, "ERROR: %s\t%s\n",kakao_signal[ALLOW_SEND_DATA_TO_COMPANION], mysql_error(my_data->conn));//записываем ошибку в лог
				fflush(my_data->log);
				sprintf(my_data->query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[ALLOW_SEND_DATA_TO_COMPANION],my_data->id );

				mysql_query(my_data->conn,my_data->query_buf);//записываем текст ошибки в бд

				/*принимаем ответ пользователя чтобы удалить запись из бд*/
				if( (ret=recvall(my_data->sock,my_data->recv_buf,sizeof(my_data->recv_buf),0))==0 ) {
					strcpy(to_id,"");
					return 0;//если клиент отключился
				}
				else if(ret<0) {
					fprintf(my_data->log,"ERROR %s: RECVALL mysql query do we have the dialog?(%d)\n",kakao_signal[ALLOW_SEND_DATA_TO_COMPANION],ret ); fflush(my_data->log);
					strcpy(to_id,"");
					return 0;//ждем следующий сигнал
				}

				if(strcmp(my_data->recv_buf,"y")==0) {
					sprintf(my_data->query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_data->id );
					mysql_query(my_data->conn,my_data->query_buf);//записываем текст ошибки в бд
				}
				strcpy(to_id,"");
				return 0;//ждем следующий сигнал
			}
			result=mysql_store_result(my_data->conn);
			row=mysql_fetch_row(result);
			mysql_free_result(result);
			if(atoi(row[0])==0)//если его нет, то создаем запись
			{
				sprintf(my_data->query_buf, "INSERT INTO dialog (user1_id,user2_id) VALUES (\"%s\",\"%s\")",to_id,my_data->id);
				if(mysql_query(my_data->conn,my_data->query_buf)!=0)// вставляем запись в dialog(с кем мы имели диалог)
				{//обрабатываем ошибку
					fprintf(my_data->log, "ERROR: %s\t%s\n",kakao_signal[ALLOW_SEND_DATA_TO_COMPANION], mysql_error(my_data->conn));//записываем ошибку в лог
					fflush(my_data->log);
					mysql_query(my_data->conn,"ROLLBACK");
					sprintf(my_data->query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[ALLOW_SEND_DATA_TO_COMPANION],my_data->id );
					mysql_query(my_data->conn,my_data->query_buf);//записываем текст ошибки в бд

					/*принимаем ответ пользователя чтобы удалить запись из бд*/
					if( (ret=recvall(my_data->sock,my_data->recv_buf,sizeof(my_data->recv_buf),0))==0 ) {//если клиент отключился
						strcpy(to_id,"");
						return 0;
					}
					else if(ret<0) {
						fprintf(my_data->log,"ERROR %s: RECVALL mysql_error: %s(%d)\n",kakao_signal[ALLOW_SEND_DATA_TO_COMPANION],mysql_error(my_data->conn),ret ); fflush(my_data->log);
						strcpy(to_id,"");
						return 0;
					}

					if(strcmp(my_data->recv_buf,"y")==0)
					{
						sprintf(my_data->query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_data->id );
						mysql_query(my_data->conn,my_data->query_buf);//записываем текст ошибки в бд
					}
					strcpy(to_id,"");
					return 0;
				}
			}
		}
		mysql_query(my_data->conn,"COMMIT");
		return 1;//можно отправлять данные
	} 
	else {
		strcpy(to_id,"");
		return 0;//нельзя отправлять данные
	}
}
int allow_send_data_to_party(some_data *my_data,char *party_id)//при отправке каких либо данных(файлов или текста или изображения) проверяем можно ли отправлять их группе и если можно выполняем какие либо другие действия 
{
	uint32_t sz;
	MYSQL_RES *result;
	MYSQL_ROW row;
	short int ret;
	/*убеждаемся что мы состоим в этой группе(вдруг пользователь отправит чужой id группы)*/
	sprintf(my_data->query_buf, "SELECT count(*) FROM party_users WHERE party_id = \"%s\" AND user_id = \"%s\"",party_id,my_data->id );
	if(mysql_query(my_data->conn,my_data->query_buf)!=0)// получаем запись состоит ли пользователь в группе
	{//обрабатываем ошибку
		fprintf(my_data->log, "ERROR: %s\t%s\n",kakao_signal[ALLOW_SEND_DATA_TO_PARTY], mysql_error(my_data->conn));//записываем ошибку в лог
		fflush(my_data->log);
		mysql_query(my_data->conn,"ROLLBACK");
		sprintf(my_data->query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[ALLOW_SEND_DATA_TO_PARTY],my_data->id );

		mysql_query(my_data->conn,my_data->query_buf);//записываем текст ошибки в бд

		/*принимаем ответ пользователя чтобы удалить запись из бд*/
		if( (ret=recvall(my_data->sock,my_data->recv_buf,sizeof(my_data->recv_buf),0))==0 ) {
			strcpy(party_id,"");
			return 0;//если клиент отключился
		}
		else if(ret<0) {
			fprintf(my_data->log,"ERROR %s: RECVALL mysql query is in a party?(%d)\n",kakao_signal[ALLOW_SEND_DATA_TO_PARTY],ret ); fflush(my_data->log);
			strcpy(party_id,"");
			return 0;
		}

		if(strcmp(my_data->recv_buf,"y")==0) {
			sprintf(my_data->query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",my_data->id );
			mysql_query(my_data->conn,my_data->query_buf);//записываем текст ошибки в бд
		}
		strcpy(party_id,"");
		return 0;
	}
	result=mysql_store_result(my_data->conn);// Извлечение результатов запроса
	row=mysql_fetch_row(result);
	mysql_free_result(result);
	if(atoi(row[0]))
		return 1;
	else {//если пользоваетль пытается нас обмануть
		strcpy(party_id,"");
		return 0;
	}
}
int block_user(some_data *data,char *from_id,char *to_id)
{
	uint32_t sz;
	MYSQL_RES *result;
	MYSQL_ROW row;
	short int ret;
	sprintf(data->query_buf, "SELECT count(*) FROM blocked_users WHERE from_id=\"%s\" AND to_id=\"%s\"",from_id,to_id);
	if(mysql_query(data->conn,data->query_buf)!=0)// есть ли у нас запись с данным пользователем
	{
		fprintf(data->log, "ERROR: %s\t%s\n",kakao_signal[BLOCK_USER], mysql_error(data->conn));//записываем ошибку в лог
		fflush(data->log);
		sprintf(data->query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[BLOCK_USER],data->id );

		mysql_query(data->conn,data->query_buf);//записываем текст ошибки в бд

		/*принимаем ответ пользователя чтобы удалить запись из бд*/
		if( (ret=recvall(data->sock,data->recv_buf,sizeof(data->recv_buf),0))==0 ) return 1;//если клиент отключился
		else if(ret<0) {
			fprintf(data->log,"ERROR %s: RECVALL mysql query select count block user(%d)\n",kakao_signal[BLOCK_USER],ret ); fflush(data->log);
			return 1;//возвращаем что пользователь заблокирован
		}

		if(strcmp(data->recv_buf,"y")==0) {
			sprintf(data->query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",data->id );
			mysql_query(data->conn,data->query_buf);//записываем текст ошибки в бд
		}
		return 1;//возвращаем что пользователь заблокирован
	}
	result=mysql_store_result(data->conn);
	row=mysql_fetch_row(result);
	mysql_free_result(result);
	return atoi(row[0]);
}
int user_writing(some_data *data,char *from_id,char *to_id)
{
	uint32_t sz;
	MYSQL_RES *result;
	MYSQL_ROW row;
	short int ret;
	sprintf(data->query_buf, "SELECT CASE WHEN NOW()-when_writed<=%d THEN '1' ELSE '0' END AS t1 FROM companion_writes WHERE user_id=\"%s\" AND to_user_id=\"%s\"",WRITING_TIME,from_id,to_id);
	if(mysql_query(data->conn,data->query_buf)!=0)// получаем печатает ли пользователь сообщение
	{
		fprintf(data->log, "ERROR: %s\t%s\n",kakao_signal[USER_WRITING], mysql_error(data->conn));//записываем ошибку в лог
		fflush(data->log);
		sprintf(data->query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[USER_WRITING],data->id );

		mysql_query(data->conn,data->query_buf);//записываем текст ошибки в бд

		/*принимаем ответ пользователя чтобы удалить запись из бд*/
		if( (ret=recvall(data->sock,data->recv_buf,sizeof(data->recv_buf),0))==0 ) return 0;//если клиент отключился
		else if(ret<0) {
			fprintf(data->log,"ERROR %s: RECVALL mysql query writing?(%d)\n",kakao_signal[USER_WRITING],ret ); fflush(data->log);
			return 0;
		}

		if(strcmp(data->recv_buf,"y")==0) {
			sprintf(data->query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",data->id );
			mysql_query(data->conn,data->query_buf);//записываем текст ошибки в бд
		}
		return 0;
	}
	result=mysql_store_result(data->conn);
	row=mysql_fetch_row(result);
	mysql_free_result(result);
	if(mysql_num_rows(result)) {
		return atoi(row[0]);
	}
	else {
		return 0;
	}
}
void party_writing(some_data *data,char *party_id)
{
	uint32_t sz;
	MYSQL_RES *result;
	MYSQL_ROW row;
	short int ret;
	sprintf(data->query_buf, "SELECT t2.nickname FROM party_writes AS t1 INNER JOIN users AS t2 ON (NOW()-t1.when_writed<=%d) AND t1.user_id=t2.id AND t1.to_party_id=\"%s\" AND t1.user_id!=\"%s\"",WRITING_TIME,party_id,data->id);
	if(mysql_query(data->conn,data->query_buf)!=0)// получаем nickname тех кто в группе набирает сообщения
	{
		fprintf(data->log, "ERROR: %s\t%s\n",kakao_signal[PARTY_WRITING], mysql_error(data->conn));//записываем ошибку в лог
		fflush(data->log);
		sprintf(data->query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[PARTY_WRITING],data->id );

		mysql_query(data->conn,data->query_buf);//записываем текст ошибки в бд

		/*принимаем ответ пользователя чтобы удалить запись из бд*/
		if( (ret=recvall(data->sock,data->recv_buf,sizeof(data->recv_buf),0))==0 ) return;
		else if(ret<0) {
			fprintf(data->log,"ERROR %s: RECVALL mysql query select nickname user party writing(%d)\n",kakao_signal[PARTY_WRITING],ret ); fflush(data->log);
			return;
		}

		if(strcmp(data->recv_buf,"y")==0) {
			sprintf(data->query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",data->id );
			mysql_query(data->conn,data->query_buf);//записываем текст ошибки в бд
		}
		return;
	}
	result=mysql_store_result(data->conn);
	if(mysql_num_rows(result))
	{
		for(int i=0; (row=mysql_fetch_row(result)); ++i )
		{
			if(i<MAX_PARTY_NICKNAME_WRITINGS)//показываем никнеймы только n человек
			{
				/*отправляем размер имени юзера печатающего сообщение*/
				sz=htonl(get_str_size(row[0]));
				if( !(ret=send(data->sock,&sz,sizeof(sz),0)) ) {
					fprintf(data->log, "ERROR %s : SEND size of name user writing?(%d)\n",kakao_signal[PARTY_WRITING],ret ); fflush(data->log);
					return;
				}
				/*отправляем имя юзера печатающего сообщение */
				if( !(ret=sendall(data->sock,row[0],ntohl(sz))) ) {
					fprintf(data->log, "ERROR %s : SENDALL name user writing?(%d)\n",kakao_signal[PARTY_WRITING],ret ); fflush(data->log);
					return;
				}
			}
			else//если больше n человек
			{
				/*отправляем true, чтобы пользователь мог знать сколько еще человек печатает в группе*/
				sz=htonl(1);
				if( !(ret=send(data->sock,&sz,sizeof(sz),0)) ) {
					fprintf(data->log, "ERROR %s : SEND size of 1 message party writing(%d)\n",kakao_signal[PARTY_WRITING],ret ); fflush(data->log);
				}
			}
		}
	}
	mysql_free_result(result);
	/*отправляем нулевой размер чтобы юзер знал где конец*/
	sz=htonl(0);
	if( !(ret=send(data->sock,&sz,sizeof(sz),0)) ) {
		fprintf(data->log, "ERROR %s : SEND size of 0 message(%d)\n",kakao_signal[PARTY_WRITING],ret ); fflush(data->log);
	}
}
void user_online(some_data *data,char *people_id)
{
	uint32_t sz;
	MYSQL_RES *result;
	MYSQL_ROW row;
	short int ret;
	sprintf(data->query_buf, "SELECT CASE WHEN NOW()-when_online<=%d THEN '%d' WHEN NOW()-when_online<=%d THEN '%d' ELSE '%d' END AS t1 FROM users WHERE id=\"%s\"",ONLINE_TIME,ONLINE,NOTONLINE_TIME,WAS_ONLINE_RECENTLY,NOT_ONLINE,people_id);
	if(mysql_query(data->conn,data->query_buf)!=0)// получаем когда пользователь был онлайн
	{
		fprintf(data->log, "ERROR: %s\t%s\n",kakao_signal[USER_ONLINE], mysql_error(data->conn));//записываем ошибку в лог
		fflush(data->log);
		sprintf(data->query_buf, "UPDATE errors SET error=\"%s\" WHERE user_id=\"%s\"",kakao_error[USER_ONLINE],data->id );

		mysql_query(data->conn,data->query_buf);//записываем текст ошибки в бд

		/*принимаем ответ пользователя чтобы удалить запись из бд*/
		if( (ret=recvall(data->sock,data->recv_buf,sizeof(data->recv_buf),0))==0 ) return;//если клиент отключился
		else if(ret<0) {
			fprintf(data->log,"ERROR %s: RECVALL mysql query when was online?(%d)\n",kakao_signal[USER_ONLINE],ret ); fflush(data->log);
			mysql_free_result(result);
			return;//ждем следующий сигнал
		}

		if(strcmp(data->recv_buf,"y")==0) {
			sprintf(data->query_buf, "UPDATE errors SET error=NULL WHERE user_id=\"%s\"",data->id );
			mysql_query(data->conn,data->query_buf);//записываем текст ошибки в бд
		}
		mysql_free_result(result);
		return;//ждем следующий сигнал
	}
	result=mysql_store_result(data->conn);
	row=mysql_fetch_row(result);
	mysql_free_result(result);
	if(mysql_num_rows(result))
	{
		/*отправляем когда был онлайн */
		sz=htonl(atoi(row[0]));
		if( !(ret=send(data->sock,&sz,sizeof(sz),0)) ) {
			fprintf(data->log, "ERROR %s : SEND  when was online?(%d)\n",kakao_signal[USER_ONLINE],ret ); fflush(data->log);
			return;//ждем следующий сигнал
		}
	}
	else
	{
		/*отправляем не в сети */
		sz=htonl(NOT_ONLINE);
		if( !(ret=send(data->sock,&sz,sizeof(sz),0)) ) {
			fprintf(data->log, "ERROR %s : SEND  NOT_ONLINE(%d)\n",kakao_signal[USER_ONLINE],ret ); fflush(data->log);
			return;//ждем следующий сигнал
		}
	}
}
int check_password(MYSQL *conn,const char *nickname,const char *password)
{
	MYSQL_RES *result;
	short int ret;
	char query_buf[MAX_QUERY_BUF];
	sprintf(query_buf,"SELECT id FROM users WHERE nickname = \"%s\" AND password = \"%s\"",nickname,password);
	if(mysql_query(conn,query_buf)!=0)
	{// Делаем запрос на извлечение данных с введенным никнеймом
		mysql_query(conn,"ROLLBACK");
		return -1;
	}
	result=mysql_store_result(conn);// Извлечение результатов запроса (необходимо извлекать после каждого запроса, который возвращает записи)
	ret=mysql_num_rows(result);
	mysql_free_result(result);
	if(ret>0)
		return 1;//если пароль от пользователя верный
	else
		return 0;//если пароль от пользователя неверный
}
int check_unique_nickname(MYSQL *conn,const char* nickname)
{
	MYSQL_RES *result;
	short int ret;
	char query_buf[MAX_QUERY_BUF];
	sprintf(query_buf,"SELECT id FROM users WHERE nickname = \"%s\"",nickname);
	if(mysql_query(conn,query_buf)!=0)// Делаем запрос на извлечение данных с введенным никнеймом
	{
		mysql_query(conn,"ROLLBACK");
		return -1;
	}
	result=mysql_store_result(conn);// Извлечение результатов запроса (необходимо извлекать после каждого запроса, который возвращает записи)
	ret=mysql_num_rows(result);
	mysql_free_result(result);
	if(ret==0)
		return 1;//если никнейм уникален
	else
		return 0;// если никнейм не уникален
}
char* get_id(MYSQL *conn,const char* nickname)
{
	MYSQL_RES *result;
	char query_buf[MAX_QUERY_BUF];	
	sprintf(query_buf,"SELECT id FROM users WHERE nickname = \"%s\"",nickname);
	if(mysql_query(conn,query_buf)!=0)// Делаем запрос на извлечение id нашего пользователя
	{
		mysql_query(conn,"ROLLBACK");
		return "Произошла ошибка";
	}
	result=mysql_store_result(conn);
	//если количество возвращенных записей = 1
	if(mysql_num_rows(result) == 1) {
		mysql_free_result(result);
		return (mysql_fetch_row(result))[0];//row[0]
	}
	else {
		mysql_free_result(result);
		return "-1";
	}

}
void create_server_files(void)
{
	char buf[(MAX_PATH*2)+256];

	sprintf(buf,"if ! [ -d \"%s\" ]; then mkdir -p %s; fi",PATHS[PATH_COMPANION_FILES],PATHS[PATH_COMPANION_FILES]);//создаем команду для создания дирректории
	system(buf);//создаем каталог если его нет

	sprintf(buf,"if ! [ -d \"%s\" ]; then mkdir -p %s; fi",PATHS[PATH_COMPANION_PHOTOS],PATHS[PATH_COMPANION_PHOTOS]);//создаем команду для создания дирректории
	system(buf);//создаем каталог если его нет

	sprintf(buf,"if ! [ -d \"%s\" ]; then mkdir -p %s; fi",PATHS[PATH_PARTY_FILES],PATHS[PATH_PARTY_FILES]);//создаем команду для создания дирректории
	system(buf);//создаем каталог если его нет

	sprintf(buf,"if ! [ -d \"%s\" ]; then mkdir -p %s; fi",PATHS[PATH_PARTY_PHOTOS],PATHS[PATH_PARTY_PHOTOS]);//создаем команду для создания дирректории
	system(buf);//создаем каталог если его нет

	sprintf(buf,"if ! [ -d \"%s\" ]; then mkdir -p %s; fi",PATHS[PATH_USER_AVATARS],PATHS[PATH_USER_AVATARS]);//создаем команду для создания дирректории
	system(buf);//создаем каталог если его нет

	sprintf(buf,"if ! [ -d \"%s\" ]; then mkdir -p %s; fi",PATHS[PATH_PARTY_AVATARS],PATHS[PATH_PARTY_AVATARS]);//создаем команду для создания дирректории
	system(buf);//создаем каталог если его нет

}
