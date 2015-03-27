#ifndef data_h__INCLUDED
#define data_h__INCLUDED

#include "IBString.h"
#include <deque>

using namespace std;

struct OHLC
{
    IBString date;
	double open, high, low, close;
    int volume;
    //OHLC() { Reset(); }
    //OHLC(const IBString& _date, double _open, double _high, double _low, double _close, int _volume);
	//void Reset() { date = "2000-01-01 00:00:00"; O = 0, C = 0, L = 0, C = 0; volume = 0;}
};

class Data{
    public:
        //virtual void update();
        virtual void update(const IBString& date, double open, double high, double low, double close, int volume)=0;
        virtual ~Data(){};
};

class HistoricalData: public Data{
    public:
        // ignore barCount, WAP, hasGaps
        virtual void update(const IBString& date, double open, double high, double low, double close, int volume);
        virtual ~HistoricalData(){};
    private:
        deque<OHLC> hisData;
        //hard coded window for now
        static const unsigned len = 500;
};

#endif
