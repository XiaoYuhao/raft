#ifndef _SERVER_H_
#define _SERVER_H_
#include<cstdlib>
#include<vector>
#include<string>
#include"timer.h"
#include"pthreadpool.h"

using std::string;

const u_int8_t LEADER    = 0x01;
const u_int8_t FOLLOWER  = 0x02;
const u_int8_t CANDIDATE = 0x03;

struct server_info{
    string port;
    string ip_addr;
};

class Server{
//infomation    
    u_int32_t server_id;
    u_int32_t server_num;
    vector<server_info> servers_info;

//Persistent state on all servers    
    u_int8_t state;
    u_int64_t current_term;
    u_int32_t voted_for;
    std::vector<u_int32_t> log;

    u_int32_t voted_num;

//Volatile state on all servers
    u_int64_t commit_index;
    u_int64_t last_applied;

//Volatile state on leaders
    u_int64_t next_index;
    u_int64_t match_index;

// 
    Timer timer;
    ThreadPool pool;
    static Server* _instance;
    Server(string config_file);
    void election_timeout();
    int timeout_val;
    void election();
    void request_vote();
    void remote_procedure_call(u_int32_t server_id, u_int8_t rpc_type);
    void send_to_servers();
    void read_config(string config_file);
public:
    static Server* create(string _config);
    void work();

    

};

#endif