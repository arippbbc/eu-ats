#include "Client.h"
#include "EPosixClientSocket.h"
#include "EPosixClientSocketPlatform.h"

#include "Contract.h"
#include "Order.h"
#include "OrderState.h"
#include <vector>
#include <algorithm>
#include <functional>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
//#include <time.h>
//#include <sys/time.h>
#include <memory>
#include "Timer.h"
//#include <windows.h>

using namespace std;

ofstream log_connect;
ofstream bidfile, askfile, l2file;
FILE *audf;

bool BIDIRECTION = false;
bool lmtEntry = true;
bool USETRAIL = false;
//bool USETRAIL = true;
bool FIRSTORDER = true;
bool HAS_POSITION = false;
bool GOLONG = false, GOSHORT = false;

// reqHistorical only accept "YYYYMMDD HH:MM:SS"
char curTime[20];
char curDate[20];

const int totalQuantity = 100000;
const int PING_DEADLINE = 10; // seconds
const int SLEEP_BETWEEN_PINGS = 30; // seconds

const double epsilon = 0.00001;
const double tick = 0.00005;
double sl = 20*tick;
double tp = 30*tick;
double trail = 15*tick;

vector<double> xopen, low, high, last;
vector<double> emafast, emaslow;
int cnt = 0;

double bid=0.0, ask=0.0, mid= 0.0, h=0.0, l=0.0;

const size_t NUM_OF_BARS = 0;
int NUM_OF_TICKS = 200;
//int NUM_OF_TICKS = 2;
double avgPrice = 0.0;

string clientIdStr = "";

//Contract aud;
//Contract es;
//Contract aapl;
int reqaud=1;

const int fast=28, slow=80;        
//const int fast=3, slow=8;        
Order s_order_parent, s_order_stoploss, s_order_profittaking, s_order_trailing;
Order l_order_parent, l_order_stoploss, l_order_profittaking, l_order_trailing;

const Contract audusd = makeContract("FOREX", "AUDUSD");

// member funcs
    Client::Client(int clientId)
    : m_pClient(new EPosixClientSocket(this))
    , m_dataCenter(new DataCenter())
    , m_clientId(clientId)
      , m_state(ST_CONNECT)
      //better to set up initial value to be false
      //, m_noposition(false)
      //, m_modified(false)
      , m_sleepDeadline(0)
      //, m_contract(Contract())
    , m_orderId(0)
      , m_taglist(new vector<TagValueSPtr>())
      // FIXME
      , m_reqId(1000)
      //, m_data(new HistoricalData())
{
}

Client::~Client()
{
}

bool Client::connect(const char *host, unsigned int port)
{
    // trying to connect
    printf( "Connecting to %s:%d clientId:%d\n", !( host && *host) ? "127.0.0.1" : host, port, m_clientId);

    bool bRes = m_pClient->eConnect( host, port, m_clientId);

    if (bRes) {
        printf( "Connected to %s:%d clientId:%d\n", !( host && *host) ? "127.0.0.1" : host, port, m_clientId);
    }
    else
        printf( "Cannot connect to %s:%d clientId:%d\n", !( host && *host) ? "127.0.0.1" : host, port, m_clientId);

    //2014-02-09 start to read portfolio and account summary here?
    return bRes;
}

void Client::disconnect() const
{
    m_pClient->eDisconnect();

    if(audf){
        fclose(audf);
    }

    if(log_connect.is_open()){
        log_connect.close();
    }
    printf ( "Disconnected\n");
}

bool Client::isConnected() const
{
    return m_pClient->isConnected();
}

void Client::processMessages()
{
    fd_set readSet, writeSet, errorSet;

    struct timeval tval;
    tval.tv_usec = 0;
    tval.tv_sec = 0;

    time_t now = time(NULL);

    switch (m_state) {
        case ST_CHECK:
            initialCheck();
            break;
    }

    if( m_sleepDeadline > 0) {
        // initialize timeout with m_sleepDeadline - now
        tval.tv_sec = m_sleepDeadline - now;
    }

    if( m_pClient->fd() >= 0 ) {

        FD_ZERO( &readSet);
        errorSet = writeSet = readSet;

        FD_SET( m_pClient->fd(), &readSet);

        if( !m_pClient->isOutBufferEmpty())
            FD_SET( m_pClient->fd(), &writeSet);

        FD_CLR( m_pClient->fd(), &errorSet);

        int ret = select( m_pClient->fd() + 1, &readSet, &writeSet, &errorSet, &tval);

        if( ret == 0) { // timeout
            return;
        }

        if( ret < 0) {	// error
            disconnect();
            // potential double free
            if(audf) audf = NULL;
            return;
        }

        if( m_pClient->fd() < 0)
            return;

        if( FD_ISSET( m_pClient->fd(), &errorSet)) {
            // error on socket
            m_pClient->onError();
        }

        if( m_pClient->fd() < 0)
            return;

        if( FD_ISSET( m_pClient->fd(), &writeSet)) {
            // socket is ready for writing
            m_pClient->onSend();
        }

        if( m_pClient->fd() < 0)
            return;

        if( FD_ISSET( m_pClient->fd(), &readSet)) {
            // socket is ready for reading
            m_pClient->onReceive();
        }
    }
}

