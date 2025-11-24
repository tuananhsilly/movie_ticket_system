// file: src/client/client.c
#include "client.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int client_connect(const char *host, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_port        = htons(port);
    if (inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return -1;
    }
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

int client_send_line(int sockfd, const char *line) {
    int len = (int)strlen(line);
    int sent = 0;
    while (sent < len) {
        int n = send(sockfd, line + sent, len - sent, 0);
        if (n <= 0) return -1;
        sent += n;
    }
    return 0;
}

int client_recv_line(int sockfd, char *buf, int maxlen) {
    int i = 0;
    char c;
    int n;
    while (i < maxlen - 1) {
        n = recv(sockfd, &c, 1, 0);
        if (n == 1) {
            buf[i++] = c;
            if (c == '\n') break;
        } else if (n == 0) {
            if (i == 0) return 0;
            break;
        } else {
            return -1;
        }
    }
    buf[i] = '\0';
    return i;
}
