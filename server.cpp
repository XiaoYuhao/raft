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
#include <sys/epoll.h>  
#include <signal.h>  
#include <sys/wait.h>  
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>
#include <fstream>
#include"server.h"
#include"network.h"
#include"package.h"

using std::string;
using std::fstream;
using std::ios;

Server::Server(string config_file){
    read_config(config_file);
    state = FOLLOWER;   //初始状态为follower
    voted_for = -1;     //未投票

    commit_index = 0;
    last_applied = 0;
    next_index   = 0;
    match_index  = 0;
}

void Server::election_timeout(){    //选举超时处理
    /*状态转变为candidate， term加1， 并开启一轮新的选举*/
    state = CANDIDATE;
    current_term++;
    voted_for = server_id;  //给自己投一票


}

void Server::election(){            //开启一轮选举
    
}

Server* Server::create(string _config){
    if(_instance==NULL){
        _instance = new Server(_config);
    }
    return _instance;
}

void Server::work(){
    int listen_sock = startup(11234);
    struct sockaddr_in remote;
    socklen_t len = sizeof(struct sockaddr_in);

    int epfd = epoll_create(1024);
    struct epoll_event events[1024];

    addfd(epfd, listen_sock);

    int ret = 0;
    for(;;){
        int nfds = epoll_wait(epfd, events, 1024, -1);
        if((nfds<0)&&(errno!=EINTR)){
            printf("epoll failure.\n");
            break;
        }
        for(int i=0;i<nfds;i++){
            int sockfd = events[i].data.fd;
            if(sockfd == listen_sock){
                int client_sock = accept(listen_sock, (struct sockaddr*)&remote, &len);
                addfd(epfd, client_sock);
            }
            else if(events[i].events&EPOLLIN){
                package_header header;
                ret = recv(sockfd, (void *)&header, sizeof(header), MSG_PEEK);
                if((ret<0)&&(errno!=EINTR)){
                    continue;
                }
                if(ret<=0){
                    continue;
                }
                if(header.package_type == REQ_APPEND){      //收到来自leader的append entry请求或者心跳包
                    request_append_package rap;
                    ret = recv(sockfd, (void *)&rap, sizeof(rap), MSG_DONTWAIT);
                    rap.tohost();
                    //TODO
                    append_result_package arp;
                    if(current_term > rap.term){            //拒绝响应过期的term
                        arp.setdata(current_term, APPEND_FAIL);
                    }
                    else if(current_term == rap.term){
                        current_term = rap.term;
                        arp.setdata(current_term, APPEND_SUCCESS);
                    }
                    else{                                   //current_term过期，若自身为leader，则将状态转变为follower
                        current_term = rap.term;
                        state = FOLLOWER;
                        arp.setdata(current_term, APPEND_SUCCESS);
                    }
                    ret = send(sockfd, (void *)&arp, sizeof(arp), MSG_DONTWAIT);
                }
                if(header.package_type == REQ_VOTE){
                    request_vote_package rvp;
                    ret = recv(sockfd, (void *)&rvp, sizeof(rvp), MSG_DONTWAIT);
                    rvp.tohost();
                    //TODO
                    vote_result_package vrp;
                    if(current_term > rvp.term){
                        vrp.setdata(current_term, VOTE_GRANT_FALSE);
                    }
                    else if(voted_for == -1 || voted_for == rvp.candidate_id){
                        voted_for = rvp.candidate_id;
                        vrp.setdata(current_term, VOTE_GRANT_TRUE);
                    }
                    else{
                        vrp.setdata(current_term, VOTE_GRANT_FALSE);
                    }
                    ret = send(sockfd, (void *)&vrp, sizeof(vrp), MSG_DONTWAIT);
                }
            }
        }

    }
}


void Server::read_config(string config_file){
    fstream file;
    file.open(config_file, ios::in);
    file>>server_num;
    file>>server_id;
    server_info info;
    for(int i=0;i<server_num;i++){
        file>>info.ip_addr;
        file>>info.port;
        servers_info.push_back(info);
    }
    file.close();
    /*
    std::cout<<server_num<<" "<<server_id<<std::endl;
    for(int i=0;i<server_num;i++){
        std::cout<<servers_info[i].ip_addr<<std::endl;
        std::cout<<servers_info[i].port<<std::endl;
    }
    */
}

void Server::request_vote(){
    
}

void Server::send_rpc(){

}


Server* Server::_instance = NULL;
//Server* server = Server::create();
