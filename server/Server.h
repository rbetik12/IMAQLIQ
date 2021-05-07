#include <vector>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <thread>

class Server {
public:
    static Server* GetInstance();
    static Server* Create(short port);

    ~Server();

    void Run();
    void Shutdown();
private:
    static void HandleSIGTERM(int signal);
    static void HandleSIGHUP(int signal);

    Server(short port);

    void AcceptThread();
private:
    static Server* instance;

    int serverFd;
    bool isRunning;
    std::vector<int> clientSockets;
    struct sockaddr_in address;
    std::unique_ptr<std::thread> acceptThread;
};