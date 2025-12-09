// file: src/server/handlers/handlers.h
#ifndef HANDLERS_H
#define HANDLERS_H

#include "../server.h"
#include "../../common/protocol.h"

void handle_register(client_session_t *session, const request_t *req);
void handle_login(client_session_t *session, const request_t *req);
void handle_search_movie(client_session_t *session, const request_t *req);
void handle_list_movie(client_session_t *session, const request_t *req);
void handle_list_show(client_session_t *session, const request_t *req);
void handle_get_seats(client_session_t *session, const request_t *req);
void handle_book_seats(client_session_t *session, const request_t *req);
void handle_add_movie(client_session_t *session, const request_t *req);

void handle_create_user(client_session_t *session, const request_t *req);
void handle_grant_role(client_session_t *session, const request_t *req);
void handle_revoke_role(client_session_t *session, const request_t *req);
#endif
