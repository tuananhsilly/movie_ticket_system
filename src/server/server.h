// file: src/server/server.h
#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include "../common/models.h"
#include "../common/protocol.h"

#define SERVER_PORT 9000
#define MAX_CLIENTS  FD_SETSIZE
#define LINE_BUFFER_SIZE 1024

typedef enum {
    SESSION_STATE_NOT_AUTH = 0,
    SESSION_STATE_LOGGED_IN
} session_state_t;

typedef struct {
    int sockfd;
    session_state_t state;
    char username[USERNAME_MAX_LEN];
    uint32_t roles; // bitmask
} client_session_t;

/**
 * Khởi tạo server: tạo socket, bind, listen.
 */
int server_init(int *listen_fd);

/**
 * Khởi tạo mảng session.
 */
void sessions_init(client_session_t sessions[], int size);

/**
 * Thêm client mới sau khi accept.
 */
int sessions_add_client(client_session_t sessions[], int size, int client_fd);

/**
 * Xóa client khi disconnect.
 */
void sessions_remove_client(client_session_t sessions[], int idx);

/**
 * Xử lý 1 dòng command từ client.
 */
void server_handle_line(client_session_t *session, const char *line);

#endif
