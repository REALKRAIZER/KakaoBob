#include <stdio.h>
int recvall(int socket,char*buf,int len,int flag);
void bind_to_addr(int socket,char*ip,int port);
int sendall(int socket,char*buf,int size);
int recv_file(int socket,char *path,float *size);
int send_file(int socket,char *path);