// file: src/server/handlers/handlers.h
#ifndef HANDLERS_H
#define HANDLERS_H

#include "../server.h"
#include "../../common/protocol.h"

void handle_register(client_session_t *session, const request_t *req);
void handle_login(client_session_t *session, const request_t *req);
void handle_search_movie(client_session_t *session, const request_t *req);
void handle_list_movie(client_session_t *session, const request_t *req);
#endif
