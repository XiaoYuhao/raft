#include<iostream>
#include<fstream>
#include<sstream>
#include<string>
#include<cstdio>
#include<time.h>

using std::string;
using std::stringstream;
using std::ofstream;
using std::istream;
using std::ostream;

namespace Log{

string get_current_time(){
    string ret;
    time_t now = time(0);
    tm *ltm = localtime(&now);
    char current_time[30];
    sprintf(current_time, "%d_%02d_%02d_%02d_%02d_%02d",ltm->tm_year + 1900, 
        ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
    return string(current_time);
}
stringstream ostr;
stringstream& info(){
    ostr<<get_current_time() + " INFO  : ";
    return ostr;
}
/*
string info(){
    return get_current_time() + " INFO  : ";
}
string debug(){
    return get_current_time() + " DEBUG : ";
}
string error(){
    return get_current_time() + " ERROR : ";
}
string warn(){
    return get_current_time() + " WARN  : ";
}*/
class Logger{
    string log_name;
    ofstream out;
public:
    Logger(string _log_name);
    ~Logger();
    Logger & operator << (stringstream &str);
    //Logger & operator << (const string &str);
};

};