void Client::reqCurrentTime()
{
    //printf("Requesting Current Time\n");

    // set ping deadline to "now + n seconds"
    //m_sleepDeadline = time( NULL) + PING_DEADLINE;

    //m_state = ST_PING_ACK;

    m_pClient->reqCurrentTime();
}

// FIXME if const Instrument then getContract must return const
void Client::subscribeInstrument(const Instrument& inst){
    //cout << "Current size of instrument list is " << m_subscribedInst.size() << endl;
    //cout << "Current instrument list is " << inst.getContract().symbol << endl;
    auto it=find(m_subscribedInst.begin(), m_subscribedInst.end(), inst);
    if(it==m_subscribedInst.end()){
        m_subscribedInst.push_back(inst);
    }
    else{
        printf("Warning: ignoring an attempt to subscribe already subscribed instrument!\n");
    }
}

void Client::unsubscribeInstrument(const Instrument& inst){
    auto it=find(m_subscribedInst.begin(), m_subscribedInst.end(), inst);
    if(it!=m_subscribedInst.end()){
        m_subscribedInst.erase(it);
    }
    else{
        printf("Warning: ignoring an attempt to unsubscribe non-existing instrument!\n");
    }
}


/*  
    void Client::placeOrder()
    {
    return;
    }

    void Client::cancelOrder()
    {
    printf("Client %d: Cancelling Order %ld\n", m_clientId, m_orderId);

    m_state = ST_CANCELORDER_ACK;

    m_pClient->cancelOrder(m_orderId);
    printf("Client %d: cancel order function called!", m_clientId);
    }
    */

// possible duplicate messages
void Client::orderStatus(OrderId orderId, const IBString &status, int filled,
        int remaining, double avgFillPrice, int permId, int parentId,
        double lastFillPrice, int clientId, const IBString& whyHeld)

{
    printf("Order: id=%ld, parentId=%d, status=%s, filled price=%.5f, average filled price=%.5f\n", orderId, parentId, status.c_str(), lastFillPrice, avgFillPrice);
}

void Client::test(){
    //cout << "Current size of instrument list is " << m_subscribedInst.size() << endl;
    Instrument AU("FOREX", "AUDUSD");
    this->subscribeInstrument(AU);

    //Instrument EU("FOREX", "EURUSD");
    //this->subscribeInstrument(EU);

    auto endDateTime = getCurrentTime();
    endDateTime = endDateTime.substr(0, 4) + endDateTime.substr(5,2) + endDateTime.substr(8,11);
    cout << endDateTime << endl;
    // FIXME
    IBString whatToShow = "BID";
    int useRTH = 1;
    int formatDate = 1;
    for(auto inst = m_subscribedInst.begin(), end = m_subscribedInst.end(); inst!=end; ++ inst){
        auto contract = inst->getContract();
        //for(int i = 0; i < TFSIZE; ++i){
        for(int i = M1; i < M1+1; ++i){
            auto barSize = barSizeStr[i];
            auto duration = durationStr[i]; 
            // FIXME instrument alone is not enough to identify request ID
            m_reqHDPool.insert({m_reqId, inst->getInstrumentID()});
            m_pClient->reqHistoricalData(m_reqId++, contract, endDateTime, duration, barSize, whatToShow, useRTH, formatDate, m_taglist);
            //m_reqTDPool.insert({m_reqId, inst->getInstrumentID()});

            // FIXME
            //auto reqMktDepthLevel = 2;
            //m_pClient->reqMktDepth(m_reqId++, contract, reqMktDepthLevel, m_taglist);
        }
    }
}

