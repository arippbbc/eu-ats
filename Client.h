#ifndef client_h__INCLUDED
#define client_h__INCLUDED

#include "EWrapper.h"
//#include <windows.h>*/
#include "Contract.h"
#include "Instrument.h"
#include <memory>
#include <vector>
#include "EPosixClientSocket.h"
#include "EPosixClientSocketPlatform.h"
#include "Data.h"
#include <unordered_map>

//using namespace std;
class Instrument;
class DataCenter;

extern int NUM_OF_TICKS;
extern double trail;

// 2014-02-24 half pip function to remove warnings
double halfpip(double price);

class EPosixClientSocket;

enum State {
    ST_CONNECT,
    ST_CHECK,
    ST_PLACEORDER,
    ST_PLACEORDER_ACK,
    //ST_LONG,
    //ST_SHORT,
    ST_CANCELORDER,
    ST_CANCELORDER_ACK,
    ST_PING,
    ST_PING_ACK,
    ST_IDLE
};

enum Sig{
    _SHORT=-1,
    _NEUTRAL=0,
    _LONG=1
};


class Client : public EWrapper
{

    private:
        // FIXME
        typedef std::unordered_map<TickerId, std::string> reqIdMap;
    public:
            Client(int clientId = 0);
            Client();
            ~Client();
            void processMessages();

    public:
            bool connect(const char * host, unsigned int port);
            void disconnect() const;
            bool isConnected() const;
            void subscribeInstrument(const Instrument &inst);
            void unsubscribeInstrument(const Instrument &inst);

    private:
            void reqCurrentTime();
            //void placeOrder(const IBString &action);
            //void placeOrder();
            //void cancelOrder();

    public:
            // events
            void tickPrice(TickerId tickerId, TickType field, double price, int canAutoExecute);
            void tickSize(TickerId tickerId, TickType field, int size);
            void tickOptionComputation( TickerId tickerId, TickType tickType, double impliedVol, double delta,
                    double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice);
            void tickGeneric(TickerId tickerId, TickType tickType, double value);
            void tickString(TickerId tickerId, TickType tickType, const IBString& value);
            void tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const IBString& formattedBasisPoints,
                    double totalDividends, int holdDays, const IBString& futureExpiry, double dividendImpact, double dividendsToExpiry);
            void orderStatus(OrderId orderId, const IBString &status, int filled,
                    int remaining, double avgFillPrice, int permId, int parentId,
                    double lastFillPrice, int clientId, const IBString& whyHeld);
            void openOrder(OrderId orderId, const Contract&, const Order&, const OrderState&);
            void openOrderEnd();
            void winError(const IBString &str, int lastError);
            void connectionClosed();
            void updateAccountValue(const IBString& key, const IBString& val,
                    const IBString& currency, const IBString& accountName);
            void updatePortfolio(const Contract& contract, int position,
                    double marketPrice, double marketValue, double averageCost,
                    double unrealizedPNL, double realizedPNL, const IBString& accountName);
            void updateAccountTime(const IBString& timeStamp);
            void accountDownloadEnd(const IBString& accountName);
            void nextValidId(OrderId orderId);
            void contractDetails(int reqId, const ContractDetails& contractDetails);
            void bondContractDetails(int reqId, const ContractDetails& contractDetails);
            void contractDetailsEnd(int reqId);
            void execDetails(int reqId, const Contract& contract, const Execution& execution);
            void execDetailsEnd(int reqId);
            void error(const int id, const int errorCode, const IBString errorString);
            void updateMktDepth(TickerId id, int position, int operation, int side,
                    double price, int size);
            void updateMktDepthL2(TickerId id, int position, IBString marketMaker, int operation,
                    int side, double price, int size);
            void updateNewsBulletin(int msgId, int msgType, const IBString& newsMessage, const IBString& originExch);
            void managedAccounts(const IBString& accountsList);
            void receiveFA(faDataType pFaDataType, const IBString& cxml);
            void historicalData(TickerId reqId, const IBString& date, double open, double high,
                    double low, double close, int volume, int barCount, double WAP, int hasGaps);
            void scannerParameters(const IBString &xml);
            void scannerData(int reqId, int rank, const ContractDetails &contractDetails,
                    const IBString &distance, const IBString &benchmark, const IBString &projection,
                    const IBString &legsStr);
            void scannerDataEnd(int reqId);
            void realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
                    long volume, double wap, int count);
            void currentTime(long time);
            void fundamentalData(TickerId reqId, const IBString& data);
            void deltaNeutralValidation(int reqId, const UnderComp& underComp);
            void tickSnapshotEnd(int reqId);
            void marketDataType(TickerId reqId, int marketDataType);
            void commissionReport( const CommissionReport& commissionReport);
            void position( const IBString& account, const Contract& contract, int position, double avgCost);
            void positionEnd();
            void accountSummary( int reqId, const IBString& account, const IBString& tag, const IBString& value, const IBString& curency);
            void accountSummaryEnd( int reqId);
            void verifyMessageAPI( const IBString& apiData);
            void verifyCompleted( bool isSuccessful, const IBString& errorText);
            void displayGroupList( int reqId, const IBString& groups);
            void displayGroupUpdated( int reqId, const IBString& contractInfo);

            // public function
            void test();
            void demo();
            void initialCheck();

            // forward some function for EPosixClient
            void reqHistoricalData(TickerId id, const Contract &contract,
                    const IBString &endDateTime, const IBString &durationStr, const IBString &barSizeSetting,
                    const IBString &whatToShow, int useRTH, int formatDate);

            inline std::vector<Instrument> getsubscribedInst() {return m_subscribedInst;}

            //int getreqHistoricalDataId(){
            //    return reqHistoricalDataId++;
            //}

    private:
            std::shared_ptr<EPosixClientSocket> m_pClient;
            reqIdMap m_reqTDPool;
            reqIdMap m_reqHDPool;
            std::vector<Instrument> m_subscribedInst;
            std::shared_ptr<DataCenter> m_dataCenter;
            //DataCenter dCenter;
            int m_clientId;
            State m_state;
            // this is the flag for the adjustment of stoploss and profit taking order after parent order
            // gets filled
            //bool m_noposition;
            //bool m_modified;
            time_t m_sleepDeadline;
            //Contract m_contract;
            OrderId m_orderId;
            const TagValueListSPtr m_taglist;
            // FIXME possible ID handling class
            int m_reqId;


};

#endif

