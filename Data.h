#ifndef data_h__INCLUDED
#define data_h__INCLUDED

#include "IBString.h"
#include <deque>
#include "Enumerations.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include "Client.h"

using namespace std;

class Client;

struct DataPoint{
    virtual ~DataPoint();
};

//struct OHLC: public DataPoint
struct OHLC
{
    IBString date;
	double open, high, low, close;
    int volume;
    //OHLC() { Reset(); }
    //OHLC(const IBString& _date, double _open, double _high, double _low, double _close, int _volume);
	//void Reset() { date = "2000-01-01 00:00:00"; O = 0, C = 0, L = 0, C = 0; volume = 0;}
};

// FIXME
// this is a better design, so realtime spread is tracable
//struct L2: public DataPoint{
struct L2{
    IBString date;
    double bid;
    double ask;
    int bidVolume;
    int askVolume;
};

class Data{
    public:
        //virtual void update();
        virtual void update(string date, double open, double high, double low, double close, int volume) {};
        virtual void update(int side, double price, int size) {};
        virtual ~Data(){};
    private:
        //shared_ptr<Client> d_client;
};

//typedef shared_ptr<Contract, Data> DataCenter;

class HistoricalData: public Data{
    public:
        // ignore barCount, WAP, hasGaps
        virtual void update(string date, double open, double high, double low, double close, int volume);
        //virtual ~HistoricalData(){};
    private:
        deque<OHLC> histData;
        // FIXME, is there enough memory to keep all historical data? hard coded window for now
        //static const unsigned len = 5000;
};

class TickData: public Data{
    public:
        // ignore barCount, WAP, hasGaps
        virtual void update(int side, double price, int size);
        //virtual ~TickData(){};
    private:
        deque<L2> tickData;
        // FIXME, is there enough memory to keep all historical data? hard coded window for now
};

class DepthData: public Data{
    public:
        virtual void update();
    private:
        deque<L2> l2Data;
};

class DataCenter{
    friend Client;
    private:
        typedef shared_ptr<Data> dataPtr;
        //typedef unordered_map<IBString, unordered_map<IBString, dataPtr> >  HdataMap;
        typedef unordered_map<IBString, dataPtr> HdataMap;
        typedef unordered_map<IBString, dataPtr> TdataMap;
        HdataMap HDCenter;
        TdataMap TDCenter;

    public:
        HdataMap& getHistData() {return HDCenter;} 
        TdataMap& getTickData() {return TDCenter;} 
        //DataCenter();
        //~DataCenter(){};
};

#endif
