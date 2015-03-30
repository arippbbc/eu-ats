#ifndef enumerations_h__INCLUDED
#define enumerations_h__INCLUDED
#include "IBString.h"

enum TIMEFRAME{
    S1,
    S5,
    S15,
    S30,
    M1,
    M2,
    M3,
    M5,
    M15,
    M30,
    H1,
    D1,
    TFSIZE
};

static const IBString barSizes[TFSIZE] = {
    "1 sec"
    ,"5 secs"
    ,"15 secs"
    ,"30 secs"
    ,"1 min"
    ,"2 mins"
    ,"3 mins"
    ,"5 mins"
    ,"15 mins"
    ,"30 mins"
    ,"1 hour"
    ,"1 day"
};

#endif
