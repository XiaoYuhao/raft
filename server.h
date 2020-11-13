#ifndef _SERVER_H_
#define _SERVER_H_
#include<cstdlib>
#include<vector>
#include<string>
#include<map>
#include<atomic>
#include"logger.h"
#include"timer.h"
#include"pthreadpool.h"

using std::string;
using std::atomic_uint_fast32_t;
using std::atomic_bool;
using std::atomic_uint;
using std::atomic_ullong;
using std::map;
using namespace Log;

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

    u_int32_t timeout_val;
    atomic_bool timeout_flag;

//Persistent state on all servers    
    atomic_uint state;
    atomic_ullong current_term;
    atomic_uint voted_for;
    vector<u_int32_t> log;

    atomic_uint voted_num;

//Volatile state on all servers
    u_int64_t commit_index;
    u_int64_t last_applied;

//Volatile state on leaders
    u_int64_t next_index;
    u_int64_t match_index;

// 
    Timer timer;
    ThreadPool pool;
    Logger logger;
    static Server* _instance;
    Server(string config_file);
    void election_timeout();
    void election();
    void request_vote();
    void remote_vote_call(u_int32_t remote_id);
    void request_heartbeat();
    void remote_append_call(u_int32_t remote_id);
    void send_to_servers();
    void read_config(string config_file);
    void start_server();
public:
    static Server* create(string _config);
    void work();

    

};

#endif