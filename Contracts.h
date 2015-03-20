#ifndef contracts_h__INCLUDED
#define contracts_h__INCLUDED 
#include "Contract.h"
#include "IBString.h"

/* 
struct Forex: public struct Contract{

}
*/

inline void init(Contract &c, const IBString &a, const IBString &b){
    c.symbol = a;
    c.secType = "CASH";
    c.currency = b;
    c.exchange = "IDEALPRO";
}
//#define FX(A,B) extern const Contract A##B;
namespace Forex{
    namespace Major{
        //FX(AUD,USD);
        //FX(EUR,USD);
        extern const Contract& AUDUSD();
        extern const Contract& EURUSD();
        extern const Contract& NZDUSD();
        extern const Contract& USDCAD();
        extern const Contract& USDJPY();
        extern const Contract& GBPUSD();
        extern const Contract& USDCHF();
    }
    /*
    namespace Cross{
        extern const Contract& EURGBP();
        extern const Contract& EURCHF();
        extern const Contract& AUDNZD();
        extern const Contract& EURJPY();
        extern const Contract& GBPJPY();
        extern const Contract& AUDJPY();
        extern const Contract& NZDJPY();
    }
    */
}

#endif
