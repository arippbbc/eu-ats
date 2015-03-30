#include "PosixTestClient.h"
#include "EPosixClientSocket.h"
#include "EPosixClientSocketPlatform.h"

#include "Contract.h"
#include "Order.h"
#include "OrderState.h"
#include <vector>
#include <algorithm>
//#include <functional>
#include <numeric>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <ctime>
#include <sys/time.h>

using namespace std;

FILE *log_connect;
//FILE *raw;
FILE *audf, *eurf, *gbpf, *nzdf, *jpyf, *cadf, *allf;
FILE *audf2, *eurf2, *jpyf2;

// reqHistorical only accept "YYYYMMDD HH:MM:SS"
char curTime[20];
char curDate[20];

const int PING_DEADLINE = 10; // seconds
const int SLEEP_BETWEEN_PINGS = 30; // seconds

const double epsilon = 0.00001;

string clientIdStr = "";
        
Contract aud, eur, gbp, jpy, cad, nzd;
Contract es;
int reqaud=1, reqeur=2, reqgbp=3, reqnzd=4, reqjpy=5, reqcad=6;
int reqaud2=11, reqeur2=12, reqgbp2=13, reqnzd2=14, reqjpy2=15, reqcad2=16;
//int reqes = 10;
//

bool isEmpty(FILE *file)
{
    long savedOffset = ftell(file);
    fseek(file, 0, SEEK_END);

    if (ftell(file) == 0)
    {
        return true;
    }

    fseek(file, savedOffset, SEEK_SET);
    return false;
}

/* 2014-02-23 global contract definition */
void initializeContract(){
    aud.symbol = "AUD";
    aud.secType = "CASH";
    aud.exchange = "IDEALPRO";
    aud.currency = "USD";

    eur.symbol = "EUR";
    eur.secType = "CASH";
    eur.exchange = "IDEALPRO";
    eur.currency = "USD";

    gbp.symbol = "GBP";
    gbp.secType = "CASH";
    gbp.exchange = "IDEALPRO";
    gbp.currency = "USD";

    nzd.symbol = "NZD";
    nzd.secType = "CASH";
    nzd.exchange = "IDEALPRO";
    nzd.currency = "USD";

    jpy.symbol = "USD";
    jpy.secType = "CASH";
    jpy.exchange = "IDEALPRO";
    jpy.currency = "JPY";

    cad.symbol = "USD";
    cad.secType = "CASH";
    cad.exchange = "IDEALPRO";
    cad.currency = "CAD";

    es.symbol = "ES";
    es.secType = "FUT";
    es.exchange = "GLOBEX";
    es.currency = "USD";
    es.expiry = "20140321";
}

///////////////////////////////////////////////////////////
// member funcs
PosixTestClient::PosixTestClient(bool tflag)
    //PosixTestClient::PosixTestClient()
	: m_pClient(new EPosixClientSocket(this))
      //, m_clientId(clientId)
	, m_state(ST_CONNECT)
    //better to set up initial value to be false
    , m_noposition(false)
    , m_modified(false)
	, m_sleepDeadline(0)
    , m_contract(Contract())
	, m_orderId(0)
    , HAS_POSITION(0)
    , USETRAIL(tflag)
{
    //m_contract.symbol = "AUD";
    //m_contract.secType = "CASH";
    //m_contract.exchange = "IDEALPRO";
    //m_contract.currency = "USD";
    //ostringstream convert;
    //convert << m_clientId;
    //clientIdStr = convert.str();
}

PosixTestClient::~PosixTestClient()
{
}

bool PosixTestClient::connect(const char *host, unsigned int port, int clientId)
{
    m_clientId = clientId;
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
	return bRes;
}

