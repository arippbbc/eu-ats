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
#include <time.h>
#include <sys/time.h>
#include <windows.h>

using namespace std;

struct OHLC
{
	double O, H, L, C;
	OHLC() { Reset(); }
	void Reset() { O = 0, C = 0, L = 0, C = 0; }
};

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
        
Contract aud;
Contract es;
Contract aapl;
int reqaud=1;

const int fast=28, slow=80;        
//const int fast=3, slow=8;        
Order s_order_parent, s_order_stoploss, s_order_profittaking, s_order_trailing;
Order l_order_parent, l_order_stoploss, l_order_profittaking, l_order_trailing;

double halfpip(double price){
    int lastdigit = int(price*100000)%10;
    switch(lastdigit){
        case 0:
        case 5:
            return price;
        case 1:
        case 2:
            return price-lastdigit/100000.;
        case 3:
        case 4:
        case 6:
        case 7:
            return price+(5-lastdigit)/100000.;
        case 8:
        case 9:
            return price+(10-lastdigit)/100000.;
        default:
            return price;
    }
}

/* 2014-02-23 global contract definition */
void initializeContract(){
    aud.symbol = "AUD";
    aud.secType = "CASH";
    aud.exchange = "IDEALPRO";
    aud.currency = "USD";

    //Order s_order_parent, s_order_stoploss, s_order_profittaking;
    //Order l_order_parent, l_order_stoploss, l_order_profittaking;
}

///////////////////////////////////////////////////////////
// member funcs
Client::Client(int clientId)
    //Client::Client()
	: m_pClient(new EPosixClientSocket(this))
      //, m_clientId(clientId)
	, m_state(ST_CONNECT)
    //better to set up initial value to be false
    , m_noposition(false)
    , m_modified(false)
	, m_sleepDeadline(0)
    , m_contract(Contract())
	, m_orderId(0)
    , m_clientId(clientId)
    , m_taglist(new vector<TagValueSPtr>())
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
    initializeContract();
    m_contract = aud;
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

// possible duplicate messages
void Client::orderStatus(OrderId orderId, const IBString &status, int filled,
	   int remaining, double avgFillPrice, int permId, int parentId,
	   double lastFillPrice, int clientId, const IBString& whyHeld)

{
    printf("Order: id=%ld, parentId=%d, status=%s, filled price=%.5f, average filled price=%.5f\n", orderId, parentId, status.c_str(), lastFillPrice, avgFillPrice);
}

void Client::barRecord(){
}

void Client::test(){
    const Contract contract = aud;
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

    m_pClient->reqMarketDataType(1);
    int reqMktDataId = 10;
    //m_pClient->reqMktData(reqMktDataId, contract, "225", false, m_taglist);
    //m_pClient->cancelMktData(reqMktDataId);

    int reqMktDepthId = 20;
    int reqMktDepthLevel = 20;

    m_pClient->reqMktDepth(reqMktDepthId, contract, reqMktDepthLevel, m_taglist);
    //m_pClient->cancelMktDepth(reqMktDepthId);

    int reqRealTimeBarsId = 30;
    int realTimebarSize = 5;
    const IBString whatToShow = "TRADES";
    const int useRTH = 0;
    const TagValueListSPtr realTimeBarsOptions = m_taglist;
    //m_pClient->reqRealTimeBars(reqRealTimeBarsId, contract, realTimebarSize, whatToShow, useRTH, realTimeBarsOptions);
    //m_pClient->cancelRealTimeBars(reqRealTimeBarsId);

    const int reqHistoricalDataId = 40;
    const IBString endDateTime = "20150315 00:00:00";
    const IBString barSize = "5 mins";
    const IBString duration = "1 D";
    const int formatDate = 1;
    const TagValueListSPtr chartOption = m_taglist;
    //m_pClient->reqHistoricalData(reqHistoricalDataId, contract, endDateTime, duration, barSize, whatToShow, useRTH, formatDate, chartOption);
    //m_pClient->cancelHistoricalData(reqHistoricalDataId);
}

