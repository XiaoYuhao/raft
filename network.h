#ifndef _NETWORK_H
#define _NETWORK_H

int startup(int _port);

void addfd(int epollfd, int fd);

void removefd(int epollfd, int fd);

int get_client_sockfd();

#endif