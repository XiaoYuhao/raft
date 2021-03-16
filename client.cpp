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
#include <time.h>
#include <sys/time.h>
#include <random>
#include <iostream>
#include "package.h"

using namespace std;

int connect_to_server(const char *ip_addr, const int port){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip_addr);
    socklen_t len = sizeof(sockaddr_in);

    int ret = 0;
    ret = connect(sockfd, (sockaddr*)&server, len);
    if(ret<0){
        printf("connect server failed...\n");
        return -1;
    }
    return sockfd;
}

char server_ip[64] = "47.116.133.175";
int port = 11234;

void db_set(const char *key, const char *val){
    while(1){
        int sockfd = connect_to_server(server_ip, port);
        client_set_package csp(key, val);
        int ret = send(sockfd, (void*)&csp, ntohs(csp.header.package_length), 0);
        client_set_res_package csrp;
        ret = recv(sockfd, (void*)&csrp, sizeof(csrp), MSG_WAITALL);
        printf("size = %d ret = %d\n", sizeof(csrp), ret);
        
        csrp.tohost();
        //cout<<csrp.header.package_type<<" "<<csrp.header.package_length<<" "<<csrp.status<<" "<<csrp.ip_addr<<endl;
        char buf[512];
        memcpy(buf, (void*)&csrp, sizeof(csrp));
        for(int i=0;i<sizeof(csrp);i++){
            printf("%02x ", buf[i]);
        }
        printf("\n");
        if(csrp.status==RES_SUCCESS){
            cout<<"Set key value successfully."<<endl;
            close(sockfd);
            break;
        }
        if(csrp.status==RES_FAIL){
            cout<<"Set key value failed."<<endl;
            close(sockfd);
            break;
        }
        if(csrp.status==RES_REDIRECT){
            port = csrp.port;
            strcpy(server_ip, csrp.ip_addr);
            cout<<"Redirect to current leader "<<server_ip<<" : "<<port<<endl;
        }
    }
    return;
}

void db_get(const char *key){
    while(1){
        int sockfd = connect_to_server(server_ip, port);
        client_get_package cgp(key);
        int ret = send(sockfd, (void*)&cgp, ntohs(cgp.header.package_length), 0);
        package_header header;
        client_get_res_package cgrp;
        fd_set rfd;
        FD_ZERO(&rfd);
        FD_SET(sockfd, &rfd);
        ret = 0;
        switch(select(sockfd+1, &rfd, NULL, NULL, NULL)){
            case -1:
                cout<<"select error.\n";
                break;
            case 0:
                cout<<"select timeout.\n";
                break;
            default:
                ret = recv(sockfd, (void*)&header, sizeof(header), MSG_PEEK);
                ret = recv(sockfd, (void*)&cgrp, ntohs(header.package_length), MSG_WAITALL);
                if(ret == 0){
                    cout<<"server quit.\n";
                    break;
                }
                break;
        }
        cgrp.tohost();
        //cout<<csrp.header.package_type<<" "<<csrp.header.package_length<<" "<<csrp.status<<" "<<csrp.ip_addr<<endl;
        /*char buf[512];
        memcpy(buf, (void*)&csrp, sizeof(csrp));
        for(int i=0;i<sizeof(csrp);i++){
            printf("%02x ", buf[i]);
        }
        printf("\n");*/
        if(cgrp.status==RES_SUCCESS){
            cout<<"Get key "<<key<<" successfully value = "<<cgrp.buf<<endl;
            close(sockfd);
            break;
        }
        if(cgrp.status==RES_FAIL){
            cout<<"Get key value failed."<<endl;
            close(sockfd);
            break;
        }
        if(cgrp.status==RES_REDIRECT){
            port = cgrp.port;
            strcpy(server_ip, cgrp.buf);
            cout<<"Redirect to current leader "<<server_ip<<" : "<<port<<endl;
        }
    }
    return;
}

int main(int argc, char *argv[]){
    if(argc<=1){
        printf("please input correct params!\n");
        return 0;
    }
    if(!strcmp(argv[1], "set")&&argc==4){
        db_set(argv[2], argv[3]);
        return 0;
    }
    else if(!strcmp(argv[1], "get")&&argc==3){
        db_get(argv[2]);
    }
    else{
        printf("please input correct params!\n");
        return 0;
    }
    //create_data();
    return 0;
}