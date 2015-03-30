#ifndef algo_h__INCLUDED
#define algo_h__INCLUDED

#include <memory>
#include "Contract.h"
#include "Data.h"

using namespace std;

//class StopOrders;

class AlgoBase
{
    public:
        bool operator==(const AlgoBase &algo) const{
            return false;
        }
        bool operator!=(const AlgoBase &algo) const{
            return !(*this==algo);
        }
        bool operator==(AlgoBase &algo){
            return false;
        }
        bool operator!=(AlgoBase &algo){
            return !(*this==algo);
        }
        AlgoBase(const string &_algoName):algoName(_algoName){};
        virtual ~AlgoBase();
    private:
        string algoName;
    //shared_ptr<Contract> contract;
    //shared_ptr<StopOrders> orders;
    //shared_ptr<Data> data;
};

typedef shared_ptr<AlgoBase> AlgoPtr;
/*
inline bool operator==(const AlgoBase &lhs, const AlgoBase &rhs){ return false;}
inline bool operator!=(const AlgoBase &lhs, const AlgoBase &rhs){ return !(lhs==rhs);}
*/


/*
// one particular algorithm implementation
class MyCoolAlgoCrossingMAs : public Algo
{

};

// another algorithm implementation
class MyEvenCoolerAlgoRandomBuyNeverSell : public Algo
{

};
*/

#endif
