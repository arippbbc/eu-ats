#include "Contract.h"
#include "Instrument.h"
#include "IBString.h"
#include <algorithm>
#include "Algo.h"

using namespace std; 

void Instrument::subscribeAlgo(const AlgoPtr &algo){
    auto it=find(subscribedAlgos.begin(), subscribedAlgos.end(), algo);
    if(it!=subscribedAlgos.end()){
        subscribedAlgos.push_back(algo);
    }
    else{
        printf("Warning: ignoring an attempt to subscribe already subscribed algo!\n");
    }
}

void Instrument::unsubscribeAlgo(const AlgoPtr &algo){
    auto it=find(subscribedAlgos.begin(), subscribedAlgos.end(), algo);
    if(it!=subscribedAlgos.end()){
        subscribedAlgos.erase(it);
    }
    else{
        printf("Warning: ignoring an attempt to unsubscribe non-existing algo!\n");
    }
}

Contract makeForex(const string &sym){
    Contract fx;
    fx.symbol = sym.substr(0,3);
    fx.secType = "CASH";
    fx.currency = sym.substr(3,3);
    fx.exchange = "IDEALPRO";
    return fx;
}

