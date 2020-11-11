#include<iostream>
#include"package.h"
#include"network.h"

int main(){
    int sockfd = connect_to_server(11234, "47.116.133.175");
    if(sockfd<=0){
        std::cout<<"connect to server failed."<<std::endl;
    }
    else{
        std::cout<<sockfd<<std::endl;
        std::cout<<"connect to server successful."<<std::endl;
    }
    return 0;
}