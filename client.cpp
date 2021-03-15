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
#include "network.h"
#include "package.h"

using namespace std;

char server_ip[64] = "111.231.146.249";
int port = 11234;

void db_set(const char *key, const char *val){
    while(1){
        int sockfd = connect_to_server(port, server_ip);
        client_set_package csp(key, val);
        int ret = send(sockfd, (void*)&csp, ntohs(csp.header.package_length), 0);
        client_set_res_package csrp;
        ret = recv(sockfd, (void*)&csrp, sizeof(csrp), MSG_DONTWAIT);
        csrp.tohost();
        cout<<csrp.status<<" "<<csrp.ip_addr<<" "<<csrp.port<<endl;
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

int main(int argc, char *argv[]){
    if(argc<=1){
        printf("please input correct params!\n");
        return 0;
    }
    int sockfd = connect_to_server(11234, "0.0.0.0");
    if(sockfd<=0){
        cout<<"connect failed."<<endl;
    }
    if(!strcmp(argv[1], "set")&&argc==4){
        db_set(argv[2], argv[3]);
        return 0;
    }
    else if(!strcmp(argv[1], "get")&&argc==3){
        
    }
    else{
        printf("please input correct params!\n");
        return 0;
    }
    //create_data();
    return 0;
}