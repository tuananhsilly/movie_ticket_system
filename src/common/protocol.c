// file: src/common/protocol.c
#include "protocol.h"
#include <string.h>
#include <stdio.h>

static command_t command_from_string(const char *cmd) {
    if (strcmp(cmd, "REGISTER") == 0) return CMD_REGISTER;
    if (strcmp(cmd, "LOGIN") == 0)    return CMD_LOGIN;
    if (strcmp(cmd, "QUIT") == 0)     return CMD_QUIT;
    if (strcmp(cmd, "SEARCH_MOVIE") == 0) return CMD_SEARCH_MOVIE;
    if (strcmp(cmd, "LIST_MOVIE") == 0) return CMD_LIST_MOVIE;
    if (strcmp(cmd, "GET_SEATS") == 0) return CMD_GET_SEATS;
    if (strcmp(cmd, "LIST_SHOW") == 0) return CMD_LIST_SHOW;
    if (strcmp(cmd, "BOOK_SEATS") == 0) return CMD_BOOK_SEATS;
    if (strcmp(cmd, "ADD_MOVIE") == 0) return CMD_ADD_MOVIE;    
    if (strcmp(cmd, "CREATE_USER") == 0) return CMD_CREATE_USER;
    if (strcmp(cmd, "GRANT_ROLE") == 0) return CMD_GRANT_ROLE;
    if (strcmp(cmd, "REVOKE_ROLE") == 0) return CMD_REVOKE_ROLE;
    return CMD_UNKNOWN;
}

int parse_request_line(const char *line, request_t *req) {
    if (!line || !req) return -1;
    memset(req, 0, sizeof(*req));

    char buf[512];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *saveptr = NULL;
    char *token = strtok_r(buf, " ", &saveptr);
    if (!token) return -1;

    req->cmd = command_from_string(token);
    if (req->cmd == CMD_UNKNOWN) {
        return -1;
    }

    int argc = 0;
    while ((token = strtok_r(NULL, " ", &saveptr)) != NULL && argc < 20) {
        strncpy(req->args[argc], token, sizeof(req->args[argc]) - 1);
        req->args[argc][sizeof(req->args[argc]) - 1] = '\0';
        argc++;
    }
    req->argc = argc;
    return 0;
}
