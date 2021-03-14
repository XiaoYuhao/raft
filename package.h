#ifndef _PACKAGE_H_
#define _PACKAGE_H_

#include<cstdlib>
#include<cstring>
#include<arpa/inet.h>

u_int64_t htonll(u_int64_t val){
    return (((u_int64_t) htonl(val)) << 32) + htonl(val >> 32);
}
u_int64_t ntohll(u_int64_t val){
    return (((u_int64_t) ntohl(val)) << 32) + ntohl(val >> 32);
}

const u_int8_t REQ_VOTE             = 0x01;
const u_int8_t VOTE_RES             = 0x11;

const u_int8_t REQ_APPEND           = 0x02;
const u_int8_t APPEND_RES           = 0x12;

//const u_int8_t CLIENT_REQ           = 0x03;
//const u_int8_t RES_CLIENT           = 0x13;

const u_int8_t VOTE_GRANT_TRUE      = 0x01;
const u_int8_t VOTE_GRANT_FALSE     = 0x00;

const u_int8_t APPEND_SUCCESS       = 0x01;
const u_int8_t APPEND_FAIL          = 0x00;

//const u_int8_t PROCEDURE_ERROR      = 0x91;
//const u_int8_t PROCEDURE_NOT_FOUND  = 0x92;
//const u_int8_t PROCEDURE_SUCCESS    = 0x01;

const u_int8_t RES_SUCCESS          = 0x01;
const u_int8_t RES_FAIL             = 0x00;

const u_int8_t CLIENT_GET_REQ       = 0x31;
const u_int8_t CLIENT_SET_REQ       = 0x32;
const u_int8_t CLIENT_DEL_REQ       = 0x33;

const u_int8_t RES_CLIENT_GET       = 0x41;
const u_int8_t RES_CLIENT_SET       = 0x42;
const u_int8_t RES_CLIENT_DEL       = 0x43;



struct package_header{
    u_int8_t package_type;
    u_int8_t package_seq;
    u_int16_t package_length;
};

struct request_vote_package{
    package_header header;
    u_int64_t term;
    u_int32_t candidate_id;
    u_int64_t lastlog_index;
    u_int64_t lastlog_term;
    request_vote_package(){}
    void setdata(u_int64_t _term, u_int32_t _candidate_id, u_int64_t _lastlog_index, u_int64_t _lastlog_term){
        header.package_type = REQ_VOTE;
        header.package_seq = 0;
        header.package_length = htons(sizeof(request_vote_package));
        term = htonll(_term);
        candidate_id = htonl(_candidate_id);
        lastlog_index = htonll(_lastlog_index);
        lastlog_term = htonll(_lastlog_term);
    }
    void tohost(){
        header.package_length = ntohs(header.package_length);
        term = ntohll(term);
        candidate_id = ntohl(candidate_id);
        lastlog_index = ntohll(lastlog_index);
        lastlog_term = ntohll(lastlog_term);
    }
};

struct vote_result_package{
    package_header header;
    u_int64_t term;
    u_int8_t vote_granted;
    vote_result_package(){}
    void setdata(u_int64_t _term, u_int8_t _vote_granted){
        header.package_type = VOTE_RES;
        header.package_seq = 0;
        header.package_length = htons(sizeof(vote_result_package));
        term = htonll(_term);
        vote_granted = _vote_granted;
    }
    void tohost(){
        header.package_length = ntohs(header.package_length);
        term = ntohll(term);
    }
};

