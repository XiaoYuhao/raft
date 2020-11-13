#include<iostream>
//#include"package.h"
//#include"network.h"
//#include"logger.h"
#include"pthreadpool.h"

void func(int id){
    std::cout<<"message "<<"from "<<id<<std::endl;
    return ;
}

int main(){
    ThreadPool pool;
    for(int i=0;i<20;i++){
        pool.append(std::bind(func, i));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    return 0;
}