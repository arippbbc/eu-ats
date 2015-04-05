#ifndef enumerations_h__INCLUDED
#define enumerations_h__INCLUDED
#include "IBString.h"

/*
 FIXME it might be a better idea to track only M1 historial data
 since high dataframe could be derived from M1 assuming M1 is the 
 highest resolution under track
*/
enum TIMEFRAME{
    S1=0,
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

static const IBString barSizeStr[TFSIZE] = {
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

// assume 500 bars
static const IBString durationStr[TFSIZE] = {
    "500 S"
    ,"2500 S"
    ,"7500 S"
    ,"15000 S"
    "1 D"
    ,"2 D"
    ,"3 D"
    ,"5 D"
    ,"15 D"
    ,"30 D"
    ,"60 D"
    ,"50 W"
};

#endif
