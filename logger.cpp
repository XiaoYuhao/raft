#include"logger.h"
using namespace Log;
using std::ios;

Logger::Logger(string _log_name){
    log_name = get_current_time() + _log_name;
    out.open(log_name, ios::app);
}

Logger::~Logger(){
    out.close();
}
Logger & Logger::operator << (stringstream &str){
    out<<str;
}
/*
Logger & Logger::operator<<(const char *str){
    out<<str;
}

Logger & Logger::operator<<(const string &str){
    out<<str;
}*/