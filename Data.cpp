#include "Data.h"
#include <deque>
#include <cstdio>

//OHLC::OHLC(const IBString& _date, double _open, double _high, double _low, double _close, int _volume):date(_date), open(_open), high(_high), low(_low), close(_close), volume(_volume){}

void HistoricalData::update(const IBString& date, double open, double high, double low, double close, int volume){
    if(hisData.size()==len){
        hisData.pop_front(); 
    }
    hisData.push_back({date, open, high, low, close, volume});
    printf("historicalData: %s|%f|%f|%f|%f|%d\n", date.c_str(), open, high, low, close, volume);
}
