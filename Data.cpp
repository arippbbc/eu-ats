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

DataCenter::DataCenter(shared_ptr<Client> _client):d_client(_client){
    //cout << d_client->getsubscribedInst().size() << endl;
    auto Instruments = d_client->getsubscribedInst();
    for(auto in = Instruments.begin(), end = Instruments.end(); in!=end; ++ in){
        auto c = in->getContract();
        auto endDateTime = getCurrentTime();
        endDateTime = endDateTime.substr(0, 4) + endDateTime.substr(5,2) + endDateTime.substr(8,11);
        IBString whatToShow = "BID";
        int useRTH = 1;
        int formatDate = 1;
        for(int i = 0; i < TFSIZE; ++i){
            auto barSize = barSizeStr[i];
            auto duration = durationStr[i]; 
            d_client->reqHistoricalData(d_client->getreqHistoricalDataId(), c, endDateTime, duration, barSize, whatToShow, useRTH, formatDate);
        }
    }
}
