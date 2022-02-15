#include "PassiveSocket.h"       // Include header for active socket object definition
#include "readConfigFile.h"
#include <csignal>

#include <pthread.h>

#ifdef linux
    #include <sys/syscall.h>
    //getpid()
    pid_t gettid() { return syscall(SYS_gettid); }
#else
    #include <windows.h>
    #define getpid() GetCurrentProcessId()
    #define gettid() GetCurrentThreadId()

    // usually defined with #include <unistd.h>
    static void sleep( unsigned int microsecond ) { Sleep( microsecond * 1000 ); }
    static void usleep( unsigned int microsecond ) { Sleep( microsecond ); }
#endif


#define MAX_CLIENT 1000
#define MAX_PACKET 4096

char ip[32] = {0};
int port = -1;
char exitKey[32] = {0}; // password to quit

// server's socket
CPassiveSocket serverSocket;

// server's client_connect socket
CActiveSocket *pClientSocket[MAX_CLIENT] = { NULL };
pthread_t client_thread_Ids[MAX_CLIENT];
// bool runFlag = true;
char rcvBuff[MAX_CLIENT][256] = { 0 };


void close_app(int s)
{
    for (long i = 0; i < MAX_CLIENT; i++)
    {
        if (NULL != pClientSocket[i])
        {
            printf("close_app stop pClientSocket[%ld]\r\n", i); fflush(stdout);
            // pClientSocket[i]->Close();
            // delete pClientSocket[i];
            // pClientSocket[i] = NULL;
        }
    }

    printf("close serverSocket\r\n"); fflush(stdout);
    serverSocket.Close();

    exit(0);
}

void * client_thread(void * m)
{
#ifdef linux
    long i = (long) m; // clientNo
#else
    long long i = (long long) m; // clientNo
#endif
    pthread_detach(pthread_self());
    printf("create client[%ld] thread : %d \r\n", i, gettid()); fflush(stdout);

    while(true) // while(runFlag)
    {
        //----------------------------------------------------------------------
        // Receive request from client
        //----------------------------------------------------------------------
        if (NULL != pClientSocket[i] && pClientSocket[i]->Receive(MAX_PACKET))
        {
            // CSocketError : 1 ~ 17
            if (CActiveSocket::SocketInvalidSocket <= pClientSocket[i]->GetSocketError())
            {
                 printf("client[%ld]_thread GetSocketError : %d \r\n", i, pClientSocket[i]->GetSocketError()); fflush(stdout);
                 break;
            }

            memset(rcvBuff[i], 0, sizeof(rcvBuff[i]));
            memcpy(rcvBuff[i], pClientSocket[i]->GetData(), pClientSocket[i]->GetBytesReceived());
            // password to quit
            if(!memcmp(exitKey, rcvBuff[i], sizeof(exitKey)))
            {
                printf("@@@@ user require client thread quit @@@@ : %ld\r\n", i);
#ifdef linux
                // only linux, server receive exitKey to release resource.
                // when windows, server's bug? will exit all server's client_thread.
                // runFlag = false; // not use one runFlag, for many threads
                break;
#endif
            }
            else
            {
                printf("pClientSocket[%ld] Receive : %s\r\n", i, rcvBuff[i]); fflush(stdout);
                //------------------------------------------------------------------
                // Send response to client
                //------------------------------------------------------------------
                pClientSocket[i]->Send(pClientSocket[i]->GetData(), pClientSocket[i]->GetBytesReceived());
            }
        }
    }

    memset(rcvBuff[i], 0, sizeof(rcvBuff[i]));
    pClientSocket[i]->Close();
    delete pClientSocket[i];
    pClientSocket[i] = NULL;
    printf("exit client[%ld]_thread : %d \r\n", i, gettid()); fflush(stdout);
    pthread_exit(NULL);

    return 0;
}

int main(int argc, char **argv)
{
    char pathBuf[MAX_PATH] = {0};
    memset(pathBuf, 0, sizeof(pathBuf));
    getCurrentPath(CONF_FILE_PATH, pathBuf);
    printf("path:%s\r\n", pathBuf); fflush(stdout);
    if (0 != readConfigFile_String(pathBuf, "INICnet", "ip", ip))
    {
        printf("no ip key\r\n");
    }
    printf("ip:%s\n", ip); fflush(stdout);
    port = readConfigFile_Int(pathBuf, "INICnet", "port");
    if (-1 == port)
    {
        printf("no port key\r\n"); fflush(stdout);
    }
    printf("port:%d\n", port); fflush(stdout);
    if (0 != readConfigFile_String(pathBuf, "INICnet", "exitKey", exitKey))
    {
        printf("no exitKey key\r\n"); fflush(stdout);
    }
    // printf("exitKey:%s\n", exitKey); fflush(stdout);


    //--------------------------------------------------------------------------
    // Initialize our serverSocket object 
    //--------------------------------------------------------------------------
    serverSocket.Initialize();

    serverSocket.Listen("", port);  // serverSocket.Listen("127.0.0.1", 6789);

    std::signal(SIGINT, close_app);

#ifdef linux
    long i = 0;
#else
    long long i = 0;
#endif
    while (true)
    {
        if ((pClientSocket[i] = serverSocket.Accept()) != NULL)
        {
            printf("pClientSocket[%ld] : %p \r\n", i, pClientSocket[i]); fflush(stdout);
            if(pthread_create(&client_thread_Ids[i], NULL, client_thread, (void *)i) == 0)
            {
                ++i;
                if (i >= MAX_CLIENT)
                {
                    printf("up to  MAX_CLIENT \r\n"); fflush(stdout);
                }
            }
            usleep(500);
        }
    }

    close_app(0);
    // serverSocket.Close();

    return 0;
}
