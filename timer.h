#ifndef _TIMER_H_
#define _TIMER_H_
#include<cstdlib>
#include<unistd.h>
#include<signal.h>
#include<cstring>
#include<sys/time.h>

typedef int Status;

class Timer{
    itimerval tick;
public:
    Timer(){}
    Status init(sighandler_t _handler){
        signal(SIGALRM, _handler);
        memset(&tick, 0 ,sizeof(tick));
    }
    Status start(itimerval _tick){
        tick = _tick;
        return setitimer(ITIMER_REAL, &tick, NULL);
    }
    Status stop(){
        memset(&tick, 0, sizeof(tick));
        return setitimer(ITIMER_REAL, &tick, NULL);
    }
};

#endif