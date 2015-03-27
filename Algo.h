#ifndef algo_h__INCLUDED
#define algo_h__INCLUDED

#include <memory>
#include "Contrach.h"

class AlgoBaseClass 
{
private:
    shared_ptr<Contract*> contract;
    shared_ptr<StopOrders*> orders;
    shared_ptr<Data*> data;
};

// one particular algorithm implementation
class MyCoolAlgoCrossingMAs : public AlgoBaseClass
{

};

// another algorithm implementation
class MyEvenCoolerAlgoRandomBuyNeverSell : public AlgoBaseClass
{
};

#endif