void PosixTestClient::disconnect() const
{
	m_pClient->eDisconnect();

	printf ( "Disconnected\n");

    /*
    if(audf){
        fclose(audf);
    }

    if(eurf){
        fclose(eurf);
    }

    if(gbpf){
        fclose(gbpf);
    }

    if(nzdf){
        fclose(nzdf);
    }

    if(jpyf){
        fclose(jpyf);
    }

    if(cadf){
        fclose(cadf);
    }
    */

    if(allf){
        fclose(allf);
    }

    if(audf2){
        fclose(audf2);
    }

    if(eurf2){
        fclose(eurf2);
    }

    if(jpyf2){
        fclose(jpyf2);
    }
}

bool PosixTestClient::isConnected() const
{
	return m_pClient->isConnected();
}

void PosixTestClient::processMessages()
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
			log_connect = NULL;
            /*
            audf = NULL;
            eurf = NULL;
            gbpf = NULL;
            nzdf = NULL;
            jpyf = NULL;
            cadf = NULL;
            */
            allf = NULL;

            audf2 = NULL;
            eurf2 = NULL;
            jpyf2 = NULL;
            disconnect();
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

void PosixTestClient::reqCurrentTime()
{
    //printf("Requesting Current Time\n");

	// set ping deadline to "now + n seconds"
    //m_sleepDeadline = time( NULL) + PING_DEADLINE;

    //m_state = ST_PING_ACK;

	m_pClient->reqCurrentTime();
}

void PosixTestClient::placeOrder()
{
    return;
}

void PosixTestClient::cancelOrder()
{
	printf("Client %d: Cancelling Order %ld\n", m_clientId, m_orderId);

	m_state = ST_CANCELORDER_ACK;

	m_pClient->cancelOrder(m_orderId);
    printf("Client %d: cancel order function called!", m_clientId);
}

// possible duplicate messages
void PosixTestClient::orderStatus(OrderId orderId, const IBString &status, int filled,
	   int remaining, double avgFillPrice, int permId, int parentId,
	   double lastFillPrice, int clientId, const IBString& whyHeld)

{
}

void PosixTestClient::barRecord(){
    m_pClient->reqCurrentTime();
    if(m_contract.secType == "CASH"){
        m_pClient->reqHistoricalData(5, m_contract, curTime, "2 D", "5 mins", "MIDPOINT", 0, 1);
    }
    
    else {
        m_pClient->reqHistoricalData(5, m_contract, curTime, "2 D", "5 mins", "TRADES", 0, 1);
    }
}

void PosixTestClient::tickRecord(){
    m_pClient->reqCurrentTime();
    m_pClient->reqMktData(reqaud, aud, "", false);
    m_pClient->reqMktData(reqeur, eur, "", false);
    m_pClient->reqMktData(reqgbp, gbp, "", false);
    m_pClient->reqMktData(reqnzd, nzd, "", false);
    m_pClient->reqMktData(reqjpy, jpy, "", false);
    m_pClient->reqMktData(reqcad, cad, "", false);
    
    m_pClient->reqMktDepth(reqaud2, aud, 20);
    m_pClient->reqMktDepth(reqeur2, eur, 20);
    //m_pClient->reqMktDepth(reqgbp, gbp, 2);
    //m_pClient->reqMktDepth(reqnzd, nzd, 2);
    m_pClient->reqMktDepth(reqjpy2, jpy, 20);
    //m_pClient->reqMktDepth(reqcad, cad, 2);
}
 
// this is called automatically on connection, should mannually maintain valid orderId
void PosixTestClient::nextValidId(OrderId orderId)
{
	m_orderId = orderId;
}

