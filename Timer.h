#ifndef timer_h_INLCUDED
#define timer_h_INLCUDED

#include <string>
#include <ctime>
#include <chrono>
#include <sys/time.h>

using namespace std;

inline string getCurrentTime(){
    auto cur = std::chrono::system_clock::now();
    auto curTime = std::chrono::system_clock::to_time_t(cur);
    char buffer [20];
    //localtime is not thread safe
    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", localtime(&curTime));
    return string(buffer);
}

inline string getCurrentTime0(){
    timeval curTime;
    gettimeofday(&curTime, NULL);
    unsigned long micro = curTime.tv_usec;

    char buffer [20];
    //localtime is not thread safe
    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", localtime((const time_t*)&curTime.tv_sec));

    char currentTime2[30];
    sprintf(currentTime2, "%s.%06Lu", buffer, micro); 
    return string(currentTime2);
}

#endif
