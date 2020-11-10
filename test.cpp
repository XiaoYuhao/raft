#include<iostream>
#include"server.h"



Server* server = Server::create("config");
int main(){
    server->work();
    return 0;
}