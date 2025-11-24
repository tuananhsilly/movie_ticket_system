// file: src/server/main_server.c
#include "server.h"
#include "../common/db.h"
#include "../common/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>

static ssize_t recv_line(int fd, char *buf, size_t maxlen) {
    size_t i = 0;
    char c;
    ssize_t n;

    while (i < maxlen - 1) {
        n = recv(fd, &c, 1, 0);
        if (n == 1) {
            buf[i++] = c;
            if (c == '\n') {
                break;
            }
        } else if (n == 0) {
            // đóng kết nối từ phía client
            if (i == 0) return 0;
            break;
        } else {
            // lỗi
            return -1;
        }
    }
    buf[i] = '\0';
    return (ssize_t)i;
}

int main() {
    // Khởi tạo DB users
    if (db_init("data/users.csv") != 0) {
        fprintf(stderr, "Cannot init DB\n");
        return 1;
    }

    // Khởi tạo DB movies
    if (db_load_movies("data/movies.csv") != 0) {
        fprintf(stderr, "Cannot init DB movies\n");
        return 1;
    }

    // Khởi tạo socket listen
    int listen_fd;
    if (server_init(&listen_fd) != 0) {
        fprintf(stderr, "Cannot init server socket\n");
        return 1;
    }
    printf("Server listening on port %d...\n", SERVER_PORT);

    client_session_t sessions[MAX_CLIENTS];
    sessions_init(sessions, MAX_CLIENTS);

    fd_set read_fds;
    int max_fd = listen_fd;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(listen_fd, &read_fds);

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (sessions[i].sockfd != -1) {
                FD_SET(sessions[i].sockfd, &read_fds);
                if (sessions[i].sockfd > max_fd) {
                    max_fd = sessions[i].sockfd;
                }
            }
        }

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select");
            continue;
        }

        // Kết nối mới
        if (FD_ISSET(listen_fd, &read_fds)) {
            struct sockaddr_in cli_addr;
            socklen_t cli_len = sizeof(cli_addr);
            int new_fd = accept(listen_fd, (struct sockaddr *)&cli_addr, &cli_len);
            if (new_fd < 0) {
                perror("accept");
            } else {
                int idx = sessions_add_client(sessions, MAX_CLIENTS, new_fd);
                if (idx < 0) {
                    printf("Too many clients\n");
                    close(new_fd);
                } else {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "INFO New_connection");
                    log_msg("INFO", new_fd, msg);
                }
            }
        }

        // Xử lý các client
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (sessions[i].sockfd != -1 && FD_ISSET(sessions[i].sockfd, &read_fds)) {
                char line[LINE_BUFFER_SIZE];
                ssize_t n = recv_line(sessions[i].sockfd, line, sizeof(line));
                if (n <= 0) {
                    // client đóng hoặc lỗi
                    char msg[128];
                    snprintf(msg, sizeof(msg), "INFO Connection_closed");
                    log_msg("INFO", sessions[i].sockfd, msg);
                    sessions_remove_client(sessions, i);
                } else {
                    server_handle_line(&sessions[i], line);
                    // nếu là QUIT, ở đây ta có thể kiểm tra và đóng, nhưng đơn giản là:
                    if (strncmp(line, "QUIT", 4) == 0) {
                        sessions_remove_client(sessions, i);
                    }
                }
            }
        }
    }

    close(listen_fd);
    return 0;
}
