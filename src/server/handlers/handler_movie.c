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
                 "MOVIE %u %s %s %d %s\n",
                 results[i].id,
                 results[i].title,
                 results[i].genre,
                 results[i].duration_min,
                 strlen(results[i].description) > 0 ? results[i].description : "N/A");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
    }

    // 7. Gửi END - gửi ngay lập tức
    snprintf(resp, sizeof(resp), "END\n");
    send_line(session->sockfd, resp);
    log_msg("SEND", session->sockfd, resp);
}

//HANDLE LIST MOVIE
static int parse_timeslot(const char *param,
                          char *out_date, size_t date_sz,
                          char *out_from, size_t from_sz,
                          char *out_to, size_t to_sz) {
    // format: YYYY-MM-DD_HH:MM-HH:MM
    // ví dụ: 2024-05-01_18:00-23:00
    char buf[128];
    strncpy(buf, param, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *under = strchr(buf, '_');
    if (!under) return -1;
    *under = '\0';
    const char *date = buf;
    const char *rest = under + 1;

    char *dash = strchr(rest, '-');
    if (!dash) return -1;
    *dash = '\0';
    const char *from = rest;
    const char *to   = dash + 1;

    if (strlen(date) == 0 || strlen(from) == 0 || strlen(to) == 0) return -1;

    strncpy(out_date, date, date_sz - 1);
    out_date[date_sz - 1] = '\0';

    strncpy(out_from, from, from_sz - 1);
    out_from[from_sz - 1] = '\0';

    strncpy(out_to, to, to_sz - 1);
    out_to[to_sz - 1] = '\0';

    return 0;
}

void handle_list_movie(client_session_t *session, const request_t *req) {
    char resp[512];

    // 1. Kiểm tra đã login?
    if (session->state != SESSION_STATE_LOGGED_IN) {
        snprintf(resp, sizeof(resp),
                 "ERR LIST_MOVIE NOT_AUTHENTICATED Please_login_first\n");
        send(session->sockfd, resp, strlen(resp), 0);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 2. Kiểm tra role - CUSTOMER/ADMIN/MANAGER được phép xem phim
    if (!(session->roles & (ROLE_CUSTOMER | ROLE_ADMIN | ROLE_MANAGER))) {
        snprintf(resp, sizeof(resp),
                 "ERR LIST_MOVIE NO_PERMISSION Role_not_allowed\n");
        send(session->sockfd, resp, strlen(resp), 0);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 3. Kiểm tra args
    // Nếu không có args, trả về tất cả phim (cho admin/manager dễ quản lý)
    Movie results[100];
    int found = 0;

    if (req->argc < 1) {
        // Không có filter -> trả về tất cả phim
        found = db_get_all_movies(results, 100);
    } else if (req->argc < 2) {
        snprintf(resp, sizeof(resp),
                 "ERR LIST_MOVIE INVALID_ARGS Bad_filter\n");
        send(session->sockfd, resp, strlen(resp), 0);
        log_msg("SEND", session->sockfd, resp);
        return;
    } else {
        const char *filter_type = req->args[0];
        const char *param       = req->args[1];

        if (strcmp(filter_type, "GENRE") == 0) {
            found = db_list_movies_by_genre(param, results, 100);
        } else if (strcmp(filter_type, "CINEMA") == 0) {
            found = db_list_movies_by_cinema(param, results, 100);
        } else if (strcmp(filter_type, "TIMESLOT") == 0) {
            char date[DATE_STR_LEN];
            char from[TIME_STR_LEN];
            char to[TIME_STR_LEN];
            if (parse_timeslot(param, date, sizeof(date), from, sizeof(from), to, sizeof(to)) != 0) {
                snprintf(resp, sizeof(resp),
                         "ERR LIST_MOVIE INVALID_ARGS Bad_filter\n");
                send(session->sockfd, resp, strlen(resp), 0);
                log_msg("SEND", session->sockfd, resp);
                return;
            }
            found = db_list_movies_by_timeslot(date, from, to, results, 100);
        } else {
            snprintf(resp, sizeof(resp),
                     "ERR LIST_MOVIE INVALID_ARGS Bad_filter\n");
            send(session->sockfd, resp, strlen(resp), 0);
            log_msg("SEND", session->sockfd, resp);
            return;
        }
    }
    if (found <= 0) {
        snprintf(resp, sizeof(resp),
                 "OK LIST_MOVIE EMPTY 0\n");
        send(session->sockfd, resp, strlen(resp), 0);
        log_msg("SEND", session->sockfd, resp);

        snprintf(resp, sizeof(resp), "END\n");
        send(session->sockfd, resp, strlen(resp), 0);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // header
    snprintf(resp, sizeof(resp),
             "OK LIST_MOVIE FOUND %d\n", found);
    send(session->sockfd, resp, strlen(resp), 0);
    log_msg("SEND", session->sockfd, resp);

    // từng dòng MOVIE
    for (int i = 0; i < found; i++) {
        snprintf(resp, sizeof(resp),
                 "MOVIE %u %s %s %d %s\n",
                 results[i].id,
                 results[i].title,
                 results[i].genre,
                 results[i].duration_min,
                 strlen(results[i].description) > 0 ? results[i].description : "N/A");
        send(session->sockfd, resp, strlen(resp), 0);
        log_msg("SEND", session->sockfd, resp);
    }

    snprintf(resp, sizeof(resp), "END\n");
    send(session->sockfd, resp, strlen(resp), 0);
    log_msg("SEND", session->sockfd, resp);
}

void handle_add_movie(client_session_t *session, const request_t *req) {
    char resp[512];
    
    // 1. Kiểm tra đã login?
    if (session->state != SESSION_STATE_LOGGED_IN) {
        snprintf(resp, sizeof(resp),
                 "ERR ADD_MOVIE NOT_AUTHENTICATED Please_login_first\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    // 2. Kiểm tra role: chỉ MANAGER hoặc ADMIN
    if (!(session->roles & ROLE_MANAGER) && !(session->roles & ROLE_ADMIN)) {
        snprintf(resp, sizeof(resp),
                 "ERR ADD_MOVIE NO_PERMISSION Only_manager_or_admin_can_add_movie\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    // 3. Kiểm tra args: ADD_MOVIE <title> <genre> <duration_min> <description>
    if (req->argc < 4) {
        snprintf(resp, sizeof(resp),
                 "ERR ADD_MOVIE INVALID_ARGS Need_title_genre_duration_description\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    const char *title = req->args[0];
    const char *genre = req->args[1];
    int duration_min = atoi(req->args[2]);
    const char *description = req->args[3];
    
    // 4. Validate
    if (strlen(title) == 0 || strlen(genre) == 0 || duration_min <= 0) {
        snprintf(resp, sizeof(resp),
                 "ERR ADD_MOVIE INVALID_ARGS Invalid_title_genre_or_duration\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    // 5. Thêm vào database
    uint32_t movie_id = 0;
    int ret = db_add_movie(title, genre, duration_min, description, &movie_id);
    
    if (ret == 0) {
        snprintf(resp, sizeof(resp),
                 "OK ADD_MOVIE CREATED %u\n", movie_id);
    } else {
        snprintf(resp, sizeof(resp),
                 "ERR ADD_MOVIE FAILED Internal_error\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    snprintf(resp, sizeof(resp), "END\n");
    send_line(session->sockfd, resp);
    log_msg("SEND", session->sockfd, resp);
}