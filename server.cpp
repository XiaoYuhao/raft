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
#include <sstream>
#include <iostream>
#include <random>
#include <algorithm>
#include <time.h>
#include "server.h"
#include "network.h"
#include "package.h"

using std::string;
using std::fstream;
using std::istringstream;
using std::ios;
using std::bind;

Server::Server(string config_file):log_append_queue(1){
    read_config(config_file);
    logger.openlog("test.log");
    database = SSTable::create();

    state = FOLLOWER;   //初始状态为follower
    voted_for = -1;     //未投票
    voted_num = 0;

    commit_index = 0;
    //last_applied = 0;

    log_data_file.open("data.db", ios::in|ios::out);
    load_log();
    
    std::default_random_engine random(time(NULL));
    std::uniform_int_distribution<u_int32_t> dis1(2500,6000);
    timeout_val = dis1(random);             //随机选取一个timeout的时间，区间为150ms~300ms
    timeout_flag = true;

}

Server::~Server(){
    log_data_file.close();
}

void Server::election_timeout(){    //选举超时处理
    if(state == LEADER) return;     //自身状态为leader，不需要进行选举
    if(!timeout_flag){              //timeout_flag为false
        timeout_flag = true;
        return;
    }
    /*状态转变为candidate， term加1， 并开启一轮新的选举*/
    //std::cout<<"election timeout."<<std::endl;
    logger.info("election timeout.\n");
    state = CANDIDATE;
    current_term++;
    voted_for = server_id;  //给自己投一票
    voted_num = 1;

    request_vote();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    if(voted_num > server_num / 2){    //得票数超过半数，赢得选举
        state = LEADER;
        next_index.clear();            //成为Leader后，初始化nextindex和matchindex数组
        match_index.clear();
        for(int i=0;i<server_num;i++){
            next_index.push_back(max_index+1);
            match_index.push_back(0);
        }
        string noop = "NOOP";
        write_log(noop);
        log_append_queue.append(bind(&Server::append_log, this, max_index, -1));
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
                sockfd_ip[client_sock] = inet_ntoa(remote.sin_addr);
                addfd(epfd, client_sock);
            }
            else if(events[i].events&EPOLLIN){
                package_header header;
                ret = recv(sockfd, (void *)&header, sizeof(header), MSG_PEEK);
                if((ret<0)&&(errno!=EINTR)){
                    continue;
                }
                if(ret<=0){                                 //出错或断线
                    removefd(epfd, sockfd);
                    continue;
                }
                if(header.package_type == REQ_APPEND){      //收到来自leader的append entry请求或者心跳包
                    request_append_package rap;
                    ret = recv(sockfd, (void *)&rap, ntohs(header.package_length), MSG_DONTWAIT);
                    rap.tohost();
                    //std::cout<<"receive a request append package from "<<sockfd_ip[sockfd]<<" current_term "<<current_term<<" term "<<rap.term<<std::endl;
                    //logger.debug("receive a request append package from %s current_term %d term %d \n", sockfd_ip[sockfd].c_str(), (u_int64_t)current_term, (u_int64_t)rap.term);
                    //TODO
                    append_result_package arp;
                    if(current_term > rap.term){            //拒绝响应过期的term
                        arp.setdata(current_term, APPEND_FAIL);
                    }
                    else{                                   //若current_term过期，若自身为leader，则将状态转变为follower
                        current_term = rap.term;
                        timeout_flag = false;
                        voted_for = -1;
                        state = FOLLOWER;
                        leader_id = rap.leader_id;
                        string log_entry = string(rap.log_entry);
                        if(log_entry == "heartbeat"){       //收到的是心跳包
                            if(rap.leader_commit > commit_index){
                                commit_index = std::min(rap.leader_commit, max_index);
                            }
                            follower_apply_log();
                            arp.setdata(current_term, APPEND_SUCCESS);
                        }
                        else if(index_term.count(rap.prevlog_index)&&index_term[rap.prevlog_index]==rap.prevlog_term){
                            //需要处理追加、覆盖等情况
                            cover_log(rap.prevlog_index+1, max_index);
                            max_index = rap.prevlog_index;
                            //write_log(log_entry);
                            copy_log(log_entry);
                            if(rap.leader_commit > commit_index){
                                commit_index = std::min(rap.leader_commit, max_index);
                            }
                            follower_apply_log();
                            arp.setdata(current_term, APPEND_SUCCESS);
                        }
                        else{
                            arp.setdata(current_term, APPEND_FAIL);
                        }
                    }
                    ret = send(sockfd, (void *)&arp, sizeof(arp), MSG_DONTWAIT);
                }
                if(header.package_type == REQ_VOTE){        //收到来自candidate的vote请求包
                    request_vote_package rvp;
                    ret = recv(sockfd, (void *)&rvp, sizeof(rvp), MSG_DONTWAIT);
                    rvp.tohost();
                    //std::cout<<"receive a request vote package from "<<sockfd_ip[sockfd]<<" current_term "<<current_term<<" term "<<rvp.term<<std::endl;
                    logger.debug("receive a request vote package from %s current_term %d term %d \n", sockfd_ip[sockfd].c_str(), (u_int64_t)current_term, (u_int64_t)rvp.term);
                    //TODO
                    vote_result_package vrp;
                    if(current_term >= rvp.term){
                        vrp.setdata(current_term, VOTE_GRANT_FALSE);
                    }
                    else if(((state == FOLLOWER && voted_for == -1)||state == CANDIDATE) && rvp.lastlog_term >= index_term[max_index] && rvp.lastlog_index >= max_index){  //如果是follower，且之前没有给其它candidate投票，且候选者的日志比较新
                        current_term = rvp.term;
                        state == FOLLOWER;
                        voted_for = rvp.candidate_id;
                        vrp.setdata(current_term, VOTE_GRANT_TRUE);
                        //std::cout<<"vote for "<<voted_for<<std::endl;
                        logger.debug("vote for %d \n", (u_int32_t)voted_for);
                        timeout_flag = false;   //  若给某个server投票，则应该期待其成为leader，取消本次timeout
                    }
                    else{
                        current_term = rvp.term;
                        vrp.setdata(current_term, VOTE_GRANT_FALSE);
                    }
                    ret = send(sockfd, (void *)&vrp, sizeof(vrp), MSG_DONTWAIT);
                }
                if(header.package_type == CLIENT_SET_REQ){
                    client_set_package csp;
                    ret = recv(sockfd, (void *)&csp, ntohs(header.package_length), MSG_DONTWAIT);
                    //重定位Leader
                    if(state!=LEADER){
                        client_set_res_package csrp;
                        csrp.setdata(RES_REDIRECT, servers_info[leader_id].ip_addr.c_str(), std::stoi(servers_info[leader_id].port));
                        ret = send(sockfd, (void*)&csrp, sizeof(csrp), MSG_DONTWAIT);
                        //printf("size = %d ret = %d\n", sizeof(csrp), ret);
                        logger.info("Redirect client to current leader %s : %d.\n", csrp.ip_addr, ntohl(csrp.port));
                        /*char buf[512];
                        memcpy(buf, (void*)&csrp, sizeof(csrp));
                        for(int i=0;i<sizeof(csrp);i++){
                            printf("%02x ", buf[i]);
                        }
                        printf("\n");*/
                    }
                    else{
                        string req = "SET " + string(csp.buf) + " " + string(csp.buf+ntohl(csp.key_len));
                        //std::cout<<"Recv client set request : "<<req<<std::endl;
                        char bugfree[128] = "what is the bug?"; //vpfrint有未知bug，设置buf避免segmentfault
                        logger.info("Recv client set request [%s].\n", req.c_str());
                        write_log(req);
                        log_append_queue.append(bind(&Server::append_log, this, max_index, sockfd));       //log_append_queue是一只有单个线程的线程池
                    }
                }
                if(header.package_type == CLIENT_GET_REQ){
                    //TODO
                    client_get_package cgp;
                    ret = recv(sockfd, (void *)&cgp, ntohs(header.package_length), MSG_DONTWAIT);
                    //重定位
                    client_get_res_package cgrp;
                    if(state!=LEADER){
                        cgrp.setdata(RES_REDIRECT, servers_info[leader_id].ip_addr.c_str(), std::stoi(servers_info[leader_id].port));
                        ret = send(sockfd, (void*)&cgrp, ntohs(cgrp.header.package_length), MSG_DONTWAIT);
                        logger.info("Redirect client to current leader %s : %d.\n", cgrp.buf, ntohl(cgrp.port));
                    }
                    else{
                        string key = string(cgp.buf);
                        string val = database->db_get(key);
                        //char bugfree[128] = "what is the bug?"; //vpfrint有未知bug，设置buf避免segmentfault
                        //logger.info("Recv client get request [GET %s] RES = %s.\n", cgp.buf, val.c_str());
                        cgrp.setdata(RES_SUCCESS, val.c_str(), 0);
                        ret = send(sockfd, (void*)&cgrp, ntohs(cgrp.header.package_length), MSG_DONTWAIT);
                    }

                }
            }
        }

    }
    //std::cout<<"server error."<<std::endl;
    logger.error("server error. \n");
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
        info.fd = -1;
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
    file.open("apply.log", ios::in);
    file.peek();
    if(!file.eof()) file>>last_applied;
    else last_applied = 0;
    file.close(); 
}

