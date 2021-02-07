#define MAX_NICKNAME 21
#define MAX_PASSWORD 65
#define MAX_PARTYNAME 21
#define MAX_PATH 4096
#define MAX_FILENAME 256
#define WRITING_TIME 2
#define MAX_PARTY_NICKNAME_WRITINGS 2
#define ONLINE_TIME 60
#define NOTONLINE_TIME 3600
#define MAX_BUF 4096
#define MAX_QUERY_BUF 256+MAX_BUF
#define LENGTH_NUM_MES 11//10^9 (или 9 нулей после цифры)
#define FPS_DIALOG_MESSAGES 1000000*4
#define FPS_DIALOG 1000000*2
#define FPS_MESSAGES_LIST 1000000*4
#define FPS_UNREAD_DIALOGS 1000000*4
#define FPS_ERROR 1000000*4
#define MYSQL_INT 11//10^9 (или 9 нулей после цифры)
uint32_t sz;//размер сообщения

typedef struct some_data
{
	FILE *log;
	MYSQL *conn;
	char *query_buf;
	char *id;
	int sock;
	char *recv_buf;
}some_data;
typedef struct user_info
{
	char nickname[MAX_NICKNAME];
	char password[MAX_PASSWORD];
	char id[MYSQL_INT];
	char to_id[MYSQL_INT];
	char party_id[MYSQL_INT];
}info;
typedef struct send_data
{
	char signal[50];
	char to_id[MYSQL_INT];
	char data[MAX_BUF];

}send_data;
enum kakao_signal
{
	SEND_MESSAGE,
	SHOW_MESSAGES,
	GET_REG_FORM,
	GET_LOG_FORM,
	SHOW_MY_SUBS,
	SHOW_FRIEND_LIST,
	SHOW_MESSAGES_LIST,
	SHOW_PEOPLE_LIST,
	SHOW_PROFILE_WINDOW,
	CHANGE_PASSWORD,
	CHANGE_NICKNAME,
	DELETE_FRIEND,
	DELETE_SUB,
	DELETE_FRIEND_OR_SUB,
	ADD_FRIEND,
	ADD_SUB,
	ADD_FRIEND_OR_SUB,
	UPDATE_DIALOG_MESSAGES,
	BOB_START_INIT,
	DARK_ERROR,
	BOB_CREATE_PARTY,
	SHOW_PROFILE_PARTY,
	CHANGE_PARTY_NAME,
	CHANGE_PARTY_DESCRIPTION,
	SHOW_PARTY_APPOINT_ADMINS,
	SHOW_PARTY_DELETE_ADMINS,
	PARTY_ADD_ADMIN,
	PARTY_DELETE_ADMIN,
	SHOW_PARTY_ADD_FRIENDS,
	SHOW_PARTY_DELETE_FRIENDS,
	PARTY_ADD_FRIEND,
	PARTY_DELETE_USER,
	PARTY_EXIT,
	DELETE_MESSAGE_EVERYWHERE,
	EDIT_MESSAGE,
	I_WRITE,
	UPDATE_DIALOG,
	SHOW_DIALOG_WINDOW,
	USER_ONLINE,
	SHOW_PARTY_MESSAGES_LIST,
	BLOCK_USER,
	UNBLOCK_USER,
	USER_WRITING,
	PARTY_WRITING,
	UPDATE_MESSAGES_LIST,
	UPDATE_PARTY_MESSAGES_LIST,
	UNREAD_DIALOGS,
	UPDATE_UNREAD_DIALOGS,
	BOB_SEND_FILE,
	ALLOW_SEND_DATA_TO_COMPANION,
	DOWNLOAD,
	ALLOW_SEND_DATA_TO_PARTY,
	BOB_SEND_PHOTO,
	SHOW_SETTING_PROFILE,
	DELETE_MY_AVATAR,
	CHANGE_MY_AVATAR,
	SHOW_PROFILE_PARTY_AVATAR,
	CHANGE_PARTY_AVATAR,
	DELETE_PARTY_AVATAR,
	SHOW_PROFILE_PARTY_USERS,
	DELETE_MY_MESSAGES_FROM_DIALOG
};
 char *kakao_signal[]=
{
	"SEND_MESSAGE",
	"SHOW_MESSAGES",
	"GET_REG_FORM",
	"GET_LOG_FORM",
	"SHOW_MY_SUBS",
	"SHOW_FRIEND_LIST",
	"SHOW_MESSAGES_LIST",
	"SHOW_PEOPLE_LIST",
	"SHOW_PROFILE_WINDOW",
	"CHANGE_PASSWORD",
	"CHANGE_NICKNAME",
	"DELETE_FRIEND",
	"DELETE_SUB",
	"DELETE_FRIEND_OR_SUB",
	"ADD_FRIEND",
	"ADD_SUB",
	"ADD_FRIEND_OR_SUB",
	"UPDATE_DIALOG_MESSAGES",
	"BOB_START_INIT",
	"DARK_ERROR",
	"BOB_CREATE_PARTY",
	"SHOW_PROFILE_PARTY",
	"CHANGE_PARTY_NAME",
	"CHANGE_PARTY_DESCRIPTION",
	"SHOW_PARTY_APPOINT_ADMINS",
	"SHOW_PARTY_DELETE_ADMINS",
	"PARTY_ADD_ADMIN",
	"PARTY_DELETE_ADMIN",
	"SHOW_PARTY_ADD_FRIENDS",
	"SHOW_PARTY_DELETE_FRIENDS",
	"PARTY_ADD_FRIEND",
	"PARTY_DELETE_USER",
	"PARTY_EXIT",
	"DELETE_MESSAGE_EVERYWHERE",
	"EDIT_MESSAGE",
	"I_WRITE",
	"UPDATE_DIALOG",
	"SHOW_DIALOG_WINDOW",
	"USER_ONLINE",
	"SHOW_PARTY_MESSAGES_LIST",
	"BLOCK_USER",
	"UNBLOCK_USER",
	"USER_WRITING",
	"PARTY_WRITING",
	"UPDATE_MESSAGES_LIST",
	"UPDATE_PARTY_MESSAGES_LIST",
	"UNREAD_DIALOGS",
	"UPDATE_UNREAD_DIALOGS",
	"BOB_SEND_FILE",
	"ALLOW_SEND_DATA_TO_COMPANION",
	"DOWNLOAD",
	"ALLOW_SEND_DATA_TO_PARTY",
	"BOB_SEND_PHOTO",
	"SHOW_SETTING_PROFILE",
	"DELETE_MY_AVATAR",
	"CHANGE_MY_AVATAR",
	"SHOW_PROFILE_PARTY_AVATAR",
	"CHANGE_PARTY_AVATAR",
	"DELETE_PARTY_AVATAR",
	"SHOW_PROFILE_PARTY_USERS",
	"DELETE_MY_MESSAGES_FROM_DIALOG"
};
 char *kakao_error[]=
{
	"Не удалось отправить сообщение:(",
	"Не удалось получить сообщения:(",
	"Не получилось зарегистрироваться:(\nЭто не ваша вина",
	"Не получилось авторизоваться:(\nЭто не ваша вина",
	"Не удалось получить список ваших подписчиков:(",
	"Не удалось получить список друзей:(",
	"Не удалось получить список сообщений:(",
	"Не удалось получить список пользователей:(",
	"Не получилось загрузить профиль:(",
	"Не получилось изменить пароль:(",
	"Не получилось изменить никнейм:(",
	"Не получилось удалить друга:(",
	"Не получилось удалить подписчика:(",
	"Не получилось удалить друга или подписчика:(",
	"Не получилось добавить друга:(",
	"Не получилось добавить подписчика:(",
	"Не получилось добавить друга или подписчика:(",
	"Произошла ошибка при обновлении окна:(",
	"Произошла ошибка при инициализации программы:(",
	"Произошла непонятная ошибка:("
	"Не удалось создать группу:(",
	"Не удалось показать профиль группы;(",
	"Не удалось показать имя группы:(",
	"Не получилось изменить описание группы:(",
	"Произошла ошибка:(",
	"Произошла ошибка:(",
	"Не получилось добавить администратора:(",
	"Не получилось удалить администратора:(",
	"Произошла ошибка:(",
	"Произошла ошибка:(",
	"Не получилось добавить друга:(",
	"Не получилось удалить пользователя:(",
	"Не получилось выйти из группы:(",
	"Не получилось удалить сообщение:(",
	"Не получилось изменить сообщение:(",
	"Произошла ошибка при печати сообщения:(",
	"Произошла ошибка при обновлении диалогового окна;(",
	"Произошла ошибка при обновлении диалогового окна;(",
	"Не можем получить в сети ли пользователь;(",
	"Не удается получить список сообщений:(",
	"Не получилось заблокировать пользователя;(",
	"Не получилось разблокировать пользователя;(",
	"Ошибка;(",
	"Ошибка;(",
	"Не получилось обновить окно сообщений;(",
	"Не получилось обновить окно сообщений;(",
	"Не получилось обновить окно непрочитанных диалогов;(",
	"Не получилось обновить окно непрочитанных диалогов;(",
	"Не получилось отправить файл;(",
	"Ошибка;(",
	"Не получилось скачать файл или изображение;(",
	"Не получилось отправить файл;(",
	"Не получилось отправить фото;(",
	"Не удалось получить некоторые данные о вашем профиле;(",
	"Не получилось удалить аватарку;(",
	"Не получилось изменить аватарку;(",
	"Не удалось получить аватарку;(",
	"Не удалось изменить аватарку;(",
	"Не получилось удалить аватарку;(",
	"Не удалось получить пользователей группы;(",
	"Не получилось удалить сообщения;("
};

typedef enum H_Position 
{
	LEFT,
	RIGHT,
	CENTER,
} mes_hposition;
 char *text_mes_hpos[]=
{
	"LEFT",
	"RIGHT",
	"CENTER",
};

typedef enum dialog_type
{
	COMPANION,
	PARTY
} dialog_type;

 char *text_dialog_type[]=
{
	"COMPANION",
	"PARTY"
};

typedef enum mes_type
{
	BOB_FILE,
	PHOTO,
	TEXT
} mes_type;

typedef enum when_online 
{
	ONLINE,
	WAS_ONLINE_RECENTLY,
	NOT_ONLINE
} when_online;
 char *server_text_when_online[]=
{
	"ONLINE",
	"WAS_ONLINE_RECENTLY",
	"NOT_ONLINE",
};