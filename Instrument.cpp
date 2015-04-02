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

Contract makeContract(const string &type, const string &id){
    if(type == "FOREX"){
        Contract fx;
        fx.symbol = id.substr(0,3);
        fx.secType = "CASH";
        fx.currency = id.substr(3,3);
        fx.exchange = "IDEALPRO";
        return fx;
    }
}

double halfpip(double price){
    int lastdigit = int(price*100000)%10;
    switch(lastdigit){
        case 0:
        case 5:
            return price;
        case 1:
        case 2:
            return price-lastdigit/100000.;
        case 3:
        case 4:
        case 6:
        case 7:
            return price+(5-lastdigit)/100000.;
        case 8:
        case 9:
            return price+(10-lastdigit)/100000.;
        default:
            return price;
    }
}
