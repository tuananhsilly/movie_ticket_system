// file: src/common/protocol.h
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>

typedef enum {
    CMD_UNKNOWN = 0,
    CMD_REGISTER,
    CMD_LOGIN,
    CMD_SEARCH_MOVIE, 
    CMD_QUIT
} command_t;

typedef struct {
    command_t cmd;
    int argc;
    char args[4][128]; // đủ cho REGISTER/LOGIN/QUIT; sau này có thể tăng
} request_t;

/**
 * Parse 1 dòng request từ client (đã trim \n).
 * Trả 0 nếu OK, -1 nếu lỗi format hoặc command không hợp lệ.
 */
int parse_request_line(const char *line, request_t *req);

/**
 * Build response line cho các trường hợp OK/ERR cơ bản.
 * Bạn có thể dùng trực tiếp sprintf trong server, nhưng để đây để tham khảo.
 */

#endif
