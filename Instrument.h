#ifndef instrument_h__INCLUDED 
#define instrument_h__INCLUDED

#include "Contract.h"
#include "IBString.h"
#include "Algo.h"
#include <memory>
#include <vector>

using namespace std;

inline bool operator==(const Contract &lhs, const Contract &rhs){
    return lhs.symbol == rhs.symbol
        && lhs.secType == rhs.secType
        && lhs.currency == rhs.currency
        && lhs.expiry == rhs.expiry
        && lhs.strike == rhs.strike;
}

inline bool operator!=(const Contract &lhs, const Contract &rhs){
    return !(lhs == rhs);
}

class Instrument{
    public:
        Instrument(const Contract &_c):c(_c){};
        const Contract getContract() const { return c;}
        Contract getContract() { return c;}
        // FIXME
        bool inline operator==(const Instrument &inst) const { return (*this).c==inst.c; }
        bool inline operator!=(const Instrument &inst) const { return !((*this).c==inst.c); }
    private:
        void subscribeAlgo(const AlgoPtr &algo);
        void unsubscribeAlgo(const AlgoPtr &algo);
    private:
        Contract c;
        vector<AlgoPtr> subscribedAlgos;
};

Contract makeForex(const string &sym);

double halfpip(double price);

#endif
