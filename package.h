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

const u_int8_t VOTE_GRANT_TRUE      = 0x01;
const u_int8_t VOTE_GRANT_FALSE     = 0x00;

const u_int8_t APPEND_SUCCESS       = 0x01;
const u_int8_t APPEND_FAIL          = 0x00;

const u_int8_t PROCEDURE_ERROR      = 0x91;
const u_int8_t PROCEDURE_NOT_FOUND  = 0x92;
const u_int8_t PROCEDURE_SUCCESS    = 0x01;

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
    u_int8_t log_entry[128];
    u_int64_t leader_commit;
    request_append_package(){}
    void setdata(u_int64_t _term, u_int32_t _leader_id, u_int64_t _prevlog_index, 
            u_int64_t _prevlog_term, u_int8_t *_log_entry, u_int64_t _leader_commit)
    {
        header.package_type = REQ_APPEND;
        header.package_seq = 0;
        header.package_length = htons(sizeof(request_append_package));
        term = htonll(_term);
        leader_id = htonl(_leader_id);
        prevlog_index = htonll(_prevlog_index);
        prevlog_term = htonll(_prevlog_term);
        memcpy(log_entry, _log_entry, sizeof(log_entry));
        leader_commit = htonll(_leader_commit);
    }
    void tohost(){
        header.package_length = ntohs(header.package_length);
        term = ntohll(term);
        leader_id = ntohll(leader_id);
        prevlog_index = ntohll(prevlog_index);
        prevlog_term = ntohll(prevlog_term);
        leader_commit = ntohll(leader_commit);
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



#endif