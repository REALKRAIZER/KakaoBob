#include <sha2.h>
#include <stdio.h>
#include <ctype.h>
#define MAX_HASH256 65
int get_str_size(char *str)
{
	int i=0;
	int total=0;
	for(i;str[i]!='\0';++i)
	{
		total=total+sizeof(str[i]);
	}
	if(str[i]=='\0')
	{
		total=total+1;//добавляем размер нуль символа
	}
	return total;
}
char *my_sha256(char *str,size_t n)
{
	unsigned char *hash=SHA256(str,n,NULL);
	//printf("strlen%ld strlen: %ld\n",strlen(hash),strlen("Привет"));
	static char _p_sha256[MAX_HASH256];
	for(int i=0, c=0;i!=32;++i,c+=2) {
		sprintf(_p_sha256+c,"%02x",*(hash+i) );
	}
	return _p_sha256;
}
char *from_path_to_filename(char *way)//из формата /way/way/filename.расширение извлекаем filename.расширение
{
	int ctr=-1;
	for(int i=0;way[i]!='\0';++i)
	{
		if(way[i]=='/') {
			ctr=i;
		}
	}
	if(ctr==-1) return NULL;//если не получилось извлечь filename
	else return (way+ctr+1);
}
char *from_filename_to_extension(char *filename)//из формата filename.расширение извлекаем расширение
{
	int ctr=-1;
	for(int i=0;filename[i]!='\0';++i)
	{
		if(filename[i]=='.') {
			ctr=i;
			break;
		}
	}
	if(ctr==-1) return NULL;//если не получилось извлечь расширение(его нет например)
	else return (filename+ctr+1);
}
int strcmpi(char *str1,char *str2)//сравниваем строки без учета регистра
{
	char c1;
	char c2;
	long long int i;
	for(i=0;str1[i]!='\0' && str2[i]!='\0';++i) {
		c1=tolower(str1[i]); c2=tolower(str2[i]);
		if(c1==c2) continue;
		else if(c1>c2) return 1;
		else return -1;
	}
	c1=tolower(str1[i-1]); c2=tolower(str2[i-1]);
	if(c1==c2) return 0;
	else if(c1>c2) return 1;
	else return -1;
}