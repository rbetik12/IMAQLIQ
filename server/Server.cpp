#include <iostream>
#include <memory>
#include <signal.h>
#include <fcntl.h>
#include "Server.h"


int main() {
    Server server(8080);
    server.Run();
    return 0;
}

Server::Server(short port): isRunning(false), clientId(0) {
    int opt;

    if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Can't create server socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt))) {
        perror("Can't set socket op");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverFd, (struct sockaddr *) &address,
             sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(serverFd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
}

Server::~Server() {}

void Server::ServerTick() {}

void Server::AcceptThread() {
    int clientSocket;
    int addrLen = sizeof(address);
    struct sockaddr sockaddr;

    while (isRunning) {
        if ((clientSocket = accept(serverFd, &sockaddr, (socklen_t*) &addrLen)) < 0) {
            perror("accept");
        }
        else {
            clientSockets.push_back(clientSocket);
        }
    }
}

void Server::Run() {
    isRunning = true;
    acceptThread = std::make_unique<std::thread>(&Server::AcceptThread, this);
    const int bufferSize = 4096;
    char buff[bufferSize];
    fd_set readFds;
    int ret, readAmount;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    int fileFd;
    //ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    while (isRunning) {
        FD_ZERO(&readFds);
        for (int i = 0; i < clientSockets.size(); i++) {
            ret = send(clientSockets[i], "1", 1, 0);
            if (ret == -1) {
                close(clientSockets[i]);
                clientSockets.erase(clientSockets.begin() + i);
                continue;
            }

            FD_SET(clientSockets[i], &readFds);
            ret = select(clientSockets[i] + 1, &readFds, nullptr, nullptr, &tv);
            if (ret < 0) {
                perror("ret");
            }
            if (ret == 0) {
                continue;
            }
            if (ret > 0) {
                memset(buff, 0, bufferSize);
                readAmount = read(clientSockets[i], &buff, bufferSize);
                if (readAmount > 0) {
                   fileFd = open(buff, O_CREAT | O_RDWR | O_TRUNC, 0660);
                   if (fileFd <= 0) {}
                }

                while ((readAmount = read(clientSockets[i], &buff, bufferSize)) > 0) {
                    write(fileFd, &buff, readAmount);
                }
                close(fileFd);
            }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

}