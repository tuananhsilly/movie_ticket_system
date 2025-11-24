// file: src/server/handlers/handler_auth.c
#include "handlers.h"
#include "../../common/db.h"
#include "../../common/utils.h"
#include "../../common/models.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

void handle_register(client_session_t *session, const request_t *req) {
    char resp[256];

    if (req->argc < 2) {
        snprintf(resp, sizeof(resp),
                 "ERR REGISTER INVALID_ARGS Need_username_and_password\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    const char *username = req->args[0];
    const char *password = req->args[1];

    // Tạo user với role CUSTOMER
    int ret = db_add_user(username, password, ROLE_CUSTOMER);
    if (ret == 0) {
        snprintf(resp, sizeof(resp),
                 "OK REGISTER USER_CREATED\n");
    } else {
        snprintf(resp, sizeof(resp),
                 "ERR REGISTER USER_EXISTS Username_already_taken\n");
    }

    send_line(session->sockfd, resp);
    log_msg("SEND", session->sockfd, resp);
}

void handle_login(client_session_t *session, const request_t *req) {
    char resp[256];

    if (req->argc < 2) {
        snprintf(resp, sizeof(resp),
                 "ERR LOGIN INVALID_ARGS Need_username_and_password\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    const char *username = req->args[0];
    const char *password = req->args[1];

    char stored_pw[PASSWORD_MAX_LEN];
    uint32_t roles = 0;
    if (db_find_user(username, stored_pw, sizeof(stored_pw), &roles) != 0) {
        snprintf(resp, sizeof(resp),
                 "ERR LOGIN INVALID_CREDENTIAL Wrong_username_or_password\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    if (strcmp(stored_pw, password) != 0) {
        snprintf(resp, sizeof(resp),
                 "ERR LOGIN INVALID_CREDENTIAL Wrong_username_or_password\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // Đăng nhập thành công
    session->state = SESSION_STATE_LOGGED_IN;
    session->roles = roles;
    strncpy(session->username, username, sizeof(session->username) - 1);
    session->username[sizeof(session->username) - 1] = '\0';

    char role_list[ROLE_STRING_MAX_LEN];
    roles_to_string(roles, role_list, sizeof(role_list));

    snprintf(resp, sizeof(resp),
             "OK LOGIN LOGIN_OK %s\n", role_list);
    send_line(session->sockfd, resp);
    log_msg("SEND", session->sockfd, resp);
}