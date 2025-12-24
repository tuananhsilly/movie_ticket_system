// file: src/common/protocol.h
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>

typedef enum {
    CMD_UNKNOWN = 0,
    CMD_REGISTER,
    CMD_LOGIN,
    CMD_SEARCH_MOVIE, 
    CMD_LIST_MOVIE,
    CMD_LIST_SHOW,
    CMD_GET_SEATS,
    CMD_BOOK_SEATS,
    CMD_ADD_MOVIE,
    CMD_ADD_SHOW,
    CMD_UPDATE_SHOW,
    CMD_CANCEL_SHOW,
    CMD_CREATE_USER,
    CMD_GRANT_ROLE,
    CMD_REVOKE_ROLE,
    CMD_VIEW_BOOKINGS,
    CMD_CANCEL_BOOKING,
    CMD_QUIT
} command_t;

typedef struct {
    command_t cmd;
    int argc;
    char args[20][128]; // Tăng lên để hỗ trợ BOOK_SEATS với nhiều ghế 
} request_t;

/**
 * Parse 1 dòng request từ client (đã trim \n).
 * Trả 0 nếu OK, -1 nếu lỗi format hoặc command không hợp lệ.
 */
int parse_request_line(const char *line, request_t *req);


#endif
