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
        client_set_package csp(argv[2], argv[3]);
        int ret = send(sockfd, (void*)&csp, ntohs(csp.header.package_length), 0);
        client_set_res_package csrp;
        ret = recv(sockfd, (void*)&csrp, sizeof(csrp), MSG_DONTWAIT);
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