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
#include <iostream>
#include <random>
#include <time.h>
#include "server.h"
#include "network.h"
#include "package.h"

using std::string;
using std::fstream;
using std::ios;
using std::bind;

Server::Server(string config_file){
    read_config(config_file);
    state = FOLLOWER;   //初始状态为follower
    voted_for = -1;     //未投票
    voted_num = 0;

    commit_index = 0;
    last_applied = 0;
    next_index   = 0;
    match_index  = 0;

    std::default_random_engine random(time(NULL));
    std::uniform_int_distribution<u_int32_t> dis1(5000,20000);
    timeout_val = dis1(random);             //随机选取一个timeout的时间，区间为150ms~300ms
    timeout_flag = true;
}

void Server::election_timeout(){    //选举超时处理
    if(state == LEADER) return;     //自身状态为leader，不需要进行选举
    if(!timeout_flag){              //timeout_flag为false
        timeout_flag = true;
        return;
    }
    /*状态转变为candidate， term加1， 并开启一轮新的选举*/
    std::cout<<"election timeout."<<std::endl;
    state = CANDIDATE;
    current_term++;
    voted_for = server_id;  //给自己投一票
    voted_num = 1;

    request_vote();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    if(voted_num > server_num / 2){    //得票数超过半数，赢得选举
        state = LEADER;
    }
    voted_for = -1;


}

void Server::election(){            //开启一轮选举
    
}

Server* Server::create(string _config){
    if(_instance==NULL){
        _instance = new Server(_config);
    }
    return _instance;
}

void Server::start_server(){
    int listen_sock = startup(11234);
    struct sockaddr_in remote;
    socklen_t len = sizeof(struct sockaddr_in);

    int epfd = epoll_create(1024);
    struct epoll_event events[1024];

    addfd(epfd, listen_sock);
    map<int, string> sockfd_ip;
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
                sockfd_ip[client_sock] = remote.sin_addr.s_addr;
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
                    std::cout<<"receive a request append package from "<<sockfd_ip[sockfd]<<std::endl;
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
                        timeout_flag = false;
                        voted_for = -1;
                        arp.setdata(current_term, APPEND_SUCCESS);
                    }
                    else{                                   //current_term过期，若自身为leader，则将状态转变为follower
                        current_term = rap.term;
                        timeout_flag = false;
                        voted_for = -1;
                        state = FOLLOWER;
                        arp.setdata(current_term, APPEND_SUCCESS);
                    }
                    ret = send(sockfd, (void *)&arp, sizeof(arp), MSG_DONTWAIT);
                }
                if(header.package_type == REQ_VOTE){        //收到来自candidate的vote请求包
                    std::cout<<"receive a request vote package from "<<sockfd_ip[sockfd]<<std::endl;
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
    std::cout<<"server error."<<std::endl;
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
    for(int i=0;i<servers_info.size();i++){
        if(i != server_id){
            pool.append(bind(&Server::remote_vote_call, this, i));
        }
    }
}

void Server::remote_vote_call(u_int32_t remote_id){
    std::cout<<"into remote vote call."<<std::endl;
    int port = std::stoi(servers_info[remote_id].port);
    const char *ip_addr = servers_info[remote_id].ip_addr.c_str();
    int sockfd = connect_to_server(port, ip_addr);
    if(sockfd<=0){
        std::cout<<"connect "<<ip_addr<<"failed."<<std::endl;
        close(sockfd);
        return;
    }
    //std::cout<<"ip : "<<ip_addr<<" sockfd : "<<sockfd<<std::endl;
    int ret;
    request_vote_package rvp;
    rvp.setdata(current_term, server_id, last_applied, last_applied);
    ret = send(sockfd, (void*)&rvp, sizeof(rvp), MSG_DONTWAIT);
    if(ret<0){
        close(sockfd);
        return;
    }
    vote_result_package vrp;
    fd_set rfd;
    FD_ZERO(&rfd);
    FD_SET(sockfd, &rfd);
    ret = 0;
    switch(select(sockfd+1, &rfd, NULL, NULL, NULL)){
        case -1:
            printf("select error.\n");
            break;
        case 0:
            printf("select timeout.\n");
            break;
        default:
            ret = recv(sockfd, (void*)&vrp, sizeof(vrp), MSG_DONTWAIT);
            if(ret == 0){
                printf("server quit.\n");
                break;
            }
            break;
    }
    close(sockfd);
    if(ret == 0) return;
    std::cout<<"receive vote result package from "<<ip_addr<<std::endl;
    vrp.tohost();
    if(vrp.term > current_term){        //remote term > current term
        state = FOLLOWER;
    }
    else if(vrp.term == current_term){  //remote term == current term
        if(vrp.vote_granted == VOTE_GRANT_TRUE){
            voted_num++;
        }
    }
    else{                               //remote term < current term
        //no respond
    }
    return;
}

void Server::request_heartbeat(){
    if(state == LEADER){
        for(int i=0;i<servers_info.size();i++){
            if(i!=server_id){
                pool.append(bind(&Server::remote_append_call, this, i));
            }
        }
    }
}

void Server::remote_append_call(u_int32_t remote_id){
    std::cout<<"into remote append call."<<std::endl;
    int port = std::stoi(servers_info[remote_id].port);
    const char *ip_addr = servers_info[remote_id].ip_addr.c_str();
    int sockfd = connect_to_server(port, ip_addr);
    if(sockfd<=0){
        std::cout<<"connect "<<ip_addr<<"failed."<<std::endl;
        return;
    }

    int ret;
    request_append_package rap;
    string logentry = "heartbeat";
    rap.setdata(current_term, server_id, last_applied, last_applied, (u_int8_t*)logentry.c_str(), commit_index);
    ret = send(sockfd, (void*)&rap, sizeof(rap), MSG_DONTWAIT);
    append_result_package arp;
    fd_set rfd;
    FD_ZERO(&rfd);
    FD_SET(sockfd, &rfd);
    ret = 0;
    switch(select(sockfd+1, &rfd, NULL, NULL, NULL)){
        case -1:
            printf("select error.\n");
            break;
        case 0:
            printf("select timeout.\n");
            break;
        default:
            ret = recv(sockfd, (void*)&arp, sizeof(arp), MSG_DONTWAIT);
            if(ret == 0){
                printf("server quit.\n");
                break;
            }
            break;
    }
    close(sockfd);
    if(ret == 0) return;
    std::cout<<"receive a append result package from "<<ip_addr<<std::endl;
    arp.tohost();
    if(arp.term > current_term){        //remote term > current term
        state = FOLLOWER;
    }
    else if(arp.term == current_term){  //remote term == current term
        //TODO
    }
    else{                               //remote term < current term
        //no respond
    }
    return;
}

void Server::work(){
    pool.append(bind(&Server::start_server, this));        //start_server函数负责响应其它server发来的请求
    timer.Start(timeout_val, bind(&Server::election_timeout, this));
    //timer.Start(100, bind(&Server::request_heartbeat, this));
    for(;;){
        request_heartbeat();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}


Server* Server::_instance = NULL;
//Server* server = Server::create();
