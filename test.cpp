#include"server.h"

Server* server = Server::create("config");
int main(){
    server->read_config();
}