// 2014-02-10 can't get this server time right, seems currupted, will use local machine time for now
void PosixTestClient::currentTime(long xtime)
{
    log_connect = fopen("log_connect.txt", "a");
    //raw = fopen("data/raw.csv", "a");
    /*
    audf = fopen("data/aud.csv", "a");
    eurf = fopen("data/eur.csv", "a");
    gbpf = fopen("data/gbp.csv", "a");
    nzdf = fopen("data/nzd.csv", "a");
    jpyf = fopen("data/jpy.csv", "a");
    cadf = fopen("data/cad.csv", "a");
    */
    allf = fopen("data/all.csv", "a");
    if(isEmpty(allf)){
        fprintf(allf, "Time,Id,Field,Price\n");
    }

    audf2 = fopen("data/aud2.csv", "a");
    if(isEmpty(audf2)){
        fprintf(audf2, "Time,Position,Operation,Side,Price,Size\n");
    }

    eurf2 = fopen("data/eur2.csv", "a");
    if(isEmpty(eurf2)){
        fprintf(eurf2, "Time,Position,Operation,Side,Price,Size\n");
    }

    jpyf2 = fopen("data/jpy2.csv", "a");
    if(isEmpty(jpyf2)){
        fprintf(jpyf2, "Time,Position,Operation,Side,Price,Size\n");
    }

}

void PosixTestClient::error(const int id, const int errorCode, const IBString errorString)
{
    printf("Error id=%d, errorCode=%d, msg=%s\n", id, errorCode, errorString.c_str());
    if( id == -1 && errorCode == 1100){ //if "Connectivity between IB and TWS has been lost"
        //potential double free???
        //disconnect();
        time_t t;
        t = time(NULL);
        strftime(curTime, sizeof curTime, "%Y%m%d %H:%M:%S", localtime(&t));
        fprintf(log_connect, "Disconnected at %s\n", curTime);
    }

    if( id == -1 && errorCode == 1102){ //if "Connectivity between IB and TWS has been lost"
        time_t t;
        t = time(NULL);
        strftime(curTime, sizeof curTime, "%Y%m%d %H:%M:%S", localtime(&t));
        fprintf(log_connect, "Reconnected at %s\n", curTime);
    }
}

void PosixTestClient::tickPrice(TickerId tickerId, TickType field, double price, int canAutoExecute) {
    timeval curTime;
    gettimeofday(&curTime, NULL);
    unsigned long micro = curTime.tv_usec;

    char buffer [30];
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", localtime(&curTime.tv_sec));

    char currentTime2[30] = "";
    sprintf(currentTime2, "%s.%06Lu", buffer, micro); 

    if(field==1 || field==2){
        if(tickerId == 5){
            printf("%s,%d,%d,%6.3f\n", currentTime2, tickerId, field, price);
            fprintf(allf, "%s,%d,%d,%6.3f\n", currentTime2, tickerId, field, price);
        }
        else{
            printf("%s,%d,%d,%7.5f\n", currentTime2, tickerId, field, price);
            fprintf(allf, "%s,%d,%d,%7.5f\n", currentTime2, tickerId, field, price);
        }
    }
}



void PosixTestClient::tickSize( TickerId tickerId, TickType field, int size) {
}

void PosixTestClient::tickOptionComputation( TickerId tickerId, TickType tickType, double impliedVol, double delta,
											 double optPrice, double pvDividend,
											 double gamma, double vega, double theta, double undPrice) {}
void PosixTestClient::tickGeneric(TickerId tickerId, TickType tickType, double value) {
}
void PosixTestClient::tickString(TickerId tickerId, TickType tickType, const IBString& value) {
}
void PosixTestClient::tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const IBString& formattedBasisPoints,
							   double totalDividends, int holdDays, const IBString& futureExpiry, double dividendImpact, double dividendsToExpiry) {}
void PosixTestClient::openOrder(OrderId orderId, const Contract &contract, const Order &order, const OrderState& ostate) {
}

void PosixTestClient::openOrderEnd() {}
void PosixTestClient::winError( const IBString &str, int lastError) {}
void PosixTestClient::connectionClosed() {}
void PosixTestClient::updateAccountValue(const IBString& key, const IBString& val,
										  const IBString& currency, const IBString& accountName) {}
void PosixTestClient::updatePortfolio(const Contract& contract, int position,
		double marketPrice, double marketValue, double averageCost,
		double unrealizedPNL, double realizedPNL, const IBString& accountName){
}

