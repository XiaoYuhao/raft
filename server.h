#ifndef _SERVER_H_
#define _SERVER_H_
#include<cstdlib>
#include<vector>
#include<string>
#include<fstream>
#include<map>
#include<atomic>
#include"logger.h"
#include"timer.h"
#include"pthreadpool.h"
#include"SSTable/sstable.h"

using std::string;
using std::atomic_uint_fast32_t;
using std::atomic_bool;
using std::atomic_uint;
using std::atomic_ullong;
using std::map;
using std::ifstream;
using std::ofstream;
using std::fstream;
using namespace Log;

const u_int8_t LEADER    = 0x01;
const u_int8_t FOLLOWER  = 0x02;
const u_int8_t CANDIDATE = 0x03;

struct server_info{
    string port;
    string ip_addr;
    int32_t fd;
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
    u_int32_t leader_id;

    atomic_uint voted_num;

//Volatile state on all servers
    u_int64_t commit_index;
    u_int64_t last_applied;
    u_int64_t max_index;
    //vector<u_int64_t> offset;
    map<u_int64_t, u_int64_t> log_offset;
    map<u_int64_t, u_int64_t> index_term;
    fstream log_data_file;

//Volatile state on leaders
    //u_int64_t next_index;
    //u_int64_t match_index;
    vector<u_int64_t> next_index;       //下一条要发送给follower的日志项
    vector<u_int64_t> match_index;      //已经成功复制到follower的日志项
    u_int64_t all_match_index;          //所有从节点都已复制完成的日志项，用于确定可压缩的日志项       
    u_int64_t compress_index;           //在此index之前的所有日志项可被压缩

// 
    Timer timer;
    ThreadPool pool;
    ThreadPool log_append_queue;
    Logger logger;
    SSTable* database;
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
    void client_request();
    void write_log(string log_entry);
    void load_log();
    void append_log(u_int64_t log_index, int sockfd);
    void append_log_to(u_int32_t remote_id);
    void cover_log(u_int64_t index, u_int64_t m_index); //覆盖index项之后不一致的所有日志项
    void follower_apply_log(); //follower提交已commit的日志项到状态机中去
    void leader_apply_log(); //leader提交已commit的日志项到状态机中去
    void copy_log(string log_entry); //将从leader复制过来的日志项写入日志文件中
    //void record_last_apply(); //将last_apply记录到磁盘中，这样崩溃恢复后不用从新开始apply
    void clean_log(u_int64_t log_index); //清理已持久化的log entry
public:
    static Server* create(string _config);
    void work();
    ~Server();

    

};

#endif