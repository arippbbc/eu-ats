#include "Contract.h"
#include "Instrument.h"
#include "IBString.h"
#include <algorithm>

using namespace std; 

void Instrument::subscribeAlgo(const AlgoBase &algo){
    auto it=find(subscribedAlgos.begin(), subscribedAlgos.end(), algo);
    if(it!=subscribedAlgos.end()){
        subscribedAlgos.push_back(algo);
    }
    else{
        printf("Warning: ignoring an attempt to subscribe already subscribed algo!\n");
    }
}

void Instrument::unsubscribeAlgo(const AlgoBase &algo){
    auto it=find(subscribedAlgos.begin(), subscribedAlgos.end(), algo);
    if(it!=subscribedAlgos.end()){
        subscribedAlgos.erase(it);
    }
    else{
        printf("Warning: ignoring an attempt to unsubscribe non-existing algo!\n");
    }
}
