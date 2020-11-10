#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <assert.h>  
#include <stdio.h>  
#include <unistd.h>  
#include <errno.h>  
#include <string.h>  
#include <string>
#include <map>
#include <fcntl.h>  
#include <stdlib.h>  
#include <sys/epoll.h>  
#include <signal.h>  
#include <sys/wait.h>  
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>
#include <iostream>
#include "network.h"

int startup(int _port){
    int sock=socket(AF_INET,SOCK_STREAM,0);
    if(sock<0){
        perror("socket");
        exit(1);
    }
    int val;
    if(val=fcntl(sock,F_GETFL,0)<0){
        perror("fcntl");
        close(sock);
        return 0;
    }
    if(fcntl(sock,F_SETFL,val|O_NONBLOCK)<0){
        perror("fcntl");
        close(sock);
        return 0;
    }

    int opt=1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(const void*)&opt,sizeof(opt));

    struct sockaddr_in local;
    local.sin_family=AF_INET;
    local.sin_port=htons(_port);
    local.sin_addr.s_addr=htonl(INADDR_ANY);
    socklen_t len=sizeof(local);

    if(bind(sock,(struct sockaddr*)&local,len)<0){
        perror("bind");
        exit(2);
    }
    
    if(listen(sock,5)<0){
        perror("listen");
        exit(3);
    }

    return sock;
}

void addfd(int epollfd, int fd){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
}

void removefd(int epollfd, int fd){
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

int connect_to_server(const int _port, const char *ip_addr){
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd<0)
	{
		perror("socket");
		return -1;
	}
	
	int val;
	if(val=fcntl(sockfd,F_GETFL,0)<0){//获取文件状态标志
		perror("fcntl");
		close(sockfd);
		return -1;
	}
	if(fcntl(sockfd,F_SETFL,val|O_NONBLOCK)<0){//设置文件状态标志
		perror("fcntl");
		close(sockfd);
		return -1;
	}
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(_port);
    server.sin_addr.s_addr = inet_addr(ip_addr);
    socklen_t len = sizeof(sockaddr_in);

    int ret;
    ret = connect(sockfd,(struct sockaddr*)&server,len);
    if(ret<0&&errno!=EINPROGRESS){
        //std::cout<<"connect failed."<<std::endl;
        return -1;
    }
    struct timeval tm;
    tm.tv_sec = 0;
    tm.tv_usec = 50*1000;
    fd_set wfd;
	FD_ZERO(&wfd);
	FD_SET(sockfd,&wfd);
	switch(select(sockfd+1,NULL,&wfd,NULL,&tm))
	{
		case -1:
			perror("select");
			return -1;
		case 0:
			printf("超时！\n");
			return 0;
		default:
            //std::cout<<"connect successful."<<std::endl;
			return sockfd;
	}
	return sockfd;
}