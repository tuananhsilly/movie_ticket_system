// file: src/client/client.h
#ifndef CLIENT_H
#define CLIENT_H

int client_connect(const char *host, int port);
int client_send_line(int sockfd, const char *line);
int client_recv_line(int sockfd, char *buf, int maxlen);

#endif
