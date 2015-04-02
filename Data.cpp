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

void HistoricalData::update(const OHLC &d){
    //OHLC *dptr = dynamic_cast<OHLC * const>(&d);
    if(histData.size()==len){
        histData.pop_front(); 
    }

    //histData.push_back({dptr->date, dptr->open, dptr->high, dptr->low, dptr->close, dptr->volume});
    //printf("historicalData: %s|%f|%f|%f|%f|%d\n", dptr->date.c_str(), dptr->open, dptr->high, dptr->low, dptr->close, dptr->volume);
    //histData.push_back({d.date, d.open, d.high, d.low, d.close, d.volume});
    histData.push_back(d);
    printf("historicalData: %s|%f|%f|%f|%f|%d\n", d.date.c_str(), d.open, d.high, d.low, d.close, d.volume);
}

void TickData::update(int side, double price, int size){
    double bid = 0.0, ask = 0.0;
    if(tickData.size()==0){
        if(side==1){
            // FIXME get current data in microsecond resolution
            tickData.push_back({"", price, 0.0, size, 0});
        }
        if(side==0){
            tickData.push_back({"", 0.0, price, 0, size});
        }
        return;
    }

    if(tickData.size()==1){
        if(tickData[0].bid < 0.0001)}{
            tickData[0].bid = price;
            tickData[0].bidVolume = size;
        }
        if(tickData[0].ask < 0.0001)}{
            tickData[0].ask = price;
            tickData[0].ask = size;
        }
        return;
    }

    auto sz = tickData.size();
    if(side==1){
        tickData.push_back({"", price, tickData[sz-1].ask, size, tickData[sz-1].size});
    }
    if(side==0){
        tickData.push_back({"", tickData[sz-1].bid, price, tickData[sz-1].size, size});
    }
    return;
}
