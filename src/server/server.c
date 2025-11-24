// file: src/server/server.c
#include "server.h"
#include "../common/utils.h"
#include "../common/db.h"
#include "handlers/handlers.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>  // Thêm cho TCP_NODELAY

int server_init(int *listen_fd) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(SERVER_PORT);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, 16) < 0) {
        perror("listen");
        close(sockfd);
        return -1;
    }

    *listen_fd = sockfd;
    return 0;
}

void sessions_init(client_session_t sessions[], int size) {
    for (int i = 0; i < size; i++) {
        sessions[i].sockfd = -1;
        sessions[i].state  = SESSION_STATE_NOT_AUTH;
        sessions[i].roles  = 0;
        sessions[i].username[0] = '\0';
    }
}

int sessions_add_client(client_session_t sessions[], int size, int client_fd) {
    // Set TCP_NODELAY để tắt Nagle's algorithm, gửi dữ liệu ngay lập tức
    int flag = 1;
    setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    
    for (int i = 0; i < size; i++) {
        if (sessions[i].sockfd == -1) {
            sessions[i].sockfd = client_fd;
            sessions[i].state  = SESSION_STATE_NOT_AUTH;
            sessions[i].roles  = 0;
            sessions[i].username[0] = '\0';
            return i;
        }
    }
    return -1;
}

void sessions_remove_client(client_session_t sessions[], int idx) {
    if (sessions[idx].sockfd != -1) {
        close(sessions[idx].sockfd);
        sessions[idx].sockfd = -1;
        sessions[idx].state  = SESSION_STATE_NOT_AUTH;
        sessions[idx].roles  = 0;
        sessions[idx].username[0] = '\0';
    }
}

void server_handle_line(client_session_t *session, const char *line) {
    // Log request
    log_msg("RECV", session->sockfd, line);

    char trimmed[LINE_BUFFER_SIZE];
    strncpy(trimmed, line, sizeof(trimmed) - 1);
    trimmed[sizeof(trimmed) - 1] = '\0';
    trim_newline(trimmed);

    request_t req;
    if (parse_request_line(trimmed, &req) != 0) {
        char resp[256];
        snprintf(resp, sizeof(resp),
                 "ERR UNKNOWN INVALID_COMMAND Unknown_command\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    switch (req.cmd) {
        case CMD_REGISTER:
            handle_register(session, &req);
            break;
        case CMD_LOGIN:
            handle_login(session, &req);
            break;
        case CMD_SEARCH_MOVIE:
            handle_search_movie(session, &req);
            break;
        case CMD_QUIT: {
            char resp[128];
            snprintf(resp, sizeof(resp), "OK QUIT BYE\n");
            send_line(session->sockfd, resp);
            log_msg("SEND", session->sockfd, resp);
            // đóng ở vòng select sau (hoặc ngay tại đây tuỳ bạn)
            break;
        }
        default: {
            char resp[256];
            snprintf(resp, sizeof(resp),
                     "ERR UNKNOWN INVALID_COMMAND Unknown_command\n");
            send_line(session->sockfd, resp);
            log_msg("SEND", session->sockfd, resp);
            break;
        }
    }
}
