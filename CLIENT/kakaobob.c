#include <sha2.h>
#include "play_wav.h"
#include <gtk/gtk.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <mysql.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <kakao.h>
#include <mystdlib.h>
#include <customsocket.h>
#include <minIni.h>
#include <string.h>
#define sizearray(a)  (sizeof(a) / sizeof((a)[0]))
#define PATH_BOB_CONF "/etc/KakaoBob/conf.ini"
//#define PATH_BOB_CONF "/home/piska/kakao/etc/KakaoBob/conf.ini"
char PATHS [25][MAX_PATH];
enum paths
{
	PATH_AUTOLOG,
	PATH_LOG,
	PATH_DIALOG_PHOTOS,
	PATH_USER_AVATARS,
	PATH_PARTY_AVATARS,
	PATH_INTERFACE,
	PATH_AUDIO_PAN
};
const char *  text_paths[]=
{
	"path_autolog",
	"path_log",
	"path_dialog_photos",
	"path_user_avatars",
	"path_party_avatars",
	"path_interface",
	"path_audio_pan",
	""
};
unsigned long int LIMITS[25];//здесь хранятся стандартные значения из конфига которые не нужно изменять
unsigned long int limits[25];//используется как переменная для постоянного изменения
enum limits
{
	LIMIT_PEOPLES,
	LIMIT_MESSAGES,
	LIMIT_FRIENDS,
	LIMIT_MY_SUBS,
	LIMIT_PARTY,
	LIMIT_PARTY_PEOPLES,
	LIMIT_DIALOG
};
const char * text_limits[]=
{
	"limit_peoples",
	"limit_messages",
	"limit_friends",
	"limit_my_subs",
	"limit_party",
	"limit_party_peoples",
	"limit_dialog",
	""
};
char CONNECT_SERVER[4][256];
enum connect_server
{
	LOCAL,
	DOMAIN,
	PORT
};
const char *  text_connect_server[]=
{
	"local",
	"domain",
	"port",
	""
};
#define MY_VERSION "1.0" 
pthread_mutex_t thread_lock=PTHREAD_MUTEX_INITIALIZER;//задаем стандартные параметры мьютексу
typedef enum error_window_mode {NOTCLOSE,CLOSE} error_window_mode;
GdkRectangle workarea = {0};
GtkBuilder *builder;
GtkWidget *label;//используется для меток которые постоянно изменяются(динамические),например для сообщений пользователей
MYSQL *conn;
GError *error=NULL;//иницилизируем переменную для вывода ошибок(g_warning( "%s", error->message ))
FILE *user_log,*boblog;
GtkTextBuffer *dialog_buffer;//буфер, в который пользователь пишет сообщение в диалоговом окне
GtkTextBuffer *create_party_buffer;
GtkTextBuffer *party_buffer;
GtkTextBuffer *mes_change_buffer;
GObject  * object;
pthread_t var_thread_update_dialog_messages;
pthread_t var_thread_update_dialog;
pthread_t var_thread_messages_list;
pthread_t var_thread_party_messages_list;
pthread_t var_thread_unread_dialogs;
pthread_t var_thread_check_errors;
pthread_t var_thread_play_sound;
/*переменные для вставки сообщения(label) в grid(функция show_message)*/
unsigned long int top_message=0;
const int LEFT_MESSAGE=0;
const int RIGHT_MESSAGE=1;
const char *path_to_avatar;
void* thread_result;//вовзращаемое поточной функцией значение
int last_write_second;

unsigned long int limit_peoples;
unsigned long int limit_messages;
unsigned long int limit_friends;
unsigned long int limit_my_subs;
unsigned long int limit_dialog;
unsigned long int limit_party;
unsigned long int limit_party_peoples;
typedef struct people_widget
{
	GtkWidget *image_button;
	GtkWidget *image2;
	GtkWidget *image3;
	GtkWidget *message_button;
	GtkWidget *label;
	GtkWidget *label2;
	GtkWidget *label3;
	GtkWidget *label4;
	GtkWidget *Window;
	GtkWidget *Button;
	GtkWidget *Button2;
	GtkWidget *Button3;
	GtkWidget *Button4;
	GtkWidget *Button5;
	GtkWidget *Button6;
	GtkWidget *Button7;
	GtkWidget *Button8;
	GtkWidget *MenuButton;
	GtkWidget *MenuButton2;
	GtkWidget *Box;
	GtkWidget *Box2;
	GtkWidget *Box3;
	GtkWidget *Revealer;
	GtkWidget *Revealer2;
	GtkWidget *Revealer3;
	GtkWidget *Revealer4;
	GtkWidget *Grid;
	GtkWidget *Grid2;
	GtkWidget *PeopleSearchEntry;
	GtkWidget *PeopleListBoxParrent;
	GtkWidget *TextView;
	GtkWidget *Scrolled;
	GtkWidget *Entry1;
	GtkWidget *Entry2;
	GtkWidget *popover1;
	GtkWidget *spinner;
	GtkAdjustment *Adjustment;
}people_widget;
typedef struct Window
{
	GtkWidget *Window;
	GtkWidget *Nickname;
	GtkWidget *Name;
	GtkWidget *Surname;
	GtkWidget *Password;
	GtkWidget *TextInput;
	GtkWidget *TextBox;//имеет разные назначения, в основном используется как контейнер для меток, но может быть меткой(статической), а не контейнером
	GtkWidget *Notebook;
	GtkWidget *Button;
	GtkWidget *Revealer1;
	GtkWidget *Revealer2;
	GtkWidget *label;
}window;
typedef struct Panel
{
	GtkWidget *Panel;
	GtkWidget *Button1;
	GtkWidget *Revealer1;
}panel;
char *text_when_online[]=
{
	"В сети",
	"Был(а) в сети недавно",
	"Не в сети"
};
typedef struct answer_application_popover
{
	GtkWidget *Window;
	GtkWidget *Button1;
	GtkWidget *Button2;
	GtkWidget *Button3;
}answer_popover;
typedef struct sig_data
{
	GtkWidget *ChildNotebookWidget;
	GtkWidget *BackButton;
	GtkWidget *ShowWidget;
	GtkWidget *HideWidget;
	GtkWidget *label;
	GtkWidget *widget1;
	mes_hposition hpos;
	char id[MYSQL_INT];
	char file_size[MYSQL_INT];
	char *text;
	char created_at[25];//когда написано сообщение
	bool saw;//прочитано ли сообщение
	bool photo;//содержит ли сообщение фото
	bool flag_photo_revealer;
	int mes_type;//тип сообщения
	char dialog_type[20];//тип диалога(COMPANION или PARTY)
	char path[MAX_PATH];
	char to_id[MYSQL_INT];
	char party_id[MYSQL_INT];
	char nickname[MAX_NICKNAME];
	struct sig_data *prev;
}sig_data;//данные для сигналов
typedef struct desc_mess//описание сообщения
{
	GtkWidget *textview;
	char *text;
	mes_hposition hpos;
	char type[20];
	char id[MYSQL_INT];
}desc_mess;//данные для сигналов
sig_data *node_people_list=NULL;
sig_data *node_friend_list=NULL;
sig_data *node_sub_list=NULL;
sig_data *node_messages_list=NULL;
sig_data *node_profile_window=NULL;
sig_data *node_dialog_window=NULL;
sig_data *node_profile_party=NULL;
sig_data *node_party_peoples=NULL;
sig_data *node_change_message=NULL;
sig_data *node_profile_setting=NULL;
window main_window,reg_window,log_in_window,error_window,file_chooser_window;
info my_info;//наши данные
info people_info;//данные пользователя
info party_info;//данные группы
people_widget friend_list,sub_list,people_list,message_list,people_profile;
people_widget messages_list,dialog_window,profile_setting,password_setting,setting_window;
people_widget create_party,profile_party,party_peoples_window,popover_profile_party,change_message,popover_change_mes;
panel setting_panel;
answer_popover answer_app_profile;
char buf[MAX_BUF];
int quantity_unread_dialogs_companion=0;
int quantity_unread_dialogs_party=0;
char type[20];
char log_nick[MAX_NICKNAME];
char log_pass[MAX_PASSWORD];
char filename[MAX_FILENAME];
char c;
short int ret;
int sock;
_Bool flag=FALSE;
_Bool flag_dialog=FALSE;
_Bool flag_dialog_messages=FALSE;
_Bool flag_messages_list=FALSE;
_Bool flag_party_messages_list=FALSE;
_Bool flag_panel_right=FALSE;
_Bool flag_unread_dialogs=FALSE;
bool flag_admin=false;
bool flag_unread_dialogs_sound=false;

gboolean send_event_return(GtkWidget *widget,GdkEventKey *event,sig_data *data);
gboolean edit_event_return(GtkWidget *widget,GdkEventKey  *event,sig_data *data);
void *check_errors(void *data);
void play_sound(const char *path);
void *func_thread_play_sound(void *data);
int init_auth(int sock);
void get_bob_conf();
void signal_handler(int sig);
void show_message(sig_data *data);
void send_message(GtkWidget *widget, sig_data* data);
void bob_send_file(GtkWidget *widget, sig_data* data);
void bob_send_photo(GtkWidget *widget, sig_data *data);
void show_messages(GtkWidget *widget, sig_data* data);
void photo_revealer(GtkWidget *widget, sig_data *data);
void show_photo_full(GtkWidget *widget, sig_data *data);
void download(GtkWidget *widget, sig_data *data);
void edit_message(GtkWidget *widget, sig_data *data);
void delete_message_everywhere(GtkWidget *widget, sig_data *data);
void delete_my_messages_from_dialog(GtkWidget *button, sig_data* data);
void close_and_free_all();//закрывает все соединения и освобождаем память(функция используется при выходе из программы)
void get_reg_form();
void bob_init();
void from_log_in_to_registration();
void from_registration_to_log_in();
void get_log_form();
int user_online(int sock);
void exit_from_account();
void transition_to_dialog_window(GtkWidget *widget, sig_data *data);
void show_profile_window(GtkWidget *button, sig_data* data);
void show_profile_party(GtkWidget *button, sig_data* data);
void show_profile_party_avatar(GtkWidget *back_button, sig_data *data);
void show_profile_party_users(GtkWidget *widget,sig_data* data);
void delete_party_avatar(GtkWidget *widget,sig_data* data);
void change_party_avatar(GtkWidget *widget,sig_data* data);
void clean_window(GtkWidget*window);//функция очищает контейнер от дочерних виджетов
void hide_window(GtkWidget*window);//функция скрывает дочерние виджеты контейнера
void show_window(GtkWidget*window);//функция показывает дочерние виджеты контейнера
void show_messages_list(GtkWidget *widget, sig_data* data2);
void show_party_messages_list(GtkWidget *widget, sig_data* data2);
void show_people_list();
void transition_to_friend_list(GtkWidget *button, sig_data *data);
void transition_to_sub_list(GtkWidget *button, sig_data* data);
void transition_to_messages_list(GtkWidget *widget, sig_data *data);
void transition_to_party_messages_list(GtkWidget *widget, sig_data *data);
void transition_to_people_list(GtkWidget *widget, sig_data *data);
void transition_to_profile_party(GtkWidget *widget, sig_data *data);
void transition_to_party_appoint_admins(GtkWidget *widget,sig_data* data);
void transition_to_party_delete_admins(GtkWidget *widget,sig_data* data);
void transition_to_party_add_friend(GtkWidget *widget,sig_data* data);
void show_friend_list();
void free_sig_data(sig_data *node);//очищает список sig_data
void show_my_subs();
void delete_friend(GtkWidget *widget, sig_data* data);
void delete_sub(GtkWidget *button, sig_data* data);
void add_friend(GtkWidget *widget, sig_data* data);
void add_sub(GtkWidget *button, sig_data* data);//добавляет человека, который отправил нам запрос
void back(GtkWidget *back_button, sig_data *data);
void insert_to_list_sig_data(sig_data *data,sig_data **node);
void show_dialog_window(GtkWidget *widget, sig_data* data);
void show_setting_panel(GtkWidget *button, sig_data* data);
void add_limit(GtkWidget *widget,GtkPositionType pos,sig_data* data);
void nullify_limit(GtkWidget *widget, sig_data* data);
void *update_dialog_messages(void *data);
void *update_dialog(void *data);
void *update_messages_list(void *data);
void *update_party_messages_list(void *data);
void *update_unread_dialogs(void *data);
void change_nickname(GtkWidget *back_button, sig_data *data);
void show_setting_profile(GtkWidget *widget,sig_data* data);
void hide_setting_window(GtkWidget *widget,sig_data* data);
void hide_create_party();
void hide_party_peoples_window();
void show_change_message(GtkWidget *widget, sig_data *data);
void hide_change_message();
void show_change_message_textview(GtkWidget *widget, sig_data *data);
void show_party_delete_admins(GtkWidget *widget,sig_data* data);
void show_party_appoint_admins(GtkWidget *widget,sig_data* data);
void show_party_add_friends(GtkWidget *widget,sig_data* data);
void show_create_party();
void party_add_admin(GtkWidget *widget,sig_data* data);
void party_delete_admin(GtkWidget *widget,sig_data* data);
void party_add_friend(GtkWidget *widget,sig_data* data);
void party_delete_user(GtkWidget *widget,sig_data* data);
void party_exit(GtkWidget *widget,sig_data* data);
void bob_create_party(GtkWidget *widget,sig_data* data);
void show_error_window(char *err_text,int flag);
void hide_error_window(GtkWidget *widget,sig_data* data);
void kill_error_window(GtkWidget *widget,sig_data* data);
void change_password(GtkWidget *back_button, sig_data *data);
void delete_my_avatar(GtkWidget *widget,sig_data* data);
void change_my_avatar(GtkWidget *widget,sig_data* data);
void change_party_name();
void test(GtkWidget *back_button, sig_data *data);
void unblock_user(GtkWidget *button, sig_data* data);
void block_user(GtkWidget *button, sig_data* data);
void unread_dialogs(GtkWidget *widget, sig_data* data);
int main(int argc, char  *argv[])
{
	get_bob_conf();

	boblog=fopen(PATHS[PATH_LOG],"w+");
	if(boblog==NULL) {
		g_warning( "cant open file boblog" );
		fclose(boblog);
        return 0;
	}
	if(dup2(fileno(boblog),fileno(stderr))==-1)
	{
		perror("DUP2:");
		return 0;
	}
	sock=socket(AF_INET,SOCK_STREAM,0);
	if(!sock) {
		perror("SOCKET");
		fclose(boblog);
		return 0;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(CONNECT_SERVER[PORT]));
	if(strcmp(CONNECT_SERVER[LOCAL],"no")==0) {
		struct hostent *host_entry=gethostbyname(CONNECT_SERVER[DOMAIN]);//получаем ip домена
		struct in_addr ip = *((struct in_addr*) host_entry->h_addr_list[0]); 
		addr.sin_addr.s_addr = ip.s_addr;
	}
	else {
		addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	}
	//printf("IP: %s\n", inet_ntoa(addr.sin_addr));

	if(connect(sock,(struct sockaddr*)&addr,sizeof(addr))==-1)
	{
		perror("CONNECT");
		close_and_free_all();
		return 0;
	}
	//close(sock);
	gtk_init(&argc,&argv);// Инициализируем GTK+
	bob_init();
	gtk_widget_hide(main_window.Window);// Скрываем главное окно
	ret=init_auth(sock);
	if(ret) {//если мы отправили верный логин или пароль
		printf("%s %s \n", log_nick,log_pass);
		strcpy(my_info.nickname,log_nick);//записываем никнейм
		strcpy(my_info.password,log_pass);//записываем  пароль

		gtk_widget_show_all(main_window.Window);//показываем главное окно
		show_people_list();
		show_friend_list();
		show_messages_list(NULL,NULL);
		pthread_create(&var_thread_messages_list,NULL,update_messages_list,NULL);//создаем поток в котором будет обновляться страница messages_list
		pthread_create(&var_thread_check_errors,NULL,check_errors,NULL);//создаем поток в котором будут проверяться ошибки
	}
	else if(ret==0) {
		gtk_widget_show_all(log_in_window.Window);//показываем регистрационное окно
	}
	else if(ret==-1) {
		close_and_free_all();
		return 0;
	}

	signal(SIGINT,signal_handler);
	gtk_main();//передаем управление gtk+


	/////////////////////////////////////////////////////////////
	close_and_free_all();//выполняется после того как пользователь вышел из приложения(или просто вызвалась функция gtk_main_quit)
	return 0;
}
void signal_handler(int sig)
{
	gtk_main_quit();
}
gboolean edit_event_return(GtkWidget *widget,GdkEventKey  *event,sig_data *data)
{
	if(event->keyval==GDK_KEY_Return || event->keyval==GDK_KEY_KP_Enter || event->keyval==GDK_KEY_ISO_Enter || event->keyval==GDK_KEY_3270_Enter || event->keyval==GDK_KEY_RockerEnter)
	{
		fprintf(stderr,"\nEVENT_RETURN: %s\n", kakao_signal[EDIT_MESSAGE]); fflush(stderr);
		edit_message(widget,data);
		return TRUE;//не распространять событие дальше(клавиша не пойдет в буфер)
	}
	else
	{
		fprintf(stderr,"EVENT_NOTRETURN: %s\n",event->string); fflush(stderr);
		return FALSE;//распространять событие дальше(клавиша пойдет в буфер)
	}
	//event->send_event=FALSE;//нужно обнулить, чтобы стандартный обработчик не стал обрабатывать сигнал после этого

}
void clean_window(GtkWidget*window)//функция очищает контейнер от дочерних виджетов
{
	GList *children,*iter;//создаем структуры(двусвязные списки)
	children=gtk_container_get_children(GTK_CONTAINER(window));//получаем список дочерних виджетов
	for(iter=children;iter!=NULL;iter=iter->next)
	{
		gtk_widget_destroy(GTK_WIDGET(iter->data));//удаляем виджет
	}
}
void insert_to_list_sig_data(sig_data *data,sig_data **node)
{
	if(*node!=NULL)
	{
		data->prev=*node;
	} 
	else
	{
		data->prev=NULL;
	}	
	*node=data;
}
void free_sig_data(sig_data *node)
{
	sig_data *a=NULL;
	//очищаем список
	while(node!=NULL)
	{	
		if(node->prev!=NULL)
		{
			a=node->prev;
			free(node);
			if(a->prev==NULL)
			{
				free(a);
				node=NULL;
			}
			else
			{
				node=a;
			}
		}
		else
		{
			free(node);
			node=NULL;
		}
	}
}
void hide_window(GtkWidget*window)//функция скрывает дочерние виджеты контейнера
{
	GList *children,*iter;//создаем указатели на структуры(двусвязные списки)
	children=gtk_container_get_children(GTK_CONTAINER(window));//получаем список дочерних виджетов
	for(iter=children;iter!=NULL;iter=iter->next)
	{
		gtk_widget_hide(GTK_WIDGET(iter->data));//удаляем виджет
	}
}
void show_window(GtkWidget*window)//функция показывает сам контейнер и его дочерние виджеты
{
	GList *children,*iter;//создаем структуры(двусвязные списки)
	children=gtk_container_get_children(GTK_CONTAINER(window));//получаем список дочерних виджетов
	for(iter=children;iter!=NULL;iter=iter->next)
	{
		gtk_widget_show(GTK_WIDGET(iter->data));//показываем виджет
	}
	gtk_widget_show(window);
}