void Client::demo(){
    const Contract contract = makeContract("FOREX", "AUDUSD");
    //m_pClient->isConnected();
    //m_pClient->checkMessages();

    //int version = m_pClient->serverVersion();
    //printf("Server version is %d\n", version);
    //m_pClient->setLogLevel(1);

    //string TwsConnectionTime = m_pClient->TwsConnectionTime();
    //printf("TWS connection time is %s\n", TwsConnectionTime.c_str());

    // currentTime() current system time on the server side
    //m_pClient->reqCurrentTime();

    // next valid orderId, reserver 1000 Ids
    m_pClient->reqIds(1000);

    // cancel all open order globally
    //m_pClient->reqGlobalCancel();

    //m_pClient->reqAccountUpdates(true, "aripp005");

    // this client orders
    //m_pClient->reqOpenOrders();
    // all clients orders
    //m_pClient->reqAllOpenOrders();
    // newly created order
    //m_pClient->reqAutoOpenOrders(true);

    //m_pClient->reqPositions();
    //m_pClient->cancelPositions();

    //int reqExecutionsId = 100;
    //const ExecutionFilter filter;
    //m_pClient->reqExecutions(reqExecutionsId, filter);

    //const int reqContractDetailsId = 200;
    //m_pClient->reqContractDetails(reqContractDetailsId, contract);

    //m_pClient->reqNewsBulletins(true);
    //m_pClient->cancelNewsBulletins();

    //m_pClient->reqMarketDataType(1);
    //int reqMktDataId = 10;
    //m_pClient->reqMktData(reqMktDataId, contract, "225", false, m_taglist);
    //m_pClient->cancelMktData(reqMktDataId);

    int reqMktDepthId = 20;
    int reqMktDepthLevel = 20;

    m_pClient->reqMktDepth(reqMktDepthId, contract, reqMktDepthLevel, m_taglist);
    //m_pClient->cancelMktDepth(reqMktDepthId);

    //int reqRealTimeBarsId = 30;
    //int realTimebarSize = 5;
    //const IBString whatToShow = "ASK";
    //const int useRTH = 1;
    //const TagValueListSPtr realTimeBarsOptions = m_taglist;
    //m_pClient->reqRealTimeBars(reqRealTimeBarsId, contract, realTimebarSize, whatToShow, useRTH, realTimeBarsOptions);
    //m_pClient->cancelRealTimeBars(reqRealTimeBarsId);

    //const int reqHistoricalDataId = 40;

    //IBString endDateTime = getCurrentTime();
    //endDateTime = endDateTime.substr(0, 4) + endDateTime.substr(5,2) + endDateTime.substr(8,11);
    //cout << endDateTime << endl;
    //const IBString barSize = "5 mins";
    //const IBString duration = "3600 S";
    //const int formatDate = 1;
    //const TagValueListSPtr chartOption = m_taglist;
    //m_pClient->reqHistoricalData(reqHistoricalDataId, contract, endDateTime, duration, barSize, whatToShow, useRTH, formatDate, chartOption);
    //m_pClient->cancelHistoricalData(reqHistoricalDataId);
}

// this is called automatically on connection, should mannually maintain valid orderId
void Client::nextValidId(OrderId orderId)
{
    //printf("update next valid order id: %d\n", orderId);
    m_orderId = orderId;
}

//string Client::fromLongtoTime(long time){
//char Time[20];
//}

void Client::currentTime(long xtime)
{
    time_t t = time(NULL); 
    time(&t);
    strftime(curTime, sizeof curTime, "%Y%m%d %H:%M:%S", localtime(&t)); 
    strftime(curDate, sizeof curDate, "%Y%m%d", localtime(&t));
    cout << "Client " << m_clientId << ": The current date/time is: " << curTime << endl;
    //log_connect.open(("log/"+string(curDate)+"_connect.txt").c_str(), ios::out|ios::app);
    //audf=fopen("data/aud.csv", "a");
}

void Client::error(const int id, const int errorCode, const IBString errorString)
{
    if(id==-1 && (
                errorCode==2104 
                || errorCode==2106
                || errorCode==2108
                || errorString=="Market data farm connection is inactive but should be available upon demand.usfuture.us")) return;
    printf("Error: id=%d, errorCode=%d, msg=%s\n", id, errorCode, errorString.c_str());
    /*
       if( id == -1 && errorCode == 1100){ //if "Connectivity between IB and TWS has been lost"
       disconnect();
       time_t t;
       t = time(NULL);
       strftime(curTime, sizeof curTime, "%Y%m%d %H:%M:%S", localtime(&t));
       log_connect << curTime << "|disconnected\n";
       }

       if( id == -1 && errorCode == 1102){ //if "Connectivity between IB and TWS has been lost"
       time_t t;
       t = time(NULL);
       strftime(curTime, sizeof curTime, "%Y%m%d %H:%M:%S", localtime(&t));
       log_connect << curTime << "|reconnected\n";
       }

    //printf("are we stopped out or profit taken, %d, %d\n", id, m_clientId);
    if(id == m_orderId-1 || id == m_orderId-2){
    HAS_POSITION=false;
    //printf("stopped out or profit taken\n");
    }
    */
}

