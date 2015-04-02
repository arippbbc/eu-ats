#ifdef _WIN32
#include <Windows.h>
#include <unistd.h>
#define sleep( seconds) Sleep( seconds * 1000);
#else
#include <unistd.h>
#endif

#include "Client.h"
#include "Contract.h"
//#include <cstdio>
#include <stdio.h>
//#include <csignal>
#include <signal.h>
//#include <cstdlib>
#include <stdlib.h>
//#include <pthread.h>
#include <iostream>
//#include "Timer.h"
//#include <ctime>
#include <time.h>
//#include <thread>         // std::this_thread::sleep_for
//#include <chrono>         // std::chrono::seconds
#include <memory>

//#define NUM_OF_THREADS 2
using namespace std;

const unsigned MAX_ATTEMPTS = 5000;
const unsigned SLEEP_TIME = 1;

//bool TICKBAR = false;
bool TICKBAR = true;

void signal_callback_handler(int signum){
    printf("Caught signal %d\n",signum);
    exit(signum);
}

int main(int argc, char** argv)
{
    signal(SIGINT, signal_callback_handler);
    //timestamp();
   
    //const char* host = "72.69.43.58";
    const char* host = "127.0.0.1";
    unsigned int port = 7496;
    bool tflag = false;
    int c;
    int clientId = 0;
    double trail;
    int NUM_OF_TICKS = 0;

    while ((c = getopt(argc, argv, "c:n:t:")) != -1){
        //cout << "list of parameters: " << c << endl;
        switch (c)
        {
            case 'c':
                clientId = atoi(optarg);
                break;
            case 't':
                tflag = true;
                trail = atoi(optarg)*0.0001;
                break;
            case 'n':
                NUM_OF_TICKS = atoi(optarg);
                //cout << "optarg " << atoi(optarg) << endl;
                break;
            case '?':
                if(isprint(optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                return 1;
            default:
                abort ();
        }
    }
    
    unsigned int attempt = 0;
    printf("Start of AUD.USD automatic trading Test %d on client %d\n", attempt, clientId);
    for (;;) {
        ++attempt;
        printf("Attempt %u of %u\n", attempt, MAX_ATTEMPTS);

        shared_ptr<Client> client(new Client(clientId)); 
        client->connect(host, port);

        sleep(1);
        // 2014-02-23 wait until cash farm data feed is ready
        //while(!client.iscfReady()){}
        //while(!client->isConnected()){}
        //this_thread::sleep_for(chrono::seconds(1));
        client->test();

        while(client->isConnected()) {
            client->processMessages();
        }

        // 2014-02-09 how to use checkMessage() every second? 
        // in another thread?
        if(attempt >= MAX_ATTEMPTS) {
            break;
        }

        printf("Sleeping %u seconds before next attempt\n", SLEEP_TIME);
        // safe to sleep in thread?
        sleep(SLEEP_TIME);
    }
    
    return EXIT_SUCCESS;
}
