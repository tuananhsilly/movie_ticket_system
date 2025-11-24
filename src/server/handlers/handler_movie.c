// file: src/server/handlers/handler_movie.c
#include "handlers.h"
#include "../../common/db.h"
#include "../../common/utils.h"
#include "../../common/models.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

void handle_search_movie(client_session_t *session, const request_t *req) {
    char resp[512];

    // 1. Kiểm tra đã login chưa
    if (session->state != SESSION_STATE_LOGGED_IN) {
        snprintf(resp, sizeof(resp),
                 "ERR SEARCH_MOVIE NOT_AUTHENTICATED Please_login_first\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 2. Kiểm tra role - chỉ CUSTOMER mới được phép
    if (!(session->roles & ROLE_CUSTOMER)) {
        snprintf(resp, sizeof(resp),
                 "ERR SEARCH_MOVIE NO_PERMISSION Only_CUSTOMER_role_allowed\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 3. Kiểm tra args
    if (req->argc < 1) {
        snprintf(resp, sizeof(resp),
                 "ERR SEARCH_MOVIE INVALID_ARGS Need_keyword\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    const char *keyword = req->args[0];

    // 4. Tìm kiếm phim
    Movie results[50];
    int found = db_search_movies_by_title(keyword, results, 50);

    if (found <= 0) {
        // Không có phim
        snprintf(resp, sizeof(resp),
                 "OK SEARCH_MOVIE EMPTY 0\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);

        snprintf(resp, sizeof(resp), "END\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 5. Gửi header FOUND - gửi ngay lập tức
    snprintf(resp, sizeof(resp),
             "OK SEARCH_MOVIE FOUND %d\n", found);
    send_line(session->sockfd, resp);
    log_msg("SEND", session->sockfd, resp);

    // 6. Gửi từng dòng MOVIE ... - gửi ngay lập tức từng dòng
    for (int i = 0; i < found; i++) {
        snprintf(resp, sizeof(resp),
                 "MOVIE %u %s %s %d\n",
                 results[i].id,
                 results[i].title,
                 results[i].genre,
                 results[i].duration_min);
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
    }

    // 7. Gửi END - gửi ngay lập tức
    snprintf(resp, sizeof(resp), "END\n");
    send_line(session->sockfd, resp);
    log_msg("SEND", session->sockfd, resp);
}