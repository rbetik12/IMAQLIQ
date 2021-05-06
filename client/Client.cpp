#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define PORT 8080

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[4096] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    int fileFd;
    int readAmount;
    char* filename = "file.txt";
    if ((fileFd = open(filename, O_RDONLY)) <= 0) {
        perror("File open: ");
        exit(EXIT_FAILURE);
    }
    snprintf((char*)&buffer, sizeof(buffer), "%s", filename);
    send(sock, &buffer, sizeof(buffer), 0);
    while ((readAmount = read(fileFd, &buffer, sizeof(buffer))) > 0) {
        send(sock, &buffer, readAmount, 0);
    }
    close(sock);
    return 0;
}