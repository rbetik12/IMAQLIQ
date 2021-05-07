#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define MIB 1048576
#define KIB 1024
char* progressBar[] = {
        "[|          ]",
        "[||         ]",
        "[|||        ]",
        "[||||       ]",
        "[|||||      ]",
        "[||||||     ]",
        "[|||||||    ]",
        "[||||||||   ]",
        "[|||||||||  ]",
        "[|||||||||| ]",
        "[|||||||||||]"
};

int extract_filename(const char* filepath, char** filename) {
    size_t filenameLen = 0;
    char* _filename = NULL;
    for (size_t i = strlen(filepath) - 1; i > 0; i--) {
        if (filepath[i] == '/') break;
        filenameLen += 1;
    }

    *filename = (char*) calloc(filenameLen + 1, sizeof(char));
    _filename = *filename;

    memcpy(_filename, filepath + strlen(filepath) - filenameLen, filenameLen);

    return 0;
}

int connect_to_server(short port) {
    int sock;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    return sock;
}

int main(int argc, char* argv[]) {
    int sock;
    int fileFd;
    int readAmount;
    int progressBarIndex = 0;
    short port;
    size_t bytesSent = 0;
    size_t fileSize;
    char buffer[4096] = {0};
    char* filepath = NULL;
    char* filename = NULL;
    struct stat stat;
    float fileSentProgress = 0;

    if (argc < 3) {
        fputs("client <port> <fil/home/vitaliy/code/cpp/internship/cmake-build-debug/clientepath>", stderr);
        exit(EXIT_FAILURE);
    }

    if (!(port = atoi(argv[1]))) {
        fputs("Port must be a number between 0 and 65536", stderr);
        exit(EXIT_FAILURE);
    }

    if ((sock = connect_to_server(port)) < 0) {
        fputs("Can't connect to server!", stderr);
        exit(EXIT_FAILURE);
    }

    filepath = argv[2];

    if ((fileFd = open(filepath, O_RDONLY)) <= 0) {
        perror("File open: ");
        exit(EXIT_FAILURE);
    }

    if (extract_filename(filepath, &filename)) {
        fputs("Can't extract filename", stderr);
        exit(EXIT_FAILURE);
    }

    fstat(fileFd, &stat);
    fileSize = stat.st_size;
    // Read one check byte from server
    read(sock, &buffer, 1);

    snprintf((char*)&buffer, sizeof(buffer), "%s", filename);

    send(sock, &buffer, sizeof(buffer), 0);
    memset(&buffer, 0, sizeof(buffer));
    readAmount = read(sock, &buffer, 1);

    if (readAmount > 0) {
        if (buffer[0] == 'y') {
            printf("Starting file transmission.\n");
            while ((readAmount = read(fileFd, &buffer, sizeof(buffer))) > 0) {
                bytesSent += readAmount;
                fileSentProgress = ((float) bytesSent / fileSize) * 100.0f;
                progressBarIndex = (int) (fileSentProgress / 10.0f);
                printf("\r");
                if (fileSize > MIB) {
                    printf("%%%.2f\t%s\t%lu/%zu MiB", fileSentProgress, progressBar[progressBarIndex],
                                                            bytesSent / MIB, fileSize / MIB);
                }
                else if (fileSize > KIB) {
                    printf("%%%.2f\t%s\t%lu/%zu KiB", fileSentProgress, progressBar[progressBarIndex],
                                                            bytesSent / KIB, fileSize / KIB);
                }
                else {
                    printf("%%%.2f\t%s\t%lu/%zu Bytes", fileSentProgress, progressBar[progressBarIndex],
                           bytesSent, fileSize);
                }
                fflush(stdout);
                send(sock, &buffer, readAmount, 0);
            }
            printf("\n");
            printf("Successfully sent file %s to server!\n", filename);
        }
        else {
            printf("Server can't create file. Aborting...\n");
            send(sock, "1", 1, 0);
        }
    }

    free(filename);
    close(sock);
    return 0;
}