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


int main(){
    int sockfd = connect_to_server(11234, "0.0.0.0");
    if(sockfd<=0){
        cout<<"connect failed."<<endl;
    }
    client_set_package csp("xiao", "0523");
    int ret = send(sockfd, (void*)&csp, ntohs(csp.header.package_length), 0);
    client_set_res_package csrp;
    ret = recv(sockfd, (void*)&csrp, sizeof(csrp), MSG_DONTWAIT);
    
    return 0;
}