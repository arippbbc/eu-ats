#include "Contract.h"
#include "Contracts.h"
#include "IBString.h"

//#define INIT(A,B) const Contract A##B; A##B.secType="CASH"; A##B.symbol = #A; A##B.currency = #B;

namespace Forex{
    namespace Major{
        const Contract& AUDUSD(){
            static Contract audusd;
            init(audusd, "AUD", "USD");
            return audusd;
        }

        const Contract& EURUSD(){
            static Contract eurusd;
            init(eurusd, "AUD", "USD");
            return eurusd;
        }

        const Contract& NZDUSD(){
            static Contract nzdusd;
            init(nzdusd, "NZD", "USD");
            return nzdusd;
        }

        const Contract& GBPUSD(){
            static Contract gbpusd;
            init(gbpusd, "GBP", "USD");
            return gbpusd;
        }

        const Contract& USDCHF(){
            static Contract usdchf;
            init(usdchf, "USD", "CHF");
            return usdchf;
        }

        const Contract& USDJPY(){
            static Contract usdjpy;
            init(usdjpy, "USD", "JPY");
            return usdjpy;
        }

        const Contract& USDCAD(){
            static Contract usdcad;
            init(usdcad,  "USD", "CAD");
            return usdcad;
        }

    }
}
