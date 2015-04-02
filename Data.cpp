#include "Data.h"
#include <deque>
#include <cstdio>
#include <iostream>
#include "Enumerations.h"
#include "Client.h"
#include <chrono>
#include <ctime>

using namespace std;

inline string getCurrentTime(){
    auto cur = std::chrono::system_clock::now();
    auto curTime = std::chrono::system_clock::to_time_t(cur);
    char buffer [20];
    //localtime is not thread safe
    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", localtime(&curTime));
    return string(buffer);
}

//OHLC::OHLC(const IBString& _date, double _open, double _high, double _low, double _close, int _volume):date(_date), open(_open), high(_high), low(_low), close(_close), volume(_volume){}

void HistoricalData::update(const IBString& date, double open, double high, double low, double close, int volume){
    if(hisData.size()==len){
        hisData.pop_front(); 
    }
    hisData.push_back({date, open, high, low, close, volume});
    printf("historicalData: %s|%f|%f|%f|%f|%d\n", date.c_str(), open, high, low, close, volume);
}
