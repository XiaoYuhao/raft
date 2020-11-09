#ifndef _SERVER_H_
#define _SERVER_H_
#include<cstdlib>
#include<vector>
#include"timer.h"

const u_int8_t LEADER    = 0x01;
const u_int8_t FOLLOWER  = 0x02;
const u_int8_t CANDIDATE = 0x03;

class Server{
//Persistent state on all servers    
    u_int8_t state;
    u_int64_t current_term;
    u_int32_t voted_for;
    std::vector<u_int32_t> log;

//Volatile state on all servers
    u_int64_t commit_index;
    u_int64_t last_applied;

//Volatile state on leaders
    u_int64_t next_index;
    u_int64_t match_index;

// 
    Timer timer;
    static Server* _instance;
    Server();
public:
    static Server* create();
    

};

#endif