void Client::tickPrice(TickerId tickerId, TickType field, double price, int canAutoExecute) {
    printf("tickPrice: %d|%d|%f|%d\n", tickerId, field, price, canAutoExecute);
}

void Client::tickSize( TickerId tickerId, TickType field, int size) {
    //printf("tickSize: %d|%d|%d\n", tickerId, field, size);
}

void Client::tickOptionComputation( TickerId tickerId, TickType tickType, double impliedVol, double delta,
        double optPrice, double pvDividend,
        double gamma, double vega, double theta, double undPrice) {}

void Client::tickGeneric(TickerId tickerId, TickType tickType, double value) {
    //printf("tickGeneric: %d|%d|%f\n", tickerId, tickType, value);
}

void Client::tickString(TickerId tickerId, TickType tickType, const IBString& value) {
    //printf("tickGeneric: %d|%d|%s\n", tickerId, tickType, value.c_str());
}

void Client::tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const IBString& formattedBasisPoints,
        double totalDividends, int holdDays, const IBString& futureExpiry, double dividendImpact, double dividendsToExpiry) {}
void Client::openOrder(OrderId orderId, const Contract &contract, const Order &order, const OrderState& ostate) {
    //printf("OpenOrder: orderId=%d, contract=%s, order=%s, ostate=%s\n", orderId, contract.info(),
    //order.info(), ostate.info());
}

void Client::openOrderEnd() {
    printf("OpenOrder request ends.\n");
}

void Client::winError( const IBString &str, int lastError) {
    printf("WinError: %s|%d\n", str.c_str(), lastError);
}

// no need
void Client::connectionClosed() {}

void Client::updateAccountValue(const IBString& key, const IBString& val,
        const IBString& currency, const IBString& accountName) {
    printf("updateAccountValue: key=%s, val=%s, currency=%s, accountName=%s\n", key.c_str(), val.c_str(), currency.c_str(), accountName.c_str());
}

void Client::updatePortfolio(const Contract& contract, int position,
        double marketPrice, double marketValue, double averageCost,
        double unrealizedPNL, double realizedPNL, const IBString& accountName){
    //printf("updatePortfolio: contract=%s, position=%s, marketPrice=%f, marketValue=%f, averageCost=%f, unrealizedPNL=%f, realizedPNL=%f, accountName=%s\n", 
    //contract.info().c_str(), position, marketPrice, marketValue, averageCost, unrealizedPNL, realizedPNL, accountName.c_str());
}

void Client::updateAccountTime(const IBString& timeStamp) {
    printf("updateAccountTime: timeStamp=%s\n", timeStamp.c_str());
}

void Client::accountDownloadEnd(const IBString& accountName) {
    printf("accountDownloadEnd request ends for %s\n", accountName.c_str());
}

void Client::contractDetails( int reqId, const ContractDetails& contractDetails) {
    //printf("contractDetails: reqId=%d, ContractDetails=%s\n", reqId, ContractDetails.info().c_str());
}

// no need
void Client::bondContractDetails( int reqId, const ContractDetails& contractDetails) {}

void Client::contractDetailsEnd( int reqId) {
    printf("contractDetailsEnd request ends.\n");
}

void Client::execDetails( int reqId, const Contract& contract, const Execution& execution) {
    //printf("execDetails: reqId=%d, contract=%s, execution=%s\n", reqId, contract.info().c_str(), execution.info().c_str());
}

void Client::execDetailsEnd( int reqId) {
    printf("execDetailsEnd request ends.\n");
}

// operation 0 = insert, 1 = update, 2 = delete
// for bid/ask it's always update
// side 0 = ask, 1 = bid
void Client::updateMktDepth(TickerId id, int position, int operation, int side,
        double price, int size) {
    //printf("updateMktDepth: position %d, operation %d, side %d, price %.5f, %d size\n", position, operation, side, price, size);
    // FIXME I am not sure if message of tick data comes in the right order
    auto instId = m_reqTDPool[id];
    // FIXME: auto seems to make a copy 
    auto x = m_dataCenter->getTickData();
    if(m_dataCenter->getTickData().find(instId)==m_dataCenter->getTickData().end()){ 
        m_dataCenter->getTickData().insert(make_pair(instId, shared_ptr<Data>(new TickData())));
    }
    m_dataCenter->getTickData()[instId]->update(side, price, size);
}

