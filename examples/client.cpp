#include "PassiveSocket.h"
#include "readConfigFile.h"
#include <csignal>

#include <pthread.h>

#include <iostream>
using namespace std;

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


#define MAX_PACKET  4096

char ip[32] = {0};
int port = -1;
char exitKey[32] = {0}; // password to quit

CActiveSocket clientSocket;
bool runFlag = true;
pthread_t client_thread_Id;
void *rval = NULL;

char rcvBuff[256] = { 0 };


void close_app(int s)
{
    runFlag = false;
    pthread_join(client_thread_Id, &rval);
    std::cout << "close_app client_thread_Id : " << client_thread_Id << ", rval= " << rval << std::endl;

    // when windows, server's bug? will exit all server's client_thread.
    clientSocket.Send((uint8 *)(exitKey), strlen(exitKey));
    usleep(100);

    clientSocket.Close();
    exit(0);
}

void * receive_thread(void * m)
{
    CActiveSocket *pClientSocket = (CActiveSocket *) m; // clientNo
    // pthread_detach(pthread_self()); // comment to use pthread_join
    std::cout << "create client thread : " << gettid() << std::endl;

    while(runFlag)
    {
        //----------------------------------------------------------------------
        // Receive from server
        //----------------------------------------------------------------------
        if (NULL != pClientSocket && pClientSocket->Receive(MAX_PACKET))
        {
            pClientSocket->Select();
        
            // CSocketError : 1 ~ 17
            if (CActiveSocket::SocketInvalidSocket <= pClientSocket->GetSocketError())
            {
                 std::cout << "client_thread GetSocketError : " << pClientSocket->GetSocketError() << std::endl;
                 break;
            }
            if (0 < pClientSocket->GetBytesReceived())
            {
                memset(rcvBuff, 0, sizeof(rcvBuff));
                memcpy(rcvBuff, pClientSocket->GetData(), pClientSocket->GetBytesReceived());
                std::cout << "receive_thread Receive : " << rcvBuff << std::endl;
            }
        }
    }

    memset(rcvBuff, 0, sizeof(rcvBuff));
    // pClientSocket->Close();
    // delete pClientSocket; // just pointer, no new no delete
    // pClientSocket = NULL;
    std::cout << "exit client_thread : " << gettid() << std::endl;
    pthread_exit(NULL);

    return 0;
}

int main(int argc, char **argv)
{
    std::signal(SIGINT, close_app);

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
    printf("port:%d\n", port);
    if (0 != readConfigFile_String(pathBuf, "INICnet", "exitKey", exitKey))
    {
        printf("no exitKey key\r\n"); fflush(stdout);
    }
    // printf("exitKey:%s\n", exitKey); fflush(stdout);

    char result[1024];
    char sendBuffer[256];

    clientSocket.Initialize();
    clientSocket.SetNonblocking();

    if (clientSocket.Open(ip, port)) //("192.168.162.101", 6789)
    {
        std::cout << "client success to open connection" << std::endl;
    }
    else {
        // CSocketError : 1 ~ 17
        if (CActiveSocket::SocketInvalidSocket <= clientSocket.GetSocketError())
        {
            std::cout << "client GetSocketError : " << clientSocket.GetSocketError() << std::endl;
        }
        std::cout << "client failed to open connection, and exit" << std::endl;
        return -1;
    }

    // receive thread
    if(pthread_create(&client_thread_Id, NULL, receive_thread, (void *)&clientSocket) == 0)
    {
        // ...
    }
    usleep(500);

    // send
    while (true)
    {
        //----------------------------------------------------------------------
        // Send to server.
        //----------------------------------------------------------------------
        std::cout << "send : " << std::endl;
        scanf("%s", sendBuffer);

        if(!memcmp("client$quit", sendBuffer, 11))
        {
            std::cout << "force client quit, and wait receive_thread to exit" << std::endl;

            runFlag = false;
            pthread_join(client_thread_Id, &rval);
            std::cout << "client_thread_Id : " << client_thread_Id << ", rval= " << rval << std::endl;
            clientSocket.Send((uint8 *)(exitKey), strlen(exitKey));
            break;
        }

        if (clientSocket.Send((uint8 *)sendBuffer, strlen(sendBuffer)))
        {
            // ...
        }
    }

    close_app(0);
    // clientSocket.Close();

    return 0;
}
