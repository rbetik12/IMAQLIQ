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
    Server(short port);
    ~Server();

    void Run();

private:
    void AcceptThread();
private:
    int serverFd;
    bool isRunning;
    std::vector<int> clientSockets;
    struct sockaddr_in address;
    std::unique_ptr<std::thread> acceptThread;
};