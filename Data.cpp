#include "Data.h"
#include <deque>
#include <cstdio>
#include <iostream>
#include "Enumerations.h"
#include "Client.h"
#include "Timer.h"

using namespace std;


//OHLC::OHLC(const IBString& _date, double _open, double _high, double _low, double _close, int _volume):date(_date), open(_open), high(_high), low(_low), close(_close), volume(_volume){}

void HistoricalData::update(string date, double open, double high, double low, double close, int volume){
    //OHLC *dptr = dynamic_cast<OHLC * const>(&d);
    // FIXME: keep record of fixed length?
    /*
    if(histData.size()==len){
        histData.pop_front(); 
    }
    */

    OHLC d{date, open, high, low, close, volume};
    //histData.push_back({dptr->date, dptr->open, dptr->high, dptr->low, dptr->close, dptr->volume});
    //printf("historicalData: %s|%f|%f|%f|%f|%d\n", dptr->date.c_str(), dptr->open, dptr->high, dptr->low, dptr->close, dptr->volume);
    //histData.push_back({d.date, d.open, d.high, d.low, d.close, d.volume});
    histData.push_back(d);
    //printf("historicalData: %s|%f|%f|%f|%f|%d\n", d.date.c_str(), d.open, d.high, d.low, d.close, d.volume);
}

void TickData::update(int side, double price, int size){
    double bid = 0.0, ask = 0.0;
    auto sz = tickData.size();
    if(sz==0){
        if(side==1){
            // FIXME get current data in microsecond resolution
            tickData.push_back({"", price, 0.0, size, 0});
        }
        if(side==0){
            tickData.push_back({"", 0.0, price, 0, size});
        }
        return;
    }

    if(tickData[0].bid < 0.0001){
        tickData[0].bid = price;
        tickData[0].bidVolume = size;
        return;
    }
    else if(tickData[0].ask < 0.0001){
        tickData[0].ask = price;
        tickData[0].ask = size;
        return;
    }

    if(side==1){
        tickData.push_back({"", price, tickData[sz-1].ask, size, tickData[sz-1].askVolume});
    }
    if(side==0){
        tickData.push_back({"", tickData[sz-1].bid, price, tickData[sz-1].bidVolume, size});
    }

    if(price>10.0)
        printf("%d|%6.3f|%6.3f|%d\n", tickData[sz].bidVolume, tickData[sz].bid, tickData[sz].ask, tickData[sz].askVolume);
    else
        printf("%d|%6.5f|%6.5f|%d\n", tickData[sz].bidVolume, tickData[sz].bid, tickData[sz].ask, tickData[sz].askVolume);
    return;
}
