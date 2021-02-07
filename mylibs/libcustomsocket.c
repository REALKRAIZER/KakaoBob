#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
void bind_to_addr(int socket,char*ip,int port)
{
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);
	if(bind(socket,(struct sockaddr*)&addr,sizeof(addr))==-1)
	{
		perror("bind");
		exit(1);
	}
}
int recvall(int socket,char*buf,int size,int flag)
{
	short int total=0,len;
	do{
		len=recv(socket,&buf[total],size-total,flag);
 		switch(len)
 		{
 			case -1:
 			perror("recv");
 			return -1;
 			case 0:
 			perror("recv2");
 			return 0;
 		}
 		total=total+len;
	}while(buf[total-1]!='\0' );

	return total;
}
int sendall(int socket,char*buf,int size)
{
	short int len,total=0;
	do{
		if((len=send(socket,&buf[total],size-total,0))==-1)
		{
			perror("sendall");
			return -1;
		}
		total+=len;
	}while(total!=len);
	return total;
}
int send_file(int socket,char *path)//перед использованием этой функции нужно еще отправлять название файла
{
	uint32_t sz;//размер сообщения
	int i=0;
	char tmp[512];//сюда записываем сам файл который передаем
	FILE *file=fopen(path,"rb");
	if(file==NULL) {
		perror("FILE"); fflush(stderr);
		return 0;
	}
	while( (i=fread(tmp,1,sizeof(tmp),file)) >0 )//записываем в tmp файл
	{
		sz=htonl(sizeof(char)*i);
		if(send(socket,&sz,sizeof(sz),0)==-1) {//передаем размер файла
			perror("send1");
			fclose(file);
			return 0;
		}
		if(send(socket,tmp,sizeof(char)*i,0)==-1) {//передаем файл
			perror("send2");
			fclose(file);
			return 0;
		}
		//memset(tmp,0,sizeof(tmp));
	}
	sz=htonl(0);
	if(send(socket,&sz,sizeof(sz),0)==-1) {//передаем нулевой размер 
		perror("send3");
		fclose(file);
		return 0;
	}
	if(feof(file)==0) {
		perror("fread");
		fclose(file);
		return 0;
	}
	fclose(file);
	return 1;
}

int recv_file(int socket,char *path,float *size)//перед использованием этой функции нужно еще принять название файла
{
	uint32_t sz;//размер сообщения
	int ret=0;
	unsigned long long int byte_size=0;
	char tmp[512];//сюда записываем сам файл который передаем
	FILE *file=fopen(path,"wb");//записываем принятый файл в каталог пользователя или группы
	if(file==NULL) {
		perror("FILE"); fflush(stderr);
		return 0;
	}
	while( (ret=recv(socket,&sz,sizeof(sz),MSG_WAITALL)) )//принимаем размер файла
	{
		if(ntohl(sz)==0) break;
		//memset(tmp,0,sizeof(tmp));
		if(( ret=recv(socket,tmp,ntohl(sz),MSG_WAITALL) )<0) {//принимаем файл
			perror("recv1");
			fclose(file);
			return 0;
		}
		else if(size!=NULL) {//записываем размер файла в байтах
			byte_size+=ret;
		}
		if(fwrite(tmp,1,sizeof(tmp),file)!=sizeof(tmp)) {//сохраняем принятый файл(записываем принятый файл в каталог)
			if(feof(file)==0) {
				perror("fwrite");
				fclose(file);
				return 0;
			}
		}
	}
	if(ret==-1) {
		perror("recv2");
		fclose(file);
		return 0;
	} 
	if(size!=NULL) {//записываем размер файла в мегабайтах
		*size=(float) byte_size / 1048576;
	}
	fclose(file);
	return 1;
}