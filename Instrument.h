#ifndef instrument_h__INCLUDED 
#define instrument_h__INCLUDED

#include "Contract.h"
#include "IBString.h"
#include "Algo.h"
#include <memory>
#include <vector>

using namespace std;

class Instrument{
    public:
        Instrument();
        virtual ~Instrument();
    private:
        void subscribeAlgo(const AlgoBase &algo);
        void unsubscribeAlgo(const AlgoBase &algo);
    private:
        Contract c;
        vector<AlgoBase> subscribedAlgos;
};

Contract makeForex(const string &sym){
    Contract fx;
    fx.symbol = sym.substr(0,3);
    fx.secType = "CASH";
    fx.currency = sym.substr(4,3);
    fx.exchange = "IDEALPRO";
    return fx;
}

#endif