void PosixTestClient::updateAccountTime(const IBString& timeStamp) {}
void PosixTestClient::accountDownloadEnd(const IBString& accountName) {}

void PosixTestClient::contractDetails( int reqId, const ContractDetails& contractDetails) {
}

void PosixTestClient::bondContractDetails( int reqId, const ContractDetails& contractDetails) {}
void PosixTestClient::contractDetailsEnd( int reqId) {}
void PosixTestClient::execDetails( int reqId, const Contract& contract, const Execution& execution) {}
void PosixTestClient::execDetailsEnd( int reqId) {}

void PosixTestClient::updateMktDepth(TickerId id, int position, int operation, int side,
									  double price, int size) {
    timeval curTime;
    gettimeofday(&curTime, NULL);
    unsigned long micro = curTime.tv_usec;

    char buffer [30];
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", localtime(&curTime.tv_sec));

    char currentTime2[30] = "";
    sprintf(currentTime2, "%s.%06Lu", buffer, micro); 

    switch(id){
        case 11:
            //printf("%s,%d,%d,%d,%d,%.5f,%d\n", currentTime2, position, operation, side, price, size);
                fprintf(audf2, "%s,%d,%d,%d,%d,%.5f,%d\n", currentTime2, position, operation, side, price, size);
                break;
        case 12: 
                //printf("%s,%d,%d,%d,%d,%.5f,%d\n", currentTime2, position, operation, side, price, size);
                fprintf(eurf2, "%s,%d,%d,%d,%d,%.5f,%d\n", currentTime2, position, operation, side, price, size);
                break;
        case 15:
                //printf("%s,%d,%d,%d,%d,%.3f,%d\n", currentTime2, position, operation, side, price, size);
                fprintf(jpyf2, "%s,%d,%d,%d,%d,%.3f,%d\n", currentTime2, position, operation, side, price, size);
                break;
        default: 
                break;
    }
}


void PosixTestClient::updateMktDepthL2(TickerId id, int position, IBString marketMaker, int operation,
										int side, double price, int size) {
    //printf("updateMktDepth2: position %d, marketMaker %s, operation %d, side %d, price %.5f, %d size\n", position, marketMaker.c_str(), operation, side, price, size);
}
void PosixTestClient::updateNewsBulletin(int msgId, int msgType, const IBString& newsMessage, const IBString& originExch) {}
void PosixTestClient::managedAccounts( const IBString& accountsList) {}
void PosixTestClient::receiveFA(faDataType pFaDataType, const IBString& cxml) {}
void PosixTestClient::historicalData(TickerId reqId, const IBString& date, double open, double high,
									  double low, double close, int volume, int barCount, double WAP, int hasGaps) {
}
void PosixTestClient::scannerParameters(const IBString &xml) {}
void PosixTestClient::scannerData(int reqId, int rank, const ContractDetails &contractDetails,
	   const IBString &distance, const IBString &benchmark, const IBString &projection,
	   const IBString &legsStr) {}
void PosixTestClient::scannerDataEnd(int reqId) {}
void PosixTestClient::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
								   long volume, double wap, int count) {}
void PosixTestClient::fundamentalData(TickerId reqId, const IBString& data) {}
void PosixTestClient::deltaNeutralValidation(int reqId, const UnderComp& underComp) {}
void PosixTestClient::tickSnapshotEnd(int reqId) {}
void PosixTestClient::marketDataType(TickerId reqId, int marketDataType) {}

void PosixTestClient::initialCheck(){
    m_pClient->reqAllOpenOrders();
}

void PosixTestClient::commissionReport( const CommissionReport& commissionReport) {}
void PosixTestClient::position( const IBString& account, const Contract& contract, int position) {}
void PosixTestClient::positionEnd() {}
void PosixTestClient::accountSummary( int reqId, const IBString& account, const IBString& tag, const IBString& value, const IBString& curency) {}
void PosixTestClient::accountSummaryEnd( int reqId) {}
