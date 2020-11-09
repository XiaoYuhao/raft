#include"server.h"

Server::Server* _instence = NULL;
Server* server = Server::create();

Server::Server(){
    state = FOLLOWER;

}

Server* Server::create(){
    if(_instence==NULL){
        _instance = new Server;
    }
    return _instance;
}