void Server::request_vote(){
    for(int i=0;i<servers_info.size();i++){
        if(i != server_id){
            pool.append(bind(&Server::remote_vote_call, this, i));
        }
    }
}

void Server::remote_vote_call(u_int32_t remote_id){
    //std::cout<<"into remote vote call."<<std::endl;
    int port = std::stoi(servers_info[remote_id].port);
    const char *ip_addr = servers_info[remote_id].ip_addr.c_str();
    if(servers_info[remote_id].fd<0){
        servers_info[remote_id].fd = connect_to_server(port, ip_addr);
        logger.debug("sockfd : %d \n", servers_info[remote_id].fd);
    }
    int sockfd = servers_info[remote_id].fd;
    if(sockfd<=0){
        //std::cout<<"connect "<<ip_addr<<"failed."<<std::endl;
        logger.warn("connect %s failed. \n", ip_addr);
        //close(sockfd);
        return;
    }
    //std::cout<<"ip : "<<ip_addr<<" sockfd : "<<sockfd<<std::endl;
    int ret;
    request_vote_package rvp;
    u_int64_t lastlog_index, lastlog_term;
    lastlog_index = max_index;
    lastlog_term = index_term[max_index];
    rvp.setdata(current_term, server_id, lastlog_index, lastlog_term);
    ret = send(sockfd, (void*)&rvp, sizeof(rvp), MSG_DONTWAIT);
    if(ret<0){
        servers_info[remote_id].fd = -1;
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
            logger.error("select error.\n");
            break;
        case 0:
            logger.warn("select timeout.\n");
            break;
        default:
            ret = recv(sockfd, (void*)&vrp, sizeof(vrp), MSG_DONTWAIT);
            if(ret == 0){
                logger.info("server quit.\n");
                break;
            }
            break;
    }
    if(ret<=0){
        servers_info[remote_id].fd = -1;
        close(sockfd);
        return;
    }
    vrp.tohost();
    //std::cout<<"receive vote result package from "<<ip_addr<<" current_term "<<current_term<<" term "<<vrp.term<<std::endl;
    logger.info("receive vote result package from %s current_term %d term %d \n", ip_addr, (u_int64_t)current_term, (u_int64_t)vrp.term);
    if(vrp.term > current_term){        //remote term > current term
        state = FOLLOWER;
        timeout_flag = false;
        current_term = vrp.term;
    }
    else if(vrp.term == current_term){  //remote term == current term
        if(vrp.vote_granted == VOTE_GRANT_TRUE){
            //std::cout<<"server ip "<<ip_addr<<" vote for me."<<std::endl;
            logger.info("server ip %s vote for me. \n", ip_addr);
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
    //std::cout<<"into remote append call."<<std::endl;
    //logger.debug("into remote append call. \n");
    int port = std::stoi(servers_info[remote_id].port);
    const char *ip_addr = servers_info[remote_id].ip_addr.c_str();
    if(servers_info[remote_id].fd<0){
        servers_info[remote_id].fd = connect_to_server(port, ip_addr);
    }
    int sockfd = servers_info[remote_id].fd;
    if(sockfd<=0){
        //std::cout<<"connect "<<ip_addr<<"failed."<<std::endl;
        logger.warn("connect %s failed. \n", ip_addr);
        return;
    }

    int ret = 0;

    do{
        char buf[1024];
        ret = recv(sockfd, (void*)buf, sizeof(buf), MSG_DONTWAIT);
    }while(ret>0); //首先接收完socket缓存区中的数据，可能是上一次请求的回复（因为超时未接收）

    request_append_package rap;
    string logentry = "heartbeat";
    rap.setdata(current_term, server_id, last_applied, last_applied, (char*)logentry.c_str(), commit_index);
    ret = send(sockfd, (void*)&rap, ntohs(rap.header.package_length), MSG_DONTWAIT);
    if(ret<0){
        servers_info[remote_id].fd = -1;
        close(sockfd);
        return;
    }
    append_result_package arp;
    fd_set rfd;
    FD_ZERO(&rfd);
    FD_SET(sockfd, &rfd);
    ret = 0;
    timeval timeout;
    timeout.tv_sec = 2;
    switch(select(sockfd+1, &rfd, NULL, NULL, &timeout)){
        case -1:
            //printf("select error.\n");
            logger.error("select error\n");
            break;
        case 0:
            //printf("select timeout.\n");
            logger.warn("select timeout. \n");
            break;
        default:
            ret = recv(sockfd, (void*)&arp, sizeof(arp), MSG_DONTWAIT);
            if(ret == 0){
                //printf("server quit.\n");
                logger.info("server quit. \n");
                break;
            }
            break;
    }
    if(ret<=0){
        servers_info[remote_id].fd = -1;
        close(sockfd);
        return;
    }
    arp.tohost();
    //std::cout<<"receive a append result package from "<<ip_addr<<" current_term "<<current_term<<" term "<<arp.term<<std::endl;
    //logger.info("receive a append result package from %s current_term %d term %lld \n", ip_addr, (u_int64_t)current_term, (u_int64_t)arp.term);
    if(arp.term > current_term){        //remote term > current term
        state = FOLLOWER;
        timeout_flag = false;
        current_term = arp.term;
    }
    else if(arp.term == current_term){  //remote term == current term
        //TODO
    }
    else{                               //remote term < current term
        //no respond
    }
    return;
}

void Server::client_request(){

}

void Server::write_log(string log_entry){
    log_data_file.clear();
    log_data_file.seekp(0, std::ios_base::end);
    max_index++;
    u_int64_t p = log_data_file.tellp();
    log_offset[max_index] = p;
    index_term[max_index] = current_term;
    log_data_file<<max_index<<" "<<current_term<<" "<<log_entry<<std::endl;
    /*for(auto t : log_offset){
        std::cout<<t.first<<" ---- "<<t.second<<std::endl;
        log_data_file.clear();
        log_data_file.seekg(t.second);
        string str;
        std::getline(log_data_file, str);
        std::cout<<str<<std::endl;
    }*/
}

void Server::copy_log(string log_entry){
    istringstream iss(log_entry);
    u_int64_t _index, _term;
    iss>>_index>>_term;
    max_index = _index;
    log_data_file.clear();
    log_data_file.seekp(0, std::ios_base::end);
    u_int64_t p = log_data_file.tellp();
    log_offset[max_index] = p;
    index_term[max_index] = _term;
    log_data_file<<log_entry<<std::endl;
}

void Server::load_log(){
    //ifstream infile;
    //infile.open("data.db", ios::in);
    u_int64_t index, term;
    string key, val, op, sindex;
    max_index = 0;
    index_term[0] = 0;
    log_data_file.peek();
    while(!log_data_file.eof()){
        u_int64_t p = log_data_file.tellg();
        log_data_file>>sindex>>term>>op;
        if(log_data_file.eof())break;
        if(op=="SET") log_data_file>>key>>val;
        if(op=="DEL") log_data_file>>key;

        if(sindex.at(0)=='0'){
            logger.info("jump covered log entry.\n");
            //std::cout<<"jump covered log entry."<<std::endl;
            //std::cout<<"ok"<<std::endl;
            continue;
        }
        //offset.push_back(p);
        index = std::stoi(sindex);
        log_offset[index] = p;
        index_term[index] = term;
        max_index = index;
    }
    std::cout<<"max index --- "<<max_index<<std::endl;
    for(auto &t : log_offset){
        std::cout<<t.first<<" ---- "<<t.second<<std::endl;
        if(t.first!=1) t.second+=1; 
    }
}

void Server::append_log(u_int64_t log_index, int sockfd){
    /*for(;;){
        for(int i=0;i<servers_info.size();i++){
            if(i==server_id) continue;
            pool.append(std::bind(&Server::append_log_to, this, i));
        }
        //需要思考的问题是：append_log的时机以及apply log的时机
    }*/
    vector<thread> append_log_tasks;
    for(int i=0;i<servers_info.size();i++){
        if(i==server_id) continue;
        append_log_tasks.emplace_back(&Server::append_log_to, this, i);
    }
    for(auto &thread : append_log_tasks){
        thread.join();
    }
    leader_apply_log();
    client_set_res_package csrp;
    if(last_applied >= log_index){
        csrp.setdata(RES_SUCCESS, "0.0.0.0", 11234);
    }
    else{
        csrp.setdata(RES_FAIL, "0.0.0.0", 11234);
    }
    if(sockfd<0) return;
    int ret = send(sockfd, (void*)&csrp, sizeof(csrp), MSG_DONTWAIT);
    if(ret<0) close(sockfd);
}

void Server::append_log_to(u_int32_t remote_id){
    int port = std::stoi(servers_info[remote_id].port);
    const char *ip_addr = servers_info[remote_id].ip_addr.c_str();
    if(servers_info[remote_id].fd<0){
        servers_info[remote_id].fd = connect_to_server(port, ip_addr);
    }
    int sockfd = servers_info[remote_id].fd;
    if(sockfd<=0){
        //std::cout<<"connect "<<ip_addr<<"failed."<<std::endl;
        logger.warn("connect %s failed. \n", ip_addr);
        return;
    }
    int ret = 0;

    do{
        char buf[1024];
        ret = recv(sockfd, (void*)buf, sizeof(buf), MSG_DONTWAIT);
    }while(ret>0); //首先接收完socket缓存区中的数据，可能是上一次请求的回复（因为超时未接收）

    while(next_index[remote_id]<=max_index){
        log_data_file.clear();
        log_data_file.seekg(log_offset[next_index[remote_id]]);
        string log_entry;
        std::getline(log_data_file, log_entry);
        request_append_package rap;
        u_int64_t prevlog_index, prevlog_term;
        prevlog_index = next_index[remote_id] - 1;
        prevlog_term = prevlog_index==0 ? 0 : index_term[prevlog_index];
        rap.setdata(current_term, server_id, prevlog_index, prevlog_term, (char*)log_entry.c_str(), commit_index);
        ret = send(sockfd, (void*)&rap, ntohs(rap.header.package_length), MSG_DONTWAIT);
        if(ret<0){
            servers_info[remote_id].fd = -1;
            close(sockfd);
            return;
        }
        append_result_package arp;
        fd_set rfd;
        FD_ZERO(&rfd);
        FD_SET(sockfd, &rfd);
        ret = 0;
        timeval timeout;
        timeout.tv_sec = 2;
        switch(select(sockfd+1, &rfd, NULL, NULL, &timeout)){           //设置超时时间2sec
            case -1:
                //printf("select error.\n");
                logger.error("select error\n");
                break;
            case 0:
                //printf("select timeout.\n");
                logger.warn("select timeout. \n");
                break;
            default:
                ret = recv(sockfd, (void*)&arp, sizeof(arp), MSG_DONTWAIT);
                if(ret == 0){
                    //printf("server quit.\n");
                    logger.info("server quit. \n");
                    break;
                }
                break;
        }
        if(ret<=0){
            servers_info[remote_id].fd = -1;
            close(sockfd);
            return;
        }
        arp.tohost();
        if(arp.term > current_term){            //remote term > current term
            state = FOLLOWER;
            timeout_flag = false;
            current_term = arp.term;
            return;
        }
        else if(arp.term == current_term){      //remote term == current term
            if(arp.success == APPEND_SUCCESS){
                logger.info("log append success to %s. index = %d\n", ip_addr, next_index[remote_id]);
                match_index[remote_id] = next_index[remote_id];
                next_index[remote_id]++;
            }
            else{
                logger.info("log append failed to %s. index = %d\n", ip_addr, next_index[remote_id]);
                next_index[remote_id]--;
            }
        }

    }
}

void Server::cover_log(u_int64_t index, u_int64_t m_index){
    while(index <= m_index){
        if(log_offset.count(index)==0) break;       //当前日志项中不包含index项
        log_data_file.clear();
        log_data_file.seekp(log_offset[index]);
        log_data_file<<"0";                         //index置为0，表示已覆盖
        log_offset.erase(index);
        index_term.erase(index);
        index++;
    }
}

void Server::leader_apply_log(){
    u_int64_t ready_to_apply = commit_index + 1;
    while(max_index >= ready_to_apply){
        u_int32_t match_num = 0;
        for(int i=0;i<server_num;i++){
            if(i==server_id) match_num++;                       //leader必定是完成复制了的
            if(match_index[i]>=ready_to_apply) match_num++;
        }
        if((match_num > server_num / 2)&&index_term[ready_to_apply]==current_term){         //注意只能通过计数方式提交当前term的日志项
            commit_index = ready_to_apply;
        }
        if(match_num < server_num / 2) break;
        ready_to_apply++;
    }
    follower_apply_log();
}

void Server::follower_apply_log(){
    Status status;
    u_int64_t compress_index = 0;
    while(commit_index > last_applied){
        last_applied++;
        string log_entry;
        log_data_file.clear();
        log_data_file.seekg(log_offset[last_applied]);
        std::getline(log_data_file, log_entry);
        logger.info("Apply log entry to state machine. [%s]\n", log_entry.c_str());
        istringstream iss(log_entry);
        u_int64_t _index, _term;
        string _op, _key, _val;
        iss>>_index>>_term>>_op;
        if(_op=="SET"){
            iss>>_key>>_val;
            status = database->db_set(_key, _val, _index);
            if(status == SET_SPILL) compress_index = _index;
        } 
    }
    if(compress_index>1) clean_log(compress_index-1);
}

void Server::clean_log(u_int64_t log_index){
    ofstream newfile;
    newfile.open("tmp.db", std::ios::out);

    map<u_int64_t, u_int64_t> new_log_offset;
    map<u_int64_t, u_int64_t> new_index_term;
    string log_entry;

    new_index_term[log_index] = index_term[log_index];
    while(max_index > log_index){
        log_index++;
        u_int64_t p = newfile.tellp();
        
        log_data_file.clear();
        log_data_file.seekg(log_offset[log_index]);
        std::getline(log_data_file, log_entry);
        newfile<<log_entry<<std::endl;

        new_log_offset[log_index] = p;
        new_index_term[log_index] = index_term[log_index];
    }

    log_offset = new_log_offset;
    index_term = new_index_term;

    newfile.close();
    log_data_file.close();
    remove("data.db");
    rename("tmp.db", "data.db");
    log_data_file.open("data.db", ios::in|ios::out);
}

void Server::work(){
    pool.append(bind(&Server::start_server, this));        //start_server函数负责响应其它server发来的请求
    timer.Start(timeout_val, bind(&Server::election_timeout, this));
    //timer.Start(100, bind(&Server::request_heartbeat, this));
    for(;;){
        //request_heartbeat();
        log_append_queue.append(bind(&Server::request_heartbeat, this));        //log_append_queue是一个只有一个线程的线程池
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

Server* Server::_instance = NULL;
//Server* server = Server::create();
 