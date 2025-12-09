// file: src/server/handlers/handler_admin.c
#include "handlers.h"
#include "../../common/db.h"
#include "../../common/utils.h"
#include "../../common/models.h"

#include <stdio.h>
#include <string.h>

void handle_create_user(client_session_t *session, const request_t *req) {
    char resp[256];
    
    // 1. Kiểm tra đã login?
    if (session->state != SESSION_STATE_LOGGED_IN) {
        snprintf(resp, sizeof(resp),
                 "ERR CREATE_USER NOT_AUTHENTICATED Please_login_first\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    // 2. Chỉ ADMIN mới được tạo user
    if (!(session->roles & ROLE_ADMIN)) {
        snprintf(resp, sizeof(resp),
                 "ERR CREATE_USER NO_PERMISSION Only_admin_can_create_users\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    // 3. Kiểm tra args: CREATE_USER <username> <password>
    if (req->argc < 2) {
        snprintf(resp, sizeof(resp),
                 "ERR CREATE_USER INVALID_ARGS Need_username_and_password\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    const char *username = req->args[0];
    const char *password = req->args[1];
    
    // Validate
    if (strlen(username) == 0 || strlen(password) == 0) {
        snprintf(resp, sizeof(resp),
                 "ERR CREATE_USER INVALID_ARGS Username_and_password_cannot_be_empty\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    // Tạo user với role CUSTOMER mặc định
    int ret = db_add_user(username, password, ROLE_CUSTOMER);
    
    if (ret == 0) {
        snprintf(resp, sizeof(resp),
                 "OK CREATE_USER USER_CREATED\n");
    } else {
        snprintf(resp, sizeof(resp),
                 "ERR CREATE_USER USER_EXISTS Username_already_taken\n");
    }
    
    send_line(session->sockfd, resp);
    log_msg("SEND", session->sockfd, resp);
}

void handle_grant_role(client_session_t *session, const request_t *req) {
    char resp[256];
    
    // 1. Kiểm tra đã login?
    if (session->state != SESSION_STATE_LOGGED_IN) {
        snprintf(resp, sizeof(resp),
                 "ERR GRANT_ROLE NOT_AUTHENTICATED Please_login_first\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    // 2. Chỉ ADMIN mới được grant role
    if (!(session->roles & ROLE_ADMIN)) {
        snprintf(resp, sizeof(resp),
                 "ERR GRANT_ROLE NO_PERMISSION Only_admin_can_grant_roles\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    // 3. Kiểm tra args: GRANT_ROLE <username> <role>
    if (req->argc < 2) {
        snprintf(resp, sizeof(resp),
                 "ERR GRANT_ROLE INVALID_ARGS Need_username_and_role\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    const char *username = req->args[0];
    const char *role_str = req->args[1];
    
    // Parse role string thành bitmask
    uint32_t role_mask = 0;
    if (strcmp(role_str, "CUSTOMER") == 0) {
        role_mask = ROLE_CUSTOMER;
    } else if (strcmp(role_str, "MANAGER") == 0) {
        role_mask = ROLE_MANAGER;
    } else if (strcmp(role_str, "ADMIN") == 0) {
        role_mask = ROLE_ADMIN;
    } else {
        snprintf(resp, sizeof(resp),
                 "ERR GRANT_ROLE ROLE_INVALID Role_must_be_CUSTOMER_MANAGER_or_ADMIN\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    // Kiểm tra user có tồn tại không
    char pw_buf[PASSWORD_MAX_LEN];
    if (db_find_user(username, pw_buf, sizeof(pw_buf), NULL) != 0) {
        snprintf(resp, sizeof(resp),
                 "ERR GRANT_ROLE USER_NOT_FOUND User_not_found\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    // Grant role
    int ret = db_grant_role(username, role_mask);
    
    if (ret == 0) {
        snprintf(resp, sizeof(resp),
                 "OK GRANT_ROLE SUCCESS Role_%s_granted_to_%s\n", role_str, username);
    } else {
        snprintf(resp, sizeof(resp),
                 "ERR GRANT_ROLE INTERNAL_ERROR Failed_to_grant_role\n");
    }
    
    send_line(session->sockfd, resp);
    log_msg("SEND", session->sockfd, resp);
}

void handle_revoke_role(client_session_t *session, const request_t *req) {
    char resp[256];
    
    // 1. Kiểm tra đã login?
    if (session->state != SESSION_STATE_LOGGED_IN) {
        snprintf(resp, sizeof(resp),
                 "ERR REVOKE_ROLE NOT_AUTHENTICATED Please_login_first\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    // 2. Chỉ ADMIN mới được revoke role
    if (!(session->roles & ROLE_ADMIN)) {
        snprintf(resp, sizeof(resp),
                 "ERR REVOKE_ROLE NO_PERMISSION Only_admin_can_revoke_roles\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    // 3. Kiểm tra args: REVOKE_ROLE <username> <role>
    if (req->argc < 2) {
        snprintf(resp, sizeof(resp),
                 "ERR REVOKE_ROLE INVALID_ARGS Need_username_and_role\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    const char *username = req->args[0];
    const char *role_str = req->args[1];
    
    // Parse role string thành bitmask
    uint32_t role_mask = 0;
    if (strcmp(role_str, "CUSTOMER") == 0) {
        role_mask = ROLE_CUSTOMER;
    } else if (strcmp(role_str, "MANAGER") == 0) {
        role_mask = ROLE_MANAGER;
    } else if (strcmp(role_str, "ADMIN") == 0) {
        role_mask = ROLE_ADMIN;
    } else {
        snprintf(resp, sizeof(resp),
                 "ERR REVOKE_ROLE ROLE_INVALID Role_must_be_CUSTOMER_MANAGER_or_ADMIN\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    // Kiểm tra user có tồn tại không
    char pw_buf[PASSWORD_MAX_LEN];
    if (db_find_user(username, pw_buf, sizeof(pw_buf), NULL) != 0) {
        snprintf(resp, sizeof(resp),
                 "ERR REVOKE_ROLE USER_NOT_FOUND User_not_found\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    // Revoke role
    int ret = db_revoke_role(username, role_mask);
    
    if (ret == 0) {
        snprintf(resp, sizeof(resp),
                 "OK REVOKE_ROLE SUCCESS Role_%s_revoked_from_%s\n", role_str, username);
    } else {
        snprintf(resp, sizeof(resp),
                 "ERR REVOKE_ROLE INTERNAL_ERROR Failed_to_revoke_role\n");
    }
    
    send_line(session->sockfd, resp);
    log_msg("SEND", session->sockfd, resp);
}