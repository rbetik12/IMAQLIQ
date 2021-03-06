#include "Server.h"
#include <iostream>
#include <memory>
#include <signal.h>
#include <fcntl.h>
#include <syslog.h>
#include <filesystem>
#include <ftw.h>

int main(int argc, char* argv[]) {
    pid_t pid;
    short port;
    std::unique_ptr<Server> server;

    if (argc < 2) {
        fputs("server <port>", stderr);
        exit(EXIT_FAILURE);
    }

    if (!(port = atoi(argv[1]))) {
        fputs("Port must be a number between 0 and 65536", stderr);
        exit(EXIT_FAILURE);
    }

    pid = fork();

    if (pid < 0) {
        perror("Fork");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    pid = fork();

    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    umask(0);

    chdir(".");

    long x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--) {
        close (x);
    }

    openlog ("cooldaemon", LOG_PID, LOG_DAEMON);

    server.reset(Server::Create(port));
    server->Run();
    return 0;
}

Server::Server(short port): isRunning(false) {
    int opt;

    if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        syslog(LOG_ERR, "Can't create socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt))) {
        syslog(LOG_ERR, "Can't get socket op: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverFd, (struct sockaddr *) &address,
             sizeof(address)) < 0) {
        syslog(LOG_ERR, "Bind failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (listen(serverFd, 10) < 0) {
        syslog(LOG_ERR, "Listen: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

Server::~Server() {
    for (auto sock: clientSockets) {
        close(sock);
    }
    acceptThread.reset();
    close(serverFd);
}

void Server::AcceptThread() {
    int clientSocket;
    int addrLen = sizeof(address);
    struct sockaddr sockaddr;

    syslog(LOG_INFO, "Started accept thread");
    while (isRunning) {
        if ((clientSocket = accept(serverFd, &sockaddr, (socklen_t*) &addrLen)) < 0) {
            syslog(LOG_ALERT, "accept");
        }
        else {
            clientSockets.push_back(clientSocket);
            syslog(LOG_INFO, "Accepted new user!");
        }
    }
}

void Server::Run() {
    isRunning = true;
    acceptThread = std::make_unique<std::thread>(&Server::AcceptThread, this);
    acceptThread->detach();
    const int bufferSize = 4096;
    char buff[bufferSize];
    char saveDirBuff[512];
    int ret, readAmount, fileFd;
    fd_set readFds;
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 1000;

    // Ignore SIGPIPE for proper client socket closing
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, Server::HandleSIGTERM);
    signal(SIGHUP, Server::HandleSIGHUP);

    syslog(LOG_INFO, "Started main server thread");
    while (isRunning) {
        FD_ZERO(&readFds);
        for (int i = 0; i < clientSockets.size(); i++) {
            // Here we send 1 byte to client socket to check whether is it still alive or not
            ret = send(clientSockets[i], "1", 1, 0);
            if (ret == -1) {
                close(clientSockets[i]);
                clientSockets.erase(clientSockets.begin() + i);
                continue;
            }

            FD_SET(clientSockets[i], &readFds);
            ret = select(clientSockets[i] + 1, &readFds, nullptr, nullptr, &tv);
            if (ret < 0) {
                syslog(LOG_ERR, "ret error");
            }
            if (ret == 0) {
                continue;
            }
            if (ret > 0) {
                memset(buff, 0, bufferSize);
                readAmount = read(clientSockets[i], &buff, bufferSize);
                memset(&saveDirBuff, 0, sizeof(saveDirBuff));
                snprintf((char*) &saveDirBuff, 64, "/tmp/%s", buff);

                if (readAmount > 0) {
                   fileFd = open(saveDirBuff, O_CREAT | O_RDWR | O_TRUNC, 0666);
                   if (fileFd <= 0) {
                       send(clientSockets[i], "n", 1, 0);
                   }
                   else {
                       send(clientSockets[i], "y", 1, 0);
                   }
                }
                syslog(LOG_INFO, "Got new file: %s. Saving to %s", buff, saveDirBuff);
                if (fileFd > 0) {
                    while ((readAmount = read(clientSockets[i], &buff, bufferSize)) > 0) {
                        write(fileFd, &buff, readAmount);
                    }
                    close(fileFd);
                }
                else {
                    while ((readAmount = read(clientSockets[i], &buff, bufferSize)) > 0);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

}

void Server::HandleSIGTERM(int signal) {
    Server::GetInstance()->Shutdown();
}

void Server::HandleSIGHUP(int signal) {
    Server::GetInstance()->Shutdown();
}

Server *Server::GetInstance() {
    return instance;
}

Server *Server::Create(short port) {
    if (instance == nullptr) {
        instance = new Server(port);
    }
    return instance;
}

Server* Server::instance = nullptr;

void Server::Shutdown() {
    isRunning = false;
}