void Client::tickRecord(){
    //m_pClient->reqMktDepth(reqaud, aud, 2, m_taglist);
    //m_pClient->reqMktData(reqaud, aud, "225", false, m_taglist);
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
    if(id==-1 && (errorCode==2104 || errorCode==2106)) return;
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
    //printf("tickPrice: %d|%d|%f|%d\n", tickerId, field, price, canAutoExecute);
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
// side 0 = ask, 1 = bid
void Client::updateMktDepth(TickerId id, int position, int operation, int side,
									  double price, int size) {
    //timeval curTime;
    //gettimeofday(&curTime, NULL);
    //unsigned long micro = curTime.tv_usec;

    //char buffer [30];
    //localtime is not thread safe
    //strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", localtime((const time_t*)&curTime.tv_sec));

    //char currentTime2[30] = "";
    //sprintf(currentTime2, "%s.%06Lu", buffer, micro); 
    printf("updateMktDepth: position %d, operation %d, side %d, price %.5f, %d size\n", position, operation, side, price, size);

    /*
    if(!side){
        ask = price;
    }

    if(side){
        bid = price;
    }

    if(ask>epsilon && bid>epsilon){
        mid=(ask+bid)/2.;
    }
    if(mid>epsilon){
        //fprintf(audf, "%s,%.6f\n", currentTime2 ,mid);
        //printf("%s:\t%.6f\n", currentTime2 ,mid);
        printf("%.6f\n", mid);

        if(cnt==0){
            xopen.push_back(mid);
            h=mid; l=mid;
            ++cnt;
        }

        else if(cnt<NUM_OF_TICKS-1) {
            if(h<mid) h=mid;
            if(l>mid) l=mid;
            ++cnt;
        }
        
        //else (cnt==NUM_OF_TICKS){
        else {
            high.push_back(h);
            low.push_back(l);
            last.push_back(mid);
            cnt=0;
            printf("OHLC:%.6f|%.6f|%.6f|%.6f|%d\n", xopen.back(), high.back(), low.back(), last.back(), last.size());
            unsigned sz = last.size();

            if(sz >= fast){
                if(sz == fast){
                    emafast.push_back(accumulate(last.begin(), last.end(), 0.0)/fast);
                    //printf("EMA fast: %.6f\t%d\n", emafast.back(), emafast.size());
                }
                else{
                    emafast.push_back(emafast.back() + (2.0/(fast+1.))*(last.back()-emafast.back()));
                    //printf("EMA fast: %.6f\t%d\n", emafast.back(), emafast.size());
                }
            }

            if(sz >= slow){
                if(sz == slow){
                    emaslow.push_back(accumulate(last.begin(), last.end(), 0.0)/slow);
                    //printf("EMA slow: %.6f\t%d\n", emaslow.back(), emaslow.size());
                }
                else{
                    emaslow.push_back(emaslow.back() + (2.0/(slow+1.))*(last.back()-emaslow.back()));
                    //printf("EMA slow: %.6f\t%d\n", emaslow.back(), emaslow.size());
                }
            }

            // this part is the strategy
            if(emaslow.size()>2 && emafast.size()>2){
                if(emafast[emafast.size()-2] <= emaslow[emaslow.size()-2] && emafast[emafast.size()-1] > emaslow[emaslow.size()-1]){
                    GOLONG = true;
                    GOSHORT = false;
                }
                if(emafast[emafast.size()-2] >= emaslow[emaslow.size()-2] && emafast[emafast.size()-1] < emaslow[emaslow.size()-1]){
                    GOSHORT = true;
                    GOLONG = false;
                }
            }
        }
    }

    if(!HAS_POSITION){
        double s_sp, l_sp;
        stringstream oca;
        OrderId parentId;

        if(xopen.size()>=NUM_OF_BARS){
            s_order_parent.totalQuantity = totalQuantity;
            s_order_parent.orderRef = "from client " + clientIdStr;
            s_order_parent.action = "SELL";
            s_order_stoploss.action = "BUY";
            s_order_profittaking.action = "BUY";
            s_order_trailing.action = "BUY";

            s_order_parent.orderType = "LMT";
            s_order_parent.lmtPrice = halfpip(bid-2*tick);
            //cout << "short order limit price : " << s_order_parent.lmtPrice << endl;
            s_order_stoploss.auxPrice = s_order_parent.lmtPrice + sl;
            s_order_profittaking.lmtPrice = s_order_parent.lmtPrice - tp;
            s_order_trailing.auxPrice = trail;

            s_order_stoploss.orderType = "STP";
            s_order_profittaking.orderType = "LMT";
            s_order_trailing.orderType = "TRAIL";
            s_order_stoploss.totalQuantity = s_order_parent.totalQuantity;
            s_order_profittaking.totalQuantity =  s_order_parent.totalQuantity;
            s_order_trailing.totalQuantity = s_order_parent.totalQuantity;

            s_order_parent.transmit = USETRAIL?true:false;
            s_order_parent.transmit = false;
            s_order_stoploss.transmit = false;
            //s_order_profittaking.transmit = false;

            l_order_parent.totalQuantity = totalQuantity;
            l_order_parent.orderRef = "from client " + clientIdStr;
            l_order_parent.action = "BUY";
            l_order_stoploss.action = "SELL";
            l_order_profittaking.action = "SELL";
            l_order_trailing.action = "SELL";
            
            l_order_parent.orderType = "LMT";
            l_order_parent.lmtPrice = halfpip(ask+2*tick);
            //cout << "long order limit price : " << l_order_parent.lmtPrice << endl;
            l_order_stoploss.auxPrice = l_order_parent.lmtPrice - sl;
            l_order_profittaking.lmtPrice = l_order_parent.lmtPrice + tp;
            l_order_trailing.auxPrice = trail;

            l_order_stoploss.orderType = "STP";
            l_order_profittaking.orderType = "LMT";
            l_order_trailing.orderType = "TRAIL";
            l_order_stoploss.totalQuantity = l_order_parent.totalQuantity;
            l_order_profittaking.totalQuantity =  l_order_parent.totalQuantity;
            s_order_trailing.totalQuantity = l_order_parent.totalQuantity;
            
            l_order_parent.transmit = USETRAIL?true:false;
            l_order_parent.transmit = false;
            l_order_stoploss.transmit = false;
            //l_order_profittaking.transmit = false;
            parentId = m_orderId;

            if(GOLONG){
                l_order_stoploss.parentId = parentId;
                l_order_profittaking.parentId = parentId;
                l_order_trailing.parentId = parentId;
                cout << "Client " << m_clientId << ": Placing a LMT long order, orderId " << parentId << ".\n";
                m_pClient->placeOrder(parentId, m_contract, l_order_parent);	
                // 2014-02-23 unkonw bug
                if(!USETRAIL){
                    m_pClient->placeOrder(parentId + 1, m_contract, l_order_stoploss);
                    m_pClient->placeOrder(parentId + 2, m_contract, l_order_profittaking);
                }
                else {
                    m_pClient->placeOrder(parentId + 1, m_contract, l_order_trailing);
                }
                GOLONG = false;
                HAS_POSITION = 1;
                m_modified = false;
                m_orderId = parentId + (USETRAIL?2:3);
                //m_orderId = parentId + 3;
                cout << "Client " << m_clientId << ": New orderId is " << m_orderId << ".\n";
            }
            
            if(GOSHORT){
                s_order_stoploss.parentId = parentId;
                s_order_profittaking.parentId = parentId;
                s_order_trailing.parentId = parentId;
                
                cout << "Client " << m_clientId << ": Placing a LMT short order, orderId " << parentId << ".\n";
                m_pClient->placeOrder(parentId, m_contract, s_order_parent);	
                if(!USETRAIL){
                    m_pClient->placeOrder(parentId + 1, m_contract, s_order_stoploss);
                    m_pClient->placeOrder(parentId + 2, m_contract, s_order_profittaking);
                }
                else{
                    m_pClient->placeOrder(parentId + 1, m_contract, s_order_trailing);
                }
                GOSHORT = false;
                HAS_POSITION = -1;
                m_modified = false;
                m_orderId = parentId + (USETRAIL?2:3);
                //m_orderId = parentId + 3;
                cout << "Client " << m_clientId << ": New orderId is " << m_orderId << ".\n";
            }
        } 
    }
        */
}

void Client::updateMktDepthL2(TickerId id, int position, IBString marketMaker, int operation,
										int side, double price, int size) {
    printf("updateMktDepth2: position %d, marketMaker %s, operation %d, side %d, price %.5f, %d size\n", position, marketMaker.c_str(), operation, side, price, size);
}

void Client::updateNewsBulletin(int msgId, int msgType, const IBString& newsMessage, const IBString& originExch) {
    printf("updateNewsBulletin: msgId=%d, msgType=%d, newsMessage=%s, originExch=%s\n", msgId, msgType, newsMessage.c_str(), originExch.c_str());
}

void Client::managedAccounts( const IBString& accountsList) {}
void Client::receiveFA(faDataType pFaDataType, const IBString& cxml) {}
void Client::historicalData(TickerId reqId, const IBString& date, double open, double high,
									  double low, double close, int volume, int barCount, double WAP, int hasGaps) {
    printf("historicalData: reqId=%d, %s|%f|%f|%f|%f|%d|%d|%f|%d\n", reqId, date.c_str(), open, high, low, close, volume, barCount, WAP, hasGaps);
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
