#ifndef _LOGGER_H_
#define _LOGGER_H_

#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include<cstdio>
#include<cstdlib>
#include<cstdarg>
#include<mutex>
#include<ctime>

using std::mutex;
using std::lock_guard;

namespace Log{

const int CONSOLE = 1;

class Logger{
    FILE *log_file;
    mutex log_mutex;
    int logflag;
public:
    Logger(){}
    ~Logger(){
        fclose(log_file);
    }
    void get_current_time(char *current_time){
        time_t now = time(0);
        tm *ltm = localtime(&now);
        sprintf(current_time, "%d-%02d-%02d-%02d-%02d-%02d",ltm->tm_year + 1900, 
            ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
    }
    int openlog(const char *_log_name, int _mode = CONSOLE){
        logflag = _mode;
        log_file = fopen(_log_name, "a");
        if(log_file){
            printf("open log file successfully.\n");
            return 1;
        }
        printf("open log file failed.\n");
        return 0;
    }

    int info(const char *format, ...){
        va_list argp;
        va_start(argp, format);
        char current_time[20];
        lock_guard<mutex> lg(log_mutex);
        get_current_time(current_time);
        if(logflag == CONSOLE){
            printf("%s INFO  : ", current_time);
            vprintf(format, argp);
        }
        fprintf(log_file, "%s INFO  : ", current_time);
        int ret = vfprintf(log_file, format, argp);
        va_end(argp);
        fflush(log_file);
        return ret;
    }
    int debug(const char *format, ...){
        va_list argp;
        va_start(argp, format);
        char current_time[20];
        lock_guard<mutex> lg(log_mutex);
        get_current_time(current_time);
        if(logflag == CONSOLE){
            printf("%s DEBUG : ", current_time);
            vprintf(format, argp);
        }
        fprintf(log_file, "%s DEBUG : ", current_time);
        int ret = vfprintf(log_file, format, argp);
        va_end(argp);
        fflush(log_file);
        return ret;
    }
    int error(const char *format, ...){
        va_list argp;
        va_start(argp, format);
        char current_time[20];
        lock_guard<mutex> lg(log_mutex);
        get_current_time(current_time);
        if(logflag == CONSOLE){
            printf("%s ERROR : ", current_time);
            vprintf(format, argp);
        }
        fprintf(log_file, "%s ERROR : ", current_time);
        int ret = vfprintf(log_file, format, argp);
        va_end(argp);
        fflush(log_file);
        return ret;
    }
    int warn(const char *format, ...){
        va_list argp;
        va_start(argp, format);
        char current_time[20];
        lock_guard<mutex> lg(log_mutex);
        get_current_time(current_time);
        if(logflag == CONSOLE){
            printf("%s WARN  : ", current_time);
            vprintf(format, argp);
        }
        fprintf(log_file, "%s WARN  : ", current_time);
        int ret = vfprintf(log_file, format, argp);
        va_end(argp);
        fflush(log_file);
        return ret;
    }
    /*template<class type> Logger & operator << (type val){
        out<<val;
        return *this;
    }

    void debug(string &str){
        out<<get_current_time()<<" DEBUG : "<<str;
    }
    void error(string &str){
        out<<get_current_time()<<" ERROR : "<<str;
    }
    void warn(string &str){
        out<<get_current_time()<<" WARN  : "<<str;
    }
    Logger & operator << (stringstream &str){
        out<<str;
    }
    Logger & operator << (const string &str);*/
};

};

#endif