struct request_append_package{
    package_header header;
    u_int64_t term;
    u_int32_t leader_id;
    u_int64_t prevlog_index;
    u_int64_t prevlog_term;
    u_int64_t leader_commit;
    //u_int8_t log_entry[128];
    u_int32_t log_len;
    char log_entry[4096];
    request_append_package(){}
    void setdata(u_int64_t _term, u_int32_t _leader_id, u_int64_t _prevlog_index, 
            u_int64_t _prevlog_term, const char *_log_entry, u_int64_t _leader_commit)
    {
        header.package_type = REQ_APPEND;
        header.package_seq = 0;
        //header.package_length = htons(sizeof(request_append_package));
        term = htonll(_term);
        leader_id = htonl(_leader_id);
        prevlog_index = htonll(_prevlog_index);
        prevlog_term = htonll(_prevlog_term);
        //memcpy(log_entry, _log_entry, sizeof(log_entry));
        log_len = strlen(_log_entry) + 1;
        strcpy(log_entry, _log_entry);
        leader_commit = htonll(_leader_commit);
        header.package_length = htons(sizeof(header)+sizeof(u_int64_t)*4+sizeof(u_int32_t)*2+log_len);
        log_len = htonl(log_len);
    }
    void tohost(){
        header.package_length = ntohs(header.package_length);
        term = ntohll(term);
        leader_id = ntohl(leader_id);
        prevlog_index = ntohll(prevlog_index);
        prevlog_term = ntohll(prevlog_term);
        leader_commit = ntohll(leader_commit);
        log_len = ntohl(log_len);
    }
};

struct append_result_package{
    package_header header;
    u_int64_t term;
    u_int8_t success;
    append_result_package(){}
    void setdata(u_int64_t _term, u_int8_t _success){
        header.package_type = APPEND_RES;
        header.package_seq = 0;
        header.package_length = htons(sizeof(append_result_package));
        term = htonll(_term);
        success = _success;
    }
    void tohost(){
        header.package_length = ntohs(header.package_length);
        term = ntohll(term);
    }
};
/*
struct client_request_package{
    package_header header;
    u_int32_t buf_len;
    char buf[4096];
    client_request_package(){}
    void setdata(const char *str){
        header.package_type = CLIENT_REQ;   
        buf_len = strlen(str) + 1;
        strcpy(buf, str);
        header.package_length = htons(sizeof(header)+4+buf_len);
        buf_len = htonl(buf_len);
    }
};

struct response_client_package{
    package_header header;
    u_int8_t status;
    u_int32_t buf_len;
    char buf[4096];
    response_client_package(){}
    void setdata(u_int8_t st, const char *str){
        header.package_type = RES_CLIENT;
        buf_len = strlen(str) + 1;
        strcpy(buf, str);
        header.package_length = htons(sizeof(header)+1+4+buf_len);
        buf_len = htonl(buf_len);
    } 

};*/

struct client_get_package{
    package_header header;
    u_int32_t key_len;
    char buf[1024];
    client_get_package(){}
    client_get_package(const char *key){
        header.package_type = CLIENT_GET_REQ;
        key_len = strlen(key) + 1;
        strcpy(buf, key);
        header.package_length = htons(sizeof(package_header)+4+key_len);
        key_len = htonl(key_len);
    }
};

struct client_get_res_package{
    package_header header;
    u_int32_t val_len;
    char buf[4096];
    client_get_res_package(){}
    client_get_res_package(const char *val){
        header.package_type = RES_CLIENT_GET;
        val_len = strlen(val) + 1;
        strcpy(buf, val);
        header.package_length = htons(sizeof(header)+8+val_len);
        val_len = htonl(val_len);
    }
};

struct client_set_package{
    package_header header;
    u_int32_t key_len;
    u_int32_t val_len;
    char buf[4096];
    client_set_package(){}
    client_set_package(const char *key, const char *val){
        header.package_type = CLIENT_SET_REQ;
        key_len = strlen(key) + 1;
        val_len = strlen(val) + 1;
        strcpy(buf, key);
        strcpy(buf+key_len, val);
        header.package_length = htons(sizeof(package_header)+8+key_len+val_len);
        key_len = htonl(key_len);
        val_len = htonl(val_len);
    }
};

struct client_set_res_package{
    package_header header;
    u_int8_t status;
    client_set_res_package(){}
    client_set_res_package(u_int8_t st){
        header.package_type = RES_CLIENT_SET;
        header.package_length = htons(sizeof(client_set_res_package));
        status = st;
    }
};



#endif