void Client::updateMktDepthL2(TickerId id, int position, IBString marketMaker, int operation,
        int side, double price, int size) {
    //printf("updateMktDepth2: position %d, marketMaker %s, operation %d, side %d, price %.5f, %d size\n", position, marketMaker.c_str(), operation, side, price, size);
}

void Client::updateNewsBulletin(int msgId, int msgType, const IBString& newsMessage, const IBString& originExch) {
    printf("updateNewsBulletin: msgId=%d, msgType=%d, newsMessage=%s, originExch=%s\n", msgId, msgType, newsMessage.c_str(), originExch.c_str());
}

void Client::managedAccounts( const IBString& accountsList) {}
void Client::receiveFA(faDataType pFaDataType, const IBString& cxml) {}
void Client::historicalData(TickerId reqId, const IBString& date, double open, double high,
        double low, double close, int volume, int barCount, double WAP, int hasGaps) {
    // FIXME how to identify historical data request from reqId?
    if(date.find("finished")==string::npos){
        auto instId = m_reqHDPool[reqId];
        if(m_dataCenter->getHistData().find(instId)==m_dataCenter->getHistData().end()){ 
            m_dataCenter->getHistData().insert(make_pair(instId, shared_ptr<Data>(new HistoricalData())));
        }
        m_dataCenter->HDCenter[instId]->update(date, open, high, low, close, volume);
    }
    else
        cout << "Historical Data for " << reqId << " downloaded!" << endl;
    //printf("historicalData: reqId=%d, %s|%f|%f|%f|%f|%d|%d|%f|%d\n", reqId, date.c_str(), open, high, low, close, volume, barCount, WAP, hasGaps);
}

void Client::scannerParameters(const IBString &xml) {}
void Client::scannerData(int reqId, int rank, const ContractDetails &contractDetails,
        const IBString &distance, const IBString &benchmark, const IBString &projection,
        const IBString &legsStr) {}
void Client::scannerDataEnd(int reqId) {}
void Client::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
        long volume, double wap, int count) {

    printf("realtimeBar: reqId=%d, %ld|%f|%f|%f|%f|%ld|%f|%d\n", reqId, time, open, high, low, close, volume, wap, count);
}

void Client::fundamentalData(TickerId reqId, const IBString& data) {}
void Client::deltaNeutralValidation(int reqId, const UnderComp& underComp) {}

// no need
void Client::tickSnapshotEnd(int reqId) {}
void Client::marketDataType(TickerId reqId, int marketDataType) {}

void Client::initialCheck(){
    m_pClient->reqAllOpenOrders();
}

void Client::commissionReport( const CommissionReport& commissionReport) {
    //printf("commissionReport: commissionReport=%s\n", commissionReport);
}

void Client::position( const IBString& account, const Contract& contract, int position, double avgCost) {
    //printf("positon: account=%s, contract=%s, position=%d, avgCost=%f\n", account.c_str(), contract.info().c_str(), position, avgCost);
}

void Client::positionEnd() {
    printf("positionEnd requst ends.\n");
}

void Client::accountSummary( int reqId, const IBString& account, const IBString& tag, const IBString& value, const IBString& curency) {
    printf("accountSummary: reqId=%d, account=%s, tag=%s, value=%s, curency=%s\n", reqId, account.c_str(), tag.c_str(), value.c_str(), curency.c_str());
}

void Client::accountSummaryEnd( int reqId) {
    printf("Account summary ends for %d.\n", reqId);
}

void Client::verifyMessageAPI( const IBString& apiData) {
    printf("verifyMessageAPI: apiData=%s\n", apiData.c_str());
}

void Client::verifyCompleted( bool isSuccessful, const IBString& errorText) {}
void Client::displayGroupList( int reqId, const IBString& groups) {}
void Client::displayGroupUpdated( int reqId, const IBString& contractInfo) {}


void Client::reqHistoricalData(TickerId id, const Contract &contract,
        const IBString &endDateTime, const IBString &durationStr, const IBString &barSizeSetting,
        const IBString &whatToShow, int useRTH, int formatDate){
    m_pClient->reqHistoricalData(id, contract, endDateTime, durationStr, barSizeSetting, whatToShow, useRTH, formatDate, m_taglist);
}

