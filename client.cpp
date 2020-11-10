#include<iostream>
#include"package.h"
#include"network.h"

int main(){
    int sockfd = connect_to_server(11234, "0.0.0.0");

    return 0;
}