void get_bob_conf()
{
	short int ret=0;
	/*получаем пути к различным файлам или каталогам*/
	for(int i=0;1;++i) {
		if(!ini_gets("paths", text_paths[i], NULL, PATHS[i], sizeof(*PATHS), PATH_BOB_CONF)) break;
	}

	/*получаем ограничения*/
	for(int i=0;1;++i) {
		if((ret=ini_getl("limits", text_limits[i], -1, PATH_BOB_CONF))<0) break;
		LIMITS[i]=ret;
		limits[i]=ret;
	} 

	/*получаем данные для подключения к серверу kakaobob*/
	for(int i=0;1;++i) {
		if(!ini_gets("connect_server", text_connect_server[i], NULL, CONNECT_SERVER[i], sizeof(*CONNECT_SERVER), PATH_BOB_CONF)) break;
	}
}
int init_auth(int sock)
{
	char buf[MAX_QUERY_BUF];
	int a;
	/*отправляем размер версии*/
	sz=htonl(get_str_size(MY_VERSION));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of version(%d)\n",kakao_signal[BOB_START_INIT],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_START_INIT]);
		show_error_window(buf,CLOSE);//закрываем программу
	}
	/*отправляем версию*/
	if( !(ret=sendall(sock,MY_VERSION,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL version(%d)\n",kakao_signal[BOB_START_INIT],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_START_INIT]);
		show_error_window(buf,CLOSE); //закрываем программу
	}
	if( !(recvall(sock,buf,sizeof(buf),0)) ) {//принимаем ответ сервера
		show_error_window("Сервер отключился",CLOSE); //закрываем программу
	}	
	if(strcmp(buf,"y")!=0) {//если сервер запретил открывать приложение
		show_error_window("Обновите приложение!",CLOSE); //закрываем программу
	}

	ini_gets("autolog", "login", NULL, log_nick, sizearray(log_nick), PATHS[PATH_AUTOLOG]);//узнаем login(nickname)
 	ini_gets("autolog", "password", NULL, log_pass, sizearray(log_pass), PATHS[PATH_AUTOLOG]);//узнаем password

	/*отправляем размер никнейма(логина)*/
	sz=htonl(get_str_size(log_nick));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR: SEND size of login(%d)\n",ret ); fflush(stderr);
		show_error_window(kakao_error[DARK_ERROR],CLOSE); //закрываем программу
	}
	/*отправляем никнейм(логин)*/
	if( !(ret=sendall(sock,log_nick,ntohl(sz))) ) {
		fprintf(stderr, "ERROR: SENDALL login(%d)\n",ret ); fflush(stderr);
		show_error_window(kakao_error[DARK_ERROR],CLOSE); //закрываем программу
	}

	/*отправляем размер пароля*/
	sz=htonl(get_str_size(log_pass));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR: SEND size of pass(%d)\n",ret ); fflush(stderr);
		show_error_window(kakao_error[DARK_ERROR],CLOSE); //закрываем программу
	}
	/*отправляем пароль*/
	if( !(ret=sendall(sock,log_pass,ntohl(sz))) ) {
		fprintf(stderr, "ERROR: SENDALL pass(%d)\n",ret ); fflush(stderr);
		show_error_window(kakao_error[DARK_ERROR],CLOSE); //закрываем программу
	}

	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {//принимаем ответ(правильный ли логин и пароль)
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR: RECV answer(%d)\n",ret ); fflush(stderr);
		show_error_window(kakao_error[DARK_ERROR],CLOSE); //закрываем программу
	}
	a=ntohl(sz);
	if(!a) return 0;//если неверный логин или пароль
	/*принимаем размер моего id*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR AUTH: RECV size of my id(%d)\n",ret ); fflush(stderr);
		show_error_window(kakao_error[DARK_ERROR],NOTCLOSE); //закрываем программу
	}
	/*принимаем моЙ id*/
	if( (ret=recvall(sock,my_info.id,ntohl(sz),MSG_WAITALL))==0 ) {//принимаем ответ
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR AUTH: RECVALL my id(%d)\n",ret ); fflush(stderr);
		show_error_window(kakao_error[DARK_ERROR],NOTCLOSE); //закрываем программу
	}
	if(a) return 1;
}
void delete_message_everywhere(GtkWidget *widget, sig_data *data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE]); fflush(stderr);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[DELETE_MESSAGE_EVERYWHERE]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[DELETE_MESSAGE_EVERYWHERE]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[DELETE_MESSAGE_EVERYWHERE],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[DELETE_MESSAGE_EVERYWHERE]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*отправляем размер id сообщения*/
	sz=htonl(get_str_size(data->id));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of message id(%d)\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[DELETE_MESSAGE_EVERYWHERE]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем id сообщения*/
	if( !(ret=sendall(sock,data->id,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL message id(%d)\n",kakao_signal[DELETE_MESSAGE_EVERYWHERE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[DELETE_MESSAGE_EVERYWHERE]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	hide_change_message();
	show_messages(NULL,data);
}
void edit_message(GtkWidget *widget, sig_data *data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[EDIT_MESSAGE]); fflush(stderr);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[EDIT_MESSAGE]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[EDIT_MESSAGE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[EDIT_MESSAGE]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[EDIT_MESSAGE],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[EDIT_MESSAGE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[EDIT_MESSAGE]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем размер id сообщения*/
	sz=htonl(get_str_size(data->id));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of message id(%d)\n",kakao_signal[EDIT_MESSAGE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[EDIT_MESSAGE]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем id сообщения*/
	if( !(ret=sendall(sock,data->id,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL message id(%d)\n",kakao_signal[EDIT_MESSAGE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[EDIT_MESSAGE]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	GtkTextBuffer *edit_buf=gtk_text_view_get_buffer(GTK_TEXT_VIEW(change_message.TextView));;
	GtkTextIter start, end;//итераторы буфера
	gtk_text_buffer_get_start_iter(edit_buf, &start);// Получаем начальную позицию в буфере 
	gtk_text_buffer_get_end_iter(edit_buf, &end);// Получаем конечную позицию в буфере 
	gchar *text = gtk_text_iter_get_text (&start, &end);//берем текст из буфера
	/*отправляем размер сообщение*/
	sz=htonl(get_str_size(text));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of message(%d)\n",kakao_signal[EDIT_MESSAGE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[EDIT_MESSAGE]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сообщение*/
	if( !(ret=sendall(sock,text,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL message(%d)\n",kakao_signal[EDIT_MESSAGE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[EDIT_MESSAGE]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	hide_change_message();
	gtk_label_set_text(GTK_LABEL(data->label),text);//меняем текст метки в диалоговом окне
}
void show_message(sig_data *data)
{
	GtkWidget *box=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
	GtkWidget *box2=gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
	GtkWidget *button2;
	GtkWidget *button3;
	GtkWidget *image_button;
	GtkWidget *image;
	GtkWidget *scrolled;

	GtkWidget *grid=gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid),5);
	gtk_grid_set_column_spacing(GTK_GRID(grid),10);

	if(data->mes_type==BOB_FILE)//если сообщение содержит файл
	{
		char buf[300];
		sprintf(buf,"Скачать %s",data->text);
		data->label=gtk_label_new(buf);
		gtk_label_set_line_wrap(GTK_LABEL(data->label),TRUE);//разрешаем перенос
		gtk_label_set_line_wrap_mode(GTK_LABEL(data->label),PANGO_WRAP_WORD_CHAR);//включаем перенос символа слова

		button2=gtk_button_new();

		sprintf(buf,"%s MB",data->file_size);
		GtkWidget *label=gtk_label_new(buf);//имя и размер файла
		gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);//разрешаем перенос
		gtk_label_set_line_wrap_mode(GTK_LABEL(label),PANGO_WRAP_WORD_CHAR);//включаем перенос символа слова
		PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов

		PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_ULTRAHEAVY);//настраиваем толщину метки
		pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
		gtk_label_set_attributes (GTK_LABEL(label),attr_of_label);//устанавливаем атрибуты для метки 
		pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов
		g_signal_connect(G_OBJECT(button2),"clicked",G_CALLBACK(download),data);

		GtkWidget *box2=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
		gtk_container_add(GTK_CONTAINER(button2),box2);
		gtk_container_add(GTK_CONTAINER(box2),data->label);
		gtk_container_add(GTK_CONTAINER(box2),label);
	}
	else if(data->mes_type==PHOTO)
	{
		char buf[300];
		button2=gtk_button_new();
		gtk_button_set_relief(GTK_BUTTON(button2),GTK_RELIEF_NONE);
		gtk_widget_set_focus_on_click(button2,FALSE);

		gint width;
		gint height;
		gtk_window_get_size (GTK_WINDOW (main_window.Window), &width, &height);
		GdkPixbuf *pixbuf=gdk_pixbuf_new_from_file_at_size (data->path,width/3,height/3,NULL);
		image=gtk_image_new_from_pixbuf (pixbuf);
		

		gtk_container_add(GTK_CONTAINER(button2),image);
		gtk_container_add(GTK_CONTAINER(box2),button2);

		GtkWidget *revealer=gtk_revealer_new();
		button3=gtk_button_new();
		gtk_button_set_relief(GTK_BUTTON(button3),GTK_RELIEF_NONE);
		gtk_widget_set_focus_on_click(button3,FALSE);

		GtkWidget *box=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);

		sprintf(buf,"Скачать %s",data->text);
		GtkWidget *label=gtk_label_new(buf);
		sprintf(buf,"%s MB",data->file_size);
		GtkWidget *label2=gtk_label_new(buf);
		gtk_label_set_line_wrap(GTK_LABEL(label),TRUE);//разрешаем перенос
		gtk_label_set_line_wrap_mode(GTK_LABEL(label),PANGO_WRAP_WORD_CHAR);//включаем перенос символа слова

		PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов

		PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_ULTRAHEAVY);//настраиваем толщину метки
		pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
		gtk_label_set_attributes (GTK_LABEL(label2),attr_of_label);//устанавливаем атрибуты для метки 
		pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов
		gtk_container_add(GTK_CONTAINER(button3),box);
		gtk_container_add(GTK_CONTAINER(box),label);
		gtk_container_add(GTK_CONTAINER(box),label2);

		gtk_container_add(GTK_CONTAINER(revealer),button3);
		gtk_container_add(GTK_CONTAINER(box2),revealer);
		g_signal_connect(G_OBJECT(button3),"clicked",G_CALLBACK(download),data);
		data->widget1=revealer;
		data->flag_photo_revealer=false;
		g_signal_connect(G_OBJECT(button2),"enter",G_CALLBACK(photo_revealer),data);
		g_signal_connect(G_OBJECT(button3),"leave",G_CALLBACK(photo_revealer),data);
		g_signal_connect(G_OBJECT(button2),"clicked",G_CALLBACK(show_photo_full),data);
	}
	else//если сообщение содержит только текст
	{	
		data->label=gtk_label_new(data->text);
		gtk_label_set_line_wrap(GTK_LABEL(data->label),TRUE);//разрешаем перенос
		gtk_label_set_line_wrap_mode(GTK_LABEL(data->label),PANGO_WRAP_WORD_CHAR);//включаем перенос символа слова
		gtk_label_set_selectable (GTK_LABEL(data->label),TRUE);//делаем метку выделяемой
	}	

	GtkWidget *label2=gtk_label_new(data->created_at);//когда было отправлено сообщение
	PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов

	PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_ULTRAHEAVY);//настраиваем толщину метки
	pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
	gtk_label_set_attributes (GTK_LABEL(label2),attr_of_label);//устанавливаем атрибуты для метки 
	pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов

	if(data->hpos==LEFT)
	{
		if(strcmp(data->dialog_type,text_dialog_type[PARTY])==0)
		{
			sprintf(buf,"%s: ",data->nickname);
			GtkWidget *label3=gtk_label_new(buf);//никнейм отправителя
			gtk_widget_set_halign(GTK_WIDGET(label3),GTK_ALIGN_START);//ставим метку в начало
			PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов

			PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_ULTRAHEAVY);//настраиваем толщину метки
			pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
			gtk_label_set_attributes (GTK_LABEL(label3),attr_of_label);//устанавливаем атрибуты для метки 
			pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов

			gtk_grid_attach(GTK_GRID(grid),label3,0,0,1,1);
			if(data->mes_type==BOB_FILE) {//если сообщение содержит файл
				gtk_grid_attach(GTK_GRID(grid),button2,0,1,2,1);
				gtk_grid_attach(GTK_GRID(grid),label2,0,2,2,1);
				gtk_widget_set_hexpand(GTK_WIDGET(button2),TRUE);//включаем раскрытие по горизонтали
				gtk_widget_set_halign(GTK_WIDGET(button2),GTK_ALIGN_START);//выравниваем кнопку файла
			}
			else if(data->mes_type==PHOTO) {//если сообщение содержит фото
				gtk_grid_attach(GTK_GRID(grid),box2,0,1,2,1);
				gtk_grid_attach(GTK_GRID(grid),label2,0,2,2,1);
				gtk_widget_set_hexpand(GTK_WIDGET(button2),TRUE);//включаем раскрытие по горизонтали
				gtk_widget_set_halign(GTK_WIDGET(button2),GTK_ALIGN_START);//выравниваем кнопку изображения
				gtk_widget_set_hexpand(GTK_WIDGET(button3),TRUE);//включаем раскрытие по горизонтали
				gtk_widget_set_halign(GTK_WIDGET(button3),GTK_ALIGN_START);//выравниваем кнопку скачать фото
			}
			else {
				gtk_widget_set_halign(GTK_WIDGET(data->label),GTK_ALIGN_START);//ставим метку в начало
				gtk_grid_attach(GTK_GRID(grid),data->label,1,0,1,1);
				gtk_widget_set_hexpand(GTK_WIDGET(data->label),TRUE);//включаем раскрытие по горизонтали
				gtk_grid_attach(GTK_GRID(grid),label2,0,1,2,1);
			}

			gtk_widget_set_hexpand(GTK_WIDGET(label2),TRUE);//включаем раскрытие по горизонтали
			gtk_widget_set_halign(GTK_WIDGET(label2),GTK_ALIGN_START);//ставим метку в начало
		}
		else if(strcmp(data->dialog_type,text_dialog_type[COMPANION])==0)
		{
			//gtk_label_set_justify(GTK_LABEL(data->label),GTK_JUSTIFY_LEFT);//выравниваем метку
			//gtk_widget_set_halign(GTK_WIDGET(data->label),GTK_ALIGN_START);//ставим метку в начало
			if(data->mes_type==BOB_FILE) {//если сообщение содержит файл
				gtk_grid_attach(GTK_GRID(grid),button2,1,0,1,1);
				gtk_widget_set_hexpand(GTK_WIDGET(button2),TRUE);//включаем раскрытие по горизонтали
				gtk_widget_set_halign(GTK_WIDGET(button2),GTK_ALIGN_START);//выравниваем кнопку файла
			}
			else if(data->mes_type==PHOTO) {//если сообщение содержит фото
				gtk_grid_attach(GTK_GRID(grid),box2,1,0,1,1);
				gtk_widget_set_hexpand(GTK_WIDGET(button2),TRUE);//включаем раскрытие по горизонтали
				gtk_widget_set_halign(GTK_WIDGET(button2),GTK_ALIGN_START);//выравниваем кнопку изображения
				gtk_widget_set_hexpand(GTK_WIDGET(button3),TRUE);//включаем раскрытие по горизонтали
				gtk_widget_set_halign(GTK_WIDGET(button3),GTK_ALIGN_START);//выравниваем кнопку скачать фото
			}
			else {
				gtk_grid_attach(GTK_GRID(grid),data->label,1,0,1,1);
				gtk_widget_set_hexpand(GTK_WIDGET(data->label),TRUE);//включаем раскрытие по горизонтали
				gtk_widget_set_halign(GTK_WIDGET(data->label),GTK_ALIGN_START);//ставим метку в начало
			}

			gtk_grid_attach(GTK_GRID(grid),label2,0,1,2,1);
			gtk_widget_set_hexpand(GTK_WIDGET(label2),TRUE);//включаем раскрытие по горизонтали
			gtk_widget_set_halign(GTK_WIDGET(label2),GTK_ALIGN_START);//ставим метку в начало
		}

		gtk_grid_attach(GTK_GRID(dialog_window.Grid),grid,LEFT_MESSAGE,top_message,1,1);//вставляем сообщение в grid
	}
	else if(data->hpos==RIGHT)
	{
		//gtk_label_set_justify(GTK_LABEL(data->label),GTK_JUSTIFY_RIGHT);//выравниваем метку
		if(data->mes_type==BOB_FILE) {//если сообщение содержит файл
			gtk_grid_attach(GTK_GRID(grid),button2,0,0,2,1);
			gtk_widget_set_hexpand(GTK_WIDGET(button2),TRUE);//включаем раскрытие по горизонтали
			gtk_widget_set_halign(GTK_WIDGET(button2),GTK_ALIGN_END);//выравниваем кнопку файла
		}
		else if(data->mes_type==PHOTO) {//если сообщение содержит фото
			gtk_grid_attach(GTK_GRID(grid),box2,0,0,2,1);
			gtk_widget_set_hexpand(GTK_WIDGET(button2),TRUE);//включаем раскрытие по горизонтали
			gtk_widget_set_halign(GTK_WIDGET(button2),GTK_ALIGN_END);//выравниваем кнопку изображения
			gtk_widget_set_hexpand(GTK_WIDGET(button3),TRUE);//включаем раскрытие по горизонтали
			gtk_widget_set_halign(GTK_WIDGET(button3),GTK_ALIGN_END);//выравниваем кнопку скачать фото
		}
		else {
			gtk_grid_attach(GTK_GRID(grid),data->label,0,0,2,1);
			gtk_widget_set_halign(GTK_WIDGET(data->label),GTK_ALIGN_END);//ставим метку в конец
			gtk_widget_set_hexpand(GTK_WIDGET(data->label),TRUE);//включаем раскрытие по горизонтали
		}
		GtkWidget *button=gtk_button_new();
		gtk_button_set_relief(GTK_BUTTON(button),GTK_RELIEF_NONE);
		gtk_widget_set_focus_on_click(button,FALSE);
		GtkWidget *image=gtk_image_new();
		gtk_image_set_from_icon_name(GTK_IMAGE(image),"gtk-properties",GTK_ICON_SIZE_SMALL_TOOLBAR);
		gtk_container_add(GTK_CONTAINER(button),image);

		gtk_grid_attach(GTK_GRID(grid),button,2,0,1,2);

		g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(show_change_message),data);

		gtk_grid_attach(GTK_GRID(grid),label2,0,1,1,1);
		gtk_widget_set_hexpand(GTK_WIDGET(label2),TRUE);//включаем раскрытие по горизонтали
		gtk_widget_set_halign(GTK_WIDGET(label2),GTK_ALIGN_END);//ставим метку в конец

		if(data->saw) {//если сообщение прочитано
			GtkWidget *image2=gtk_image_new();//иконка показывающая что сообщение прочитано
			gtk_image_set_from_icon_name(GTK_IMAGE(image2),"gtk-apply",GTK_ICON_SIZE_SMALL_TOOLBAR);
			gtk_grid_attach(GTK_GRID(grid),image2,1,1,1,1);
		}

		gtk_grid_attach(GTK_GRID(dialog_window.Grid),grid,RIGHT_MESSAGE,top_message,1,1);//вставляем сообщение в grid
		
	}
	else if(data->hpos==CENTER)
	{
		gtk_widget_set_halign(GTK_WIDGET(data->label),GTK_ALIGN_CENTER);//ставим метку по центру
		PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов

		PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_ULTRAHEAVY);//настраиваем толщину метки
		pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
		gtk_label_set_attributes (GTK_LABEL(data->label),attr_of_label);//устанавливаем атрибуты для метки 
		pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов

		if(data->mes_type==BOB_FILE) {//если сообщение содержит файл
			gtk_grid_attach(GTK_GRID(grid),button2,0,0,1,1);
			gtk_widget_set_hexpand(GTK_WIDGET(button2),TRUE);//включаем раскрытие по горизонтали
			gtk_widget_set_halign(GTK_WIDGET(button2),GTK_ALIGN_CENTER);//выравниваем кнопку файла
		}
		else if(data->mes_type==PHOTO) {//если сообщение содержит фото
			gtk_grid_attach(GTK_GRID(grid),box2,0,0,1,1);
			gtk_widget_set_hexpand(GTK_WIDGET(button2),TRUE);//включаем раскрытие по горизонтали
			gtk_widget_set_halign(GTK_WIDGET(button2),GTK_ALIGN_CENTER);//выравниваем кнопку изображения
			gtk_widget_set_hexpand(GTK_WIDGET(button3),TRUE);//включаем раскрытие по горизонтали
			gtk_widget_set_halign(GTK_WIDGET(button3),GTK_ALIGN_CENTER);//выравниваем кнопку скачать фото
		}
		else {
			gtk_grid_attach(GTK_GRID(grid),data->label,0,0,1,1);
			gtk_widget_set_hexpand(GTK_WIDGET(data->label),TRUE);//включаем раскрытие по горизонтали
		}

		gtk_grid_attach(GTK_GRID(grid),label2,0,1,1,1);
		gtk_widget_set_hexpand(GTK_WIDGET(label2),TRUE);//включаем раскрытие по горизонтали
		gtk_widget_set_halign(GTK_WIDGET(label2),GTK_ALIGN_CENTER);//ставим метку по центру

		gtk_grid_attach(GTK_GRID(dialog_window.Grid),grid,LEFT_MESSAGE,top_message,2,1);//вставляем сообщение в grid
	}

	gtk_widget_show_all(grid);
	top_message++;
	show_window(dialog_window.Grid);//обновляем окно сообщений
}
void photo_revealer(GtkWidget *widget, sig_data *data)
{
	if(data->flag_photo_revealer) {
		data->flag_photo_revealer=!data->flag_photo_revealer;
		gtk_revealer_set_reveal_child(GTK_REVEALER(data->widget1),FALSE);
	}
	else {
		data->flag_photo_revealer=!data->flag_photo_revealer;
		gtk_revealer_set_reveal_child(GTK_REVEALER(data->widget1),TRUE);
	}
}
void show_photo_full(GtkWidget *widget, sig_data *data)
{
	fprintf(stderr,"SIGNAL: SHOW_PHOTO_FULL\n"); fflush(stderr);
	GtkWidget *window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window),workarea.width,workarea.height);
	GtkWidget *scrolled=gtk_scrolled_window_new(NULL,NULL);

	gtk_container_add(GTK_CONTAINER(window),scrolled);

	GdkPixbuf *pixbuf=gdk_pixbuf_new_from_file_at_size (data->path,workarea.width,workarea.height,NULL);
	printf("workarea.width %d workarea.height %d\n",workarea.width,workarea.height );
	GtkWidget *image=gtk_image_new_from_pixbuf (pixbuf);

	gtk_container_add(GTK_CONTAINER(scrolled),image);
	gtk_widget_show_all(window);

}
void download(GtkWidget *widget, sig_data *data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[DOWNLOAD]); fflush(stderr);
	char path[MAX_PATH];
	char filename[MAX_FILENAME];
	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
	gint res;

	dialog = gtk_file_chooser_dialog_new ("Выбрать каталог",NULL,action,"Отменить",GTK_RESPONSE_CANCEL,"Выбрать",GTK_RESPONSE_ACCEPT,NULL);
	//g_signal_connect(G_OBJECT(dialog),"response",G_CALLBACK(download),data);
	res = gtk_dialog_run (GTK_DIALOG (dialog));
	if (res == GTK_RESPONSE_ACCEPT)
	{
		GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
		char *file_path=gtk_file_chooser_get_filename (chooser);
		/*отправляем размер сигнала*/
		sz=htonl(get_str_size(kakao_signal[DOWNLOAD]));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[DOWNLOAD]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем сигнал*/
		if( !(ret=sendall(sock,kakao_signal[DOWNLOAD],ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[DOWNLOAD]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем размер типа диалога*/
		sz=htonl(get_str_size(data->dialog_type));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of type of dialog(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[DOWNLOAD]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем тип диалога*/
		if( !sendall(sock,data->dialog_type,ntohl(sz)) ) {
			fprintf(stderr, "ERROR %s : SENDALL type of dialog(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[DOWNLOAD]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*отправляем размер id сообщения*/
		sz=htonl(get_str_size(data->id));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of message id(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[DOWNLOAD]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем id сообщения*/
		if( !(ret=sendall(sock,data->id,ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL message id(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[DOWNLOAD]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*отправляем тип сообщения*/
		sz=htonl(data->mes_type);
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND message type(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[DOWNLOAD]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*принимаем размер имени файла или фото*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of name of file or photo(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[DOWNLOAD]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем имя файла*/
		if( (ret=recvall(sock,filename,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL name of file or photo(%d)\n",kakao_signal[DOWNLOAD],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[DOWNLOAD]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		sprintf(path,"%s/%s",file_path,filename);
		recv_file(sock,path,NULL);
		g_free (file_path);
		gtk_widget_destroy(dialog);
	}
	else if(res == GTK_RESPONSE_CANCEL)
	{
		gtk_widget_destroy(dialog);
	}
}
void show_change_message(GtkWidget *widget, sig_data *data)
{
	fprintf(stderr,"SIGNAL: SHOW_CHANGE_MESSAGE\n"); fflush(stderr);
	gtk_window_resize(GTK_WINDOW(change_message.Window),workarea.width/4,workarea.height/4);//устанавливаем размер окна
	if(data->hpos==RIGHT){//если от меня отправлено сообщение
		gtk_widget_show(change_message.Button);
	}
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(change_message.Button2),G_SIGNAL_MATCH_FUNC,0,0,NULL,show_change_message_textview,0); while(ret!=0);//удаляем сигнал кнопки редактировать
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(change_message.Button),G_SIGNAL_MATCH_FUNC,0,0,NULL,delete_message_everywhere,0); while(ret!=0);//удаляем сигнал кнопки удалить везде
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(change_message.Button3),G_SIGNAL_MATCH_FUNC,0,0,NULL,edit_message,0); while(ret!=0);//удаляем сигнал кнопки изменить сообщение
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(change_message.TextView),G_SIGNAL_MATCH_FUNC,0,0,NULL,edit_event_return,0); while(ret!=0);//удаляем сигнал текстового поля
	g_signal_connect(G_OBJECT(change_message.Button),"clicked",G_CALLBACK(delete_message_everywhere),data);//назначаем сигнал кнопке удалить везде
	gtk_widget_show_all(change_message.Window);
	if(data->mes_type==BOB_FILE) {//если сообщение содержит файл
		gtk_widget_hide(change_message.Button2);//скрываем кнопку редактировать
		gtk_widget_hide(change_message.Box);
	}
	else if(data->mes_type==PHOTO) {//если сообщение содержит фото
		gtk_widget_hide(change_message.Button2);//скрываем кнопку редактировать
		gtk_widget_hide(change_message.Box);
	}
	else {
		g_signal_connect(G_OBJECT(change_message.Button2),"clicked",G_CALLBACK(show_change_message_textview),data);//назначаем сигнал кнопке редактировать
		g_signal_connect(G_OBJECT(change_message.Button3),"clicked",G_CALLBACK(edit_message),data);//назначаем сигнал кнопке изменить сообщение
		g_signal_connect(G_OBJECT(change_message.TextView),"key-press-event",G_CALLBACK(edit_event_return),data);//подключаем сигнал к текстовому полю 
	}
}
void hide_change_message()
{
	fprintf(stderr,"SIGNAL: HIDE_CHANGE_MESSAGE\n"); fflush(stderr);
	gtk_widget_hide(change_message.Window);
	gtk_widget_hide(change_message.Box);
}
void show_change_message_textview(GtkWidget *widget, sig_data *data)
{	
	fprintf(stderr,"SIGNAL: SHOW_CHANGE_MESSAGE_TEXTVIEW\n"); fflush(stderr);
	gtk_window_resize(GTK_WINDOW(change_message.Window),workarea.width/2,workarea.height/2);//устанавливаем размер окна

	const gchar *text = gtk_label_get_text(GTK_LABEL(data->label));//берем текст из диалогового окна
	gtk_text_buffer_set_text (gtk_text_view_get_buffer(GTK_TEXT_VIEW(change_message.TextView)),text,-1);//вставляем текст в textview для редактирования текста
	gtk_widget_show(change_message.Box);
}
void send_message(GtkWidget *widget, sig_data *data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[SEND_MESSAGE]); fflush(stderr);
	GtkTextIter start, end;//итераторы буфера
	gtk_text_buffer_get_start_iter(dialog_buffer, &start);// Получаем начальную позицию в буфере
	gtk_text_buffer_get_end_iter(dialog_buffer, &end);// Получаем конечную позицию в буфере
	gchar *text = gtk_text_iter_get_text (&start, &end);
	//gchar *text = gtk_text_buffer_get_text (buffer,&start, &end,FALSE);// Получаем написанное пользователем сообщение
	if(strlen(text))//если в буфере есть текст(если он не пуст)
	{
		/*отправляем размер сигнала*/
		sz=htonl(get_str_size(kakao_signal[SEND_MESSAGE]));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SEND_MESSAGE]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем сигнал*/
		if( !(ret=sendall(sock,kakao_signal[SEND_MESSAGE],ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SEND_MESSAGE]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*отправляем размер типа диалога*/
		sz=htonl(get_str_size(data->dialog_type));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of type dialog(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SEND_MESSAGE]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем тип диалога*/
		if( !(ret=sendall(sock,data->dialog_type,ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL type(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SEND_MESSAGE]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*отправляем размер сообщения*/
		sz=htonl(get_str_size(text));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of message(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SEND_MESSAGE]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем сообщение*/
		if( !(ret=sendall(sock,text,ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL message(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SEND_MESSAGE]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		if(strcmp(data->dialog_type,text_dialog_type[COMPANION])==0) 
		{
			/*отправляем размер id пользователя*/
			sz=htonl(get_str_size(data->to_id));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(stderr, "ERROR %s : SEND size of people id(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[SEND_MESSAGE]);
				show_error_window(buf,NOTCLOSE); 
				return;//выходим из функции, вынуждая пользователя заново отправить сигнал
			}
			/*отправляем id пользователя*/
			if( !(ret=sendall(sock,data->to_id,ntohl(sz))) ) {
				fprintf(stderr, "ERROR %s : SENDALL people id(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[SEND_MESSAGE]);
				show_error_window(buf,NOTCLOSE); 
				return;//выходим из функции, вынуждая пользователя заново отправить сигнал
			}
			/*принимаем ответ(можно ли отправлять данные)*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
				show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
			}
			else if(ret<0) {
				fprintf(stderr, "ERROR %s : RECV answer(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[SEND_MESSAGE]);
				show_error_window(buf,NOTCLOSE);
				return;
			}
			//если ответ отрицательный
			if(!ntohl(sz)) return;
		}
		else if(strcmp(data->dialog_type,text_dialog_type[PARTY])==0) 
		{
			/*отправляем размер id группы*/
			sz=htonl(get_str_size(data->party_id));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(stderr, "ERROR %s : SEND size of party id(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[SEND_MESSAGE]);
				show_error_window(buf,NOTCLOSE); 
				return;//выходим из функции, вынуждая пользователя заново отправить сигнал
			}
			/*отправляем id группы*/
			if( !(ret=sendall(sock,data->party_id,ntohl(sz))) ) {
				fprintf(stderr, "ERROR %s : SENDALL party id(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[SEND_MESSAGE]);
				show_error_window(buf,NOTCLOSE); 
				return;//выходим из функции, вынуждая пользователя заново отправить сигнал
			}
			/*принимаем ответ(можно ли отправлять данные)*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
				show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
			}
			else if(ret<0) {
				fprintf(stderr, "ERROR %s : RECV answer(%d)\n",kakao_signal[SEND_MESSAGE],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[SEND_MESSAGE]);
				show_error_window(buf,NOTCLOSE);
				return;
			}
			//если ответ отрицательный
			if(!ntohl(sz)) return;
		}

		fprintf(stderr,"DATA: %s sizeof: %d\n",data->to_id,get_str_size(data->to_id) );
		gtk_text_buffer_set_text (dialog_buffer,"",-1);// Удаляем текст из буфера
		data->text=text;
		data->hpos=RIGHT;
		data->mes_type=TEXT;
		data->saw=false;
		strcpy(data->created_at,"Сейчас");
		show_message(data);//выводим сообщение
		//gtk_adjustment_set_value(GTK_ADJUSTMENT(dialog_window.Adjustment),gtk_adjustment_get_upper (GTK_ADJUSTMENT(dialog_window.Adjustment)));//пролистываем окно в конец
		//show_messages(widget,data);

	}
}

void show_messages(GtkWidget *widget, sig_data* data)
{
	free_sig_data(node_change_message);
	node_change_message=NULL;
	MYSQL_RES res3;
	top_message=0;
	unsigned long int i;
	short int ret;
	char limit[MYSQL_INT];
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[SHOW_MESSAGES]); fflush(stderr);
	clean_window(dialog_window.Grid);//очищаем окно от сообщений
	sprintf(limit,"%ld",limits[LIMIT_DIALOG]);//переводим число в строку

	if(strcmp(data->dialog_type,text_dialog_type[COMPANION])==0) 
	{
		/*отправляем размер сигнала*/
		sz=htonl(get_str_size(kakao_signal[SHOW_MESSAGES]));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем сигнал*/
		if( !(ret=sendall(sock,kakao_signal[SHOW_MESSAGES],ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем размер типа диалога*/
		sz=htonl(get_str_size(data->dialog_type));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of type of dialog(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем тип диалога*/
		if( !sendall(sock,data->dialog_type,ntohl(sz)) ) {
			fprintf(stderr, "ERROR %s : SENDALL type of dialog(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		sz=htonl(get_str_size(data->to_id));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {//отправляем размер id собеседника
			fprintf(stderr, "ERROR %s : SEND size of to_id(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		if( !sendall(sock,data->to_id,ntohl(sz)) ) {//отправляем id собеседника
			fprintf(stderr, "ERROR %s : SENDALL to_id(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}


	}
	else if(strcmp(data->dialog_type,text_dialog_type[PARTY])==0) 
	{
		/*отправляем размер сигнала*/
		sz=htonl(get_str_size(kakao_signal[SHOW_MESSAGES]));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем сигнал*/
		if( !(ret=sendall(sock,kakao_signal[SHOW_MESSAGES],ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		sz=htonl(get_str_size(data->dialog_type));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {//отправляем размер типа диалога
			fprintf(stderr, "ERROR %s : SEND size of type of dialog(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		if( !sendall(sock,data->dialog_type,ntohl(sz)) ) {//отправляем тип диалога
			fprintf(stderr, "ERROR %s : SENDALL type of dialog(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		sz=htonl(get_str_size(data->party_id));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {//отправляем размер party_id
			fprintf(stderr, "ERROR %s : SEND size of party_id(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		if( !sendall(sock,data->party_id,ntohl(sz)) ) {//отправляем party_id
			fprintf(stderr, "ERROR %s : SENDALL party_id(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
	}
	else {
		flag_dialog=FALSE;
		return;
	}

	sz=htonl(get_str_size(limit));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {//отправляем размер limit
		fprintf(stderr, "ERROR %s : SEND size of limit(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	if( !sendall(sock,limit,ntohl(sz)) ) {//отправляем limit 
		fprintf(stderr, "ERROR %s : SENDALL limit(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	while( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL)) )//принимаем размер id сообщения
	{
		if(!ntohl(sz)) break;//если конец сообщений, то выходим из цикла

		sig_data *data2=(sig_data*)malloc(sizeof(sig_data));
		//создаем список который хранит указатели на структуры чтобы освобождать память
		insert_to_list_sig_data(data2,&node_change_message);//вставляем в список структуру типа sig_data(чтобы потом освободить)

		if( (ret=recvall(sock,data2->id,ntohl(sz),MSG_WAITALL))==0 ) {//принимаем id сообщения
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL id of message(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*принимаем размер когда сообщение отправлено*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of created_at message(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем когда сообщение отправлено*/
		if( (ret=recvall(sock,data2->created_at,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL created_at message(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*принимаем тип сообщения*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV mes type?%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		switch ( ntohl(sz) ) {
	        case TEXT:  
	        data2->mes_type=TEXT;
			    if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {//принимаем размер текста
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECV size of text message(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}

				if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {//принимаем текст
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECVALL  message(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}  
				data2->text=buf;
	            break;
	        case PHOTO:
	      	    data2->mes_type=PHOTO;
				char filename[MAX_FILENAME];
	            if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {//принимаем размер имени фото
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECV size of name of photo(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}

				if( (ret=recvall(sock,filename,ntohl(sz),MSG_WAITALL))==0 ) {//принимаем имя фото
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECVALL  name of photo(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}  
				data2->text=filename;
	            /*принимаем размер размера файла*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECV size of size of photo(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}
				/*принимаем размер файла*/
				if( (ret=recvall(sock,data2->file_size,ntohl(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECVALL size of photo(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
					show_error_window(buf,NOTCLOSE); 
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}
				char buf[(MAX_PATH*2)+256];

				snprintf(data2->path,sizeof(data2->path),"%s/%s",PATHS[PATH_DIALOG_PHOTOS],data2->id);//создаем путь к дирректории с id сообщения
				sprintf(buf,"if ! [ -d \"%s\" ]; then mkdir -p %s; fi",data2->path,data2->path);//создаем команду для создания дирректории
				system(buf);//создаем каталог если это нужно

				sprintf(data2->path,"%s/%s/%s",PATHS[PATH_DIALOG_PHOTOS],data2->id,filename);//путь до фото
				FILE *p;
				if((p=fopen(data2->path,"r"))==NULL) {//если у нас нет этого фото
					/*отправляем положительный ответ*/
					sz=htonl(1);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(stderr, "ERROR %s : SEND answer +(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
						sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
						show_error_window(buf,NOTCLOSE); 
						return;//выходим из функции, вынуждая пользователя заново отправить сигнал
					}
					recv_file(sock,data2->path,NULL);//принимаем фото
				}
				else {//если у нас есть это фото, то мы его не принимаем
					fclose(p);
					/*отправляем отрицательный ответ*/
					sz=htonl(0);
					if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
						fprintf(stderr, "ERROR %s : SEND answer -(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
						sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
						show_error_window(buf,NOTCLOSE); 
						return;//выходим из функции, вынуждая пользователя заново отправить сигнал
					}
				}
	            break;
	        case BOB_FILE:
	            data2->mes_type=BOB_FILE;
	            if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {//принимаем размер имени файла
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECV size of filename(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}

				if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {//принимаем имя файла
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECVALL  filename(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}  
				data2->text=buf;
	            /*принимаем размер размера файла*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECV size of size of file(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}
				/*принимаем размер файла*/
				if( (ret=recvall(sock,data2->file_size,ntohl(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECVALL size of file(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
					show_error_window(buf,NOTCLOSE); 
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}
		        break;
		        default: return;
    	}
    	/*принимаем размер положения сообщения*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of position of message(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем положение сообщения*/
		if( (ret=recvall(sock,type,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL position of message(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		
		strcpy(data2->dialog_type,data->dialog_type);
		strcpy(data2->to_id,data->to_id);
		strcpy(data2->party_id,data->party_id);

		if(strcmp(type,text_mes_hpos[RIGHT])==0)//если сообщение отправлено от меня
		{
			data2->hpos=RIGHT;

			/*принимаем прочитано ли сообщение*/
			if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
				show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
			}
			else if(ret<0) {
				fprintf(stderr, "ERROR %s : RECV saw message(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
				show_error_window(buf,NOTCLOSE);
				return;//выходим из функции, вынуждая пользователя заново отправить сигнал
			}
			data2->saw=ntohl(sz);
			show_message(data2);//выводим сообщение
		}
		/*иначе если сообщение отправлено от собеседника*/
		else if(strcmp(type,text_mes_hpos[LEFT])==0)
		{
			if(strcmp(data->dialog_type,text_dialog_type[PARTY])==0) 
			{
				/*принимаем размер никнейма отправителя*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECV size of sender nickname(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}
				/*принимаем никнейм отправителя*/
				if( (ret=recvall(sock,data2->nickname,ntohl(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECVALL sender nickname(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
					show_error_window(buf,NOTCLOSE); 
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}
			}
			data2->hpos=LEFT;
			show_message(data2);//выводим сообщение
		}
		/*иначе если сообщение важное*/
		else if(strcmp(type,text_mes_hpos[CENTER])==0)
		{
			data2->hpos=CENTER;
			show_message(data2);//выводим сообщение
		}
		else {
			fprintf(stderr, "ERROR: unknow position of message%s\n",kakao_error[SHOW_MESSAGES] ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
			show_error_window(buf,NOTCLOSE);
			continue;
		}
	}
	if(ret==0){
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0){
		fprintf(stderr, "ERROR %s : RECV size of id of message(%d)\n",kakao_signal[SHOW_MESSAGES],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
}
void get_reg_form()
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[GET_REG_FORM]);
	strcpy(my_info.nickname,gtk_entry_get_text(GTK_ENTRY(reg_window.Nickname)));// Получаем текст из регистрационного поля(никнейм)

	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[GET_REG_FORM]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[GET_REG_FORM],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[GET_REG_FORM]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[GET_REG_FORM],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[GET_REG_FORM],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[GET_REG_FORM]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}


	/*отправляем размер никнейма*/
	sz=htonl(get_str_size(my_info.nickname));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of nickname(%d)\n",kakao_signal[GET_REG_FORM],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[GET_REG_FORM]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем никнейм*/
	if( !(ret=sendall(sock,my_info.nickname,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL nickname(%d)\n",kakao_signal[GET_REG_FORM],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[GET_REG_FORM]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*принимаем размер ответа уникальности никнейма*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV size of unique nickname answer no\"n\"(%d)\n",kakao_signal[GET_REG_FORM],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[GET_REG_FORM]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*принимаем ответ уникальности никнейма*/
	if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECVALL unique nickname answer no\"n\"(%d)\n",kakao_signal[GET_REG_FORM],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[GET_REG_FORM]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	if(strcmp(buf,"n")==0){//если никнейм не уникален
		// выводим подсказку
		gtk_revealer_set_reveal_child(GTK_REVEALER(reg_window.Revealer1),TRUE);
		gtk_revealer_set_reveal_child(GTK_REVEALER(reg_window.Revealer2),FALSE);
		return;// Прекращаем выполнение функции
	}
	//Если никнейм уникален ->
	strcpy(my_info.password,gtk_entry_get_text(GTK_ENTRY(reg_window.Password)));// Получаем текст из регистрационного поля(пароль)

	/*отправляем размер пароля*/
	sz=htonl(get_str_size(my_info.password));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of nickname(%d)\n",kakao_signal[GET_REG_FORM],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[GET_REG_FORM]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем пароль*/
	if( !(ret=sendall(sock,my_info.password,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL nickname(%d)\n",kakao_signal[GET_REG_FORM],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[GET_REG_FORM]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	//fprintf(stderr, "%s\n%s", my_info.nickname,my_info.password); fflush(stderr);
	ini_puts("autolog", "login",my_info.nickname, PATHS[PATH_AUTOLOG]);//записываем логин(никнейм)
	ini_puts("autolog", "password",my_info.password, PATHS[PATH_AUTOLOG]);//записываем пароль

	gtk_widget_hide(reg_window.Window);//закрываем окно регистрации,также эта функция затирает адреса на которые указывают nickname и password(все ссылки reg_window)
	gtk_widget_hide(log_in_window.Window);//закрываем окно входа,также эта функция затирает переменные nickname и password(все ссылки reg_window)
	/*принимаем размер моего id*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV size of my id(%d)\n",kakao_signal[GET_REG_FORM],ret ); fflush(stderr);
		show_error_window(kakao_error[DARK_ERROR],NOTCLOSE); //закрываем программу
	}
	/*принимаем моЙ id*/
	if( (ret=recvall(sock,my_info.id,ntohl(sz),MSG_WAITALL))==0 ) {//принимаем ответ
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECVALL my id(%d)\n",kakao_signal[GET_REG_FORM],ret ); fflush(stderr);
		show_error_window(kakao_error[DARK_ERROR],NOTCLOSE); //закрываем программу
	}

	gtk_widget_show_all(main_window.Window);//показываем главное окно
	show_people_list();
	show_friend_list();
	show_messages_list(NULL,NULL);
	pthread_create(&var_thread_messages_list,NULL,update_messages_list,NULL);//создаем поток в котором будет обновляться страница messages_list
	pthread_create(&var_thread_check_errors,NULL,check_errors,NULL);//создаем поток в котором будут проверяться ошибки

}
void get_log_form()
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[GET_LOG_FORM]);
	strcpy(my_info.nickname,gtk_entry_get_text(GTK_ENTRY(log_in_window.Nickname)));//получаем текст из поля
	strcpy(my_info.password,gtk_entry_get_text(GTK_ENTRY(log_in_window.Password)));//получаем текст из поля
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[GET_LOG_FORM]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[GET_LOG_FORM],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[GET_LOG_FORM]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[GET_LOG_FORM],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[GET_LOG_FORM],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[GET_LOG_FORM]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*отправляем размер никнейма*/
	sz=htonl(get_str_size(my_info.nickname));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of nickname(%d)\n",kakao_signal[GET_LOG_FORM],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[GET_LOG_FORM]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем никнейм*/
	if( !(ret=sendall(sock,my_info.nickname,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL nickname(%d)\n",kakao_signal[GET_LOG_FORM],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[GET_LOG_FORM]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*отправляем размер пароля*/
	sz=htonl(get_str_size(my_info.password));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of nickname(%d)\n",kakao_signal[GET_LOG_FORM],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[GET_LOG_FORM]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем пароль*/
	if( !(ret=sendall(sock,my_info.password,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL nickname(%d)\n",kakao_signal[GET_LOG_FORM],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[GET_LOG_FORM]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*принимаем размер ответа уникальности никнейма*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV size of unique nickname answer no\"n\"(%d)\n",kakao_signal[GET_LOG_FORM],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[GET_LOG_FORM]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*принимаем ответ уникальности никнейма*/
	if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECVALL unique nickname answer no\"n\"(%d)\n",kakao_signal[GET_LOG_FORM],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[GET_LOG_FORM]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	if(strcmp(buf,"y")==0)//если ответ положительный(данные правильные)
	{
		ini_puts("autolog", "login",my_info.nickname, PATHS[PATH_AUTOLOG]);//записываем логин(никнейм)
		ini_puts("autolog", "password",my_info.password, PATHS[PATH_AUTOLOG]);//записываем пароль
		gtk_widget_hide(reg_window.Window);//скрываем окно регистрации
		gtk_widget_hide(log_in_window.Window);//скрываем окно авторизации
		/*принимаем размер моего id*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of my id(%d)\n",kakao_signal[GET_LOG_FORM],ret ); fflush(stderr);
			show_error_window(kakao_error[DARK_ERROR],NOTCLOSE); //закрываем программу
		}
		/*принимаем моЙ id*/
		if( (ret=recvall(sock,my_info.id,ntohl(sz),MSG_WAITALL))==0 ) {//принимаем ответ
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL my id(%d)\n",kakao_signal[GET_LOG_FORM],ret ); fflush(stderr);
			show_error_window(kakao_error[DARK_ERROR],NOTCLOSE); //закрываем программу
		}

		gtk_widget_show_all(main_window.Window);//показываем главное окно
		show_people_list();
		show_friend_list();
		show_messages_list(NULL,NULL);
		pthread_create(&var_thread_messages_list,NULL,update_messages_list,NULL);//создаем поток в котором будет обновляться страница messages_list
		pthread_create(&var_thread_check_errors,NULL,check_errors,NULL);//создаем поток в котором будут проверяться ошибки
	}
	else//если ответ отрицательный
	{
		gtk_revealer_set_reveal_child(GTK_REVEALER(log_in_window.Revealer2),FALSE);
		gtk_revealer_set_reveal_child(GTK_REVEALER(log_in_window.Revealer1),TRUE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	//show_messages();//показываем все сообщения которые были отправлены пользователем
	//gtk_widget_hide();// Скрываем диалоговое окно
}
void from_registration_to_log_in()
{
	gtk_revealer_set_reveal_child(GTK_REVEALER(reg_window.Revealer1),FALSE);
	gtk_revealer_set_reveal_child(GTK_REVEALER(reg_window.Revealer2),FALSE);
	gtk_widget_hide(reg_window.Window);//скрываем окно регистрации
	gtk_widget_show_all(log_in_window.Window);//показываем окно входа
}
void from_log_in_to_registration()
{
	gtk_revealer_set_reveal_child(GTK_REVEALER(log_in_window.Revealer2),FALSE);
	gtk_revealer_set_reveal_child(GTK_REVEALER(log_in_window.Revealer1),FALSE);
	gtk_widget_hide(log_in_window.Window);//скрываем окно входа
	gtk_widget_show_all(reg_window.Window);//показываем окно регистрации
}
void close_and_free_all()
{
	printf("SIGNAL: CLOSE_AND_FREE_ALL\n");
	char buf[MAX_PATH+256];
	free_sig_data(node_people_list);
	free_sig_data(node_profile_window);
	free_sig_data(node_messages_list);
	free_sig_data(node_friend_list);
	free_sig_data(node_sub_list);
	free_sig_data(node_dialog_window);
	free_sig_data(node_profile_party);
	free_sig_data(node_party_peoples);
	free_sig_data(node_change_message);
	free_sig_data(node_profile_setting);
	gtk_widget_destroy(reg_window.Window);//закрываем окно регистрации,также эта функция затирает адреса на которые указывают nickname и password(все ссылки reg_window)
	gtk_widget_destroy(log_in_window.Window);//закрываем окно входа,также эта функция затирает переменные nickname и password(все ссылки reg_window)
	gtk_widget_destroy(setting_window.Window);

	sprintf(buf,"rm -R %s/*",PATHS[PATH_DIALOG_PHOTOS]);//очищаем кэш изображений
	system(buf);
	sprintf(buf,"rm -R %s/*",PATHS[PATH_USER_AVATARS]);//очищаем кэш всех аватарок
	system(buf);
	sprintf(buf,"rm -R %s/*",PATHS[PATH_PARTY_AVATARS]);//очищаем кэш всех аватарок групп
	system(buf);
	g_free(error);// Освобождаем память
	mysql_close(conn);// Закрываем соединение(и освобождаем память)
	close(sock);
	fclose(boblog);
}
void show_setting_panel(GtkWidget *button, sig_data* data)
{
	fprintf(stderr,"SIGNAL: SHOW_SETTING_PANEL\n"); fflush(stderr);
	//gtk_revealer_set_reveal_child(GTK_REVEALER(setting_panel.Panel),FALSE);
	if(flag_panel_right==FALSE)
	{
		gtk_revealer_set_reveal_child(GTK_REVEALER(setting_panel.Revealer1),TRUE);
		gtk_widget_hide(main_window.Button);
	}
	else
	{
		gtk_revealer_set_reveal_child(GTK_REVEALER(setting_panel.Revealer1),FALSE);
		gtk_widget_show(main_window.Button);
	}
	flag_panel_right=!flag_panel_right;
}
void exit_from_account()
{
	user_log=fopen(PATHS[PATH_AUTOLOG],"w+");//стираем данные из файла
	fclose(user_log);
	gtk_main_quit();
}
void bob_init()
{
	/*загружаем тему*/
	/*GtkCssProvider *provider;
	GFile *file;
	GdkScreen *screen;
	screen = gdk_screen_get_default();
	gchar *path;
	path = g_build_filename("/home/piska/kakao/usr/share/themes/KakaoBob/Midnight-BlueNight/gtk-3.0/gtk.css", NULL);
	file = g_file_new_for_path(path);

	g_free(path);
	provider = gtk_css_provider_new();
	gtk_css_provider_load_from_file(provider, file, NULL);
	gtk_style_context_add_provider_for_screen(screen,
	GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
	gtk_style_context_reset_widgets(screen);
	g_object_unref(provider);*/


	builder=gtk_builder_new();
	if(gtk_builder_add_from_file(builder,PATHS[PATH_INTERFACE],&error)==0)
	{//связываем файл glade с builder
		g_warning( "%s", error->message );
        close_and_free_all();
        exit(1);
	}
	//Получаем указатели на виджеты
//	|							  |
//	V 							  V
	main_window.Notebook=GTK_WIDGET(gtk_builder_get_object(builder,"notebook"));
	main_window.Window=GTK_WIDGET(gtk_builder_get_object(builder,"main_window"));
	main_window.Button=GTK_WIDGET(gtk_builder_get_object(builder,"button_setting"));
	main_window.label=GTK_WIDGET(gtk_builder_get_object(builder,"label_unread_dialogs"));

	setting_window.Window=GTK_WIDGET(gtk_builder_get_object(builder,"setting_window"));
	setting_window.Box=GTK_WIDGET(gtk_builder_get_object(builder,"settings"));

	setting_panel.Panel=GTK_WIDGET(gtk_builder_get_object(builder,"panel_right"));
	setting_panel.Revealer1=GTK_WIDGET(gtk_builder_get_object(builder,"panel_right_revealer"));
	setting_panel.Button1=GTK_WIDGET(gtk_builder_get_object(builder,"setting_panel_profile"));

	profile_setting.Entry1=GTK_WIDGET(gtk_builder_get_object(builder,"setting_profile_nickname"));
	profile_setting.Revealer=GTK_WIDGET(gtk_builder_get_object(builder,"revealer_change_nick_setting_profile"));
	profile_setting.Revealer2=GTK_WIDGET(gtk_builder_get_object(builder,"revealer_change_nick_setting_profile2"));
	profile_setting.Grid=GTK_WIDGET(gtk_builder_get_object(builder,"setting_profile"));
	profile_setting.image2=GTK_WIDGET(gtk_builder_get_object(builder,"setting_avatar"));
	profile_setting.Button=GTK_WIDGET(gtk_builder_get_object(builder,"setting_button_avatar"));
	profile_setting.Revealer3=GTK_WIDGET(gtk_builder_get_object(builder,"revealer_avatar_setting_profile"));
	profile_setting.Button2=GTK_WIDGET(gtk_builder_get_object(builder,"setting_profile_delete_avatar"));

	password_setting.Revealer=GTK_WIDGET(gtk_builder_get_object(builder,"revealer_change_pass_setting"));
	password_setting.Revealer2=GTK_WIDGET(gtk_builder_get_object(builder,"revealer_change_pass_setting2"));
	password_setting.Revealer3=GTK_WIDGET(gtk_builder_get_object(builder,"revealer_change_pass_setting3"));
	password_setting.Entry1=GTK_WIDGET(gtk_builder_get_object(builder,"setting_old_pass"));
	password_setting.Entry2=GTK_WIDGET(gtk_builder_get_object(builder,"setting_new_pass"));
	password_setting.Grid=GTK_WIDGET(gtk_builder_get_object(builder,"setting_password"));

	messages_list.Box3=GTK_WIDGET(gtk_builder_get_object(builder,"messages_box"));
	messages_list.Window=GTK_WIDGET(gtk_builder_get_object(builder,"messages_window"));
	messages_list.Box=GTK_WIDGET(gtk_builder_get_object(builder,"message_list"));
	messages_list.Scrolled=GTK_WIDGET(gtk_builder_get_object(builder,"messages_scrolled"));
	messages_list.label=GTK_WIDGET(gtk_builder_get_object(builder,"messages_companion_label"));
	messages_list.label2=GTK_WIDGET(gtk_builder_get_object(builder,"messages_party_label"));
	messages_list.label3=GTK_WIDGET(gtk_builder_get_object(builder,"messages_label_unread_dialogs_with_companions"));
	messages_list.label4=GTK_WIDGET(gtk_builder_get_object(builder,"messages_label_unread_dialogs_with_parties"));
	messages_list.Button=GTK_WIDGET(gtk_builder_get_object(builder,"messages_companion_button"));
	messages_list.Button2=GTK_WIDGET(gtk_builder_get_object(builder,"messages_party_button"));

	dialog_window.Box=GTK_WIDGET(gtk_builder_get_object(builder,"dialog_main_box"));
	dialog_window.Box2=GTK_WIDGET(gtk_builder_get_object(builder,"dialog_box"));
	dialog_window.Box3=GTK_WIDGET(gtk_builder_get_object(builder,"dialog_writing_nicknames_box"));
	dialog_window.Grid=GTK_WIDGET(gtk_builder_get_object(builder,"dialog_messagesbox"));
	dialog_window.label=GTK_WIDGET(gtk_builder_get_object(builder,"dialog_when_online_label"));
	dialog_window.label2=GTK_WIDGET(gtk_builder_get_object(builder,"dialog_writing_n_peoples_label"));
	dialog_window.spinner=GTK_WIDGET(gtk_builder_get_object(builder,"dialog_spinner"));
	dialog_window.TextView=GTK_WIDGET(gtk_builder_get_object(builder,"dialog_messageinput"));
	dialog_window.Button=GTK_WIDGET(gtk_builder_get_object(builder,"dialog_send_message"));
	dialog_window.Button2=GTK_WIDGET(gtk_builder_get_object(builder,"dialog_back"));
	dialog_window.Button3=GTK_WIDGET(gtk_builder_get_object(builder,"dialog_nickname"));
	dialog_window.Button5=GTK_WIDGET(gtk_builder_get_object(builder,"dialog_button_choice_file"));
	dialog_window.popover1=GTK_WIDGET(gtk_builder_get_object(builder,"popover4"));
	dialog_window.Button7=GTK_WIDGET(gtk_builder_get_object(builder,"dialog_button_choice_file2"));
	dialog_window.Button8=GTK_WIDGET(gtk_builder_get_object(builder,"dialog_delete_my_messages_button"));
	dialog_window.Scrolled=GTK_WIDGET(gtk_builder_get_object(builder,"dialog_scrolled"));
	dialog_window.Adjustment=GTK_ADJUSTMENT(gtk_builder_get_object(builder,"dialog_adjustment"));

	reg_window.Window=GTK_WIDGET(gtk_builder_get_object(builder,"reg_window"));
	reg_window.Nickname=GTK_WIDGET(gtk_builder_get_object(builder,"reg_nickname"));
	reg_window.Password=GTK_WIDGET(gtk_builder_get_object(builder,"reg_password"));
	reg_window.TextBox=GTK_WIDGET(gtk_builder_get_object(builder,"reg_prompt"));
	reg_window.Revealer1=GTK_WIDGET(gtk_builder_get_object(builder,"revealer_reg_prompt"));
	reg_window.Revealer2=GTK_WIDGET(gtk_builder_get_object(builder,"revealer_reg_prompt2"));

	log_in_window.Window=GTK_WIDGET(gtk_builder_get_object(builder,"log_in_window"));
	log_in_window.Nickname=GTK_WIDGET(gtk_builder_get_object(builder,"log_in_nickname"));
	log_in_window.Password=GTK_WIDGET(gtk_builder_get_object(builder,"log_in_password"));
	log_in_window.TextBox=GTK_WIDGET(gtk_builder_get_object(builder,"log_in_prompt"));
	log_in_window.Revealer1=GTK_WIDGET(gtk_builder_get_object(builder,"revealer_log_in_prompt"));
	log_in_window.Revealer2=GTK_WIDGET(gtk_builder_get_object(builder,"revealer_log_in_prompt2"));

	error_window.Window=GTK_WIDGET(gtk_builder_get_object(builder,"error_window"));
	error_window.TextBox=GTK_WIDGET(gtk_builder_get_object(builder,"error_text"));
	error_window.Button=GTK_WIDGET(gtk_builder_get_object(builder,"error_button_close"));

	sub_list.Box=GTK_WIDGET(gtk_builder_get_object(builder,"friend_request_list"));
	sub_list.Box2=GTK_WIDGET(gtk_builder_get_object(builder,"friend_request_box"));
	sub_list.Scrolled=GTK_WIDGET(gtk_builder_get_object(builder,"friend_request_scrolled"));
	sub_list.Button2=GTK_WIDGET(gtk_builder_get_object(builder,"friend_request_button_back"));
	sub_list.PeopleSearchEntry=GTK_WIDGET(gtk_builder_get_object(builder,"friend_request_entry"));

	friend_list.Box=GTK_WIDGET(gtk_builder_get_object(builder,"friend_list"));
	friend_list.Window=GTK_WIDGET(gtk_builder_get_object(builder,"friend_window"));
	friend_list.Box2=GTK_WIDGET(gtk_builder_get_object(builder,"friend_box"));
	friend_list.PeopleSearchEntry=GTK_WIDGET(gtk_builder_get_object(builder,"friend_entry"));
	friend_list.Grid2=GTK_WIDGET(gtk_builder_get_object(builder,"friend_grid_search"));
	friend_list.Revealer=GTK_WIDGET(gtk_builder_get_object(builder,"revealer_friend_entry"));
	friend_list.Button2=GTK_WIDGET(gtk_builder_get_object(builder,"friend_update"));
	friend_list.Button3=GTK_WIDGET(gtk_builder_get_object(builder,"to_friend_request_button"));
	friend_list.Button4=GTK_WIDGET(gtk_builder_get_object(builder,"friend_update"));
	friend_list.Scrolled=GTK_WIDGET(gtk_builder_get_object(builder,"friend_scrolled"));

	people_list.Window=GTK_WIDGET(gtk_builder_get_object(builder,"people_window"));
	people_list.Box=GTK_WIDGET(gtk_builder_get_object(builder,"people_list"));
	people_list.Button2=GTK_WIDGET(gtk_builder_get_object(builder,"people_update"));
	people_list.PeopleSearchEntry=GTK_WIDGET(gtk_builder_get_object(builder,"people_search_entry"));
	people_list.PeopleListBoxParrent=GTK_WIDGET(gtk_builder_get_object(builder,"people_search_child"));
	people_list.Scrolled=GTK_WIDGET(gtk_builder_get_object(builder,"people_scrolled"));

	people_profile.Window=GTK_WIDGET(gtk_builder_get_object(builder,"people_profile_window"));
	people_profile.Revealer=GTK_WIDGET(gtk_builder_get_object(builder,"friend_menu_revealer"));
	people_profile.Button6=GTK_WIDGET(gtk_builder_get_object(builder,"image_profile_button"));
	people_profile.image2=GTK_WIDGET(gtk_builder_get_object(builder,"image_profile"));
	people_profile.message_button=GTK_WIDGET(gtk_builder_get_object(builder,"message_button_profile"));
	people_profile.Button=GTK_WIDGET(gtk_builder_get_object(builder,"friend_button_profile"));
	people_profile.Button2=GTK_WIDGET(gtk_builder_get_object(builder,"profile_back"));
	people_profile.Button3=GTK_WIDGET(gtk_builder_get_object(builder,"profile_update"));
	people_profile.Button4=GTK_WIDGET(gtk_builder_get_object(builder,"profile_button_block"));
	people_profile.Button5=GTK_WIDGET(gtk_builder_get_object(builder,"profile_button_unblock"));
	people_profile.MenuButton=GTK_WIDGET(gtk_builder_get_object(builder,"friend_menubutton_profile"));
	people_profile.MenuButton2=GTK_WIDGET(gtk_builder_get_object(builder,"profile_menubutton_block"));
	people_profile.label=GTK_WIDGET(gtk_builder_get_object(builder,"nickname_profile"));
	people_profile.label2=GTK_WIDGET(gtk_builder_get_object(builder,"when_online_profile"));
	people_profile.Box=GTK_WIDGET(gtk_builder_get_object(builder,"profile_box_block"));

	answer_app_profile.Window=GTK_WIDGET(gtk_builder_get_object(builder,"answerapp_window"));
	answer_app_profile.Button1=GTK_WIDGET(gtk_builder_get_object(builder,"answerapp1"));
	answer_app_profile.Button2=GTK_WIDGET(gtk_builder_get_object(builder,"answerapp2"));

	create_party.Window=GTK_WIDGET(gtk_builder_get_object(builder,"create_party_window"));
	create_party.Entry1=GTK_WIDGET(gtk_builder_get_object(builder,"create_party_name"));
	create_party.TextView=GTK_WIDGET(gtk_builder_get_object(builder,"create_party_description"));
	create_party.Revealer=GTK_WIDGET(gtk_builder_get_object(builder,"create_party_revealer"));

	profile_party.Window=GTK_WIDGET(gtk_builder_get_object(builder,"party_window"));
	profile_party.Button=GTK_WIDGET(gtk_builder_get_object(builder,"party_back"));
	profile_party.Entry1=GTK_WIDGET(gtk_builder_get_object(builder,"party_name"));
	profile_party.Button5=GTK_WIDGET(gtk_builder_get_object(builder,"party_avatar_button"));
	profile_party.Button6=GTK_WIDGET(gtk_builder_get_object(builder,"profile_party_change_avatar_button"));
	profile_party.Button7=GTK_WIDGET(gtk_builder_get_object(builder,"profile_party_delete_avatar_button"));
	profile_party.image3=GTK_WIDGET(gtk_builder_get_object(builder,"party_image"));
	profile_party.MenuButton=GTK_WIDGET(gtk_builder_get_object(builder,"profile_party_avatar_setting_button"));
	profile_party.Revealer=GTK_WIDGET(gtk_builder_get_object(builder,"revealer_party_name1"));
	profile_party.Revealer2=GTK_WIDGET(gtk_builder_get_object(builder,"revealer_party_name2"));
	profile_party.Revealer3=GTK_WIDGET(gtk_builder_get_object(builder,"revealer_party_description1"));
	profile_party.Revealer4=GTK_WIDGET(gtk_builder_get_object(builder,"revealer_party_description2"));
	profile_party.TextView=GTK_WIDGET(gtk_builder_get_object(builder,"party_description"));
	profile_party.Button2=GTK_WIDGET(gtk_builder_get_object(builder,"party_button_change_description"));
	profile_party.PeopleSearchEntry=GTK_WIDGET(gtk_builder_get_object(builder,"party_search_entry"));
	profile_party.Scrolled=GTK_WIDGET(gtk_builder_get_object(builder,"party_scrolled"));
	profile_party.Box=GTK_WIDGET(gtk_builder_get_object(builder,"party_users"));

	popover_profile_party.Box=GTK_WIDGET(gtk_builder_get_object(builder,"popover_profile_party_admin_box"));
	popover_profile_party.Box2=GTK_WIDGET(gtk_builder_get_object(builder,"popover_profile_party_user_box"));
	popover_profile_party.Box3=GTK_WIDGET(gtk_builder_get_object(builder,"popover_profile_party_box"));

	party_peoples_window.Window=GTK_WIDGET(gtk_builder_get_object(builder,"party_peoples_window"));
	party_peoples_window.Box=GTK_WIDGET(gtk_builder_get_object(builder,"party_change_peoples"));
	party_peoples_window.Scrolled=GTK_WIDGET(gtk_builder_get_object(builder,"party_peoples_scrolled"));
	party_peoples_window.PeopleSearchEntry=GTK_WIDGET(gtk_builder_get_object(builder,"party_peoples_search"));

	change_message.Window=GTK_WIDGET(gtk_builder_get_object(builder,"change_message_window"));
	change_message.Button=GTK_WIDGET(gtk_builder_get_object(builder,"change_message_delete_everywhere_button"));
	change_message.Button2=GTK_WIDGET(gtk_builder_get_object(builder,"change_message_window_button_edit"));
	change_message.Button3=GTK_WIDGET(gtk_builder_get_object(builder,"change_message_window_edit_message"));
	change_message.TextView=GTK_WIDGET(gtk_builder_get_object(builder,"popover_change_message_messageinput"));
	change_message.Box=GTK_WIDGET(gtk_builder_get_object(builder,"change_message_window_box"));
	change_message.Box2=GTK_WIDGET(gtk_builder_get_object(builder,"change_message_main_box"));
	//reg_window.Surname=GTK_WIDGET(gtk_builder_get_object(builder,"reg_surname"));

	//Notebook=GTK_WIDGET(gtk_builder_get_object(builder,"notebook"));
	if( error_window.Button==NULL || error_window.TextBox==NULL || error_window.Window==NULL || reg_window.Revealer2==NULL || reg_window.Revealer1==NULL || log_in_window.Revealer2==NULL || log_in_window.Revealer1==NULL ||
   	password_setting.Revealer3==NULL || password_setting.Grid==NULL || profile_setting.Grid==NULL || setting_window.Box==NULL || password_setting.Entry2==NULL ||
	password_setting.Entry1==NULL || password_setting.Revealer2==NULL || password_setting.Revealer==NULL || profile_setting.Revealer2==NULL ||
	profile_setting.Revealer==NULL || profile_setting.Entry1==NULL || setting_window.Window==NULL || setting_panel.Button1==NULL || main_window.Button==NULL ||
	setting_panel.Panel==NULL || dialog_window.Adjustment==NULL || sub_list.Scrolled==NULL || friend_list.Scrolled==NULL || messages_list.Scrolled==NULL ||
	people_list.Scrolled==NULL || dialog_window.Scrolled==NULL || dialog_window.Button3==NULL || people_profile.MenuButton==NULL || dialog_window.Button2==NULL ||
	messages_list.Window==NULL || dialog_window.Box2==NULL || dialog_window.Box==NULL || messages_list.Box==NULL || sub_list.PeopleSearchEntry==NULL ||
	friend_list.Button4==NULL || friend_list.Button3==NULL || sub_list.Button2==NULL || sub_list.Box2==NULL || friend_list.Window==NULL ||
	people_profile.Button3==NULL || people_profile.Button2==NULL || people_list.Button2==NULL || dialog_window.Button==NULL || messages_list.Box3==NULL ||
	friend_list.Button2==NULL || friend_list.Grid2==NULL || sub_list.Box==NULL || people_profile.Revealer==NULL ||
	friend_list.Box2==NULL || friend_list.PeopleSearchEntry==NULL || people_profile.label==NULL || people_profile.Button==NULL ||
	people_profile.image2==NULL || people_profile.Window==NULL || people_list.Window==NULL || people_list.PeopleListBoxParrent==NULL ||
	main_window.Notebook==NULL || people_list.PeopleSearchEntry==NULL || main_window.Window==NULL || dialog_window.Grid==NULL || dialog_window.TextView==NULL ||
	reg_window.Window==NULL || reg_window.Nickname==NULL || reg_window.Password==NULL || reg_window.TextBox==NULL || log_in_window.Window==NULL ||
	log_in_window.Nickname==NULL || log_in_window.Password==NULL || log_in_window.TextBox==NULL || friend_list.Box==NULL || people_list.Box==NULL ||
	create_party.Window==NULL || create_party.Entry1==NULL || create_party.TextView==NULL || create_party.Revealer==NULL || profile_party.Window==NULL || profile_party.Button==NULL ||
	profile_party.Entry1==NULL || profile_party.Revealer==NULL || profile_party.Revealer2==NULL || profile_party.TextView==NULL || profile_party.Revealer3==NULL ||
	profile_party.Revealer4==NULL || profile_party.Button2==NULL || profile_party.PeopleSearchEntry==NULL || profile_party.Scrolled==NULL || profile_party.Box==NULL ||
	party_peoples_window.Window==NULL || party_peoples_window.Box==NULL || party_peoples_window.Scrolled==NULL || party_peoples_window.PeopleSearchEntry==NULL || setting_panel.Revealer1==NULL ||
	popover_profile_party.Box==NULL || popover_profile_party.Box2==NULL || popover_profile_party.Box3==NULL || change_message.Window==NULL || change_message.Button==NULL ||
	change_message.TextView==NULL || change_message.Box==NULL || change_message.Button2==NULL || change_message.Button3==NULL || people_profile.label2==NULL || dialog_window.Box3==NULL
	|| dialog_window.label==NULL || dialog_window.label2==NULL || messages_list.label==NULL || messages_list.label2==NULL || messages_list.Button==NULL
	|| messages_list.Button2==NULL || people_profile.MenuButton2==NULL || people_profile.Button4==NULL || people_profile.Button5==NULL || people_profile.Box==NULL || main_window.label==NULL
	|| dialog_window.Button5==NULL || dialog_window.Button7==NULL 
	|| profile_setting.image2==NULL || profile_setting.Button==NULL || profile_setting.Revealer3==NULL || profile_setting.Button2==NULL || people_profile.Button6==NULL ||
	profile_party.Button5==NULL || profile_party.image3==NULL || profile_party.MenuButton==NULL || profile_party.Button6==NULL || profile_party.Button7==NULL || dialog_window.Button8==NULL
	|| dialog_window.popover1==NULL || messages_list.label3==NULL || messages_list.label4==NULL) {
		g_critical("error: Не могу получить виджет\n");
		close_and_free_all();
        exit(1);
	}
	gdk_threads_init();
	gtk_builder_connect_signals(builder,NULL);//подключаем сигналы из builder
	g_object_unref(builder);//освобождаем память(builder нам больше не пригодится)
	//. g_object_get(object, "gtk-enable-animation",NULL);
	//g_object_set(object, "gtk-enable-animation", 1, NULL);
	/*здесь мы сможем редактировать или удалять сообщение*/

	gdk_monitor_get_workarea(gdk_display_get_monitor_at_window(gdk_display_get_default(),GDK_WINDOW(main_window.Window)),&workarea);//получаем разрешение экрана
	printf ("W: %u x H:%u\n", workarea.width, workarea.height);

	dialog_buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(dialog_window.TextView));// Получаем буфер из диалогового окна
	create_party_buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(create_party.TextView));// Получаем буфер описания группы
	party_buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(profile_party.TextView));// Получаем буфер описания группы
	gtk_window_resize(GTK_WINDOW(main_window.Window),workarea.width/2,workarea.height/2);//устанавливаем размер окна
	gtk_window_resize(GTK_WINDOW(log_in_window.Window),workarea.width/2,workarea.height/2);//устанавливаем размер окна
	gtk_window_resize(GTK_WINDOW(reg_window.Window),workarea.width/2,workarea.height/2);//устанавливаем размер окна
	gtk_window_resize(GTK_WINDOW(create_party.Window),workarea.width/2,workarea.height/2);//устанавливаем размер окна
	gtk_window_resize(GTK_WINDOW(party_peoples_window.Window),workarea.width/2,workarea.height/2);//устанавливаем размер окна
	gtk_window_resize(GTK_WINDOW(setting_window.Window),workarea.width/5,workarea.height/2);//устанавливаем размер окна
	gtk_window_set_title(GTK_WINDOW(main_window.Window),"KakaoBob");
	gtk_window_set_title(GTK_WINDOW(log_in_window.Window),"KakaoBob - вход");
	gtk_window_set_title(GTK_WINDOW(reg_window.Window),"KakaoBob - регистрация");
	gtk_window_set_title(GTK_WINDOW(create_party.Window),"KakaoBob - создание группы");

	
	/*if(workarea.height>720)//если высота экрана больше 720 пикселей
	{
		path_to_avatar="/home/piska/Изображения/defaultavatar1280.jpg";
		gtk_image_set_from_file(GTK_IMAGE(people_profile.image2),path_to_avatar);
	}
	else//если равно или меньше
	{
		//ничего не делаем, но должны загружать изображение меньше, также изображения должны сжиматься
	}
	gtk_image_set_from_file(GTK_IMAGE(people_profile.image2),path_to_avatar);
	gtk_image_set_from_file(GTK_IMAGE(people_profile.image2),path_to_avatar);*/
	gtk_revealer_set_reveal_child(GTK_REVEALER(reg_window.Revealer1),FALSE);
	gtk_revealer_set_reveal_child(GTK_REVEALER(reg_window.Revealer2),FALSE);
	gtk_revealer_set_reveal_child(GTK_REVEALER(log_in_window.Revealer1),FALSE);
	gtk_revealer_set_reveal_child(GTK_REVEALER(log_in_window.Revealer2),FALSE);
	gtk_revealer_set_reveal_child(GTK_REVEALER(create_party.Revealer),FALSE);
	gtk_revealer_set_reveal_child(GTK_REVEALER(setting_panel.Revealer1),FALSE);
	gtk_revealer_set_transition_duration(GTK_REVEALER(log_in_window.Revealer1),250);
	gtk_revealer_set_transition_duration(GTK_REVEALER(log_in_window.Revealer2),250);
	gtk_revealer_set_transition_duration(GTK_REVEALER(reg_window.Revealer1),250);
	gtk_revealer_set_transition_duration(GTK_REVEALER(reg_window.Revealer2),250);
	gtk_revealer_set_transition_duration(GTK_REVEALER(create_party.Revealer),250);
	gtk_revealer_set_transition_duration(GTK_REVEALER(setting_panel.Revealer1),250);
}


void transition_to_friend_list(GtkWidget *button, sig_data *data)
{
	fprintf(stderr,"SIGNAL: TRANSITION_TO_FRIEND_LIST\n"); fflush(stderr);
	hide_window(friend_list.Window);
	show_window(friend_list.Scrolled);
	show_friend_list(NULL,data);
}
void transition_to_sub_list(GtkWidget *button, sig_data *data)
{
	fprintf(stderr,"SIGNAL: TRANSITION_TO_SUB_LIST\n"); fflush(stderr);
	hide_window(friend_list.Window);
	show_window(sub_list.Scrolled);
	show_my_subs(NULL,data);
}
void transition_to_dialog_window(GtkWidget *widget, sig_data *data)
{
	fprintf(stderr,"SIGNAL: TRANSITION_TO_DIALOG_WINDOW\n"); fflush(stderr);
	hide_window(messages_list.Window);
	show_window(dialog_window.Box);
	short int num_search_page = gtk_notebook_page_num(GTK_NOTEBOOK(main_window.Notebook),messages_list.Window);//получаем номер страницы 'сообщения'
	gtk_notebook_set_current_page(GTK_NOTEBOOK(main_window.Notebook),num_search_page);//выбираем страницу
	flag_messages_list=FALSE;//закрываем поток, обновляющий страницу messages_list
	flag_party_messages_list=FALSE;//закрываем поток, обновляющий страницу party_messages_list
	flag_dialog=FALSE;
	if(pthread_join(var_thread_update_dialog,&thread_result)==-1){//ждем закрытия потока
		perror("pthread_join");
		exit(1);
	}
	if(pthread_join(var_thread_update_dialog_messages,&thread_result)==-1){//ждем закрытия потока
		perror("pthread_join");
		exit(1);
	}

	show_messages(NULL,data); 
	show_dialog_window(widget,data);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(dialog_window.Adjustment),gtk_adjustment_get_upper(GTK_ADJUSTMENT(dialog_window.Adjustment)));
	
	pthread_create(&var_thread_update_dialog_messages,NULL,update_dialog_messages,(void*)data);//создаем поток в котором будут обновляться сообщения
	pthread_create(&var_thread_update_dialog,NULL,update_dialog,(void*)data);//создаем поток в котором будет обновляться диалоговое окно

	pthread_create(&var_thread_unread_dialogs,NULL,update_unread_dialogs,(void*)data);//создаем поток в котором будет обновляться количество непрочитанных диалогов
}
void transition_to_messages_list(GtkWidget *widget, sig_data *data)
{
	fprintf(stderr,"SIGNAL: TRANSITION_TO_MESSAGES_LIST\n"); fflush(stderr);
	hide_window(messages_list.Window);
	show_window(messages_list.Box3);

	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(messages_list.Button2),G_SIGNAL_MATCH_FUNC,0,0,NULL,show_party_messages_list,0); while(ret!=0);//удаляем прошлый сигнал show_party_messages_list кнопки группа
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(messages_list.Button),G_SIGNAL_MATCH_FUNC,0,0,NULL,transition_to_messages_list,0); while(ret!=0);//удаляем прошлый сигнал перехода в messages_list кнопки собеседник
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(messages_list.Button),G_SIGNAL_MATCH_FUNC,0,0,NULL,show_messages_list,0); while(ret!=0);//удаляем сигнал кнопки собеседник
	g_signal_connect(G_OBJECT(messages_list.Button2),"clicked",G_CALLBACK(transition_to_party_messages_list),NULL);//добавляем кнопке группа новый сигнал перехода в party_messages_list
	g_signal_connect(G_OBJECT(messages_list.Button),"clicked",G_CALLBACK(show_messages_list),NULL);//добавляем кнопке собеседник новый сигнал show_messages_list

	PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов

	PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_NORMAL);//настраиваем толщину метки
	pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
	gtk_label_set_attributes (GTK_LABEL(messages_list.label2),attr_of_label);//устанавливаем атрибуты для метки 
	pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов

	attr_of_label=pango_attr_list_new();//создаем список атрибутов

	attr_width=pango_attr_weight_new(PANGO_WEIGHT_ULTRAHEAVY);//настраиваем толщину метки
	pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
	gtk_label_set_attributes (GTK_LABEL(messages_list.label),attr_of_label);//устанавливаем атрибуты для метки Собеседник
	pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов


	short int num_search_page = gtk_notebook_page_num(GTK_NOTEBOOK(main_window.Notebook),messages_list.Window);//получаем номер страницы 'сообщения'
	gtk_notebook_set_current_page(GTK_NOTEBOOK(main_window.Notebook),num_search_page);//выбираем страницу
	flag_messages_list=FALSE;//закрываем потоки, обновляющие страницу messages_list
	flag_party_messages_list=FALSE;//закрываем потоки, обновляющие страницу party_messages_list
	flag_dialog=FALSE;//закрываем потоки, обновляющие сообщения и диалоговое окно
	flag_unread_dialogs=FALSE;//закрываем потоки, обновляющие непрочитанные диалоги
	show_messages_list(NULL,NULL);
	pthread_create(&var_thread_messages_list,NULL,update_messages_list,NULL);//создаем поток в котором будет обновляться страница messages_list
}
void transition_to_party_messages_list(GtkWidget *widget, sig_data *data)
{
	fprintf(stderr,"SIGNAL: TRANSITION_TO_PARTY_MESSAGES_LIST\n"); fflush(stderr);
	hide_window(messages_list.Window);
	show_window(messages_list.Box3);

	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(messages_list.Button),G_SIGNAL_MATCH_FUNC,0,0,NULL,show_messages_list,0); while(ret!=0);//удаляем прошлый сигнал кнопки собеседник show_messages_list
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(messages_list.Button2),G_SIGNAL_MATCH_FUNC,0,0,NULL,transition_to_party_messages_list,0); while(ret!=0);//удаляем прошлый сигнал перехода в party_messages_list кнопки группа
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(messages_list.Button2),G_SIGNAL_MATCH_FUNC,0,0,NULL,show_party_messages_list,0); while(ret!=0);//удаляем сигнал кнопки группа
	g_signal_connect(G_OBJECT(messages_list.Button),"clicked",G_CALLBACK(transition_to_messages_list),NULL);//добавляем кнопке собеседник новый сигнал перехода в messages_list
	g_signal_connect(G_OBJECT(messages_list.Button2),"clicked",G_CALLBACK(show_party_messages_list),NULL);//добавляем кнопке группа новый сигнал show_party_messages_list

	PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов

	PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_ULTRAHEAVY);//настраиваем толщину метки
	pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
	gtk_label_set_attributes (GTK_LABEL(messages_list.label2),attr_of_label);//устанавливаем атрибуты для метки 
	pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов

	attr_of_label=pango_attr_list_new();//создаем список атрибутов

	attr_width=pango_attr_weight_new(PANGO_WEIGHT_NORMAL);//настраиваем толщину метки
	pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
	gtk_label_set_attributes (GTK_LABEL(messages_list.label),attr_of_label);//устанавливаем атрибуты для метки Собеседник
	pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов


	short int num_search_page = gtk_notebook_page_num(GTK_NOTEBOOK(main_window.Notebook),messages_list.Window);//получаем номер страницы 'сообщения'
	gtk_notebook_set_current_page(GTK_NOTEBOOK(main_window.Notebook),num_search_page);//выбираем страницу
	flag_messages_list=FALSE;//закрываем потоки, обновляющие страницу messages_list
	flag_party_messages_list=FALSE;//закрываем потоки, обновляющие страницу party_messages_list
	flag_dialog=FALSE;//закрываем потоки, обновляющие сообщения и диалоговое окно
	flag_unread_dialogs=FALSE;//закрываем потоки, обновляющие непрочитанные диалоги
	show_party_messages_list(NULL,NULL);
	pthread_create(&var_thread_party_messages_list,NULL,update_party_messages_list,NULL);//создаем поток в котором будет обновляться страница messages_list
}
void transition_to_people_list(GtkWidget *widget, sig_data *data)
{
	fprintf(stderr,"SIGNAL: TRANSITION_TO_PEOPLE_LIST\n"); fflush(stderr);
	hide_window(people_list.Window);
	show_window(people_list.PeopleListBoxParrent);

	short int num_search_page = gtk_notebook_page_num(GTK_NOTEBOOK(main_window.Notebook),people_list.Window);//получаем номер страницы поиска людей
	gtk_notebook_set_current_page(GTK_NOTEBOOK(main_window.Notebook),num_search_page);//выбираем страницу поиска
	show_people_list();
}
void transition_to_profile_party(GtkWidget *widget, sig_data *data)
{
	fprintf(stderr,"SIGNAL: TRANSITION_TO_PROFILE_PARTY\n"); fflush(stderr);
	gtk_revealer_set_reveal_child(GTK_REVEALER(profile_party.Revealer),FALSE);
	gtk_revealer_set_reveal_child(GTK_REVEALER(profile_party.Revealer2),FALSE);
	gtk_revealer_set_reveal_child(GTK_REVEALER(profile_party.Revealer3),FALSE);
	gtk_revealer_set_reveal_child(GTK_REVEALER(profile_party.Revealer4),FALSE);
	gtk_revealer_set_transition_duration(GTK_REVEALER(profile_party.Revealer),250);
	gtk_revealer_set_transition_duration(GTK_REVEALER(profile_party.Revealer2),250);
	gtk_revealer_set_transition_duration(GTK_REVEALER(profile_party.Revealer3),250);
	gtk_revealer_set_transition_duration(GTK_REVEALER(profile_party.Revealer4),250);
	strcpy(party_info.id,data->party_id);//записываем id группы
	strcpy(party_info.nickname,data->nickname);//записываем имя группы
	hide_window(messages_list.Window);
	gtk_widget_show(profile_party.Window);
	show_profile_party(widget,data);
}
void show_dialog_window(GtkWidget *widget, sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[SHOW_DIALOG_WINDOW]); fflush(stderr);
	char buf[MAX_BUF];
	uint32_t sz;
	short int ret;
	gtk_widget_hide(dialog_window.label);//скрываем метку когда был онлайн
	gtk_widget_hide(dialog_window.label2);//скрываем метку сколько человек печатает сообщение еще
	clean_window(dialog_window.Box3);//удаляем никнеймы кто печатает сообщение
	gtk_spinner_stop(GTK_SPINNER(dialog_window.spinner));
	//объявляем указатель на структуру и выделяем память
	sig_data *data2=(sig_data*)malloc(sizeof(sig_data));
	//создаем список который хранит указатели на структуры чтобы освобождать память
	insert_to_list_sig_data(data2,&node_dialog_window);//вставляем в список структуру типа sig_data(чтобы потом освободить)
	//иницилизируем структуру(в дальнейшем передаем сигналу указатель на эту структуру)
	if(strcmp(data->dialog_type,text_dialog_type[COMPANION])==0) {//если диалог с собеседником
		strcpy(data2->to_id,data->to_id);//id пользователя
	}
	else if(strcmp(data->dialog_type,text_dialog_type[PARTY])==0) {//если диалог с группой
		strcpy(data2->party_id,data->party_id);//id группы
	}
	strcpy(data2->dialog_type,data->dialog_type);//тип диалога(с группой или пользователем)
	strcpy(data2->nickname,data->nickname);//никнейм пользователя или имя группы
	data2->ChildNotebookWidget=messages_list.Window;//передаем указатель на дочерний виджет notebook(нужно чтобы выбирать страницу на которую переходить)
	data2->ShowWidget=dialog_window.Box;//передаем дочерний box,который мы хотим показать
	data2->HideWidget=messages_list.Window;//передаем дочерний box,дочерние виджеты которого мы скроем, чтобы показать один, из которого мы вызывали функцию
	/*удаляем сигналы всех объектов*/
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(dialog_window.Button),G_SIGNAL_MATCH_FUNC,0,0,NULL,send_message,0); while(ret!=0);//удаляем сигнал кнопки отправки сообщения
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(dialog_window.Button2),G_SIGNAL_MATCH_FUNC,0,0,NULL,back,0); while(ret!=0);//удаляем сигнал кнопки назад 
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(dialog_window.Button3),G_SIGNAL_MATCH_FUNC,0,0,NULL,show_profile_window,0); while(ret!=0);//удаляем сигнал кнопки перехода в профиль
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(dialog_window.Button3),G_SIGNAL_MATCH_FUNC,0,0,NULL,transition_to_profile_party,0); while(ret!=0);// удаляем сигнал показа аватарки у кнопки перехода в профиль группы
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(dialog_window.Scrolled),G_SIGNAL_MATCH_FUNC,0,0,NULL,add_limit,0); while(ret!=0);//удаляем сигнал прокручеваемого окна
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(dialog_window.TextView),G_SIGNAL_MATCH_FUNC,0,0,NULL,send_event_return,0); while(ret!=0);//удаляем сигнал у текстового поля в диалоговом окне
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(dialog_window.Button2),G_SIGNAL_MATCH_FUNC,0,0,NULL,transition_to_messages_list,0); while(ret!=0);//удаляем сигнал у кнопки назад
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(dialog_window.Button2),G_SIGNAL_MATCH_FUNC,0,0,NULL,transition_to_party_messages_list,0); while(ret!=0);//удаляем сигнал у кнопки назад
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(dialog_window.Button5),G_SIGNAL_MATCH_FUNC,0,0,NULL,bob_send_file,0); while(ret!=0);//удаляем сигнал у кнопки отправки файлов
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(dialog_window.Button7),G_SIGNAL_MATCH_FUNC,0,0,NULL,bob_send_photo,0); while(ret!=0);//удаляем сигнал у кнопки отправки фото
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(dialog_window.Button8),G_SIGNAL_MATCH_FUNC,0,0,NULL,delete_my_messages_from_dialog,0); while(ret!=0);//удаляем сигнал у кнопки удаления моих сообщений
	/*добавляем свои сигналы*/
	if(data!=NULL)
	{
		g_signal_connect(G_OBJECT(dialog_window.Button),"clicked",G_CALLBACK(send_message),data);//подключаем сигнал к кнопке отправки сообщения
		g_signal_connect(G_OBJECT(dialog_window.Button2),"clicked",G_CALLBACK(back),data);//подключаем сигнал к кнопке назад
	}

	if(strcmp(data->dialog_type,text_dialog_type[COMPANION])==0){//если диалог с собеседником
		g_signal_connect(G_OBJECT(dialog_window.Button3),"clicked",G_CALLBACK(show_profile_window),data2);//подключаем сигнал к кнопке перехода в профиль 
		g_signal_connect(G_OBJECT(dialog_window.Button2),"clicked",G_CALLBACK(transition_to_messages_list),data2);//подключаем сигнал к кнопке назад для перехода в messages_list
	}
	else if(strcmp(data->dialog_type,text_dialog_type[PARTY])==0) {//если диалог с группой
		g_signal_connect(G_OBJECT(dialog_window.Button3),"clicked",G_CALLBACK(transition_to_profile_party),data2);//подключаем сигнал к кнопке перехода в профиль группы
		g_signal_connect(G_OBJECT(dialog_window.Button2),"clicked",G_CALLBACK(transition_to_party_messages_list),data2);//подключаем сигнал к кнопке назад для перехода в party_messages_list
	}
	g_signal_connect(G_OBJECT(dialog_window.Scrolled),"edge-reached",G_CALLBACK(add_limit),data);//подключаем сигнал к прокручиваемогу окну
	g_signal_connect(G_OBJECT(dialog_window.TextView),"key-press-event",G_CALLBACK(send_event_return),data);//подключаем сигнал к текстовому полю в диалоговом окне
	g_signal_connect(G_OBJECT(dialog_window.Button5),"file-set",G_CALLBACK(bob_send_file),data2);//подключаем сигнал к кнопке отправки файлов
	g_signal_connect(G_OBJECT(dialog_window.Button7),"file-set",G_CALLBACK(bob_send_photo),data2);//подключаем сигнал к кнопке отправки фото
	g_signal_connect(G_OBJECT(dialog_window.Button8),"clicked",G_CALLBACK(delete_my_messages_from_dialog),data2);//подключаем сигнал к кнопке удаления моих сообщений

	gtk_button_set_label(GTK_BUTTON(dialog_window.Button3),data->nickname);

	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[SHOW_DIALOG_WINDOW]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_DIALOG_WINDOW]);
		show_error_window(buf,NOTCLOSE); 
		return;
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[SHOW_DIALOG_WINDOW],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_DIALOG_WINDOW]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*отправляем размер типа диалога*/
	sz=htonl(get_str_size(data->dialog_type));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of dialog type(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_DIALOG_WINDOW]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем тип диалога*/
	if( !(ret=sendall(sock,data->dialog_type,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL dialog type(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_DIALOG_WINDOW]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	if(strcmp(data->dialog_type,text_dialog_type[COMPANION])==0)
	{
		/*отправляем размер id собеседника*/
		sz=htonl(get_str_size(data->to_id));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of people id(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_DIALOG_WINDOW]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем id собеседника*/
		if( !(ret=sendall(sock,data->to_id,ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL people id(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_DIALOG_WINDOW]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем печатает ли юзер сообщение */
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV writing?(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_DIALOG_WINDOW]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		if(ntohl(sz)) {//если печатает сообщение
			
			gtk_spinner_start(GTK_SPINNER(dialog_window.spinner));
			label=gtk_label_new("печатает");
			PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
			PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_MEDIUM);//настраиваем толщину метки
	
			pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов

			gtk_label_set_attributes (GTK_LABEL(label),attr_of_label);//устанавливаем атрибуты для метки
			pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов

			gtk_container_add(GTK_CONTAINER(dialog_window.Box3),label);
			gtk_widget_show(label);
			return;
		}

		/*принимаем когда был онлайн */
		ret=user_online(sock);

		gtk_label_set_text(GTK_LABEL(dialog_window.label),text_when_online[ret]);//метка когда пользователь был онлайн
		PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
		PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_NORMAL);//настраиваем толщину метки
		PangoAttribute *attr_color;
		if(ret==ONLINE) {
			attr_color=pango_attr_foreground_new (29490,55704,5898);//настраиваем цвет метки(зеленый)
		}
		else {
			attr_color=pango_attr_foreground_new (65535,0,0);//настраиваем цвет метки(красный)
		}

		pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
		pango_attr_list_insert (attr_of_label,attr_color);//вставляем атрибут в список атрибутов

		gtk_label_set_attributes (GTK_LABEL(dialog_window.label),attr_of_label);//устанавливаем атрибуты для метки
		pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов
		gtk_widget_show(dialog_window.label);//показываем метку когда был онлайн
	}
	else if(strcmp(data->dialog_type,text_dialog_type[PARTY])==0)
	{
		/*отправляем размер id группы*/
		sz=htonl(get_str_size(data->party_id));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of party id(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_DIALOG_WINDOW]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем id группы*/
		if( !(ret=sendall(sock,data->party_id,ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL party id(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_DIALOG_WINDOW]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем размер никнейма тех кто печатает сообщение*/
		for(int i=0,c=0; (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL));++i )
		{
			if(!ntohl(sz)) break;//если конец
			if(i<MAX_PARTY_NICKNAME_WRITINGS)//принимаем никнеймы только n человек
			{
				/*принимаем никнейм*/
				if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECVALL nickname(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_DIALOG_WINDOW]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}
				gtk_spinner_start(GTK_SPINNER(dialog_window.spinner));
				sprintf(buf,"%s печатает",people_info.nickname);
				label=gtk_label_new(buf);
				PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
				PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_MEDIUM);//настраиваем толщину метки
		
				pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов

				gtk_label_set_attributes (GTK_LABEL(label),attr_of_label);//устанавливаем атрибуты для метки
				pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов

				gtk_container_add(GTK_CONTAINER(dialog_window.Box3),label);
				gtk_widget_show(label);
			}
			else//иначе если в группу пишет больше n человек, то мы просто выводим их количество
			{
				++c;
				sprintf(buf,"и еще %d человек",c);
				gtk_label_set_text(GTK_LABEL(dialog_window.label2),buf);
				gtk_widget_show(dialog_window.label2);
			}

		}
		if( ret==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of nickname(%d)\n",kakao_signal[SHOW_DIALOG_WINDOW],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_DIALOG_WINDOW]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		
	}
}
void delete_my_messages_from_dialog(GtkWidget *button, sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG]); fflush(stderr);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[DELETE_MY_MESSAGES_FROM_DIALOG]);
		show_error_window(buf,NOTCLOSE); 
		return;
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[DELETE_MY_MESSAGES_FROM_DIALOG],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[DELETE_MY_MESSAGES_FROM_DIALOG]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
}

int user_online(int sock)
{
	uint32_t sz;
	char buf[MAX_BUF];
	short int ret;
	/*принимаем когда был онлайн */
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV when was online?(%d)\n",kakao_signal[USER_ONLINE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[USER_ONLINE]);
		show_error_window(buf,NOTCLOSE);
		return 0;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	return ntohl(sz);
}
void show_my_subs(GtkWidget *button, sig_data* data2)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[SHOW_MY_SUBS]); fflush(stderr);
	//free_sig_data(node_sub_list);
	//node_sub_list=NULL;
	char limit[MYSQL_INT];
	sprintf(limit,"%ld",limits[LIMIT_MY_SUBS]);//переводим число в строку
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[SHOW_MY_SUBS]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MY_SUBS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[SHOW_MY_SUBS],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MY_SUBS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	clean_window(sub_list.Box);//очищаем контейнер от виджетов

	strcpy(people_info.nickname,gtk_entry_get_text(GTK_ENTRY(sub_list.PeopleSearchEntry)));
	/*отправляем размер никнейма*/
	sz=htonl(get_str_size(people_info.nickname));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of nickname(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MY_SUBS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем никнейм*/
	if( !(ret=sendall(sock,people_info.nickname,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL nickname(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MY_SUBS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем размер limit*/
	sz=htonl(get_str_size(limit));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of limit(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MY_SUBS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем limit*/
	if( !sendall(sock,limit,ntohl(sz)) ) {
		fprintf(stderr, "ERROR %s : SENDALL limit(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MY_SUBS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*принимаем размер id подписчика*/
	while( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL)) )
	{

		if(!ntohl(sz)) break;//если конец
		/*принимаем id подписчика*/
		if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL sub id(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MY_SUBS]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*принимаем размер никнейма подписчика*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of sub nickname(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MY_SUBS]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем никнейм подписчика*/
		if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL sub nickname(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MY_SUBS]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*принимаем когда был онлайн */
		ret=user_online(sock);

		label=gtk_label_new(text_when_online[ret]);//метка когда пользователь был онлайн
		PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
		PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_NORMAL);//настраиваем толщину метки
		PangoAttribute *attr_color;
		if(ret==ONLINE) {
			attr_color=pango_attr_foreground_new (29490,55704,5898);//настраиваем цвет метки(зеленый)
		}
		else {
			attr_color=pango_attr_foreground_new (65535,0,0);//настраиваем цвет метки(красный)
		}

		pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
		pango_attr_list_insert (attr_of_label,attr_color);//вставляем атрибут в список атрибутов

		gtk_label_set_attributes (GTK_LABEL(label),attr_of_label);//устанавливаем атрибуты для метки
		pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов
		//объявляем указатель на структуру и выделяем память
		sig_data *data=(sig_data*)malloc(sizeof(sig_data));
		//создаем список который хранит указатели на структуры чтобы освобождать память
		insert_to_list_sig_data(data,&node_sub_list);//вставляем в список структуру типа sig_data(чтобы потом освободить)
		//иницилизируем структуру(в дальнейшем передаем сигналу указатель на эту структуру)
		strcpy(data->to_id,people_info.id);//id пользователя
		strcpy(data->nickname,people_info.nickname);//никнейм пользователя
		data->ChildNotebookWidget=friend_list.Window;//передаем указатель на дочерний виджет notebook(нужно чтобы выбирать страницу на которую переходить)
		data->ShowWidget=sub_list.Scrolled;//передаем дочерний box,который мы хотим показать
		data->HideWidget=friend_list.Window;//передаем дочерний box,дочерние виджеты которого мы скроем, чтобы показать один, из которого мы вызывали функцию
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/*создаем виджет,подробности о виджете смотреть в файле messenger.glade*/
		sub_list.Button=gtk_button_new();
		gtk_button_set_relief(GTK_BUTTON(sub_list.Button),GTK_RELIEF_NONE);
		gtk_widget_set_focus_on_click(sub_list.Button,FALSE);
		gtk_widget_set_hexpand(GTK_WIDGET(sub_list.Button),TRUE);//включаем раскрытие виджета по горизонтали

		/*sub_list.image_button=gtk_image_new();
		gtk_image_set_from_icon_name(GTK_IMAGE(sub_list.image_button),"gtk-missing-image",GTK_ICON_SIZE_SMALL_TOOLBAR);*/

		sub_list.label=gtk_label_new(people_info.nickname);

		sub_list.Box3=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);
		//gtk_container_add(GTK_CONTAINER(sub_list.Box3),sub_list.image_button);
		gtk_container_add(GTK_CONTAINER(sub_list.Box3),sub_list.label);
		gtk_container_add(GTK_CONTAINER(sub_list.Box3),label);//когда был онлайн
		gtk_container_add(GTK_CONTAINER(sub_list.Button),sub_list.Box3);


		gtk_box_pack_start(GTK_BOX(sub_list.Box),sub_list.Button,0,0,0);//здесь мы добавляем grid в Box
		gtk_widget_show_all(GTK_WIDGET(sub_list.Box));
		g_signal_connect(G_OBJECT(sub_list.Button),"clicked",G_CALLBACK(show_profile_window),data);
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
  	}
  	if( ret==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV size of sub id(%d)\n",kakao_signal[SHOW_MY_SUBS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MY_SUBS]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	if(data2!=NULL)
	{
		g_signal_connect(G_OBJECT(sub_list.Button2),"clicked",G_CALLBACK(back),data2);
	}

}

void show_friend_list()
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[SHOW_FRIEND_LIST]); fflush(stderr);
	char limit[MYSQL_INT];
	sprintf(limit,"%ld",limits[LIMIT_FRIENDS]);//переводим число в строку
	uint32_t sz;
	short int ret;
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[SHOW_FRIEND_LIST]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_FRIEND_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[SHOW_FRIEND_LIST],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_FRIEND_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	clean_window(friend_list.Box);//очищаем контейнер от виджетов
	strcpy(people_info.nickname,gtk_entry_get_text(GTK_ENTRY(friend_list.PeopleSearchEntry)));

	/*отправляем размер никнейма*/
	sz=htonl(get_str_size(people_info.nickname));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of SearchEntry nickname(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_FRIEND_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем никнейм*/
	if( !(ret=sendall(sock,people_info.nickname,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL SearchEntry nickname(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_FRIEND_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем размер limit*/
	sz=htonl(get_str_size(limit));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of limit(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_FRIEND_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем limit*/
	if( !sendall(sock,limit,ntohl(sz)) ) {
		fprintf(stderr, "ERROR %s : SENDALL limit(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_FRIEND_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*принимаем размер id друга*/
	while( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL)) )
	{
		if(!ntohl(sz)) break;//если конец
		/*принимаем id друга*/
		if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL friend id(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_FRIEND_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*принимаем размер никнейма друга*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of friend nickname(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_FRIEND_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем никнейм друга*/
		if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL friend nickname(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_FRIEND_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		
		/*принимаем когда был онлайн */
		ret=user_online(sock);

		label=gtk_label_new(text_when_online[ret]);//метка когда пользователь был онлайн
		PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
		PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_NORMAL);//настраиваем толщину метки
		PangoAttribute *attr_color;
		if(ret==ONLINE) {
			attr_color=pango_attr_foreground_new (29490,55704,5898);//настраиваем цвет метки(зеленый)
		}
		else {
			attr_color=pango_attr_foreground_new (65535,0,0);//настраиваем цвет метки(красный)
		}

		pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
		pango_attr_list_insert (attr_of_label,attr_color);//вставляем атрибут в список атрибутов

		gtk_label_set_attributes (GTK_LABEL(label),attr_of_label);//устанавливаем атрибуты для метки
		pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов

		//объявляем указатель на структуру и выделяем память
		sig_data *data=(sig_data*)malloc(sizeof(sig_data));
		//создаем список который хранит указатели на структуры чтобы освобождать память
		insert_to_list_sig_data(data,&node_friend_list);//вставляем в список структуру типа sig_data(чтобы потом освободить)
		//иницилизируем структуру
		strcpy(data->to_id,people_info.id);//id друга
		strcpy(data->nickname,people_info.nickname);//никнейм друга
		data->ChildNotebookWidget=friend_list.Window;//передаем указатель на дочерний виджет notebook(нужно чтобы выбирать страницу на которую переходить)
		data->ShowWidget=friend_list.Scrolled;//передаем дочерний box,который мы хотим показать
		data->HideWidget=friend_list.Window;//передаем дочерний box,дочерние виджеты которого мы скроем, чтобы показать один, из которого мы вызывали функцию
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/*создаем виджет,подробности о виджете смотреть в файле messenger.glade*/
		friend_list.Button=gtk_button_new();
		gtk_button_set_relief(GTK_BUTTON(friend_list.Button),GTK_RELIEF_NONE);
		gtk_widget_set_focus_on_click(friend_list.Button,FALSE);
		gtk_widget_set_hexpand(GTK_WIDGET(friend_list.Button),TRUE);//включаем раскрытие виджета по горизонтали

		/*friend_list.image_button=gtk_image_new();
		gtk_image_set_from_icon_name(GTK_IMAGE(friend_list.image_button),"gtk-missing-image",GTK_ICON_SIZE_SMALL_TOOLBAR);*/

		friend_list.label=gtk_label_new(people_info.nickname);

		friend_list.Box3=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);
		//gtk_container_add(GTK_CONTAINER(friend_list.Box3),friend_list.image_button);
		gtk_container_add(GTK_CONTAINER(friend_list.Box3),friend_list.label);
		gtk_container_add(GTK_CONTAINER(friend_list.Box3),label);//когда был онлайн
		gtk_container_add(GTK_CONTAINER(friend_list.Button),friend_list.Box3);


		gtk_box_pack_start(GTK_BOX(friend_list.Box),friend_list.Button,0,0,0);//здесь мы добавляем grid в Box
		gtk_widget_show_all(GTK_WIDGET(friend_list.Box));
		g_signal_connect(G_OBJECT(friend_list.Button),"clicked",G_CALLBACK(show_profile_window),data);
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
															
	}
	if( ret==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV size of friend id(%d)\n",kakao_signal[SHOW_FRIEND_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_FRIEND_LIST]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
  		
}
void show_party_messages_list(GtkWidget *widget, sig_data* data2)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST]); fflush(stderr);
	bool flag_text=false;
	bool flag_file=false;
	bool flag_photo=false;
	char limit[MYSQL_INT];
	char buf[MAX_BUF];
	uint32_t sz;
	short int ret;
	sprintf(limit,"%ld",limits[LIMIT_MESSAGES]);//переводим число в строку

	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[SHOW_PARTY_MESSAGES_LIST]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[SHOW_PARTY_MESSAGES_LIST],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	clean_window(messages_list.Box);//очищаем контейнер от виджетов
	/*отправляем размер limit*/
	sz=htonl(get_str_size(limit));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of limit(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем limit*/
	if( !sendall(sock,limit,ntohl(sz)) ) {
		fprintf(stderr, "ERROR %s : SENDALL limit(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*принимаем размер id группы*/
	while( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL)) )
	{
		if(!ntohl(sz)) break;//если конец
		message_list.Grid=gtk_grid_new();
		/*принимаем id группы*/
		if( (ret=recvall(sock,party_info.id,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL party id(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*принимаем размер имени группы*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of party name(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем имя группы*/
		if( (ret=recvall(sock,party_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL party name(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		GtkWidget *box=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);//бокс который упаковываем spinner и label печатает;
		gtk_widget_set_hexpand(box,TRUE);
		gtk_widget_set_halign(box,GTK_ALIGN_START);//ставим box в начало

		GtkWidget *spinner=gtk_spinner_new();
		gtk_spinner_start(GTK_SPINNER(spinner));
		gtk_container_add(GTK_CONTAINER(box),spinner);

		/*принимаем размер никнейма тех кто печатает сообщение*/
		for(int i=0,c=0; (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL));++i )
		{
			if(!ntohl(sz)) {
				if(c) {
					sprintf(buf,"и еще %d человек",c);
					label=gtk_label_new(buf);
					PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
					PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_ULTRAHEAVY);//настраиваем толщину метки
				
					pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов

					gtk_label_set_attributes (GTK_LABEL(label),attr_of_label);//устанавливаем атрибуты для метки
					pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов
					gtk_container_add(GTK_CONTAINER(box),label);
				}
				break;//если конец
			}
			if(i<MAX_PARTY_NICKNAME_WRITINGS)//принимаем никнеймы только n человек
			{
				/*принимаем никнейм*/
				if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECVALL nickname(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}

				sprintf(buf,"%s печатает",people_info.nickname);
				label=gtk_label_new(buf);

				PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
				PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_MEDIUM);//настраиваем толщину метки
			
				pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов

				gtk_label_set_attributes (GTK_LABEL(label),attr_of_label);//устанавливаем атрибуты для метки
				pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов

				gtk_container_add(GTK_CONTAINER(box),label);
			}
			else//иначе если в группу пишет больше n человек, то мы просто выводим их количество
			{
				++c;
			}
			gtk_grid_attach(GTK_GRID(message_list.Grid),box,2,0,1,1);

		}
		if( ret==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of nickname(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем тип сообщения*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV mes type?(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		switch ( ntohl(sz) ) {
	        case TEXT:    
	            flag_text=true;
	            /*принимаем размер последнего сообщения*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECV size of last message with my companion TEXT(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}
				/*принимаем последнее сообщение*/
				if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECVALL last message with my companion TEXT(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}
	            break;
	        case PHOTO:
	            flag_photo=true;
	            /*принимаем размер имени фото*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECV size of last message with my companion PHOTO(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}
				/*принимаем фото*/
				if( (ret=recvall(sock,filename,ntohl(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECVALL last message with my companion PHOTO(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}
	            break;
	        case BOB_FILE:
	        	/*принимаем размер имени файла*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECV size of last message with my companion BOB_FILE(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}
				/*принимаем последнее сообщение*/
				if( (ret=recvall(sock,filename,ntohl(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECVALL last message with my companion BOB_FILE(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}
	            flag_file=true;
	            break;
    	}
		/*принимаем размер никнейма отправителя*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of sender nickname(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем никнейм отправителя*/
		if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL sender nickname(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		if(strcmpi(my_info.nickname,people_info.nickname)==0) {//если сообщение отправлено от меня
			sprintf(people_info.nickname,"Я:");
		} 
		else {//если сообщение отправлено не от меня
			sprintf(people_info.nickname,"%s:",people_info.nickname);
		}
		//объявляем указатель на структуру и выделяем память
		sig_data *data=(sig_data*)malloc(sizeof(sig_data));
		//создаем список который хранит указатели на структуры чтобы освобождать память
		insert_to_list_sig_data(data,&node_messages_list);//вставляем в список структуру типа sig_data(чтобы потом освободить)
		//иницилизируем структуру(в дальнейшем передаем сигналу указатель на эту структуру)
		strcpy(data->party_id,party_info.id);//id группы
		strcpy(data->nickname,party_info.nickname);//имя группы
		strcpy(data->dialog_type,text_dialog_type[PARTY]);//указываем тип диалога

		data->ChildNotebookWidget=messages_list.Window;//передаем указатель на дочерний виджет notebook(нужно чтобы выбирать страницу на которую переходить)
		data->ShowWidget=messages_list.Box3;//передаем дочерний box,который мы хотим показать
		data->HideWidget=dialog_window.Box;//передаем дочерний box,дочерние виджеты которого мы скроем, чтобы показать один, из которого мы вызывали функцию
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/*создаем виджет,подробности о виджете смотреть в файле messenger.glade*/
		message_list.image2=gtk_image_new();
		gtk_image_set_from_icon_name(GTK_IMAGE(message_list.image2),"gtk-missing-image",GTK_ICON_SIZE_LARGE_TOOLBAR);

		/*message_list.image_button=gtk_button_new();
		gtk_button_set_relief(GTK_BUTTON(message_list.image_button),GTK_RELIEF_NONE);
		gtk_widget_set_focus_on_click(message_list.image_button,FALSE);
		gtk_container_add(GTK_CONTAINER(message_list.image_button),message_list.image2);*/

		message_list.Box=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);

		label=gtk_label_new(people_info.nickname);//создаем метку
		//gtk_widget_set_hexpand(GTK_WIDGET(label),TRUE);//включаем раскрытие виджета по горизонтали
		gtk_widget_set_halign(label,GTK_ALIGN_START);//ставим метку в начало
		PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
		PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_ULTRAHEAVY);//настраиваем толщину метки

		pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов

		gtk_label_set_attributes (GTK_LABEL(label),attr_of_label);//устанавливаем атрибуты для метки
		pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов
		gtk_container_add(GTK_CONTAINER(message_list.Box),label);//упаковываем никнейм отправителя в box

		if(flag_text) {
			label=gtk_label_new(buf);//создаем метку
		}
		else if (flag_file) {
			sprintf(buf,"Скачать: %s",filename);
			label=gtk_label_new(buf);//создаем метку
			PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
			PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_ULTRAHEAVY);//настраиваем толщину метки

			pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов

			gtk_label_set_attributes (GTK_LABEL(label),attr_of_label);//устанавливаем атрибуты для метки
			pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов
		}
		else if(flag_photo) {
			sprintf(buf,"Фото");
			label=gtk_label_new(buf);//создаем метку
			PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
			PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_ULTRAHEAVY);//настраиваем толщину метки

			pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов

			gtk_label_set_attributes (GTK_LABEL(label),attr_of_label);//устанавливаем атрибуты для метки
			pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов
		}

		gtk_widget_set_hexpand(GTK_WIDGET(label),TRUE);//включаем раскрытие виджета по горизонтали
		gtk_widget_set_halign(label,GTK_ALIGN_START);//ставим метку в начало
		gtk_container_add(GTK_CONTAINER(message_list.Box),label);//упаковываем сообщение в box

		/*принимаем размер количества непрочитанных мной сообщений*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of quantity unread messages by me(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем количество непрочитанных мной сообщений*/
		if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL quantity unread messages by me(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		if(strcmp(buf,"0")!=0)//если есть новые непрочитанные сообщения
		{
			label=gtk_label_new(buf);//создаем метку
			gtk_widget_set_hexpand(GTK_WIDGET(label),TRUE);//включаем раскрытие виджета по горизонтали
			gtk_widget_set_halign(label,GTK_ALIGN_END);//ставим метку в начало
			PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
			PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_ULTRAHEAVY);//настраиваем толщину метки
			PangoAttribute *attr_color=pango_attr_foreground_new (65535,0,0);//настраиваем цвет метки(красный)

			pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
			pango_attr_list_insert (attr_of_label,attr_color);//вставляем атрибут в список атрибутов

			gtk_label_set_attributes (GTK_LABEL(label),attr_of_label);//устанавливаем атрибуты для метки
			pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов
			gtk_container_add(GTK_CONTAINER(message_list.Box),label);//упаковываем количество непрочитанных сообщений в box
		}

		message_list.message_button=gtk_button_new();//создаем кнопку 
		gtk_widget_set_hexpand(message_list.message_button,TRUE);//раскрываем кнопку по горизонтали
		gtk_container_add(GTK_CONTAINER(message_list.message_button),message_list.Box);//упаковываем сообщение в кнопку
		//gtk_button_set_relief(GTK_BUTTON(message_list.message_button),GTK_RELIEF_NONE);
		gtk_widget_set_focus_on_click(message_list.message_button,FALSE);

		message_list.label=gtk_label_new(data->nickname);//имя собеседника или группы
		gtk_widget_set_halign(message_list.label,GTK_ALIGN_START);//ставим метку в начало

		gtk_grid_set_row_spacing(GTK_GRID(message_list.Grid),5);
		gtk_grid_set_column_spacing(GTK_GRID(message_list.Grid),10);
		//gtk_grid_attach(GTK_GRID(message_list.Grid),message_list.image_button,0,0,1,2);
		gtk_grid_attach(GTK_GRID(message_list.Grid),message_list.label,1,0,1,1);//вставляем никнейм

		gtk_grid_attach(GTK_GRID(message_list.Grid),message_list.message_button,1,1,2,1);//вставляем кнопку с сообщением(переход в диалоговое окно)
		gtk_box_pack_start(GTK_BOX(messages_list.Box),message_list.Grid,0,0,0);//здесь мы добавляем grid в box
		gtk_widget_show_all(GTK_WIDGET(messages_list.Box));
		g_signal_connect(G_OBJECT(message_list.message_button),"clicked",G_CALLBACK(transition_to_dialog_window),data);
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	}	
	if( ret==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV size of party id(%d)\n",kakao_signal[SHOW_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_MESSAGES_LIST]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
}
void show_messages_list(GtkWidget *widget, sig_data* data2)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[SHOW_MESSAGES_LIST]); fflush(stderr);
	char buf[MAX_BUF];
	bool flag_text=false;
	bool flag_file=false;
	bool flag_photo=false;
	char limit[MYSQL_INT];
	uint32_t sz;
	short int ret;
	sprintf(limit,"%ld",limits[LIMIT_MESSAGES]);//переводим число в строку
	//free_sig_data(node_messages_list);
	//node_messages_list=NULL;
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[SHOW_MESSAGES_LIST]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[SHOW_MESSAGES_LIST],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	clean_window(messages_list.Box);//очищаем контейнер от виджетов

	/*отправляем размер limit*/
	sz=htonl(get_str_size(limit));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of limit(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем limit*/
	if( !sendall(sock,limit,ntohl(sz)) ) {
		fprintf(stderr, "ERROR %s : SENDALL limit(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}	

	/*принимаем размер id собеседника*/
	while( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL)) )
	{
		if(!ntohl(sz)) break;//если конец
		flag_text=false;
		flag_file=false;
		flag_photo=false;
		message_list.Grid=gtk_grid_new();

		/*принимаем id собеседника*/
		if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL companion id(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем размер никнейма собеседника*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of companion nickname(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем никнейм собеседника*/
		if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL companion nickname(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*принимаем печатает ли юзер сообщение */
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV writing?(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			continue;
		}

		if(ntohl(sz)) {//если печатает сообщение
			GtkWidget *box=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);//бокс который упаковываем spinner и label печатает;
			GtkWidget *spinner=gtk_spinner_new();

			gtk_spinner_start(GTK_SPINNER(spinner));

			label=gtk_label_new("печатает");

			PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
			PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_MEDIUM);//настраиваем толщину метки
		
			pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов

			gtk_label_set_attributes (GTK_LABEL(label),attr_of_label);//устанавливаем атрибуты для метки
			pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов

			gtk_container_add(GTK_CONTAINER(box),spinner);
			gtk_container_add(GTK_CONTAINER(box),label);
			gtk_widget_set_hexpand(box,TRUE);//раскрываем кнопку по горизонтали
			gtk_widget_set_halign(box,GTK_ALIGN_START);//ставим box в начало
			gtk_grid_attach(GTK_GRID(message_list.Grid),box,2,0,1,1);//вставляем когда был онлайн
		}
		else {
			/*принимаем когда был онлайн */
			ret=user_online(sock);
			message_list.label2=gtk_label_new(text_when_online[ret]);//метка когда пользователь был онлайн
			PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
			PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_NORMAL);//настраиваем толщину метки
			PangoAttribute *attr_color;
			if(ret==ONLINE) {
				attr_color=pango_attr_foreground_new (29490,55704,5898);//настраиваем цвет метки(зеленый)
			}
			else {
				attr_color=pango_attr_foreground_new (65535,0,0);//настраиваем цвет метки(красный)
			}

			pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
			pango_attr_list_insert (attr_of_label,attr_color);//вставляем атрибут в список атрибутов

			gtk_label_set_attributes (GTK_LABEL(message_list.label2),attr_of_label);//устанавливаем атрибуты для метки
			pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов

			gtk_widget_set_hexpand(message_list.label2,TRUE);//раскрываем метку по горизонтали
			gtk_widget_set_halign(message_list.label2,GTK_ALIGN_START);//ставим метку в начало
			gtk_grid_attach(GTK_GRID(message_list.Grid),message_list.label2,2,0,1,1);//вставляем когда был онлайн
		}

		/*принимаем тип сообщения*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV mes type?(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		switch ( ntohl(sz) ) {
	        case TEXT:    
	            flag_text=true;
	            /*принимаем размер последнего сообщения*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECV size of last message with my companion TEXT(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}
				/*принимаем последнее сообщение*/
				if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECVALL last message with my companion TEXT(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}
	            break;
	        case PHOTO:
	            flag_photo=true;
	            /*принимаем размер имени фото*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECV size of last message with my companion PHOTO(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}
				/*принимаем имя фото*/
				if( (ret=recvall(sock,filename,ntohl(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECVALL last message with my companion PHOTO(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}
	            break;
	        case BOB_FILE:
	        	/*принимаем размер имени файла*/
				if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECV size of last message with my companion BOB_FILE(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}
				/*принимаем имя файла*/
				if( (ret=recvall(sock,filename,ntohl(sz),MSG_WAITALL))==0 ) {
					show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
				}
				else if(ret<0) {
					fprintf(stderr, "ERROR %s : RECVALL last message with my companion BOB_FILE(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
					sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
					show_error_window(buf,NOTCLOSE);
					return;//выходим из функции, вынуждая пользователя заново отправить сигнал
				}
	            flag_file=true;
	            break;
    	}

		//объявляем указатель на структуру и выделяем память
		sig_data *data=(sig_data*)malloc(sizeof(sig_data));
		//создаем список который хранит указатели на структуры чтобы освобождать память
		insert_to_list_sig_data(data,&node_messages_list);//вставляем в список структуру типа sig_data(чтобы потом освободить)
		//иницилизируем структуру(в дальнейшем передаем сигналу указатель на эту структуру)
		strcpy(data->to_id,people_info.id);//id пользователя
		strcpy(data->nickname,people_info.nickname);//никнейм пользователя
		strcpy(data->dialog_type,text_dialog_type[COMPANION]);//указываем тип диалога

		data->ChildNotebookWidget=messages_list.Window;//передаем указатель на дочерний виджет notebook(нужно чтобы выбирать страницу на которую переходить)
		data->ShowWidget=messages_list.Box3;//передаем дочерний box,который мы хотим показать
		data->HideWidget=dialog_window.Box;//передаем дочерний box,дочерние виджеты которого мы скроем, чтобы показать один, из которого мы вызывали функцию

		/*принимаем размер никнейма отправителя*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of sender nickname(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем никнейм отправителя*/
		if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL sender nickname(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		if(strcmpi(my_info.nickname,people_info.nickname)==0) {//если сообщение отправлено от меня
			sprintf(people_info.nickname,"Я:");
		} 
		else {//если сообщение отправлено не от меня
			sprintf(people_info.nickname,"%s:",people_info.nickname);
		}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
		/*создаем виджет,подробности о виджете смотреть в файле messenger.glade*/
		message_list.image2=gtk_image_new();
		gtk_image_set_from_icon_name(GTK_IMAGE(message_list.image2),"gtk-missing-image",GTK_ICON_SIZE_LARGE_TOOLBAR);

	/*	message_list.image_button=gtk_button_new();
		gtk_button_set_relief(GTK_BUTTON(message_list.image_button),GTK_RELIEF_NONE);
		gtk_widget_set_focus_on_click(message_list.image_button,FALSE);
		gtk_container_add(GTK_CONTAINER(message_list.image_button),message_list.image2);*/

		message_list.Box=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);

		label=gtk_label_new(people_info.nickname);//создаем метку
		//gtk_widget_set_hexpand(GTK_WIDGET(label),TRUE);//включаем раскрытие виджета по горизонтали
		gtk_widget_set_halign(label,GTK_ALIGN_START);//ставим метку в начало
		PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
		PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_ULTRAHEAVY);//настраиваем толщину метки

		pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов

		gtk_label_set_attributes (GTK_LABEL(label),attr_of_label);//устанавливаем атрибуты для метки
		pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов
		gtk_container_add(GTK_CONTAINER(message_list.Box),label);//упаковываем никнейм отправителя в box

		if(flag_text) {
			label=gtk_label_new(buf);//создаем метку
		}
		else if (flag_file) {
			sprintf(buf,"Скачать: %s",filename);
			label=gtk_label_new(buf);//создаем метку
			PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
			PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_ULTRAHEAVY);//настраиваем толщину метки

			pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов

			gtk_label_set_attributes (GTK_LABEL(label),attr_of_label);//устанавливаем атрибуты для метки
			pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов
		}
		else if(flag_photo) {
			sprintf(buf,"Фото");
			label=gtk_label_new(buf);//создаем метку
			PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
			PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_ULTRAHEAVY);//настраиваем толщину метки

			pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов

			gtk_label_set_attributes (GTK_LABEL(label),attr_of_label);//устанавливаем атрибуты для метки
			pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов
		}
		gtk_widget_set_hexpand(GTK_WIDGET(label),TRUE);//включаем раскрытие виджета по горизонтали
		gtk_widget_set_halign(label,GTK_ALIGN_START);//ставим метку в начало
		gtk_container_add(GTK_CONTAINER(message_list.Box),label);//упаковываем сообщение в box

		/*принимаем размер количества непрочитанных мной сообщений*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of quantity unread messages of my companion(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем количество непрочитанных мной сообщений*/
		if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL quantity unread messages of my companion(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		if(strcmp(buf,"0")!=0)//если есть новые непрочитанные сообщения
		{
			label=gtk_label_new(buf);//создаем метку
			gtk_widget_set_hexpand(GTK_WIDGET(label),TRUE);//включаем раскрытие виджета по горизонтали
			gtk_widget_set_halign(label,GTK_ALIGN_END);//ставим метку в начало
			PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
			PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_ULTRAHEAVY);//настраиваем толщину метки
			PangoAttribute *attr_color=pango_attr_foreground_new (65535,0,0);//настраиваем цвет метки(красный)

			pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
			pango_attr_list_insert (attr_of_label,attr_color);//вставляем атрибут в список атрибутов

			gtk_label_set_attributes (GTK_LABEL(label),attr_of_label);//устанавливаем атрибуты для метки
			pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов
			gtk_container_add(GTK_CONTAINER(message_list.Box),label);//упаковываем количество непрочитанных сообщений в box
		}

		message_list.message_button=gtk_button_new();//создаем кнопку 
		gtk_widget_set_hexpand(message_list.message_button,TRUE);//раскрываем кнопку по горизонтали
		gtk_container_add(GTK_CONTAINER(message_list.message_button),message_list.Box);//упаковываем сообщение в кнопку
		//gtk_button_set_relief(GTK_BUTTON(message_list.message_button),GTK_RELIEF_NONE);
		gtk_widget_set_focus_on_click(message_list.message_button,FALSE);

		message_list.label=gtk_label_new(data->nickname);//имя собеседника или группы
		gtk_widget_set_halign(message_list.label,GTK_ALIGN_START);//ставим метку в начало

		gtk_grid_set_row_spacing(GTK_GRID(message_list.Grid),5);
		gtk_grid_set_column_spacing(GTK_GRID(message_list.Grid),10);
		//gtk_grid_attach(GTK_GRID(message_list.Grid),message_list.image_button,0,0,1,2);
		gtk_grid_attach(GTK_GRID(message_list.Grid),message_list.label,1,0,1,1);//вставляем никнейм

		gtk_grid_attach(GTK_GRID(message_list.Grid),message_list.message_button,1,1,2,1);//вставляем кнопку с сообщением(переход в диалоговое окно)
		gtk_box_pack_start(GTK_BOX(messages_list.Box),message_list.Grid,0,0,0);//здесь мы добавляем grid в box
		gtk_widget_show_all(GTK_WIDGET(messages_list.Box));
		g_signal_connect(G_OBJECT(message_list.message_button),"clicked",G_CALLBACK(transition_to_dialog_window),data);
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	}
	if( ret==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV size of companion id(%d)\n",kakao_signal[SHOW_MESSAGES_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_MESSAGES_LIST]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
		
}
void show_people_list()
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[SHOW_PEOPLE_LIST]); fflush(stderr);
	char limit[MYSQL_INT];
	uint32_t sz;
	short int ret;
	sprintf(limit,"%ld",limits[LIMIT_PEOPLES]);//переводим число в строку
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[SHOW_PEOPLE_LIST]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PEOPLE_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[SHOW_PEOPLE_LIST],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PEOPLE_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	//free_sig_data(node_people_list);//освобождаем память, которую выделили под структуру в прошлый раз
	//node_people_list=NULL;//обязательно обнуляем список перед его начальной иницилизацией
	hide_window(people_list.Window);//скрываем все дочерние виджеты контейнера 
	clean_window(people_list.Box);//очищаем список людей
	strcpy(people_info.nickname,gtk_entry_get_text(GTK_ENTRY(people_list.PeopleSearchEntry)));

	/*отправляем размер никнейма*/
	sz=htonl(get_str_size(people_info.nickname));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of SearchEntry nickname(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PEOPLE_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем никнейм*/
	if( !(ret=sendall(sock,people_info.nickname,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL SearchEntry nickname(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PEOPLE_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем размер limit*/
	sz=htonl(get_str_size(limit));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of limit(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PEOPLE_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем limit*/
	if( !sendall(sock,limit,ntohl(sz)) ) {
		fprintf(stderr, "ERROR %s : SENDALL limit(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PEOPLE_LIST]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*принимаем размер id пользователя*/
	while( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL)) )
	{
		if(!ntohl(sz)) break;//если конец
		/*принимаем id пользователя*/
		if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL people id(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PEOPLE_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*принимаем размер никнейма пользователя*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of people nickname(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PEOPLE_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем никнейм пользователя*/
		if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL people nickname(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PEOPLE_LIST]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*принимаем когда был онлайн */
		ret=user_online(sock);

		label=gtk_label_new(text_when_online[ret]);//метка когда пользователь был онлайн
		PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
		PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_NORMAL);//настраиваем толщину метки
		PangoAttribute *attr_color;
		if(ret==ONLINE) {
			attr_color=pango_attr_foreground_new (29490,55704,5898);//настраиваем цвет метки(зеленый)
		}
		else {
			attr_color=pango_attr_foreground_new (65535,0,0);//настраиваем цвет метки(красный)
		}

		pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
		pango_attr_list_insert (attr_of_label,attr_color);//вставляем атрибут в список атрибутов

		gtk_label_set_attributes (GTK_LABEL(label),attr_of_label);//устанавливаем атрибуты для метки
		pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов

		//объявляем указатель на структуру и выделяем память
		sig_data *data=(sig_data*)malloc(sizeof(sig_data));
		//создаем список который хранит указатели на структуры чтобы освобождать память
		insert_to_list_sig_data(data,&node_people_list);//вставляем в список структуру типа sig_data(чтобы потом освободить)
		//иницилизируем структуру(в дальнейшем передаем сигналу указатель на эту структуру)
		strcpy(data->to_id,people_info.id);//id пользователя
		strcpy(data->nickname,people_info.nickname);//никнейм пользователя
		//strcpy(data->dialog_type,text_dialog_type[COMPANION]);//никнейм пользователя
		data->ChildNotebookWidget=people_list.Window;//передаем указатель на дочерний виджет notebook(нужно чтобы выбирать страницу на которую переходить)
		data->ShowWidget=people_list.PeopleListBoxParrent;//передаем дочерний box,который мы хотим показать
		data->HideWidget=people_list.Window;//передаем дочерний box,дочерние виджеты которого мы скроем, чтобы показать один, из которого мы вызывали функцию

		/*создаем виджет для people_list, подробности смотреть в messenger2.glade*/
		people_list.Button=gtk_button_new();
		gtk_button_set_relief(GTK_BUTTON(people_list.Button),GTK_RELIEF_NONE);
		gtk_widget_set_focus_on_click(people_list.Button,FALSE);
		gtk_widget_set_hexpand(GTK_WIDGET(people_list.Button),TRUE);//включаем раскрытие виджета по горизонтали

		/*people_list.image_button=gtk_image_new();//изображение которое упакуем в бокс
		gtk_image_set_from_icon_name(GTK_IMAGE(people_list.image_button),"gtk-missing-image",GTK_ICON_SIZE_LARGE_TOOLBAR);*/

		people_list.label=gtk_label_new(people_info.nickname);//никнейм пользователя

		people_list.Box3=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);//бокс который упаковываем в кнопку
		//gtk_container_add(GTK_CONTAINER(people_list.Box3),people_list.image_button);
		gtk_container_add(GTK_CONTAINER(people_list.Box3),people_list.label);
		gtk_container_add(GTK_CONTAINER(people_list.Box3),label);
		gtk_container_add(GTK_CONTAINER(people_list.Button),people_list.Box3);


		gtk_box_pack_start(GTK_BOX(people_list.Box),people_list.Button,0,0,0);//здесь мы добавляем grid в Box
		gtk_widget_show_all(GTK_WIDGET(people_list.Box));
		g_signal_connect(G_OBJECT(people_list.Button),"clicked",G_CALLBACK(show_profile_window),data);
	}
	if( ret==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV size of people id(%d)\n",kakao_signal[SHOW_PEOPLE_LIST],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PEOPLE_LIST]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}


	gtk_widget_show(people_list.PeopleListBoxParrent);//показываем наш виджет списка людей
}
void show_error_window(char *err_text,int flag)
{
	/*отключаем все сигналы*/
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(error_window.Button),G_SIGNAL_MATCH_FUNC,0,0,NULL,hide_error_window,0); while(ret!=0);
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(error_window.Button),G_SIGNAL_MATCH_FUNC,0,0,NULL,kill_error_window,0); while(ret!=0);

	gtk_label_set_text(GTK_LABEL(error_window.TextBox),err_text);//устанавливаем текст ошибки
	gtk_widget_show_all(error_window.Window);
	fprintf(stderr, "%s",err_text ); fflush(stderr);//записываем в лог ошибку
	if(flag==CLOSE){//закрываем приложение
		g_signal_connect(G_OBJECT(error_window.Button),"clicked",G_CALLBACK(kill_error_window),NULL);
		/*for(long int i=time(NULL);(time(NULL)-i)<=3;) {
			printf("DDDDDDDDDDDDDDDddd\n");
			usleep(1000000/2);
		}*/	
	}
	else {
		g_signal_connect(G_OBJECT(error_window.Button),"clicked",G_CALLBACK(hide_error_window),NULL);
	}
}
void hide_error_window(GtkWidget *widget,sig_data* data)//пользователь нажал кнопку закрыть
{
	fprintf(stderr,"SIGNAL: HIDE_ERROR_WINDOW\n"); fflush(stderr);
	gtk_widget_hide(error_window.Window);
	gtk_label_set_text(GTK_LABEL(error_window.TextBox),"Какая-то ошибка");//устанавливаем текст ошибки
}
void kill_error_window(GtkWidget *widget,sig_data* data)//пользователь нажал кнопку закрыть
{
	fprintf(stderr,"SIGNAL: KILL_ERROR_WINDOW\n"); fflush(stderr);
	//gtk_main_quit();
	close_and_free_all();
	exit(0);
}
void add_limit(GtkWidget *widget,GtkPositionType pos,sig_data* data)
{
	if(pos==GTK_POS_BOTTOM)
	{
		if(widget==people_list.Scrolled)
		{
			fprintf(stderr,"SIGNAL: ADD_LIMIT - people\n"); fflush(stderr);
			limits[LIMIT_PEOPLES]+=LIMITS[LIMIT_PEOPLES];
			show_people_list();
		}
		else if(widget==friend_list.Scrolled)
		{
			fprintf(stderr,"SIGNAL: ADD_LIMIT - friend\n"); fflush(stderr);
			limits[LIMIT_FRIENDS]+=LIMITS[limits[LIMIT_FRIENDS]];
			show_friend_list();
		}
		else if(widget==sub_list.Scrolled)
		{
			fprintf(stderr,"SIGNAL: ADD_LIMIT - sub\n"); fflush(stderr);
			limits[LIMIT_MY_SUBS]+=LIMITS[LIMIT_MY_SUBS];
			show_my_subs(NULL,NULL);
		}
		else if(widget==messages_list.Scrolled)
		{
			fprintf(stderr,"SIGNAL: ADD_LIMIT - messages\n"); fflush(stderr);
			limits[LIMIT_MESSAGES]+=LIMITS[LIMIT_MESSAGES]; 
			if(flag_messages_list) {
				show_messages_list(NULL,NULL);
			}
			else if(flag_party_messages_list) {
				show_party_messages_list(NULL,NULL);
			}
		}
		else if(widget==profile_party.Scrolled)
		{
			fprintf(stderr,"SIGNAL: ADD_LIMIT - profile_party\n"); fflush(stderr);
			limits[LIMIT_PARTY]+=LIMITS[LIMIT_PARTY];
			show_profile_party(NULL,data);
		}
		else if(widget==party_peoples_window.Scrolled)
		{
			fprintf(stderr,"SIGNAL: ADD_LIMIT - party_peoples_window\n"); fflush(stderr);
			limits[LIMIT_PARTY_PEOPLES] +=LIMITS[LIMIT_PARTY_PEOPLES];
			show_party_appoint_admins(widget,data);
		}
	}
	else if(pos==GTK_POS_TOP)
	{
		if(widget==dialog_window.Scrolled)
		{
			fprintf(stderr,"SIGNAL: ADD_LIMIT - dialog\n"); fflush(stderr);
			gdouble pos;
			limits[LIMIT_DIALOG]+=LIMITS[LIMIT_DIALOG];
			show_messages(NULL,data);
			//pos=gtk_adjustment_get_value(GTK_ADJUSTMENT(dialog_window.Adjustment));
			//printf("%f %f\n",gtk_adjustment_get_value(GTK_ADJUSTMENT(dialog_window.Adjustment)), gtk_adjustment_get_lower (GTK_ADJUSTMENT(dialog_window.Adjustment)));
			//gtk_adjustment_set_value(GTK_ADJUSTMENT(dialog_window.Adjustment),gtk_adjustment_get_lower (GTK_ADJUSTMENT(dialog_window.Adjustment))-pos);//пролистываем окно в конец
		}
	}
}
void nullify_limit(GtkWidget *widget, sig_data* data)
{
	if(widget==people_list.Scrolled)
	{
		fprintf(stderr,"SIGNAL: NULLIFY_LIMIT - people\n"); fflush(stderr);
		limits[LIMIT_PEOPLES]=LIMITS[LIMIT_PEOPLES];
		show_people_list();
	}
	else if(widget==friend_list.Scrolled)
	{
		fprintf(stderr,"SIGNAL: NULLIFY_LIMIT - friend\n"); fflush(stderr);
		limits[LIMIT_FRIENDS]=LIMITS[LIMIT_FRIENDS];
		show_friend_list();
	}
	else if(widget==sub_list.Scrolled)
	{
		fprintf(stderr,"SIGNAL: NULLIFY_LIMIT - sub\n"); fflush(stderr);
		limits[LIMIT_MY_SUBS]=LIMITS[LIMIT_MY_SUBS];
		show_my_subs(NULL,NULL);
	}
	else if(widget==messages_list.Scrolled)
	{
		fprintf(stderr,"SIGNAL: NULLIFY_LIMIT - message\n"); fflush(stderr);
		limits[LIMIT_MESSAGES]=LIMITS[LIMIT_MESSAGES];
		//show_messages_list(NULL,NULL);
	}
	else if(widget==dialog_window.Scrolled)
	{
		fprintf(stderr,"SIGNAL: NULLIFY_LIMIT - dialog\n"); fflush(stderr);
		limits[LIMIT_DIALOG]=LIMITS[LIMIT_DIALOG];
		flag_dialog=FALSE;
		//ransition_to_messages_list(NULL,NULL);
	}
	else if(widget==profile_party.Scrolled)
	{
		fprintf(stderr,"SIGNAL: NULLIFY_LIMIT - party\n"); fflush(stderr);
		limits[LIMIT_PARTY]=LIMITS[LIMIT_PARTY];
		//ransition_to_messages_list(NULL,NULL);
	}
	else if(widget==party_peoples_window.Scrolled)
	{
		fprintf(stderr,"SIGNAL: NULLIFY_LIMIT - party_peoples_window\n"); fflush(stderr);
		limits[LIMIT_PARTY_PEOPLES]=LIMITS[LIMIT_PARTY_PEOPLES];
		//ransition_to_messages_list(NULL,NULL);
	}

}

void show_profile_window(GtkWidget *button, sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[SHOW_PROFILE_WINDOW]); fflush(stderr);

	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[SHOW_PROFILE_WINDOW]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_WINDOW]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[SHOW_PROFILE_WINDOW],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_WINDOW]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*отправляем размер id профиля*/
	sz=htonl(get_str_size(data->to_id));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of profile id(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_WINDOW]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем id профиля*/
	if( !(ret=sendall(sock,data->to_id,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL profile id(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_WINDOW]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	sig_data *data2=(sig_data*)malloc(sizeof(sig_data));
	strcpy(data2->to_id,data->to_id);
	strcpy(data2->nickname,data->nickname);//никнейм пользователя
	strcpy(data2->dialog_type,text_dialog_type[COMPANION]);//тип диалога
	data2->ChildNotebookWidget=people_list.Window;//передаем указатель на дочерний виджет notebook(нужно чтобы выбирать страницу на которую переходить)
	data2->ShowWidget=people_profile.Window;//передаем дочерний box,который мы хотим показать
	data2->HideWidget=people_list.Window;//передаем дочерний box,дочерние виджеты которого мы скроем, чтобы показать один, из которого мы вызывали функцию
	insert_to_list_sig_data(data2,&node_profile_window);
	//удаляем ВСЕ сигналы связанные с этим виджетом
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(people_profile.Button),G_SIGNAL_MATCH_FUNC,0,0,NULL,delete_friend,0); while(ret!=0);
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(people_profile.Button),G_SIGNAL_MATCH_FUNC,0,0,NULL,add_friend,0); while(ret!=0);
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(answer_app_profile.Button1),G_SIGNAL_MATCH_FUNC,0,0,NULL,add_sub,0); while(ret!=0);
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(answer_app_profile.Button2),G_SIGNAL_MATCH_FUNC,0,0,NULL,delete_sub,0); while(ret!=0);
	/*удаляем сигналы кнопки обновить профиля*/
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(people_profile.Button3),G_SIGNAL_MATCH_FUNC,0,0,NULL,show_profile_window,0); while(ret!=0);
	/*удаляем сигналы кнопки назад*/
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(people_profile.Button2),G_SIGNAL_MATCH_FUNC,0,0,NULL,back,0); while(ret!=0);
	/*удаляем сигналы кнопки отправить сообщение*/
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(people_profile.message_button),G_SIGNAL_MATCH_FUNC,0,0,NULL,transition_to_dialog_window,0); while(ret!=0);
	/*удаляем сигналы кнопки заблокировать*/
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(people_profile.Button4),G_SIGNAL_MATCH_FUNC,0,0,NULL,block_user,0); while(ret!=0);
	/*удаляем сигналы кнопки разблокировать*/
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(people_profile.Button5),G_SIGNAL_MATCH_FUNC,0,0,NULL,unblock_user,0); while(ret!=0);
	/*удаляем сигнал кнопки показа полного фото*/
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(people_profile.Button6),G_SIGNAL_MATCH_FUNC,0,0,NULL,show_photo_full,0); while(ret!=0);

	/*подключаем сигнал к кнопке отправить сообщение*/
	g_signal_connect(G_OBJECT(people_profile.message_button),"clicked",G_CALLBACK(transition_to_dialog_window),data2);
	/*подключаем сигнал к кнопке обновить*/
	g_signal_connect(G_OBJECT(people_profile.Button3),"clicked",G_CALLBACK(show_profile_window),data);
	/*подключаем сигнал к кнопке назад в профиле*/
	g_signal_connect(G_OBJECT(people_profile.Button2),"clicked",G_CALLBACK(back),data);
	/*подключаем сигнал к кнопке заблокировать*/
	g_signal_connect(G_OBJECT(people_profile.Button4),"clicked",G_CALLBACK(block_user),data);
	/*подключаем сигнал к кнопке разблокировать*/
	g_signal_connect(G_OBJECT(people_profile.Button5),"clicked",G_CALLBACK(unblock_user),data);

	ret = gtk_notebook_page_num(GTK_NOTEBOOK(main_window.Notebook),people_list.Window);//получаем номер страницы поиска людей(здесь хранится виджет профиля)
	gtk_notebook_set_current_page(GTK_NOTEBOOK(main_window.Notebook),ret);//выбираем страницу поиска
	hide_window(people_list.Window);//скрываем все дочерние виджеты контейнера 
	gtk_widget_hide(people_profile.MenuButton);//скрываем кнопку меню профиля
	gtk_label_set_text(GTK_LABEL(people_profile.label),data2->nickname);//устанавливаем никнейм профиля

	/*принимаем когда был онлайн */
	ret=user_online(sock);

	gtk_label_set_text(GTK_LABEL(people_profile.label2),text_when_online[ret]);//устанавливаем когда пользователь был онлайн
	PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
	PangoAttribute *attr_color;
	if(ret==ONLINE) {
		attr_color=pango_attr_foreground_new (29490,55704,5898);//настраиваем цвет метки(зеленый)
	}
	else {
		attr_color=pango_attr_foreground_new (65535,0,0);//настраиваем цвет метки(красный)
	}

	pango_attr_list_insert (attr_of_label,attr_color);//вставляем атрибут в список атрибутов

	gtk_label_set_attributes (GTK_LABEL(people_profile.label2),attr_of_label);//устанавливаем атрибуты для метки
	pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов

	/*принимаем заблокировали ли мы пользователя*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV did we block the user?(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_WINDOW]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	if(ntohl(sz)) {//если мы заблокировали пользователя
		hide_window(people_profile.Box);
		gtk_widget_show(people_profile.Button5);//показываем кнопку разблокировать
	} 
	else {//если мы не заблокировали пользователя
		hide_window(people_profile.Box);
		gtk_widget_show(people_profile.Button4);//показываем кнопку заблокировать
	}

	/*принимаем заблокированы ли мы*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV do we block?(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_WINDOW]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	if(ntohl(sz)) {//если мы заблокированы
		gtk_button_set_label(GTK_BUTTON(people_profile.Button),"Мы заблокированы");
		gtk_widget_show(people_profile.Window);//показываем профиль
		return;//выходим 
	} 

	/*принимаем ответ имеет ли пользователь аватарку*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV does the user have an avatar?(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_WINDOW]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	if(ntohl(sz)) {//если юзер имеет аватарку
		char buf[(MAX_PATH*2)+256];

		snprintf(data->path,sizeof(data->path),"%s/%s",PATHS[PATH_USER_AVATARS],data->to_id);//создаем путь к дирректории с id пользователя
		sprintf(buf,"if ! [ -d \"%s\" ]; then mkdir -p %s; fi",data->path,data->path);//создаем команду для создания дирректории
		system(buf);//создаем каталог если это нужно

		/*принимаем размер имени изображения*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of name of the image(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_WINDOW]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем имя изображения*/
		if( (ret=recvall(sock,filename,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL name of the image(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_WINDOW]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		sprintf(data->path,"%s/%s/%s",PATHS[PATH_USER_AVATARS],data->to_id,filename);//путь до аватарки
		recv_file(sock,data->path,NULL);//принимаем аватарку
		gint width;
		gint height;
		gtk_window_get_size (GTK_WINDOW (main_window.Window), &width, &height);
		GdkPixbuf *pixbuf=gdk_pixbuf_new_from_file_at_size (data->path,workarea.width/2,height,NULL);
		gtk_image_set_from_pixbuf(GTK_IMAGE(people_profile.image2),pixbuf);//устанавливаем аватарку

		g_signal_connect(G_OBJECT(people_profile.Button6),"clicked",G_CALLBACK(show_photo_full),data);//назначаем сигнал кнопке показа полного фото

		gtk_widget_show(people_profile.Window);//показываем профиль
	} 
	else {
		gtk_image_set_from_icon_name(GTK_IMAGE(people_profile.image2),"gtk-missing-image",GTK_ICON_SIZE_DIALOG);
	}


	/*принимаем размер количества записей*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV size of quantity entrys(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_WINDOW]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*принимаем количество записей*/
	if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECVALL quantity entrys(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_WINDOW]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	if(strcmp(buf,"0")==0)//если его нет в друзьях
	{
		gtk_button_set_label(GTK_BUTTON(people_profile.Button),"Отправить заявку");
		g_signal_connect(G_OBJECT(people_profile.Button),"clicked",G_CALLBACK(add_friend),data);
	}
	else if (strcmp(buf,"1")==0)//если кто-то кому-то отправил заявку в друзья
	{
		/*принимаем размер количества записей*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of quantity entrys who is sub(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_WINDOW]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем количество записей*/
		if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL quantity entrys who is sub(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_WINDOW]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		if(atoi(buf)>0)//если мы отправили запрос в друзья
		{
			gtk_button_set_label(GTK_BUTTON(people_profile.Button),"Отменить заявку");
			g_signal_connect(G_OBJECT(people_profile.Button),"clicked",G_CALLBACK(delete_friend),data);
		}
		else//если нам отправили запрос в друзья
		{
			gtk_button_set_label(GTK_BUTTON(people_profile.Button),"Ответить на заявку");
			g_signal_connect(G_OBJECT(answer_app_profile.Button1),"clicked",G_CALLBACK(add_sub),data);
			g_signal_connect(G_OBJECT(answer_app_profile.Button2),"clicked",G_CALLBACK(delete_sub),data);
			gtk_widget_show(people_profile.MenuButton);		
			//gtk_revealer_set_reveal_child(GTK_REVEALER(people_profile.Revealer),TRUE);
		}
	}
	else if(strcmp(buf,"2")==0)//если он есть в друзьях
	{
		gtk_button_set_label(GTK_BUTTON(people_profile.Button),"Удалить из друзей");
		g_signal_connect(G_OBJECT(people_profile.Button),"clicked",G_CALLBACK(delete_friend),data);
	}

	gtk_widget_show(people_profile.Window);//показываем профиль
}
void block_user(GtkWidget *button, sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[BLOCK_USER]); fflush(stderr);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[BLOCK_USER]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[BLOCK_USER],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BLOCK_USER]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[BLOCK_USER],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[BLOCK_USER],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BLOCK_USER]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем размер id пользователя*/
	sz=htonl(get_str_size(data->to_id));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of people_id(%d)\n",kakao_signal[BLOCK_USER],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BLOCK_USER]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем id пользователя*/
	if( !(ret=sendall(sock,data->to_id,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL people_id(%d)\n",kakao_signal[BLOCK_USER],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BLOCK_USER]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	hide_window(people_profile.Box);
	gtk_widget_show(people_profile.Button5);
}
void unblock_user(GtkWidget *button, sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[UNBLOCK_USER]); fflush(stderr);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[UNBLOCK_USER]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[UNBLOCK_USER],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[UNBLOCK_USER]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[UNBLOCK_USER],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[UNBLOCK_USER],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[UNBLOCK_USER]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем размер id пользователя*/
	sz=htonl(get_str_size(data->to_id));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of people_id(%d)\n",kakao_signal[UNBLOCK_USER],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[UNBLOCK_USER]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем id пользователя*/
	if( !(ret=sendall(sock,data->to_id,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL people_id(%d)\n",kakao_signal[UNBLOCK_USER],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[UNBLOCK_USER]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	hide_window(people_profile.Box);
	gtk_widget_show(people_profile.Button4);

}
void show_profile_party(GtkWidget *button, sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[SHOW_PROFILE_PARTY]); fflush(stderr);

	/*удаляем сигналы поля названия группы при нажатии enter*/
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(profile_party.Entry1),G_SIGNAL_MATCH_FUNC,0,0,NULL,change_party_name,0); while(ret!=0);

	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(profile_party.Scrolled),G_SIGNAL_MATCH_FUNC,0,0,NULL,add_limit,0); while(ret!=0);
	//do ret=g_signal_handlers_disconnect_matched(G_OBJECT(profile_party.PeopleSearchEntry),G_SIGNAL_MATCH_FUNC,0,0,NULL,show_profile_party_users,0); while(ret!=0);//удаляем сигнал search entry
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(profile_party.Button6),G_SIGNAL_MATCH_FUNC,0,0,NULL,change_party_avatar,0); while(ret!=0);//удаляем сигнал у кнопки изменить аватар
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(profile_party.Button7),G_SIGNAL_MATCH_FUNC,0,0,NULL,delete_party_avatar,0); while(ret!=0);//удаляем сигнал у кнопки удалить аватар
	/*удаляем сигнал кнопки показа полного фото*/
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(profile_party.Button5),G_SIGNAL_MATCH_FUNC,0,0,NULL,show_photo_full,0); while(ret!=0);
	//do ret=g_signal_handlers_disconnect_matched(G_OBJECT(profile_party.PeopleSearchEntry),G_SIGNAL_MATCH_FUNC,0,0,NULL,show_party_add_admins,0); while(ret!=0);
	/*подключаем сигнал к кнопке назад в профиле*/
	g_signal_connect(G_OBJECT(profile_party.Button),"clicked",G_CALLBACK(back),data);

	g_signal_connect(G_OBJECT(profile_party.Scrolled),"edge_reached",G_CALLBACK(add_limit),data);
	//g_signal_connect(G_OBJECT(profile_party.PeopleSearchEntry),"search-changed",G_CALLBACK(show_profile_party_users),data);//подключаем сигнал к searchentry

	gint width;
	gint height;
	gtk_window_get_size (GTK_WINDOW (main_window.Window), &width, &height);
	gtk_widget_set_size_request(profile_party.Scrolled,width,height/3);//устанавливаем размер окна с участниками группы

	gtk_entry_set_text(GTK_ENTRY(profile_party.Entry1),data->nickname);//устанавливаем имя группы
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[SHOW_PROFILE_PARTY]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[SHOW_PROFILE_PARTY],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*принимаем ответ являюсь ли я админом*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV answer is an admin?(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	hide_window(popover_profile_party.Box3);//скрываем все виджета popoverа
	if(ntohl(sz)>0) //если я админ
	{
		gtk_widget_show(popover_profile_party.Box);//показываем весь бокс
		gtk_editable_set_editable(GTK_EDITABLE(profile_party.Entry1),true);//разрешаем редактировать название группы
		gtk_text_view_set_editable(GTK_TEXT_VIEW(profile_party.TextView),true);//разрешаем редактировать название группы
		gtk_widget_show(profile_party.Button2);//показываем кнопку изменить описание
		/*подключаем сигнал к полю названия группы при нажатии enter*/
		g_signal_connect(G_OBJECT(profile_party.Entry1),"activate",G_CALLBACK(change_party_name),NULL);
		/*подключаем сигнал к кнопке изменить аватар*/
		g_signal_connect(G_OBJECT(profile_party.Button6),"clicked",G_CALLBACK(change_party_avatar),data);
		/*подключаем сигнал к кнопке удалить аватар*/
		g_signal_connect(G_OBJECT(profile_party.Button7),"clicked",G_CALLBACK(delete_party_avatar),data);
		gtk_widget_show(profile_party.MenuButton);//показываем кнопку настроек аватарки
		flag_admin=true;
		//g_signal_connect(G_OBJECT(profile_party.Entry1),"activate",G_CALLBACK(show_party_add_admins),data);
	}
	else//если я не админ
	{
		/*принимаем ответ являюсь ли я пользователем группы*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV answer is an user?(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		if(!ntohl(sz)) return;
		gtk_editable_set_editable(GTK_EDITABLE(profile_party.Entry1),false);//запрещаем редактировать название группы
		gtk_text_view_set_editable(GTK_TEXT_VIEW(profile_party.TextView),false);//запрещаем редактировать название группы
		gtk_widget_hide(profile_party.Button2);//показываем кнопку изменить описание
		gtk_widget_hide(profile_party.MenuButton);//скрываем кнопку настроек аватарки
		flag_admin=false;
	}
	/*принимаем размер описания группы*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV size of description of the party(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_WINDOW]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*принимаем описание группы*/
	if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECVALL description of the party(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_WINDOW]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	gtk_text_buffer_set_text (party_buffer,buf,-1);//устанавливаем описание группы
	/*принимаем ответ имеет ли группа аватарку*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV does the party have an avatar?(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	if(ntohl(sz)) {//если группа имеет аватарку
		char buf[(MAX_PATH*2)+256];

		snprintf(data->path,sizeof(data->path),"%s/%s",PATHS[PATH_PARTY_AVATARS],data->party_id);//создаем путь к дирректории с id пользователя
		sprintf(buf,"if ! [ -d \"%s\" ]; then mkdir -p %s; fi",data->path,data->path);//создаем команду для создания дирректории
		system(buf);//создаем каталог если это нужно

		/*принимаем размер имени изображения*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of name of the image(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем имя изображения*/
		if( (ret=recvall(sock,filename,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL name of the image(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		sprintf(data->path,"%s/%s/%s",PATHS[PATH_PARTY_AVATARS],data->party_id,filename);//путь до аватарки
		recv_file(sock,data->path,NULL);//принимаем аватарку
		gint width;
		gint height;
		gtk_window_get_size (GTK_WINDOW (main_window.Window), &width, &height);
		GdkPixbuf *pixbuf=gdk_pixbuf_new_from_file_at_size (data->path,width,height,NULL);
		gtk_image_set_from_pixbuf(GTK_IMAGE(profile_party.image3),pixbuf);//устанавливаем аватарку

		g_signal_connect(G_OBJECT(profile_party.Button5),"clicked",G_CALLBACK(show_photo_full),data);//назначаем сигнал кнопке показа полного фото
	} 
	else {
		gtk_image_set_from_icon_name(GTK_IMAGE(profile_party.image3),"gtk-missing-image",GTK_ICON_SIZE_DIALOG);
	}

	gtk_widget_show(popover_profile_party.Box2);//показываем бокс обычного юзера

	strcpy(people_info.nickname,gtk_entry_get_text(GTK_ENTRY(profile_party.PeopleSearchEntry)));

	char limit[MYSQL_INT];
	sprintf(limit,"%ld",limits[LIMIT_PARTY]);//переводим число в строку
	/*отправляем размер никнейма*/
	sz=htonl(get_str_size(people_info.nickname));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of SearchEntry nickname(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем никнейм*/
	if( !(ret=sendall(sock,people_info.nickname,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL SearchEntry nickname(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем размер limit*/
	sz=htonl(get_str_size(limit));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of limit(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	//отправляем limit
	if( !sendall(sock,limit,ntohl(sz)) ) {
		fprintf(stderr, "ERROR %s : SENDALL limit(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	clean_window(profile_party.Box);//очищаем контейнер от виджетов
	/*принимаем размер id пользователя*/
	while( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL)) )
	{
		if(!ntohl(sz)) break;//если конец
		/*принимаем id пользователя*/
		if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL people id(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*принимаем размер никнейма пользователя*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of people nickname(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем никнейм пользователя*/
		if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL people nickname(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*принимаем когда был онлайн */
		ret=user_online(sock);

		profile_party.label=gtk_label_new(text_when_online[ret]);//метка когда пользователь был онлайн
		PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
		PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_NORMAL);//настраиваем толщину метки
		PangoAttribute *attr_color;
		if(ret==ONLINE) {
			attr_color=pango_attr_foreground_new (29490,55704,5898);//настраиваем цвет метки(зеленый)
		}
		else {
			attr_color=pango_attr_foreground_new (65535,0,0);//настраиваем цвет метки(красный)
		}

		pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
		pango_attr_list_insert (attr_of_label,attr_color);//вставляем атрибут в список атрибутов

		gtk_label_set_attributes (GTK_LABEL(profile_party.label),attr_of_label);//устанавливаем атрибуты для метки
		pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов

		/*принимаем ответ сервера админ ли юзер*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV answer is user an admin(%d)\n",kakao_signal[SHOW_PROFILE_PARTY],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		sig_data *data2=(sig_data*)malloc(sizeof(sig_data));
		strcpy(data2->to_id,people_info.id);//id пользователя
		strcpy(data2->nickname,people_info.nickname);//имя пользователя
		data2->ChildNotebookWidget=messages_list.Window;//передаем указатель на дочерний виджет notebook(нужно чтобы выбирать страницу на которую переходить)
		data2->ShowWidget=profile_party.Window;//передаем дочерний box,который мы хотим показать
		data2->HideWidget=messages_list.Window;//передаем дочерний box,дочерние виджеты которого мы скроем, чтобы показать один, из которого мы вызывали функцию
		insert_to_list_sig_data(data2,&node_profile_party);
		/*создаем виджет для profile_party, подробности смотреть в messenger2.glade*/
		profile_party.Button3=gtk_button_new();
		gtk_widget_set_hexpand(profile_party.Button3,TRUE);//раскрываем кнопку по горизонтали
		gtk_button_set_relief(GTK_BUTTON(profile_party.Button3),GTK_RELIEF_NONE);
		gtk_widget_set_focus_on_click(profile_party.Button3,FALSE);	

		label=gtk_label_new(people_info.nickname);//создаем метку
		gtk_widget_set_halign(label,GTK_ALIGN_START);//ставим метку в начало

		profile_party.image2=gtk_image_new();
		gtk_image_set_from_icon_name(GTK_IMAGE(profile_party.image2),"gtk-missing-image",GTK_ICON_SIZE_SMALL_TOOLBAR);

		profile_party.Box2=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);
		profile_party.Box3=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);

		gtk_container_add(GTK_CONTAINER(profile_party.Button3),profile_party.Box2);//упаковываем контейнер в кнопку
		gtk_container_add(GTK_CONTAINER(profile_party.Box2),profile_party.image2);//упаковываем фото в контейнер
		gtk_container_add(GTK_CONTAINER(profile_party.Box2),label);//упаковываем метку никнейм в контейнер
		gtk_container_add(GTK_CONTAINER(profile_party.Box3),profile_party.Button3);		
		if(ntohl(sz))//если пользователь админ
		{
			label=gtk_label_new("admin");
			PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
			PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_MEDIUM);//настраиваем толщину метки
			PangoAttribute *attr_color=pango_attr_foreground_new (29490,55704,5898);//настраиваем цвет метки(зеленый)

			pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
			pango_attr_list_insert (attr_of_label,attr_color);//вставляем атрибут в список атрибутов

			gtk_label_set_attributes (GTK_LABEL(label),attr_of_label);//устанавливаем атрибуты для метки
			pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов


			gtk_container_add(GTK_CONTAINER(profile_party.Box2),label);//упаковываем метку админ в контейнер
		}					
		if(flag_admin)
		{
			profile_party.Button4=gtk_button_new();
			//gtk_widget_set_hexpand(profile_party.Button4,TRUE);//раскрываем кнопку по горизонтали
			//gtk_widget_set_halign(profile_party.Button4,GTK_ALIGN_END);
			profile_party.image_button=gtk_image_new();
			gtk_image_set_from_icon_name(GTK_IMAGE(profile_party.image_button),"gtk-close",GTK_ICON_SIZE_SMALL_TOOLBAR);
			gtk_container_add(GTK_CONTAINER(profile_party.Button4),profile_party.image_button);
			gtk_container_add(GTK_CONTAINER(profile_party.Box3),profile_party.Button4);//упаковываем кнопку-фото в контейнер
			gtk_button_set_relief(GTK_BUTTON(profile_party.Button4),GTK_RELIEF_NONE);
			gtk_widget_set_focus_on_click(profile_party.Button4,FALSE);	
			g_signal_connect(G_OBJECT(profile_party.Button4),"clicked",G_CALLBACK(party_delete_user),data2);
		}
		gtk_container_add(GTK_CONTAINER(profile_party.Box2),profile_party.label);//упаковываем метку когда был онлайн в контейнер

		gtk_box_pack_start(GTK_BOX(profile_party.Box),profile_party.Box3,0,0,0);
		gtk_widget_show_all(profile_party.Box);
		g_signal_connect(G_OBJECT(profile_party.Button3),"clicked",G_CALLBACK(show_profile_window),data2);

	}
	if( ret==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV size of people id(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_WINDOW]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
}
void show_profile_party_avatar(GtkWidget *back_button, sig_data *data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[SHOW_PROFILE_PARTY_AVATAR]); fflush(stderr);
	/*удаляем сигнал кнопки показа полного фото*/
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(profile_party.Button5),G_SIGNAL_MATCH_FUNC,0,0,NULL,show_photo_full,0); while(ret!=0);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[SHOW_PROFILE_PARTY_AVATAR]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_AVATAR],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY_AVATAR]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[SHOW_PROFILE_PARTY_AVATAR],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_AVATAR],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY_AVATAR]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*принимаем ответ имеет ли группа аватарку*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV does the party have an avatar?(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_AVATAR],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY_AVATAR]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	if(ntohl(sz)) {//если группа имеет аватарку
		char buf[(MAX_PATH*2)+256];

		snprintf(data->path,sizeof(data->path),"%s/%s",PATHS[PATH_PARTY_AVATARS],data->party_id);//создаем путь к дирректории с id пользователя
		sprintf(buf,"if ! [ -d \"%s\" ]; then mkdir -p %s; fi",data->path,data->path);//создаем команду для создания дирректории
		system(buf);//создаем каталог если это нужно

		/*принимаем размер имени изображения*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of name of the image(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_AVATAR],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY_AVATAR]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем имя изображения*/
		if( (ret=recvall(sock,filename,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL name of the image(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_AVATAR],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY_AVATAR]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		sprintf(data->path,"%s/%s/%s",PATHS[PATH_PARTY_AVATARS],data->party_id,filename);//путь до аватарки
		recv_file(sock,data->path,NULL);//принимаем аватарку
		gint width;
		gint height;
		gtk_window_get_size (GTK_WINDOW (main_window.Window), &width, &height);
		GdkPixbuf *pixbuf=gdk_pixbuf_new_from_file_at_size (data->path,width,height,NULL);
		gtk_image_set_from_pixbuf(GTK_IMAGE(profile_party.image3),pixbuf);//устанавливаем аватарку

		g_signal_connect(G_OBJECT(profile_party.Button5),"clicked",G_CALLBACK(show_photo_full),data);//назначаем сигнал кнопке показа полного фото
	} 
	else {
		gtk_image_set_from_icon_name(GTK_IMAGE(profile_party.image3),"gtk-missing-image",GTK_ICON_SIZE_DIALOG);
	}
}
void show_profile_party_users(GtkWidget *widget,sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[SHOW_PROFILE_PARTY_USERS]); fflush(stderr);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[SHOW_PROFILE_PARTY_USERS]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size  signal(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY_USERS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[SHOW_PROFILE_PARTY_USERS],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY_USERS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	strcpy(people_info.nickname,gtk_entry_get_text(GTK_ENTRY(profile_party.PeopleSearchEntry)));

	char limit[MYSQL_INT];
	sprintf(limit,"%ld",limits[LIMIT_PARTY]);//переводим число в строку
	/*отправляем размер никнейма*/
	sz=htonl(get_str_size(people_info.nickname));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of SearchEntry nickname(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY_USERS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем никнейм*/
	if( !(ret=sendall(sock,people_info.nickname,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL SearchEntry nickname(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY_USERS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем размер limit*/
	sz=htonl(get_str_size(limit));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of limit(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY_USERS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	//отправляем limit
	if( !sendall(sock,limit,ntohl(sz)) ) {
		fprintf(stderr, "ERROR %s : SENDALL limit(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY_USERS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	clean_window(profile_party.Box);//очищаем контейнер от виджетов
	/*принимаем размер id пользователя*/
	while( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL)) )
	{
		if(!ntohl(sz)) break;//если конец
		/*принимаем id пользователя*/
		if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL people id(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY_USERS]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*принимаем размер никнейма пользователя*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of people nickname(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY_USERS]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем никнейм пользователя*/
		if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL people nickname(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY_USERS]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*принимаем когда был онлайн */
		ret=user_online(sock);

		profile_party.label=gtk_label_new(text_when_online[ret]);//метка когда пользователь был онлайн
		PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
		PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_NORMAL);//настраиваем толщину метки
		PangoAttribute *attr_color;
		if(ret==ONLINE) {
			attr_color=pango_attr_foreground_new (29490,55704,5898);//настраиваем цвет метки(зеленый)
		}
		else {
			attr_color=pango_attr_foreground_new (65535,0,0);//настраиваем цвет метки(красный)
		}

		pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
		pango_attr_list_insert (attr_of_label,attr_color);//вставляем атрибут в список атрибутов

		gtk_label_set_attributes (GTK_LABEL(profile_party.label),attr_of_label);//устанавливаем атрибуты для метки
		pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов

		/*принимаем ответ сервера админ ли юзер*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV answer is user an admin(%d)\n",kakao_signal[SHOW_PROFILE_PARTY_USERS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_PARTY_USERS]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		sig_data *data2=(sig_data*)malloc(sizeof(sig_data));
		strcpy(data2->to_id,people_info.id);//id пользователя
		strcpy(data2->nickname,people_info.nickname);//имя пользователя
		data2->ChildNotebookWidget=messages_list.Window;//передаем указатель на дочерний виджет notebook(нужно чтобы выбирать страницу на которую переходить)
		data2->ShowWidget=profile_party.Window;//передаем дочерний box,который мы хотим показать
		data2->HideWidget=messages_list.Window;//передаем дочерний box,дочерние виджеты которого мы скроем, чтобы показать один, из которого мы вызывали функцию
		insert_to_list_sig_data(data2,&node_profile_party);
		/*создаем виджет для profile_party, подробности смотреть в messenger2.glade*/
		profile_party.Button3=gtk_button_new();
		gtk_widget_set_hexpand(profile_party.Button3,TRUE);//раскрываем кнопку по горизонтали
		gtk_button_set_relief(GTK_BUTTON(profile_party.Button3),GTK_RELIEF_NONE);
		gtk_widget_set_focus_on_click(profile_party.Button3,FALSE);	

		label=gtk_label_new(people_info.nickname);//создаем метку
		gtk_widget_set_halign(label,GTK_ALIGN_START);//ставим метку в начало

		profile_party.image2=gtk_image_new();
		gtk_image_set_from_icon_name(GTK_IMAGE(profile_party.image2),"gtk-missing-image",GTK_ICON_SIZE_SMALL_TOOLBAR);

		profile_party.Box2=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);
		profile_party.Box3=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);

		gtk_container_add(GTK_CONTAINER(profile_party.Button3),profile_party.Box2);//упаковываем контейнер в кнопку
		gtk_container_add(GTK_CONTAINER(profile_party.Box2),profile_party.image2);//упаковываем фото в контейнер
		gtk_container_add(GTK_CONTAINER(profile_party.Box2),label);//упаковываем метку никнейм в контейнер
		gtk_container_add(GTK_CONTAINER(profile_party.Box3),profile_party.Button3);		
		if(ntohl(sz))//если пользователь админ
		{
			label=gtk_label_new("admin");
			PangoAttrList *attr_of_label=pango_attr_list_new();//создаем список атрибутов
			PangoAttribute *attr_width=pango_attr_weight_new(PANGO_WEIGHT_MEDIUM);//настраиваем толщину метки
			PangoAttribute *attr_color=pango_attr_foreground_new (29490,55704,5898);//настраиваем цвет метки(зеленый)

			pango_attr_list_insert (attr_of_label,attr_width);//вставляем атрибут в список атрибутов
			pango_attr_list_insert (attr_of_label,attr_color);//вставляем атрибут в список атрибутов

			gtk_label_set_attributes (GTK_LABEL(label),attr_of_label);//устанавливаем атрибуты для метки
			pango_attr_list_unref(attr_of_label);//освобождаем список атрибутов


			gtk_container_add(GTK_CONTAINER(profile_party.Box2),label);//упаковываем метку админ в контейнер
		}					
		if(flag_admin)
		{
			profile_party.Button4=gtk_button_new();
			//gtk_widget_set_hexpand(profile_party.Button4,TRUE);//раскрываем кнопку по горизонтали
			//gtk_widget_set_halign(profile_party.Button4,GTK_ALIGN_END);
			profile_party.image_button=gtk_image_new();
			gtk_image_set_from_icon_name(GTK_IMAGE(profile_party.image_button),"gtk-close",GTK_ICON_SIZE_SMALL_TOOLBAR);
			gtk_container_add(GTK_CONTAINER(profile_party.Button4),profile_party.image_button);
			gtk_container_add(GTK_CONTAINER(profile_party.Box3),profile_party.Button4);//упаковываем кнопку-фото в контейнер
			gtk_button_set_relief(GTK_BUTTON(profile_party.Button4),GTK_RELIEF_NONE);
			gtk_widget_set_focus_on_click(profile_party.Button4,FALSE);	
			g_signal_connect(G_OBJECT(profile_party.Button4),"clicked",G_CALLBACK(party_delete_user),data2);
		}
		gtk_container_add(GTK_CONTAINER(profile_party.Box2),profile_party.label);//упаковываем метку когда был онлайн в контейнер

		gtk_box_pack_start(GTK_BOX(profile_party.Box),profile_party.Box3,0,0,0);
		gtk_widget_show_all(profile_party.Box);
		g_signal_connect(G_OBJECT(profile_party.Button3),"clicked",G_CALLBACK(show_profile_window),data2);

	}
	if( ret==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV size of people id(%d)\n",kakao_signal[SHOW_PROFILE_WINDOW],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PROFILE_WINDOW]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
}
void change_party_avatar(GtkWidget *widget,sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[CHANGE_PARTY_AVATAR]); fflush(stderr);
	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	gint res;

	dialog = gtk_file_chooser_dialog_new ("Выбрать фото",NULL,action,"Отменить",GTK_RESPONSE_CANCEL,"Выбрать",GTK_RESPONSE_ACCEPT,NULL);
	//g_signal_connect(G_OBJECT(dialog),"response",G_CALLBACK(download),data);
	res = gtk_dialog_run (GTK_DIALOG (dialog));
	if (res == GTK_RESPONSE_ACCEPT)
	{
		GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
		char *new_avatar_path=gtk_file_chooser_get_filename (chooser);
		/*отправляем размер сигнала*/
		sz=htonl(get_str_size(kakao_signal[CHANGE_PARTY_AVATAR]));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[CHANGE_PARTY_AVATAR],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PARTY_AVATAR]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем сигнал*/
		if( !(ret=sendall(sock,kakao_signal[CHANGE_PARTY_AVATAR],ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[CHANGE_PARTY_AVATAR],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PARTY_AVATAR]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем размер имени фото*/
		sz=htonl(get_str_size(from_path_to_filename(new_avatar_path)));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of name of photo(%d)\n",kakao_signal[CHANGE_PARTY_AVATAR],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PARTY_AVATAR]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем имя фото*/
		if( !(ret=sendall(sock,from_path_to_filename(new_avatar_path),ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL name of photo(%d)\n",kakao_signal[CHANGE_PARTY_AVATAR],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PARTY_AVATAR]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем новую аватарку*/
		if(!send_file(sock,new_avatar_path)) {//если произошла ошибка
			/*отправляем ноль*/
			sz=htonl(0);
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(stderr, "ERROR %s : SEND zero(%d)\n",kakao_signal[CHANGE_PARTY_AVATAR],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PARTY_AVATAR]);
				show_error_window(buf,NOTCLOSE); 
				return;
			}
		}
		sprintf(buf,"rm -R %s/%s/*",PATHS[PATH_PARTY_AVATARS],data->party_id);//очищаем кэш аватарки нашей группы
		system(buf);
		show_profile_party_avatar(NULL,data);
		
		g_free (new_avatar_path);
		gtk_widget_destroy(dialog);
	}
	else if(res == GTK_RESPONSE_CANCEL)
	{
		gtk_widget_destroy(dialog);
	}
}
void delete_party_avatar(GtkWidget *widget,sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[DELETE_PARTY_AVATAR]); fflush(stderr);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[DELETE_PARTY_AVATAR]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[DELETE_PARTY_AVATAR],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[DELETE_PARTY_AVATAR]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[DELETE_PARTY_AVATAR],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[DELETE_PARTY_AVATAR],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[DELETE_PARTY_AVATAR]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	sprintf(buf,"rm -R %s/%s/*",PATHS[PATH_PARTY_AVATARS],data->party_id);//очищаем кэш аватарки группы
	system(buf);
	show_profile_party_avatar(NULL,data);
}
void change_party_name()
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[CHANGE_PARTY_NAME]); fflush(stderr);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[CHANGE_PARTY_NAME]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[CHANGE_PARTY_NAME],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PARTY_NAME]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[CHANGE_PARTY_NAME],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[CHANGE_PARTY_NAME],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PARTY_NAME]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	strcpy(party_info.nickname,gtk_entry_get_text(GTK_ENTRY(profile_party.Entry1)));
	/*отправляем размер имени группы*/
	sz=htonl(get_str_size(party_info.nickname));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of name of party(%d)\n",kakao_signal[CHANGE_PARTY_NAME],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PARTY_NAME]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем имя группы*/
	if( !(ret=sendall(sock,party_info.nickname,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL party name(%d)\n",kakao_signal[CHANGE_PARTY_NAME],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PARTY_NAME]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*принимаем ответ получилось ли изменить имя*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV answer change party name?(%d)\n",kakao_signal[CHANGE_PARTY_NAME],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PARTY_NAME]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	if(ntohl(sz)) {//если ответ положительный(получилось изменить имя)
		gtk_revealer_set_reveal_child(GTK_REVEALER(profile_party.Revealer),TRUE);
		gtk_revealer_set_reveal_child(GTK_REVEALER(profile_party.Revealer2),FALSE);
	}
	else{//если ответ отрицательный
		gtk_revealer_set_reveal_child(GTK_REVEALER(profile_party.Revealer),FALSE);
		gtk_revealer_set_reveal_child(GTK_REVEALER(profile_party.Revealer2),TRUE);
	}
}
void change_party_description()
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[CHANGE_PARTY_DESCRIPTION]); fflush(stderr);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[CHANGE_PARTY_DESCRIPTION]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[CHANGE_PARTY_DESCRIPTION],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PARTY_DESCRIPTION]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[CHANGE_PARTY_DESCRIPTION],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[CHANGE_PARTY_DESCRIPTION],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PARTY_DESCRIPTION]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	GtkTextIter start, end;
	gtk_text_buffer_get_start_iter(party_buffer, &start);// Получаем начальную позицию в буфере
	gtk_text_buffer_get_end_iter(party_buffer, &end);// Получаем конечную позицию в буфере
	strcpy(buf,gtk_text_iter_get_text (&start, &end));//получаем описание группы
	/*отправляем размер описания группы*/
	sz=htonl(get_str_size(buf));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of party description(%d)\n",kakao_signal[CHANGE_PARTY_DESCRIPTION],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PARTY_DESCRIPTION]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем описание группы*/
	if( !(ret=sendall(sock,buf,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL party description(%d)\n",kakao_signal[CHANGE_PARTY_DESCRIPTION],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PARTY_DESCRIPTION]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*принимаем ответ получилось ли изменить описание*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV answer change party name?(%d)\n",kakao_signal[CHANGE_PARTY_NAME],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PARTY_NAME]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	if(ntohl(sz)) {//если ответ положительный(получилось изменить описание)
		gtk_revealer_set_reveal_child(GTK_REVEALER(profile_party.Revealer3),TRUE);
		gtk_revealer_set_reveal_child(GTK_REVEALER(profile_party.Revealer4),FALSE);
	}
	else{//если ответ отрицательный
		gtk_revealer_set_reveal_child(GTK_REVEALER(profile_party.Revealer3),FALSE);
		gtk_revealer_set_reveal_child(GTK_REVEALER(profile_party.Revealer4),TRUE);
	}
}
void party_add_admin(GtkWidget *widget,sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[PARTY_ADD_ADMIN]); fflush(stderr);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[PARTY_ADD_ADMIN]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[PARTY_ADD_ADMIN],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[PARTY_ADD_ADMIN]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[PARTY_ADD_ADMIN],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[PARTY_ADD_ADMIN],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[PARTY_ADD_ADMIN]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*отправляем размер id*/
	sz=htonl(get_str_size(data->to_id));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of people id(%d)\n",kakao_signal[PARTY_ADD_ADMIN],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[PARTY_ADD_ADMIN]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем id*/
	if( !(ret=sendall(sock,data->to_id,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL people id(%d)\n",kakao_signal[PARTY_ADD_ADMIN],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[PARTY_ADD_ADMIN]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	show_party_appoint_admins(widget,data);

}
void transition_to_party_appoint_admins(GtkWidget *widget,sig_data* data)
{
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(party_peoples_window.PeopleSearchEntry),G_SIGNAL_MATCH_FUNC,0,0,NULL,show_party_appoint_admins,0); while(ret!=0);
	g_signal_connect(G_OBJECT(party_peoples_window.PeopleSearchEntry),"activate",G_CALLBACK(show_party_appoint_admins),data);
	show_party_appoint_admins(widget,data);
	gtk_widget_show_all(party_peoples_window.Window);
}
void show_party_appoint_admins(GtkWidget *widget,sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS]); fflush(stderr);
	char limit[MYSQL_INT];
	sprintf(limit,"%ld",limits[LIMIT_PARTY_PEOPLES]);
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(party_peoples_window.Scrolled),G_SIGNAL_MATCH_FUNC,0,0,NULL,add_limit,0); while(ret!=0);
	g_signal_connect(G_OBJECT(party_peoples_window.Scrolled),"edge-reached",G_CALLBACK(add_limit),data);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[SHOW_PARTY_APPOINT_ADMINS]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_APPOINT_ADMINS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_APPOINT_ADMINS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	strcpy(people_info.nickname,gtk_entry_get_text(GTK_ENTRY(party_peoples_window.PeopleSearchEntry)));

	/*отправляем размер никнейма*/
	sz=htonl(get_str_size(people_info.nickname));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of SearchEntry nickname(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_APPOINT_ADMINS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем никнейм*/
	if( !(ret=sendall(sock,people_info.nickname,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL SearchEntry nickname(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_APPOINT_ADMINS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем размер limit*/
	sz=htonl(get_str_size(limit));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of limit(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_APPOINT_ADMINS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	//отправляем limit
	if( !sendall(sock,limit,ntohl(sz)) ) {
		fprintf(stderr, "ERROR %s : SENDALL limit(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_APPOINT_ADMINS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	clean_window(party_peoples_window.Box);//очищаем контейнер от виджетов
	/*принимаем размер id пользователя*/
	while( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL)) )
	{
		if(!ntohl(sz)) break;//если конец
		/*принимаем id пользователя*/
		if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL people id(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_APPOINT_ADMINS]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*принимаем размер никнейма пользователя*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of people nickname(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_APPOINT_ADMINS]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем никнейм пользователя*/
		if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL people nickname(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_APPOINT_ADMINS]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		sig_data *data2=(sig_data*)malloc(sizeof(sig_data));
		strcpy(data2->to_id,people_info.id);//id пользователя
		strcpy(data2->nickname,people_info.nickname);//имя пользователя
		insert_to_list_sig_data(data2,&node_party_peoples);
		/*создаем виджет для party_peoples_window, подробности смотреть в messenger2.glade*/
		party_peoples_window.Button=gtk_button_new();
		gtk_widget_set_hexpand(party_peoples_window.Button,TRUE);//раскрываем кнопку по горизонтали
		gtk_widget_set_halign(party_peoples_window.Button,GTK_ALIGN_END);

		label=gtk_label_new(people_info.nickname);//создаем метку

		party_peoples_window.image_button=gtk_image_new();
		gtk_image_set_from_icon_name(GTK_IMAGE(party_peoples_window.image_button),"gtk-yes",GTK_ICON_SIZE_LARGE_TOOLBAR);
		party_peoples_window.image2=gtk_image_new();
		gtk_image_set_from_icon_name(GTK_IMAGE(party_peoples_window.image2),"gtk-missing-image",GTK_ICON_SIZE_DIALOG);

		party_peoples_window.Box2=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);

		gtk_container_add(GTK_CONTAINER(party_peoples_window.Box2),party_peoples_window.image2);
		gtk_container_add(GTK_CONTAINER(party_peoples_window.Box2),label);//упаковываем метку в контейнер
		gtk_container_add(GTK_CONTAINER(party_peoples_window.Button),party_peoples_window.image_button);//упаковываем фото в кнопку
		gtk_container_add(GTK_CONTAINER(party_peoples_window.Box2),party_peoples_window.Button);
		gtk_button_set_relief(GTK_BUTTON(party_peoples_window.Button),GTK_RELIEF_NONE);
		gtk_widget_set_focus_on_click(party_peoples_window.Button,FALSE);														
	
		gtk_box_pack_start(GTK_BOX(party_peoples_window.Box),party_peoples_window.Box2,0,0,0);
		gtk_widget_show_all(party_peoples_window.Box);
		g_signal_connect(G_OBJECT(party_peoples_window.Button),"clicked",G_CALLBACK(party_add_admin),data2);
	}
	if( ret==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV size of people id(%d)\n",kakao_signal[SHOW_PARTY_APPOINT_ADMINS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_APPOINT_ADMINS]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
}
void party_delete_admin(GtkWidget *widget,sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[PARTY_DELETE_ADMIN]); fflush(stderr);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[PARTY_DELETE_ADMIN]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[PARTY_DELETE_ADMIN],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[PARTY_DELETE_ADMIN]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[PARTY_DELETE_ADMIN],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[PARTY_DELETE_ADMIN],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[PARTY_DELETE_ADMIN]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*отправляем размер id*/
	sz=htonl(get_str_size(data->to_id));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of people id(%d)\n",kakao_signal[PARTY_DELETE_ADMIN],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[PARTY_DELETE_ADMIN]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем id*/
	if( !(ret=sendall(sock,data->to_id,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL people id(%d)\n",kakao_signal[PARTY_DELETE_ADMIN],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[PARTY_DELETE_ADMIN]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	show_party_delete_admins(widget,data);

}
void transition_to_party_delete_admins(GtkWidget *widget,sig_data* data)
{
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(party_peoples_window.PeopleSearchEntry),G_SIGNAL_MATCH_FUNC,0,0,NULL,show_party_delete_admins,0); while(ret!=0);
	g_signal_connect(G_OBJECT(party_peoples_window.PeopleSearchEntry),"activate",G_CALLBACK(show_party_delete_admins),data);
	show_party_delete_admins(widget,data);
	gtk_widget_show_all(party_peoples_window.Window);
}
void show_party_delete_admins(GtkWidget *widget,sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS]); fflush(stderr);
	char limit[MYSQL_INT];
	sprintf(limit,"%ld",limits[LIMIT_PARTY_PEOPLES]);
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(party_peoples_window.Scrolled),G_SIGNAL_MATCH_FUNC,0,0,NULL,add_limit,0); while(ret!=0);
	g_signal_connect(G_OBJECT(party_peoples_window.Scrolled),"edge-reached",G_CALLBACK(add_limit),data);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[SHOW_PARTY_DELETE_ADMINS]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_DELETE_ADMINS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[SHOW_PARTY_DELETE_ADMINS],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_DELETE_ADMINS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	strcpy(people_info.nickname,gtk_entry_get_text(GTK_ENTRY(party_peoples_window.PeopleSearchEntry)));

	/*отправляем размер никнейма*/
	sz=htonl(get_str_size(people_info.nickname));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of SearchEntry nickname(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_DELETE_ADMINS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем никнейм*/
	if( !(ret=sendall(sock,people_info.nickname,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL SearchEntry nickname(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_DELETE_ADMINS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем размер limit*/
	sz=htonl(get_str_size(limit));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of limit(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_DELETE_ADMINS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	//отправляем limit
	if( !sendall(sock,limit,ntohl(sz)) ) {
		fprintf(stderr, "ERROR %s : SENDALL limit(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_DELETE_ADMINS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	clean_window(party_peoples_window.Box);//очищаем контейнер от виджетов
	/*принимаем размер id пользователя*/
	while( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL)) )
	{
		if(!ntohl(sz)) break;//если конец
		/*принимаем id пользователя*/
		if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL people id(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_DELETE_ADMINS]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*принимаем размер никнейма пользователя*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of people nickname(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_DELETE_ADMINS]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем никнейм пользователя*/
		if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL people nickname(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_DELETE_ADMINS]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		sig_data *data2=(sig_data*)malloc(sizeof(sig_data));
		strcpy(data2->to_id,people_info.id);//id пользователя
		strcpy(data2->nickname,people_info.nickname);//имя пользователя
		insert_to_list_sig_data(data2,&node_party_peoples);
		/*создаем виджет для party_peoples_window, подробности смотреть в messenger2.glade*/
		party_peoples_window.Button=gtk_button_new();
		gtk_widget_set_hexpand(party_peoples_window.Button,TRUE);//раскрываем кнопку по горизонтали
		gtk_widget_set_halign(party_peoples_window.Button,GTK_ALIGN_END);

		label=gtk_label_new(people_info.nickname);//создаем метку

		party_peoples_window.image_button=gtk_image_new();
		gtk_image_set_from_icon_name(GTK_IMAGE(party_peoples_window.image_button),"gtk-no",GTK_ICON_SIZE_LARGE_TOOLBAR);
		party_peoples_window.image2=gtk_image_new();
		gtk_image_set_from_icon_name(GTK_IMAGE(party_peoples_window.image2),"gtk-missing-image",GTK_ICON_SIZE_DIALOG);

		party_peoples_window.Box2=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);

		gtk_container_add(GTK_CONTAINER(party_peoples_window.Box2),party_peoples_window.image2);
		gtk_container_add(GTK_CONTAINER(party_peoples_window.Box2),label);//упаковываем метку в контейнер
		gtk_container_add(GTK_CONTAINER(party_peoples_window.Button),party_peoples_window.image_button);//упаковываем фото в кнопку
		gtk_container_add(GTK_CONTAINER(party_peoples_window.Box2),party_peoples_window.Button);
		gtk_button_set_relief(GTK_BUTTON(party_peoples_window.Button),GTK_RELIEF_NONE);
		gtk_widget_set_focus_on_click(party_peoples_window.Button,FALSE);														
	
		gtk_box_pack_start(GTK_BOX(party_peoples_window.Box),party_peoples_window.Box2,0,0,0);
		gtk_widget_show_all(party_peoples_window.Box);
		g_signal_connect(G_OBJECT(party_peoples_window.Button),"clicked",G_CALLBACK(party_delete_admin),data2);
	}
	if( ret==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV size of people id(%d)\n",kakao_signal[SHOW_PARTY_DELETE_ADMINS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_DELETE_ADMINS]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
}
void party_add_friend(GtkWidget *widget,sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[PARTY_ADD_FRIEND]); fflush(stderr);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[PARTY_ADD_FRIEND]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[PARTY_ADD_FRIEND],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[PARTY_ADD_FRIEND]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[PARTY_ADD_FRIEND],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[PARTY_ADD_FRIEND],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[PARTY_ADD_FRIEND]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*отправляем размер id*/
	sz=htonl(get_str_size(data->to_id));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of people id(%d)\n",kakao_signal[PARTY_ADD_FRIEND],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[PARTY_ADD_FRIEND]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем id*/
	if( !(ret=sendall(sock,data->to_id,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL people id(%d)\n",kakao_signal[PARTY_ADD_FRIEND],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[PARTY_ADD_FRIEND]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	show_party_add_friends(widget,data);

}
void party_delete_user(GtkWidget *widget,sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[PARTY_DELETE_USER]); fflush(stderr);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[PARTY_DELETE_USER]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[PARTY_DELETE_USER],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[PARTY_DELETE_USER]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[PARTY_DELETE_USER],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[PARTY_DELETE_USER],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[PARTY_DELETE_USER]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*отправляем размер id*/
	sz=htonl(get_str_size(data->to_id));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of people id(%d)\n",kakao_signal[PARTY_DELETE_USER],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[PARTY_DELETE_USER]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем id*/
	if( !(ret=sendall(sock,data->to_id,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL people id(%d)\n",kakao_signal[PARTY_DELETE_USER],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[PARTY_DELETE_USER]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	data=(sig_data*)malloc(sizeof(sig_data));
	strcpy(data->to_id,party_info.id);//id группы
	strcpy(data->nickname,party_info.nickname);//имя группы
	data->ChildNotebookWidget=messages_list.Window;//передаем указатель на дочерний виджет notebook(нужно чтобы выбирать страницу на которую переходить)
	data->ShowWidget=profile_party.Window;//передаем дочерний box,который мы хотим показать
	data->HideWidget=messages_list.Window;//передаем дочерний box,дочерние виджеты которого мы скроем, чтобы показать один, из которого мы вызывали функцию
	insert_to_list_sig_data(data,&node_profile_party);
	show_profile_party(NULL,data);

}
void transition_to_party_add_friend(GtkWidget *widget,sig_data* data)
{
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(party_peoples_window.PeopleSearchEntry),G_SIGNAL_MATCH_FUNC,0,0,NULL,show_party_add_friends ,0); while(ret!=0);
	g_signal_connect(G_OBJECT(party_peoples_window.PeopleSearchEntry),"activate",G_CALLBACK(show_party_add_friends),data);
	show_party_add_friends(widget,data);
	gtk_widget_show_all(party_peoples_window.Window);
}
void show_party_add_friends(GtkWidget *widget,sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS]); fflush(stderr);
	char limit[MYSQL_INT];
	sprintf(limit,"%ld",limits[LIMIT_PARTY_PEOPLES]);
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(party_peoples_window.Scrolled),G_SIGNAL_MATCH_FUNC,0,0,NULL,add_limit,0); while(ret!=0);
	g_signal_connect(G_OBJECT(party_peoples_window.Scrolled),"edge-reached",G_CALLBACK(add_limit),data);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[SHOW_PARTY_ADD_FRIENDS]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_ADD_FRIENDS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[SHOW_PARTY_ADD_FRIENDS],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_ADD_FRIENDS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	strcpy(people_info.nickname,gtk_entry_get_text(GTK_ENTRY(party_peoples_window.PeopleSearchEntry)));

	/*отправляем размер никнейма*/
	sz=htonl(get_str_size(people_info.nickname));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of SearchEntry nickname(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_ADD_FRIENDS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем никнейм*/
	if( !(ret=sendall(sock,people_info.nickname,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL SearchEntry nickname(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_ADD_FRIENDS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем размер limit*/
	sz=htonl(get_str_size(limit));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of limit(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_ADD_FRIENDS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	//отправляем limit
	if( !sendall(sock,limit,ntohl(sz)) ) {
		fprintf(stderr, "ERROR %s : SENDALL limit(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_ADD_FRIENDS]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	clean_window(party_peoples_window.Box);//очищаем контейнер от виджетов
	/*принимаем размер id пользователя*/
	while( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL)) )
	{
		if(!ntohl(sz)) break;//если конец
		/*принимаем id пользователя*/
		if( (ret=recvall(sock,people_info.id,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL people id(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_ADD_FRIENDS]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		/*принимаем размер никнейма пользователя*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of people nickname(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_ADD_FRIENDS]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем никнейм пользователя*/
		if( (ret=recvall(sock,people_info.nickname,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL people nickname(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_ADD_FRIENDS]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}

		sig_data *data2=(sig_data*)malloc(sizeof(sig_data));
		insert_to_list_sig_data(data2,&node_party_peoples);
		strcpy(data2->to_id,people_info.id);//id пользователя
		strcpy(data2->nickname,people_info.nickname);//имя пользователя
		/*создаем виджет для party_peoples_window, подробности смотреть в messenger2.glade*/
		party_peoples_window.Button=gtk_button_new();
		gtk_widget_set_hexpand(party_peoples_window.Button,TRUE);//раскрываем кнопку по горизонтали
		gtk_widget_set_halign(party_peoples_window.Button,GTK_ALIGN_END);

		label=gtk_label_new(people_info.nickname);//создаем метку

		party_peoples_window.image_button=gtk_image_new();
		gtk_image_set_from_icon_name(GTK_IMAGE(party_peoples_window.image_button),"gtk-yes",GTK_ICON_SIZE_LARGE_TOOLBAR);
		party_peoples_window.image2=gtk_image_new();
		gtk_image_set_from_icon_name(GTK_IMAGE(party_peoples_window.image2),"gtk-missing-image",GTK_ICON_SIZE_DIALOG);

		party_peoples_window.Box2=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,10);

		gtk_container_add(GTK_CONTAINER(party_peoples_window.Box2),party_peoples_window.image2);
		gtk_container_add(GTK_CONTAINER(party_peoples_window.Box2),label);//упаковываем метку в контейнер
		gtk_container_add(GTK_CONTAINER(party_peoples_window.Button),party_peoples_window.image_button);//упаковываем фото в кнопку
		gtk_container_add(GTK_CONTAINER(party_peoples_window.Box2),party_peoples_window.Button);
		gtk_button_set_relief(GTK_BUTTON(party_peoples_window.Button),GTK_RELIEF_NONE);
		gtk_widget_set_focus_on_click(party_peoples_window.Button,FALSE);														
	
		gtk_box_pack_start(GTK_BOX(party_peoples_window.Box),party_peoples_window.Box2,0,0,0);
		gtk_widget_show_all(party_peoples_window.Box);
		g_signal_connect(G_OBJECT(party_peoples_window.Button),"clicked",G_CALLBACK(party_add_friend),data2);
	}
	if( ret==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV size of people id(%d)\n",kakao_signal[SHOW_PARTY_ADD_FRIENDS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_PARTY_ADD_FRIENDS]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
}
void party_exit(GtkWidget *widget,sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[PARTY_EXIT]); fflush(stderr);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[PARTY_EXIT]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[PARTY_EXIT],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[PARTY_EXIT]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[PARTY_EXIT],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[PARTY_EXIT],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[PARTY_EXIT]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*принимаем ответ*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV answer%d)\n",kakao_signal[PARTY_EXIT],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[PARTY_EXIT]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	if(!ntohl(sz)) {//если ответ отрицательный
		show_error_window("Перед выходом из группы добавьте администратора!",NOTCLOSE);
		return;
	}
	transition_to_messages_list(NULL,NULL);
}
void hide_party_peoples_window()
{
	fprintf(stderr,"SIGNAL: HIDE_PARTY_PEOPLES_WINDOW\n"); fflush(stderr);
	sig_data* data=(sig_data*)malloc(sizeof(sig_data));
	strcpy(data->to_id,party_info.id);//id группы
	strcpy(data->nickname,party_info.nickname);//имя группы
	data->ChildNotebookWidget=messages_list.Window;//передаем указатель на дочерний виджет notebook(нужно чтобы выбирать страницу на которую переходить)
	data->ShowWidget=profile_party.Window;//передаем дочерний box,который мы хотим показать
	data->HideWidget=messages_list.Window;//передаем дочерний box,дочерние виджеты которого мы скроем, чтобы показать один, из которого мы вызывали функцию
	insert_to_list_sig_data(data,&node_profile_party);
	show_profile_party(NULL,data);
	gtk_widget_hide(party_peoples_window.Window);
	clean_window(party_peoples_window.Box);
}
void show_setting_profile(GtkWidget *widget,sig_data* data2)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[SHOW_SETTING_PROFILE]); fflush(stderr);
	free_sig_data(node_profile_setting);

	node_profile_setting=NULL;
	sig_data *data=(sig_data*)malloc(sizeof(sig_data));
	insert_to_list_sig_data(data,&node_profile_setting);

	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(profile_setting.Button),G_SIGNAL_MATCH_FUNC,0,0,NULL,show_photo_full,0); while(ret!=0);//удаляем сигнал кнопки показа полного фото
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(profile_setting.Button),G_SIGNAL_MATCH_FUNC,0,0,NULL,photo_revealer,0); while(ret!=0);//удаляем сигнал кнопки показа настроек аватарки

	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[SHOW_SETTING_PROFILE]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[SHOW_SETTING_PROFILE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_SETTING_PROFILE]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[SHOW_SETTING_PROFILE],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[SHOW_SETTING_PROFILE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_SETTING_PROFILE]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*принимаем ответ есть ли у нас аватарка*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV do we have an avatar?(%d)\n",kakao_signal[SHOW_SETTING_PROFILE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_SETTING_PROFILE]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	if(ntohl(sz)) {//если аватарка у нас есть
		char buf[(MAX_PATH*2)+256];
		sprintf(buf,"rm -R %s/%s/*",PATHS[PATH_USER_AVATARS],my_info.id);//очищаем кэш нашей аватарки
		system(buf);

		snprintf(data->path,sizeof(data->path),"%s/%s",PATHS[PATH_USER_AVATARS],my_info.id);//создаем путь к дирректории с моим id
		sprintf(buf,"if ! [ -d \"%s\" ]; then mkdir -p %s; fi",data->path,data->path);//создаем команду для создания дирректории
		system(buf);//создаем каталог если это нужно

		/*принимаем размер имени изображения*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of name of image(%d)\n",kakao_signal[SHOW_SETTING_PROFILE],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_SETTING_PROFILE]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*принимаем имя изображения*/
		if( (ret=recvall(sock,filename,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL name of image(%d)\n",kakao_signal[SHOW_SETTING_PROFILE],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[SHOW_SETTING_PROFILE]);
			show_error_window(buf,NOTCLOSE);
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		sprintf(data->path,"%s/%s/%s",PATHS[PATH_USER_AVATARS],my_info.id,filename);//путь до аватарки
		recv_file(sock,data->path,NULL);//принимаем аватарку

		GdkPixbuf *pixbuf=gdk_pixbuf_new_from_file_at_size (data->path,workarea.width/5,workarea.height/5,NULL);
		gtk_image_set_from_pixbuf(GTK_IMAGE(profile_setting.image2),pixbuf);//устанавливаем аватарку

		g_signal_connect(G_OBJECT(profile_setting.Button),"clicked",G_CALLBACK(show_photo_full),data);//назначаем сигнал кнопке показа полного фото
		gtk_widget_show(profile_setting.Button2);//показываем метку удалить аватарку
	}
	else {
		gtk_widget_hide(profile_setting.Button2);//скрываем метку удалить аватарку
		gtk_image_set_from_icon_name(GTK_IMAGE(profile_setting.image2),"gtk-missing-image",GTK_ICON_SIZE_DIALOG);
	}
	data->flag_photo_revealer=false;
	data->widget1=profile_setting.Revealer3;
	g_signal_connect(G_OBJECT(profile_setting.Button),"enter",G_CALLBACK(photo_revealer),data);//назначаем сигнал кнопке показа настроек аватарки

	gtk_entry_set_text(GTK_ENTRY(profile_setting.Entry1),my_info.nickname);
	gtk_revealer_set_reveal_child(GTK_REVEALER(profile_setting.Revealer),FALSE);
	gtk_revealer_set_reveal_child(GTK_REVEALER(profile_setting.Revealer2),FALSE);
	gtk_revealer_set_transition_duration(GTK_REVEALER(profile_setting.Revealer),250);
	gtk_revealer_set_transition_duration(GTK_REVEALER(profile_setting.Revealer2),250);
	hide_window(setting_window.Box);
	gtk_widget_show_all(setting_window.Window);
	gtk_widget_show(profile_setting.Grid);
}
void change_my_avatar(GtkWidget *widget,sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[CHANGE_MY_AVATAR]); fflush(stderr);
	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	gint res;

	dialog = gtk_file_chooser_dialog_new ("Выбрать фото",NULL,action,"Отменить",GTK_RESPONSE_CANCEL,"Выбрать",GTK_RESPONSE_ACCEPT,NULL);
	//g_signal_connect(G_OBJECT(dialog),"response",G_CALLBACK(download),data);
	res = gtk_dialog_run (GTK_DIALOG (dialog));
	if (res == GTK_RESPONSE_ACCEPT)
	{
		GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
		char *new_avatar_path=gtk_file_chooser_get_filename (chooser);
		/*отправляем размер сигнала*/
		sz=htonl(get_str_size(kakao_signal[CHANGE_MY_AVATAR]));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[CHANGE_MY_AVATAR],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_MY_AVATAR]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем сигнал*/
		if( !(ret=sendall(sock,kakao_signal[CHANGE_MY_AVATAR],ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[CHANGE_MY_AVATAR],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_MY_AVATAR]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем размер имени фото*/
		sz=htonl(get_str_size(from_path_to_filename(new_avatar_path)));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of name of photo(%d)\n",kakao_signal[CHANGE_MY_AVATAR],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_MY_AVATAR]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем имя фото*/
		if( !(ret=sendall(sock,from_path_to_filename(new_avatar_path),ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL name of photo(%d)\n",kakao_signal[CHANGE_MY_AVATAR],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_MY_AVATAR]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем новую аватарку*/
		if(!send_file(sock,new_avatar_path)) {//если произошла ошибка
			/*отправляем ноль*/
			sz=htonl(0);
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(stderr, "ERROR %s : SEND zero(%d)\n",kakao_signal[CHANGE_MY_AVATAR],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_MY_AVATAR]);
				show_error_window(buf,NOTCLOSE); 
				return;
			}
		}
		sprintf(buf,"rm -R %s/%s/*",PATHS[PATH_USER_AVATARS],my_info.id);//очищаем кэш нашей аватарки
		system(buf);
		show_setting_profile(NULL,NULL);
		
		g_free (new_avatar_path);
		gtk_widget_destroy(dialog);
	}
	else if(res == GTK_RESPONSE_CANCEL)
	{
		gtk_widget_destroy(dialog);
	}
}
void delete_my_avatar(GtkWidget *widget,sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[DELETE_MY_AVATAR]); fflush(stderr);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[DELETE_MY_AVATAR]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[DELETE_MY_AVATAR],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[DELETE_MY_AVATAR]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[DELETE_MY_AVATAR],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[DELETE_MY_AVATAR],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[DELETE_MY_AVATAR]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	sprintf(buf,"rm -R %s/%s/*",PATHS[PATH_USER_AVATARS],my_info.id);//очищаем кэш нашей аватарки
	system(buf);
	show_setting_profile(NULL,NULL);
}
void show_setting_password(GtkWidget *widget,sig_data* data)
{
	fprintf(stderr,"SIGNAL: SHOW_SETTING_PASSWORD\n"); fflush(stderr);
	gtk_entry_set_text(GTK_ENTRY(profile_setting.Entry1),my_info.nickname);
	gtk_revealer_set_reveal_child(GTK_REVEALER(password_setting.Revealer),FALSE);
	gtk_revealer_set_reveal_child(GTK_REVEALER(password_setting.Revealer2),FALSE);
	gtk_revealer_set_reveal_child(GTK_REVEALER(password_setting.Revealer3),FALSE);
	gtk_revealer_set_transition_duration(GTK_REVEALER(password_setting.Revealer),250);
	gtk_revealer_set_transition_duration(GTK_REVEALER(password_setting.Revealer2),250);
	gtk_revealer_set_transition_duration(GTK_REVEALER(password_setting.Revealer3),250);
	hide_window(setting_window.Box);
	gtk_widget_show_all(setting_window.Window);
	gtk_widget_show(password_setting.Grid);
}
void test(GtkWidget *back_button, sig_data *data)
{
	printf("\n\nTEST\n\n");
	//gtk_revealer_set_reveal_child(GTK_REVEALER(data->widget1),TRUE);
	//gtk_adjustment_set_value(GTK_ADJUSTMENT(dialog_window.Adjustment),gtk_adjustment_get_upper (GTK_ADJUSTMENT(dialog_window.Adjustment)));//пролистываем окно в конец
}
void show_create_party()
{
	fprintf(stderr,"SIGNAL: SHOW_CREATE_PARTY\n"); fflush(stderr);
	gtk_widget_show_all(create_party.Window);
}
void hide_create_party()
{
	fprintf(stderr,"SIGNAL: HIDE_CREATE_PARTY\n"); fflush(stderr);
	gtk_widget_hide(create_party.Window);
	gtk_text_buffer_set_text (create_party_buffer,"",-1);// стираем текст в буфере(описание группы)
	gtk_entry_set_text(GTK_ENTRY(create_party.Entry1),"");//стираем имя группы
	gtk_revealer_set_reveal_child(GTK_REVEALER(create_party.Revealer),FALSE);
}
void bob_create_party(GtkWidget *widget,sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[BOB_CREATE_PARTY]); fflush(stderr);
	GtkTextIter start, end;
	
	strcpy(buf,gtk_entry_get_text(GTK_ENTRY(create_party.Entry1)));//получаем имя группы
	if(strcmp(buf,"")==0) { //если поле пустое(имя группы)
		gtk_revealer_set_reveal_child(GTK_REVEALER(create_party.Revealer),TRUE);
		return;
	}
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[BOB_CREATE_PARTY]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[BOB_CREATE_PARTY],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_CREATE_PARTY]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[BOB_CREATE_PARTY],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[BOB_CREATE_PARTY],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_CREATE_PARTY]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*отправляем размер имени группы*/
	sz=htonl(get_str_size(buf));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of party name(%d)\n",kakao_signal[BOB_CREATE_PARTY],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_CREATE_PARTY]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем имя группы*/
	if( !(ret=sendall(sock,buf,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL party name(%d)\n",kakao_signal[BOB_CREATE_PARTY],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_CREATE_PARTY]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	gtk_text_buffer_get_start_iter(create_party_buffer, &start);// Получаем начальную позицию в буфере
	gtk_text_buffer_get_end_iter(create_party_buffer, &end);// Получаем конечную позицию в буфере
	strcpy(buf,gtk_text_iter_get_text (&start, &end));//получаем описание группы

	/*отправляем размер описания группы*/
	sz=htonl(get_str_size(buf));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of description name(%d)\n",kakao_signal[BOB_CREATE_PARTY],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_CREATE_PARTY]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем описание группы*/
	if( !(ret=sendall(sock,buf,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL description name(%d)\n",kakao_signal[BOB_CREATE_PARTY],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_CREATE_PARTY]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	hide_create_party();//скрываем окно
}
void change_password(GtkWidget *back_button, sig_data *data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[CHANGE_PASSWORD]); fflush(stderr);
	char  old_pass[MAX_PASSWORD];
	char  new_pass[MAX_PASSWORD];
	strcpy(old_pass,gtk_entry_get_text(GTK_ENTRY(password_setting.Entry1)));
	strcpy(new_pass,gtk_entry_get_text(GTK_ENTRY(password_setting.Entry2)));

	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[CHANGE_PASSWORD]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[CHANGE_PASSWORD],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PASSWORD]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[CHANGE_PASSWORD],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[CHANGE_PASSWORD],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PASSWORD]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*отправляем размер старого пароля*/
	sz=htonl(get_str_size(old_pass));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of old_pass(%d)\n",kakao_signal[CHANGE_PASSWORD],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PASSWORD]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем старый пароль*/
	if( !(ret=sendall(sock,old_pass,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL old_pass(%d)\n",kakao_signal[CHANGE_PASSWORD],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PASSWORD]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем размер нового пароля*/
	sz=htonl(get_str_size(new_pass));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of new_pass(%d)\n",kakao_signal[CHANGE_PASSWORD],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PASSWORD]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем новый пароль*/
	if( !(ret=sendall(sock,new_pass,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL new_pass(%d)\n",kakao_signal[CHANGE_PASSWORD],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PASSWORD]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*принимаем размер количества записей*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV size of quantity entrys(%d)\n",kakao_signal[CHANGE_PASSWORD],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PASSWORD]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*принимаем количество записей*/
	if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECVALL quantity entrys(%d)\n",kakao_signal[CHANGE_PASSWORD],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_PASSWORD]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	if(atoi(buf))//если старый пароль верный
	{
		strcpy(my_info.password,new_pass);
		ini_puts("autolog", "password",my_info.password, PATHS[PATH_AUTOLOG]);//меняем пароль
		gtk_revealer_set_reveal_child(GTK_REVEALER(password_setting.Revealer),TRUE);
		gtk_revealer_set_reveal_child(GTK_REVEALER(password_setting.Revealer2),FALSE);
		gtk_revealer_set_reveal_child(GTK_REVEALER(password_setting.Revealer3),FALSE);
	}
	else//если старый пароль неверный
	{
		gtk_revealer_set_reveal_child(GTK_REVEALER(password_setting.Revealer2),FALSE);
		gtk_revealer_set_reveal_child(GTK_REVEALER(password_setting.Revealer),FALSE);
		gtk_revealer_set_reveal_child(GTK_REVEALER(password_setting.Revealer3),TRUE);
	}
}
void change_nickname(GtkWidget *back_button, sig_data *data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[CHANGE_NICKNAME]); fflush(stderr);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[CHANGE_NICKNAME]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[CHANGE_NICKNAME],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_NICKNAME]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[CHANGE_NICKNAME],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[CHANGE_NICKNAME],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_NICKNAME]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	char  new_nick[MAX_NICKNAME];
	strcpy(new_nick,gtk_entry_get_text(GTK_ENTRY(profile_setting.Entry1)));

	/*отправляем размер нового никнейма*/
	sz=htonl(get_str_size(new_nick));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of new_nick(%d)\n",kakao_signal[CHANGE_NICKNAME],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_NICKNAME]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем новый никнейм*/
	if( !(ret=sendall(sock,new_nick,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL new_nick(%d)\n",kakao_signal[CHANGE_NICKNAME],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_NICKNAME]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*принимаем размер уникален ли никнейм*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV size of quantity entrys(%d)\n",kakao_signal[CHANGE_NICKNAME],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_NICKNAME]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*принимаем уникален ли никнейм*/
	if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECVALL quantity entrys(%d)\n",kakao_signal[CHANGE_NICKNAME],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[CHANGE_NICKNAME]);
		show_error_window(buf,NOTCLOSE);
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	if(atoi(buf))
	{
		gtk_entry_set_text(GTK_ENTRY(profile_setting.Entry1),new_nick);
		gtk_revealer_set_reveal_child(GTK_REVEALER(profile_setting.Revealer),TRUE);
		gtk_revealer_set_reveal_child(GTK_REVEALER(profile_setting.Revealer2),FALSE);
		strcpy(my_info.nickname,new_nick);
		ini_puts("autolog", "login",my_info.nickname, PATHS[PATH_AUTOLOG]);//записываем логин(никнейм)
	}
	else
	{
		gtk_revealer_set_reveal_child(GTK_REVEALER(profile_setting.Revealer2),TRUE);
		gtk_revealer_set_reveal_child(GTK_REVEALER(profile_setting.Revealer),FALSE);
	}
}
void hide_setting_window(GtkWidget *widget,sig_data* data)
{
	fprintf(stderr,"SIGNAL: HIDE_SETTING_WINDOW\n"); fflush(stderr);
	gtk_widget_hide(setting_window.Window); 
	
}
void back(GtkWidget *back_button, sig_data *data)
{
	fprintf(stderr,"SIGNAL: BACK\n"); fflush(stderr);
	hide_window(data->HideWidget);//скрываем все дочерние виджеты
	show_window(data->ShowWidget);//показываем тот дочерний виджет, из которого мы и вызвали функцию из которой мы выходим
	/*удаляем сигнал кнопки назад*/
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(back_button),G_SIGNAL_MATCH_FUNC,0,0,NULL,back,0); while(ret!=0);
	
	ret = gtk_notebook_page_num(GTK_NOTEBOOK(main_window.Notebook),data->ChildNotebookWidget);//получаем номер страницы 
	gtk_notebook_set_current_page(GTK_NOTEBOOK(main_window.Notebook),ret);//выбираем страницу 
}
void delete_friend(GtkWidget *button, sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[DELETE_FRIEND]); fflush(stderr);

	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[DELETE_FRIEND_OR_SUB]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[DELETE_FRIEND],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[DELETE_FRIEND]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[DELETE_FRIEND_OR_SUB],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[DELETE_FRIEND_OR_SUB],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[DELETE_FRIEND_OR_SUB]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*отправляем размер id друга*/
	sz=htonl(get_str_size(data->to_id));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of friend id(%d)\n",kakao_signal[DELETE_FRIEND],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[DELETE_FRIEND]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем id друга*/
	if( !(ret=sendall(sock,data->to_id,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL friend id(%d)\n",kakao_signal[DELETE_FRIEND],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[DELETE_FRIEND]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	//удаляем все сигналы виджета,связанные с этой функцией
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(button),G_SIGNAL_MATCH_FUNC,0,0,NULL,delete_friend,0); while(ret!=0);
	gtk_button_set_label(GTK_BUTTON(button),"Отправить заявку");
	g_signal_connect(G_OBJECT(people_profile.Button),"clicked",G_CALLBACK(add_friend),data);//добавляем сигнал
	show_friend_list(NULL,NULL);//обновляем список моих друзей
}
void delete_sub(GtkWidget *button, sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[DELETE_SUB]); fflush(stderr);

	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[DELETE_FRIEND_OR_SUB])); 
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[DELETE_SUB],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[DELETE_SUB]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[DELETE_FRIEND_OR_SUB],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[DELETE_FRIEND_OR_SUB],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[DELETE_FRIEND_OR_SUB]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*отправляем размер id подписчика*/
	sz=htonl(get_str_size(data->to_id));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of sub id(%d)\n",kakao_signal[DELETE_SUB],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[DELETE_SUB]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем id подписчика*/
	if( !(ret=sendall(sock,data->to_id,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL sub id(%d)\n",kakao_signal[DELETE_SUB],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[DELETE_SUB]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	//удаляем все сигналы виджета,связанные с этой функцией
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(button),G_SIGNAL_MATCH_FUNC,0,0,NULL,delete_sub,0); while(ret!=0);
	gtk_button_set_label(GTK_BUTTON(people_profile.Button),"Отправить заявку");
	g_signal_connect(G_OBJECT(people_profile.Button),"clicked",G_CALLBACK(add_friend),data);//добавляем сигнал
	show_profile_window(NULL,data);//обновляем окно
	show_my_subs(NULL,NULL);//обновляем список моих друзей
}
void add_friend(GtkWidget *button, sig_data* data)//отправляем запрос дружбы или добавляем подписчика
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[ADD_FRIEND]); fflush(stderr);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[ADD_FRIEND_OR_SUB]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[ADD_FRIEND],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[ADD_FRIEND]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[ADD_FRIEND_OR_SUB],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[ADD_FRIEND],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[ADD_FRIEND]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*отправляем размер id подписчика*/
	sz=htonl(get_str_size(data->to_id));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of friend id(%d)\n",kakao_signal[ADD_FRIEND],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[ADD_FRIEND]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем id подписчика*/
	if( !(ret=sendall(sock,data->to_id,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL friend id(%d)\n",kakao_signal[ADD_FRIEND],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[ADD_FRIEND]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	//удаляем все сигналы виджета,связанные с этой функцией
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(button),G_SIGNAL_MATCH_FUNC,0,0,NULL,add_friend,0); while(ret!=0);
	gtk_button_set_label(GTK_BUTTON(button),"Отменить заявку");
	g_signal_connect(G_OBJECT(people_profile.Button),"clicked",G_CALLBACK(delete_friend),data);//добавляем сигнал
	show_friend_list(NULL,data);//обновляем список моих друзей
}
void add_sub(GtkWidget *button, sig_data* data)//добавляет человека, который отправил нам запрос
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[ADD_SUB]); fflush(stderr);
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[ADD_FRIEND_OR_SUB]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[ADD_SUB],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[ADD_SUB]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[ADD_FRIEND_OR_SUB],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[ADD_SUB],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[ADD_SUB]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}

	/*отправляем размер id подписчика*/
	sz=htonl(get_str_size(data->to_id));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of sub id(%d)\n",kakao_signal[ADD_SUB],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[ADD_SUB]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем id подписчика*/
	if( !(ret=sendall(sock,data->to_id,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL sub id(%d)\n",kakao_signal[ADD_SUB],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[ADD_SUB]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	//удаляем все сигналы виджета,связанные с этой функцией
	do ret=g_signal_handlers_disconnect_matched(G_OBJECT(button),G_SIGNAL_MATCH_FUNC,0,0,NULL,add_sub,0); while(ret!=0);
	gtk_button_set_label(GTK_BUTTON(people_profile.Button),"Удалить из друзей");
	g_signal_connect(G_OBJECT(people_profile.Button),"clicked",G_CALLBACK(delete_friend),data);//добавляем сигнал
	show_profile_window(NULL,data);//обновляем окно
	show_friend_list(NULL,NULL);//обновляем список моих друзей
}
void *update_dialog(void *data)
{
	MYSQL_ROW row;
	MYSQL_RES *res;
	sig_data *data2=data;
	flag_dialog=TRUE;
	while(flag_dialog)
	{
		//pthread_mutex_lock(&thread_lock);
		gdk_threads_enter();

		show_dialog_window(NULL,data);
		gdk_threads_leave();
		//pthread_mutex_unlock(&thread_lock);
		usleep(FPS_DIALOG);
	}
	return NULL;
}
void *update_dialog_messages(void *data)
{
	MYSQL_ROW row;
	MYSQL_RES *res;
	sig_data *data2=data;
	flag_dialog=TRUE;
	char buf2[MAX_BUF];
	char buf[MAX_BUF];
	char messages_diff[LENGTH_NUM_MES]="-1";
	char messages_saw[LENGTH_NUM_MES]="0";
	uint32_t sz;
	short int ret;
	while(flag_dialog)
	{
	//	pthread_mutex_lock(&thread_lock);
		gdk_threads_enter();
		fprintf(stderr,"SIGNAL: %s\n",kakao_signal[UPDATE_DIALOG_MESSAGES]); fflush(stderr);
		if(strcmp(data2->dialog_type,text_dialog_type[COMPANION])==0) 
		{
			/*отправляем размер сигнала*/
			sz=htonl(get_str_size(kakao_signal[UPDATE_DIALOG_MESSAGES]));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_DIALOG_MESSAGES]);
				show_error_window(buf,NOTCLOSE); 
				continue;
			}
			/*отправляем сигнал*/
			if( !(ret=sendall(sock,kakao_signal[UPDATE_DIALOG_MESSAGES],ntohl(sz))) ) {
				fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_DIALOG_MESSAGES]);
				show_error_window(buf,NOTCLOSE); 
				continue;
			}

			sz=htonl(get_str_size(data2->dialog_type));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {//отправляем размер типа диалога
				fprintf(stderr, "ERROR %s : SEND size of type of dialog companion(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_DIALOG_MESSAGES]);
				show_error_window(buf,NOTCLOSE); 
				continue;
			}
			if( !sendall(sock,data2->dialog_type,ntohl(sz)) ) {//отправляем тип диалога
				fprintf(stderr, "ERROR %s : SENDALL type of dialog party(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_DIALOG_MESSAGES]);
				show_error_window(buf,NOTCLOSE); 
				continue;
			}

		}
		else if(strcmp(data2->dialog_type,text_dialog_type[PARTY])==0) 
		{
			/*отправляем размер сигнала*/
			sz=htonl(get_str_size(kakao_signal[UPDATE_DIALOG_MESSAGES]));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_DIALOG_MESSAGES]);
				show_error_window(buf,NOTCLOSE); 
				continue;
			}
			/*отправляем сигнал*/
			if( !(ret=sendall(sock,kakao_signal[UPDATE_DIALOG_MESSAGES],ntohl(sz))) ) {
				fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_DIALOG_MESSAGES]);
				show_error_window(buf,NOTCLOSE); 
				continue;
			}
			sz=htonl(get_str_size(data2->dialog_type));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {//отправляем размер типа диалога
				fprintf(stderr, "ERROR %s : SEND size of type of dialog companion(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_DIALOG_MESSAGES]);
				show_error_window(buf,NOTCLOSE); 
				continue;
			}
			if( !sendall(sock,data2->dialog_type,ntohl(sz)) ) {//отправляем тип диалога
				fprintf(stderr, "ERROR %s : SENDALL type of dialog party(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_DIALOG_MESSAGES]);
				show_error_window(buf,NOTCLOSE); 
				continue;
			}
		}
		else {
			continue;
		}
		sprintf(buf,"%ld",limits[LIMIT_DIALOG]);//переводим в строку
		/*отправляем размер limit_dialog*/
		sz=htonl(get_str_size(buf));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of limit_dialog(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_DIALOG_MESSAGES]);
			show_error_window(buf,NOTCLOSE); 
			continue;
		}
		/*отправляем limit_dialog*/
		if( !(ret=sendall(sock,buf,ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL limit_dialog(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_DIALOG_MESSAGES]);
			show_error_window(buf,NOTCLOSE); 
			continue;
		}

		/*принимаем размер разности сообщений*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of quantity of messages(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_DIALOG_MESSAGES]);
			show_error_window(buf,NOTCLOSE);
			continue;
		}
		/*принимаем разность сообщений*/
		if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL quantity of messages(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_DIALOG_MESSAGES]);
			show_error_window(buf,NOTCLOSE);
			continue;
		}

		/*принимаем размер количества прочитанных моих сообщений*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of quantity saw my messages(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_DIALOG_MESSAGES]);
			show_error_window(buf,NOTCLOSE);
			continue;
		}
		/*принимаем количество прочитанных моих сообщений*/
		if( (ret=recvall(sock,buf2,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL quantity saw my messages(%d)\n",kakao_signal[UPDATE_DIALOG_MESSAGES],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_DIALOG_MESSAGES]);
			show_error_window(buf,NOTCLOSE);
			continue;
		}

		//printf("buf %s message_dif %s\n",buf,messages_diff);
		if(strcmp(buf,messages_diff)!=0 || strcmp(buf2,messages_saw)!=0)//если показаны или прочитаны не все сообщения
		{
			strcpy(messages_diff,buf);
			strcpy(messages_saw,buf2);
			if(gtk_adjustment_get_value(dialog_window.Adjustment)==gtk_adjustment_get_upper(dialog_window.Adjustment))
			{//если окно в конце, то при отправке нам сообщений окно будет пролистываться в конец
				show_messages(NULL,data2);
				gtk_adjustment_set_value(GTK_ADJUSTMENT(dialog_window.Adjustment),gtk_adjustment_get_upper (GTK_ADJUSTMENT(dialog_window.Adjustment)));//пролистываем окно в конец
			}
			else//если пользователь пролистнул окно
			{
				show_messages(NULL,data2);
			}

		}
		gdk_threads_leave();
		//pthread_mutex_unlock(&thread_lock);
		usleep(FPS_DIALOG_MESSAGES);
	}
	return NULL;
}
void *check_errors(void *data)
{
	MYSQL_ROW row;
	MYSQL_RES *result;
	char buf[MAX_BUF];
	uint32_t sz;
	short int ret;
	conn=mysql_init(0);//иницилизируем переменную для подключения
	if(conn==NULL) {
		g_warning("Failed initilization mysql: conn Error: %s\n",mysql_error(conn));
		return NULL;
	}
	if(strcmp(CONNECT_SERVER[LOCAL],"no")==0)
	{
		if(mysql_real_connect(conn,CONNECT_SERVER[DOMAIN],"bob_user_errors","KakaoBob_is_very_very_cool212","bob",0,0,0)==NULL) {
			g_warning("Failed connect to db_user_errors: conn Error: %s\n",mysql_error(conn));
			return NULL;
		}
	}
	else
	{
		if(mysql_real_connect(conn,"localhost","bob_user_errors","KakaoBob_is_very_very_cool212","bob",0,0,0)==NULL) {
			g_warning("Failed connect to db_user_errors: conn Error: %s\n",mysql_error(conn));
			return NULL;
		}
	}
	while(1)
	{
		//pthread_mutex_lock(&thread_lock);
		//gdk_threads_enter();
		fprintf(stderr, "SIGNAL: CHECK_ERRORS\n" ); fflush(stderr);
		sprintf(buf, "SELECT error FROM errors WHERE user_id='%s' AND error IS NOT NULL",my_info.id);
		if(mysql_query(conn,buf)!=0)//выводим текст ошибки
		{
			fprintf(stderr, "SIGNAL: CHECK_ERRORS: %s\n",mysql_error(conn) ); fflush(stderr);
			return NULL;
			//выводим собщение об ошибке
		}
		result=mysql_store_result(conn);
		if(mysql_num_rows(result)>0)//если в бд есть запись с ошибкой
		{
			pthread_mutex_lock(&thread_lock);
			gdk_threads_enter();
			row=mysql_fetch_row(result);
			//fprintf(stderr, "%s\n",row[0] );
			show_error_window(row[0],NOTCLOSE); 
			sendall(sock,"y",sizeof("y"));//отправляем положительный ответ
			//выводим собщение об ошибке
			gdk_threads_leave();
			pthread_mutex_unlock(&thread_lock);
		}
		mysql_free_result(result);
		//gdk_threads_leave();
		//pthread_mutex_unlock(&thread_lock);
		usleep(FPS_ERROR);
	}
	return NULL;
}

void unread_dialogs(GtkWidget *widget, sig_data* data)
{
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[UNREAD_DIALOGS]); fflush(stderr);
	char quantity1[MYSQL_INT];
	char quantity2[MYSQL_INT];
	char buf[MYSQL_INT];
	int i_companion;
	int i_party;
	uint32_t sz;
	short int ret;
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[UNREAD_DIALOGS]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[UNREAD_DIALOGS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[UNREAD_DIALOGS]);
		show_error_window(buf,NOTCLOSE); 
		return;
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[UNREAD_DIALOGS],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[UNREAD_DIALOGS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[UNREAD_DIALOGS]);
		show_error_window(buf,NOTCLOSE); 
		return;
	}

	/*принимаем размер количества непрочитанных диалогов с собеседником*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV size of number of unread dialogs with companion(%d)\n",kakao_signal[UNREAD_DIALOGS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[UNREAD_DIALOGS]);
		show_error_window(buf,NOTCLOSE);
		return;
	}
	/*принимаем количество непрочитанных диалогов с собеседником*/
	if( (ret=recvall(sock,quantity1,ntohl(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECVALL number of unread dialogs with companion(%d)\n",kakao_signal[UNREAD_DIALOGS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[UNREAD_DIALOGS]);
		show_error_window(buf,NOTCLOSE);
		return;
	}
	/*принимаем размер количества непрочитанных диалогов с группой*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV size of number of unread dialogs with party(%d)\n",kakao_signal[UNREAD_DIALOGS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[UNREAD_DIALOGS]);
		show_error_window(buf,NOTCLOSE);
		return;
	}
	/*принимаем количество непрочитанных диалогов с группой*/
	if( (ret=recvall(sock,quantity2,ntohl(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECVALL number of unread dialogs with party(%d)\n",kakao_signal[UNREAD_DIALOGS],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[UNREAD_DIALOGS]);
		show_error_window(buf,NOTCLOSE);
		return;
	}
	i_companion=atoi(quantity1);
	i_party=atoi(quantity2);
	if(i_companion==0 || i_party==0)//если нет непрочитанных диалогов
	{
		if(i_companion==0 && i_party==0)
			gtk_widget_hide(main_window.label);

		if(i_companion==0)
			gtk_widget_hide(messages_list.label3);

		if(i_party==0)
			gtk_widget_hide(messages_list.label4);
	}
	if(i_companion>quantity_unread_dialogs_companion || i_party>quantity_unread_dialogs_party)//если непрочитанных диалогов с группой или собеседником стало больше
	{
		sprintf(buf,"%d",i_companion+i_party);
		gtk_label_set_text(GTK_LABEL(main_window.label),buf);
		gtk_widget_show(main_window.label);//общее количество непрочитанных диалогов

		if(i_companion>quantity_unread_dialogs_companion){//если непрочитанных диалогов с собеседниками стало больше
			sprintf(buf,"%d",i_companion);
			gtk_label_set_text(GTK_LABEL(messages_list.label3),buf);
			gtk_widget_show(messages_list.label3);
			quantity_unread_dialogs_companion=i_companion;
		}
		if(i_party>quantity_unread_dialogs_party) {//если непрочитанных диалогов с группами стало больше
			sprintf(buf,"%d",i_party);
			gtk_label_set_text(GTK_LABEL(messages_list.label4),buf);
			gtk_widget_show(messages_list.label4);
			quantity_unread_dialogs_party=i_party;
		}
		if(flag_unread_dialogs_sound) {
			play_sound(PATHS[PATH_AUDIO_PAN]);
       	}
	}
	if(i_companion < quantity_unread_dialogs_companion || i_party < quantity_unread_dialogs_party) //если непрочитанных диалогов с группой или собеседником стало меньше
	{
		sprintf(buf,"%d",i_companion+i_party);
		gtk_label_set_text(GTK_LABEL(main_window.label),buf);
		gtk_widget_show(main_window.label);//общее количество непрочитанных диалогов

		if(i_companion<quantity_unread_dialogs_companion){//если непрочитанных диалогов с собеседниками стало меньше
			sprintf(buf,"%d",i_companion);
			gtk_label_set_text(GTK_LABEL(messages_list.label3),buf);
			gtk_widget_show(messages_list.label3);
			quantity_unread_dialogs_companion=i_companion;
		}
		if(i_party<quantity_unread_dialogs_party) {//если непрочитанных диалогов с группами стало меньше
			sprintf(buf,"%d",i_party);
			gtk_label_set_text(GTK_LABEL(messages_list.label4),buf);
			gtk_widget_show(messages_list.label4);
			quantity_unread_dialogs_party=i_party;
		}
	}
	if(!flag_unread_dialogs_sound)
		flag_unread_dialogs_sound=true;
}
void play_sound(const char *path)
{
	pthread_create(&var_thread_play_sound,NULL,func_thread_play_sound,(void*)path);//создаем поток в котором будет проигрываться звук
}
void *func_thread_play_sound(void *data)
{
	const char *path=data;
	if (PlayWavFile(path) != 0)
       	fprintf(stderr,"Error:\n%s\n", PlayWavError(1));
}
void *update_unread_dialogs(void *data)//функция смотрит количество непрочитанных диалогов, не учитывая тот, который у нас открыт в данный момент
{
	flag_unread_dialogs=TRUE;
	uint32_t sz;
	short int ret;
	while(flag_unread_dialogs)
	{
		//pthread_mutex_lock(&thread_lock);
		gdk_threads_enter();

		fprintf(stderr,"SIGNAL: %s\n",kakao_signal[UPDATE_UNREAD_DIALOGS]); fflush(stderr);
		char quantity1[MYSQL_INT];
		char quantity2[MYSQL_INT];
		char buf[MYSQL_INT];
		int i_companion;
		int i_party;
		/*отправляем размер сигнала*/
		sz=htonl(get_str_size(kakao_signal[UPDATE_UNREAD_DIALOGS]));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[UPDATE_UNREAD_DIALOGS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_UNREAD_DIALOGS]);
			show_error_window(buf,NOTCLOSE); 
			return NULL;
		}
		/*отправляем сигнал*/
		if( !(ret=sendall(sock,kakao_signal[UPDATE_UNREAD_DIALOGS],ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[UPDATE_UNREAD_DIALOGS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_UNREAD_DIALOGS]);
			show_error_window(buf,NOTCLOSE); 
			return NULL;
		}

		/*принимаем размер количества непрочитанных диалогов с собеседником*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of number of unread dialogs with companion(%d)\n",kakao_signal[UPDATE_UNREAD_DIALOGS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_UNREAD_DIALOGS]);
			show_error_window(buf,NOTCLOSE);
			return NULL;
		}
		/*принимаем количество непрочитанных диалогов с собеседником*/
		if( (ret=recvall(sock,quantity1,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL number of unread dialogs with companion(%d)\n",kakao_signal[UPDATE_UNREAD_DIALOGS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_UNREAD_DIALOGS]);
			show_error_window(buf,NOTCLOSE);
			return NULL;
		}
		/*принимаем размер количества непрочитанных диалогов с группой*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of number of unread dialogs with party(%d)\n",kakao_signal[UPDATE_UNREAD_DIALOGS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_UNREAD_DIALOGS]);
			show_error_window(buf,NOTCLOSE);
			return NULL;
		}
		/*принимаем количество непрочитанных диалогов с группой*/
		if( (ret=recvall(sock,quantity2,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL number of unread dialogs with party(%d)\n",kakao_signal[UPDATE_UNREAD_DIALOGS],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_UNREAD_DIALOGS]);
			show_error_window(buf,NOTCLOSE);
			return NULL;
		}
		i_companion=atoi(quantity1);
		i_party=atoi(quantity2);
		if(i_companion==0 || i_party==0)//если нет непрочитанных диалогов
		{
			if(i_companion==0 && i_party==0)
				gtk_widget_hide(main_window.label);
		}
		if(i_companion>quantity_unread_dialogs_companion || i_party>quantity_unread_dialogs_party)//если непрочитанных диалогов с группой или собеседником стало больше
		{
			sprintf(buf,"%d",i_companion+i_party);
			gtk_label_set_text(GTK_LABEL(main_window.label),buf);
			gtk_widget_show(main_window.label);//общее количество непрочитанных диалогов

			if(i_companion>quantity_unread_dialogs_companion){//если непрочитанных диалогов с собеседниками стало больше
				quantity_unread_dialogs_companion=i_companion;
			}
			if(i_party>quantity_unread_dialogs_party) {//если непрочитанных диалогов с группами стало больше
				quantity_unread_dialogs_party=i_party;
			}

			if(flag_unread_dialogs_sound) 
				play_sound(PATHS[PATH_AUDIO_PAN]);
			else
				flag_unread_dialogs_sound=true;
		}
		if(i_companion < quantity_unread_dialogs_companion || i_party < quantity_unread_dialogs_party) //если непрочитанных диалогов с группой или собеседником стало меньше
		{
			sprintf(buf,"%d",i_companion+i_party);
			gtk_label_set_text(GTK_LABEL(main_window.label),buf);
			gtk_widget_show(main_window.label);//общее количество непрочитанных диалогов

			if(i_companion<quantity_unread_dialogs_companion){//если непрочитанных диалогов с собеседниками стало меньше
				quantity_unread_dialogs_companion=i_companion;
			}
			if(i_party<quantity_unread_dialogs_party) {//если непрочитанных диалогов с группами стало меньше
				quantity_unread_dialogs_party=i_party;
			}
		}

		gdk_threads_leave();
		//pthread_mutex_unlock(&thread_lock);
		usleep(FPS_UNREAD_DIALOGS);
	}
}

void *update_messages_list(void *data)
{
	flag_messages_list=TRUE;
	char total_messages[MYSQL_INT]="0";
	char total_online_companions[MYSQL_INT]="0";
	char total_notonline_companions[MYSQL_INT]="0";
	char total_writing_companions[MYSQL_INT]="0";
	char buf[MAX_BUF];
	uint32_t sz;
	short int ret;
	while(flag_messages_list)
	{
		//pthread_mutex_lock(&thread_lock);
		gdk_threads_enter();
		fprintf(stderr,"SIGNAL: %s\n",kakao_signal[UPDATE_MESSAGES_LIST]); fflush(stderr);
		/*отправляем размер сигнала*/
		sz=htonl(get_str_size(kakao_signal[UPDATE_MESSAGES_LIST]));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE); 
			continue;
		}
		/*отправляем сигнал*/
		if( !(ret=sendall(sock,kakao_signal[UPDATE_MESSAGES_LIST],ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE); 
			continue;
		}
		
		/*принимаем размер общего количества сообщений*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of total number of messages(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			continue;
		}
		/*принимаем общее количество сообщений*/
		if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL total number of messages(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			continue;
		}
		if(atoi(buf)!=atoi(total_messages))//если появились новые сообщения
		{
			strcpy(total_messages,buf);
			/*отправляем отрицательный ответ*/
			sz=htonl(0);
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(stderr, "ERROR %s : SEND 0 answer(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_MESSAGES_LIST]);
				show_error_window(buf,NOTCLOSE); 
				continue;
			}
			show_messages_list(NULL,NULL);
			unread_dialogs(NULL,NULL);

			gdk_threads_leave();
			//pthread_mutex_unlock(&thread_lock);
			usleep(FPS_MESSAGES_LIST);
			continue;
		}
		/*отправляем положительный ответ*/
		sz=htonl(1);
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND 1 answer(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE); 
			continue;
		}



		/*принимаем размер общего количества пишущих собеседников*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of writing companions(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			continue;
		}
		/*принимаем общее количество печатающих собеседников*/
		if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL writing companions(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			continue;
		}
		if(atoi(buf)!=atoi(total_writing_companions))// если кто-то начал или закончил печатать сообщение
		{
			strcpy(total_writing_companions,buf);
			/*отправляем отрицательный ответ*/
			sz=htonl(0);
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(stderr, "ERROR %s : SEND 0 answer(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_MESSAGES_LIST]);
				show_error_window(buf,NOTCLOSE); 
				continue;
			}
			show_messages_list(NULL,NULL);
			
			gdk_threads_leave();
			//pthread_mutex_unlock(&thread_lock);
			usleep(FPS_MESSAGES_LIST);
			continue;
		}
		/*отправляем положительный ответ*/
		sz=htonl(1);
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND 1 answer(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE); 
			continue;
		}



		/*принимаем размер общего количества собеседников в сети*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of total number of online companions(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			continue;
		}
		/*принимаем общее количество собеседников в сети*/
		if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL total number of online companions(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			continue;
		}
		if(atoi(buf)!=atoi(total_online_companions))// если кто-то ушел в офлайн или появился в сети
		{
			strcpy(total_online_companions,buf);
			/*отправляем отрицательный ответ*/
			sz=htonl(0);
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(stderr, "ERROR %s : SEND 0 answer(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_MESSAGES_LIST]);
				show_error_window(buf,NOTCLOSE); 
				continue;
			}
			show_messages_list(NULL,NULL);
			
			gdk_threads_leave();
			//pthread_mutex_unlock(&thread_lock);
			usleep(FPS_MESSAGES_LIST);
			continue;
		}
		/*отправляем положительный ответ*/
		sz=htonl(1);
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND 1 answer(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE); 
			continue;
		}


		/*принимаем размер общего количества собеседников не в сети*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of total number of not online companions(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			continue;
		}
		/*принимаем общее количество собеседников не в сети*/
		if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL total number of not online companions(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			continue;
		}
		if(atoi(buf)!=atoi(total_notonline_companions))// если кто-то ушел в офлайн или появился в сети
		{
			strcpy(total_notonline_companions,buf);
			/*отправляем отрицательный ответ*/
			sz=htonl(0);
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(stderr, "ERROR %s : SEND 0 answer(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_MESSAGES_LIST]);
				show_error_window(buf,NOTCLOSE); 
				continue;
			}
			show_messages_list(NULL,NULL);
			
			gdk_threads_leave();
			//pthread_mutex_unlock(&thread_lock);
			usleep(FPS_MESSAGES_LIST);
			continue;
		}
		/*отправляем положительный ответ*/
		sz=htonl(1);
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND 1 answer(%d)\n",kakao_signal[UPDATE_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE); 
			continue;
		}
		
		gdk_threads_leave();
		//pthread_mutex_unlock(&thread_lock);
		usleep(FPS_MESSAGES_LIST);
	}
	return NULL;//завершаем
}
void *update_party_messages_list(void *data)
{
	flag_party_messages_list=TRUE;
	char total_messages[MYSQL_INT]="0";
	char total_writing_group_companions[MYSQL_INT]="0";
	char buf[MAX_BUF];
	uint32_t sz;
	short int ret;
	while(flag_party_messages_list)
	{
		//pthread_mutex_lock(&thread_lock);
		gdk_threads_enter();
		fprintf(stderr,"SIGNAL: %s\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST]); fflush(stderr);
		/*отправляем размер сигнала*/
		sz=htonl(get_str_size(kakao_signal[UPDATE_PARTY_MESSAGES_LIST]));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_PARTY_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE); 
			continue;
		}
		/*отправляем сигнал*/
		if( !(ret=sendall(sock,kakao_signal[UPDATE_PARTY_MESSAGES_LIST],ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_PARTY_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE); 
			continue;
		}
		
		/*принимаем размер общего количества сообщений*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of total number of messages(%d)\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_PARTY_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			continue;
		}
		/*принимаем общее количество сообщений*/
		if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL total number of messages(%d)\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_PARTY_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			continue;
		}
		if(atoi(buf)!=atoi(total_messages))//если появились новые сообщения
		{
			strcpy(total_messages,buf);
			/*отправляем отрицательный ответ*/
			sz=htonl(0);
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(stderr, "ERROR %s : SEND 0 answer(%d)\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_PARTY_MESSAGES_LIST]);
				show_error_window(buf,NOTCLOSE); 
				continue;
			}
			show_party_messages_list(NULL,NULL);
			unread_dialogs(NULL,NULL);

			gdk_threads_leave();
			//pthread_mutex_unlock(&thread_lock);
			usleep(FPS_MESSAGES_LIST);
			continue;
		}
		/*отправляем положительный ответ*/
		sz=htonl(1);
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND 1 answer(%d)\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_PARTY_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE); 
			continue;
		}

		/*принимаем размер общего количества пишущих участников группы*/
		if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECV size of writing party companions(%d)\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_PARTY_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			continue;
		}
		/*принимаем общее количество печатающих участников группы */
		if( (ret=recvall(sock,buf,ntohl(sz),MSG_WAITALL))==0 ) {
			show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
		}
		else if(ret<0) {
			fprintf(stderr, "ERROR %s : RECVALL writing party companions(%d)\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_PARTY_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE);
			continue;
		}
		if(atoi(buf)!=atoi(total_writing_group_companions))// если кто-то начал или закончил печатать сообщение
		{
			strcpy(total_writing_group_companions,buf);
			/*отправляем отрицательный ответ*/
			sz=htonl(0);
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(stderr, "ERROR %s : SEND 0 answer(%d)\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_PARTY_MESSAGES_LIST]);
				show_error_window(buf,NOTCLOSE); 
				continue;
			}
			show_party_messages_list(NULL,NULL);
			
			gdk_threads_leave();
			//thread_mutex_unlock(&thread_lock);
			usleep(FPS_MESSAGES_LIST);
			continue;
		}
		/*отправляем положительный ответ*/
		sz=htonl(1);
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND 1 answer(%d)\n",kakao_signal[UPDATE_PARTY_MESSAGES_LIST],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[UPDATE_PARTY_MESSAGES_LIST]);
			show_error_window(buf,NOTCLOSE); 
			continue;
		}
		
		gdk_threads_leave();
		//pthread_mutex_unlock(&thread_lock);
		usleep(FPS_MESSAGES_LIST);
	}
	return NULL;//завершаем
}
gboolean send_event_return(GtkWidget *widget,GdkEventKey  *event,sig_data *data)
{
	long long int sec;
	if(event->keyval==GDK_KEY_Return || event->keyval==GDK_KEY_KP_Enter || event->keyval==GDK_KEY_ISO_Enter || event->keyval==GDK_KEY_3270_Enter || event->keyval==GDK_KEY_RockerEnter)
	{
		fprintf(stderr,"\nSEND_EVENT_RETURN: %s\n", kakao_signal[SEND_MESSAGE]); fflush(stderr);
		send_message(widget,data);
		return TRUE;//не распространять событие дальше(клавиша не пойдет в буфер)
	}
	else if(((sec=time(NULL))-last_write_second)>WRITING_TIME)
	{
		fprintf(stderr,"EVENT_NOTRETURN: %s\n",event->string); fflush(stderr);
		last_write_second=sec;
		/*отправляем размер сигнала*/
		sz=htonl(get_str_size(kakao_signal[I_WRITE]));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[I_WRITE],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[I_WRITE]);
			show_error_window(buf,NOTCLOSE); 
			return FALSE;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем сигнал*/
		if( !(ret=sendall(sock,kakao_signal[I_WRITE],ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[I_WRITE],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[I_WRITE]);
			show_error_window(buf,NOTCLOSE); 
			return FALSE;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем размер типа сообщения*/
		sz=htonl(get_str_size(data->dialog_type));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of message type(%d)\n",kakao_signal[I_WRITE],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[I_WRITE]);
			show_error_window(buf,NOTCLOSE); 
			return FALSE;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем тип сообщения*/
		if( !(ret=sendall(sock,data->dialog_type,ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL message type(%d)\n",kakao_signal[I_WRITE],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[I_WRITE]);
			show_error_window(buf,NOTCLOSE); 
			return FALSE;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		if(strcmp(data->dialog_type,text_dialog_type[COMPANION])==0)
		{
			/*отправляем размер id юзера*/
			sz=htonl(get_str_size(data->to_id));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(stderr, "ERROR %s : SEND size of people id(%d)\n",kakao_signal[I_WRITE],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[I_WRITE]);
				show_error_window(buf,NOTCLOSE); 
				return FALSE;//выходим из функции, вынуждая пользователя заново отправить сигнал
			}
			/*отправляем id юзера*/
			if( !(ret=sendall(sock,data->to_id,ntohl(sz))) ) {
				fprintf(stderr, "ERROR %s : SENDALL people id(%d)\n",kakao_signal[I_WRITE],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[I_WRITE]);
				show_error_window(buf,NOTCLOSE); 
				return FALSE;//выходим из функции, вынуждая пользователя заново отправить сигнал
			}
		}
		else if(strcmp(data->dialog_type,text_dialog_type[PARTY])==0)
		{
			/*отправляем размер id группы*/
			sz=htonl(get_str_size(data->party_id));
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(stderr, "ERROR %s : SEND size of party id(%d)\n",kakao_signal[I_WRITE],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[I_WRITE]);
				show_error_window(buf,NOTCLOSE); 
				return FALSE;//выходим из функции, вынуждая пользователя заново отправить сигнал
			}
			/*отправляем id группы*/
			if( !(ret=sendall(sock,data->party_id,ntohl(sz))) ) {
				fprintf(stderr, "ERROR %s : SENDALL party id(%d)\n",kakao_signal[I_WRITE],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[I_WRITE]);
				show_error_window(buf,NOTCLOSE); 
				return FALSE;//выходим из функции, вынуждая пользователя заново отправить сигнал
			}
		}
		return FALSE;//распространять событие дальше(клавиша пойдет в буфер)
	}
	else return FALSE;
	//event->send_event=FALSE;//нужно обнулить, чтобы стандартный обработчик не стал обрабатывать сигнал после этого

}
void bob_send_file(GtkWidget *widget, sig_data *data)
{
	char path[MAX_PATH];
	char filename[MAX_FILENAME];
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[BOB_SEND_FILE]); fflush(stderr);
	gtk_popover_popdown(GTK_POPOVER(dialog_window.popover1));//скрываем виджет отправки файлов или фото
	const gchar *text=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog_window.Button5));//считываем путь файла
	if(text!=NULL) {
		strcpy(path,text);//путь до файла
		strcpy(filename,from_path_to_filename(path));//извлекаем имя файла из пути
		g_free((gpointer) text);
	}
	else {
		g_free((gpointer) text);
		return;
	}
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[BOB_SEND_FILE]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_FILE]);
		show_error_window(buf,NOTCLOSE); 
		return;
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[BOB_SEND_FILE],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_FILE]);
		show_error_window(buf,NOTCLOSE); 
		return;
	}
	/*отправляем размер типа диалога*/
	sz=htonl(get_str_size(data->dialog_type) );
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of dialog type(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_FILE]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем тип диалога*/
	if( !(ret=sendall(sock,data->dialog_type,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_FILE]);
		show_error_window(buf,NOTCLOSE); 
		return;
	}

	if(strcmp(data->dialog_type,text_dialog_type[COMPANION])==0)  {
		/*отправляем размер id пользователя*/
		sz=htonl(get_str_size(data->to_id));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of people id(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_FILE]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем id пользователя*/
		if( !(ret=sendall(sock,data->to_id,ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL people id(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_FILE]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
	} 
	else if(strcmp(data->dialog_type,text_dialog_type[PARTY])==0) {
		/*отправляем размер id группы*/
		sz=htonl(get_str_size(data->party_id));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of party id(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_FILE]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем id группы*/
		if( !(ret=sendall(sock,data->party_id,ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL party id(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_FILE]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
	}
	/*отправляем размер имени файла*/
	sz=htonl(get_str_size(filename));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of filename(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_FILE]);
		show_error_window(buf,NOTCLOSE); 
		return;
	}
	/*отправляем имя файла*/
	if( !(ret=sendall(sock,filename,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL filename(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_FILE]);
		show_error_window(buf,NOTCLOSE); 
		return;
	}

	/*принимаем ответ(можно ли отправлять данные)*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV answer(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_FILE]);
		show_error_window(buf,NOTCLOSE);
		return;
	}
	if(ntohl(sz))//если ответ положительный
	{
		/*отправляем сам файл*/
		if(!send_file(sock,path)) {//если произошла ошибка
			/*отправляем ноль*/
			sz=htonl(0);
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(stderr, "ERROR %s : SEND zero(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_FILE]);
				show_error_window(buf,NOTCLOSE); 
				return;
			}
		}
	} 

	
}
void bob_send_photo(GtkWidget *widget, sig_data *data)
{
	char path[MAX_PATH];
	char filename[MAX_FILENAME];
	fprintf(stderr,"SIGNAL: %s\n",kakao_signal[BOB_SEND_PHOTO]); fflush(stderr);
	gtk_popover_popdown(GTK_POPOVER(dialog_window.popover1));//скрываем виджет отправки файлов или фото
	const gchar *text=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog_window.Button7));//считываем путь файла
	if(text!=NULL) {
		strcpy(path,text);//путь до файла
		strcpy(filename,from_path_to_filename(path));//извлекаем имя файла из пути
		g_free((gpointer) text);
	}
	else {
		g_free((gpointer) text);
		return;
	}
	/*отправляем размер сигнала*/
	sz=htonl(get_str_size(kakao_signal[BOB_SEND_PHOTO]));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of signal(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_PHOTO]);
		show_error_window(buf,NOTCLOSE); 
		return;
	}
	/*отправляем сигнал*/
	if( !(ret=sendall(sock,kakao_signal[BOB_SEND_PHOTO],ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_PHOTO]);
		show_error_window(buf,NOTCLOSE); 
		return;
	}
	/*отправляем размер типа диалога*/
	sz=htonl(get_str_size(data->dialog_type) );
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of dialog type(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_PHOTO]);
		show_error_window(buf,NOTCLOSE); 
		return;//выходим из функции, вынуждая пользователя заново отправить сигнал
	}
	/*отправляем тип диалога*/
	if( !(ret=sendall(sock,data->dialog_type,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL signal(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_PHOTO]);
		show_error_window(buf,NOTCLOSE); 
		return;
	}

	if(strcmp(data->dialog_type,text_dialog_type[COMPANION])==0)  {
		/*отправляем размер id пользователя*/
		sz=htonl(get_str_size(data->to_id));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of people id(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_PHOTO]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем id пользователя*/
		if( !(ret=sendall(sock,data->to_id,ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL people id(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_PHOTO]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
	} 
	else if(strcmp(data->dialog_type,text_dialog_type[PARTY])==0) {
		/*отправляем размер id группы*/
		sz=htonl(get_str_size(data->party_id));
		if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
			fprintf(stderr, "ERROR %s : SEND size of party id(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_PHOTO]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
		/*отправляем id группы*/
		if( !(ret=sendall(sock,data->party_id,ntohl(sz))) ) {
			fprintf(stderr, "ERROR %s : SENDALL party id(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(stderr);
			sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_PHOTO]);
			show_error_window(buf,NOTCLOSE); 
			return;//выходим из функции, вынуждая пользователя заново отправить сигнал
		}
	}
	/*отправляем размер имени файла*/
	sz=htonl(get_str_size(filename));
	if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
		fprintf(stderr, "ERROR %s : SEND size of filename(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_PHOTO]);
		show_error_window(buf,NOTCLOSE); 
		return;
	}
	/*отправляем имя файла*/
	if( !(ret=sendall(sock,filename,ntohl(sz))) ) {
		fprintf(stderr, "ERROR %s : SENDALL filename(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_PHOTO]);
		show_error_window(buf,NOTCLOSE); 
		return;
	}

	/*принимаем ответ(можно ли отправлять данные)*/
	if( (ret=recv(sock,&sz,sizeof(sz),MSG_WAITALL))==0 ) {
		show_error_window("Сервер отключился\n",CLOSE);//закрываем программу
	}
	else if(ret<0) {
		fprintf(stderr, "ERROR %s : RECV answer(%d)\n",kakao_signal[BOB_SEND_PHOTO],ret ); fflush(stderr);
		sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_PHOTO]);
		show_error_window(buf,NOTCLOSE);
		return;
	}
	if(ntohl(sz))//если ответ положительный
	{
		/*отправляем само фото*/
		if(!send_file(sock,path)) {//если произошла ошибка
			/*отправляем ноль*/
			sz=htonl(0);
			if( !(ret=send(sock,&sz,sizeof(sz),0)) ) {
				fprintf(stderr, "ERROR %s : SEND zero(%d)\n",kakao_signal[BOB_SEND_FILE],ret ); fflush(stderr);
				sprintf(buf,"ERROR: %s\n",kakao_error[BOB_SEND_FILE]);
				show_error_window(buf,NOTCLOSE); 
				return;
			}
		}
	} 

	
}







/*gtk_widget_destroy(reg_window.Window); - удаляет все дочерние элементы и ссылки, например:
 При вызове my_info.nickname=gtk_entry_get_text(GTK_ENTRY(reg_window.Nickname)) создается указатель на текст после вызова 
 gtk_widget_destroy(reg_window.Window); этот текст на который указывает nickname затирается*/

