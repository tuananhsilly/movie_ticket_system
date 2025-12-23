// file: src/server/handlers/handler_show.c
#include "handlers.h"
#include "../../common/db.h"
#include "../../common/utils.h"
#include "../../common/models.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>

void handle_list_show(client_session_t *session, const request_t *req) {
    char resp[512];

    // 1. Kiểm tra đã login chưa
    if (session->state != SESSION_STATE_LOGGED_IN) {
        snprintf(resp, sizeof(resp),
                 "ERR LIST_SHOW NOT_AUTHENTICATED Please_login_first\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 2. Kiểm tra args - cần ít nhất movie_id
    if (req->argc < 1) {
        snprintf(resp, sizeof(resp),
                 "ERR LIST_SHOW INVALID_ARGS Need_movie_id_[and_optional_date]\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 3. Parse movie_id
    uint32_t movie_id = (uint32_t)atoi(req->args[0]);
    if (movie_id == 0) {
        snprintf(resp, sizeof(resp),
                 "ERR LIST_SHOW INVALID_ARGS Invalid_movie_id\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 4. Parse date (optional)
    const char *date = NULL;
    if (req->argc >= 2) {
        date = req->args[1];
    }

    // 5. Query shows từ DB
    Show results[100];
    int found = db_list_shows_by_movie(movie_id, date, results, 100);

    // 6. Trả kết quả
    if (found <= 0) {
        // Không có suất chiếu
        snprintf(resp, sizeof(resp),
                 "OK LIST_SHOW EMPTY 0\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);

        snprintf(resp, sizeof(resp), "END\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 7. Gửi header FOUND
    snprintf(resp, sizeof(resp),
             "OK LIST_SHOW FOUND %d\n", found);
    send_line(session->sockfd, resp);
    log_msg("SEND", session->sockfd, resp);

    // 8. Gửi từng dòng SHOW ...
    for (int i = 0; i < found; i++) {
        snprintf(resp, sizeof(resp),
                 "SHOW %u %s %s %s %s\n",
                 results[i].id,
                 results[i].cinema_id,
                 results[i].room_id,
                 results[i].date,
                 results[i].start_time);
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
    }

    // 9. Gửi END
    snprintf(resp, sizeof(resp), "END\n");
    send_line(session->sockfd, resp);
    log_msg("SEND", session->sockfd, resp);
}

void handle_get_seats(client_session_t *session, const request_t *req) {
    char resp[512];

    // 1. Kiểm tra đã login chưa
    if (session->state != SESSION_STATE_LOGGED_IN) {
        snprintf(resp, sizeof(resp),
                 "ERR GET_SEATS NOT_AUTHENTICATED Please_login_first\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 2. Kiểm tra args - cần show_id
    if (req->argc < 1) {
        snprintf(resp, sizeof(resp),
                 "ERR GET_SEATS INVALID_ARGS Need_show_id\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 3. Parse show_id
    uint32_t show_id = (uint32_t)atoi(req->args[0]);
    if (show_id == 0) {
        snprintf(resp, sizeof(resp),
                 "ERR GET_SEATS INVALID_ARGS Invalid_show_id\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 4. Kiểm tra show có tồn tại không
    Show *show = db_find_show_by_id(show_id);
    if (!show) {
        snprintf(resp, sizeof(resp),
                 "ERR GET_SEATS SHOW_NOT_FOUND\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 5. Lấy seats từ DB
    Seat seats[200]; // Tối đa 200 ghế (10x20)
    int rows = 0, cols = 0;
    int seat_count = db_get_seats_for_show(show_id, seats, 200, &rows, &cols);
    
    if (seat_count < 0) {
        snprintf(resp, sizeof(resp),
                 "ERR GET_SEATS INTERNAL_ERROR Cannot_load_seats\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 6. Gửi header với rows và cols
    snprintf(resp, sizeof(resp),
             "OK GET_SEATS %u %d %d\n", show_id, rows, cols);
    send_line(session->sockfd, resp);
    log_msg("SEND", session->sockfd, resp);

    // 7. Gửi từng dòng SEAT ...
    for (int i = 0; i < seat_count; i++) {
        snprintf(resp, sizeof(resp),
                 "SEAT %d %d %s\n",
                 seats[i].row,
                 seats[i].col,
                 seats[i].status);
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
    }

    // 8. Gửi END
    snprintf(resp, sizeof(resp), "END\n");
    send_line(session->sockfd, resp);
    log_msg("SEND", session->sockfd, resp);
}

void handle_book_seats(client_session_t *session, const request_t *req) {
    char resp[512];

    // 1. Kiểm tra đã login chưa
    if (session->state != SESSION_STATE_LOGGED_IN) {
        snprintf(resp, sizeof(resp),
                 "ERR BOOK_SEATS NOT_AUTHENTICATED Please_login_first\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 2. Kiểm tra role CUSTOMER
    if (!(session->roles & ROLE_CUSTOMER)) {
        snprintf(resp, sizeof(resp),
                 "ERR BOOK_SEATS NO_PERMISSION Only_CUSTOMER_role_allowed\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 3. Kiểm tra args - cần ít nhất show_id và count
    if (req->argc < 3) {
        snprintf(resp, sizeof(resp),
                 "ERR BOOK_SEATS INVALID_ARGS Bad_arguments\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 4. Parse show_id
    uint32_t show_id = (uint32_t)atoi(req->args[0]);
    if (show_id == 0) {
        snprintf(resp, sizeof(resp),
                 "ERR BOOK_SEATS SHOW_NOT_FOUND\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 5. Kiểm tra show có tồn tại không
    Show *show = db_find_show_by_id(show_id);
    if (!show) {
        snprintf(resp, sizeof(resp),
                 "ERR BOOK_SEATS SHOW_NOT_FOUND\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 6. Parse seat count
    int seat_count = atoi(req->args[1]);
    if (seat_count <= 0 || seat_count > 20) {
        snprintf(resp, sizeof(resp),
                 "ERR BOOK_SEATS INVALID_ARGS Bad_arguments\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 7. Kiểm tra số lượng args đủ không
    // Format: BOOK_SEATS <show_id> <count> <row1> <col1> <row2> <col2> ...
    if (req->argc < 2 + seat_count * 2) {
        snprintf(resp, sizeof(resp),
                 "ERR BOOK_SEATS INVALID_ARGS Bad_arguments\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 8. Parse seats
    int seat_rows[20];
    int seat_cols[20];
    
    for (int i = 0; i < seat_count; i++) {
        seat_rows[i] = atoi(req->args[2 + i * 2]);
        seat_cols[i] = atoi(req->args[3 + i * 2]);
        
        // Kiểm tra ghế có hợp lệ không
        if (seat_rows[i] < 1 || seat_rows[i] > show->rows ||
            seat_cols[i] < 1 || seat_cols[i] > show->cols) {
            snprintf(resp, sizeof(resp),
                     "ERR BOOK_SEATS SEAT_INVALID Row_or_col_out_of_range\n");
            send_line(session->sockfd, resp);
            log_msg("SEND", session->sockfd, resp);
            return;
        }
    }

    // 9. Kiểm tra tất cả ghế có available không
    for (int i = 0; i < seat_count; i++) {
        if (db_check_seat_available(show_id, seat_rows[i], seat_cols[i]) != 0) {
            // Ghế không available
            snprintf(resp, sizeof(resp),
                     "ERR BOOK_SEATS SEAT_TAKEN Seat_%d,%d_already_booked\n",
                     seat_rows[i], seat_cols[i]);
            send_line(session->sockfd, resp);
            log_msg("SEND", session->sockfd, resp);
            return;
        }
    }

    // 10. Tạo booking (atomic operation)
    // Cần user_id - tạm thời dùng 0 hoặc lưu trong session
    // TODO: Cần thêm user_id vào session
    uint32_t user_id = session->user_id; // Tạm thời, cần lấy từ session
    uint32_t booking_id = 0;
    
    if (db_create_booking(user_id, show_id, seat_rows, seat_cols, seat_count, &booking_id) != 0) {
        snprintf(resp, sizeof(resp),
                 "ERR BOOK_SEATS INTERNAL_ERROR Cannot_create_booking\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 11. Tạo ticket IDs (có thể dùng booking_id làm base)
    char ticket_ids[256] = "";
    for (int i = 0; i < seat_count; i++) {
        char ticket_id[16];
        snprintf(ticket_id, sizeof(ticket_id), "%u", booking_id * 100 + i + 1);
        if (i > 0) strcat(ticket_ids, ",");
        strcat(ticket_ids, ticket_id);
    }

    // 12. Gửi response thành công
    snprintf(resp, sizeof(resp),
             "OK BOOK_SEATS BOOKED %s\n", ticket_ids);
    send_line(session->sockfd, resp);
    log_msg("SEND", session->sockfd, resp);
}

void handle_add_show(client_session_t *session, const request_t *req) {
    char resp[512];
    
    // 1. Kiểm tra đã login?
    if (session->state != SESSION_STATE_LOGGED_IN) {
        snprintf(resp, sizeof(resp),
                 "ERR ADD_SHOW NOT_AUTHENTICATED Please_login_first\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    // 2. Kiểm tra role: chỉ ADMIN được phép
    if (!(session->roles & ROLE_ADMIN)) {
        snprintf(resp, sizeof(resp),
                 "ERR ADD_SHOW NO_PERMISSION Only_admin_can_add_show\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    // 3. Kiểm tra args: ADD_SHOW <movie_id> <cinema_id> <room_id> <date> <start_time> <end_time> <rows> <cols>
    if (req->argc < 8) {
        snprintf(resp, sizeof(resp),
                 "ERR ADD_SHOW INVALID_ARGS Need_movie_id_cinema_room_date_start_end_rows_cols\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    // 4. Parse arguments
    uint32_t movie_id = (uint32_t)atoi(req->args[0]);
    const char *cinema_id = req->args[1];
    const char *room_id = req->args[2];
    const char *date = req->args[3];
    const char *start_time = req->args[4];
    const char *end_time = req->args[5];
    int rows = atoi(req->args[6]);
    int cols = atoi(req->args[7]);
    
    // 5. Validate arguments
    if (movie_id == 0 || strlen(cinema_id) == 0 || strlen(room_id) == 0 || 
        strlen(date) == 0 || strlen(start_time) == 0 || strlen(end_time) == 0 ||
        rows <= 0 || cols <= 0) {
        snprintf(resp, sizeof(resp),
                 "ERR ADD_SHOW INVALID_ARGS Invalid_parameters\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    // 6. Thêm vào database
    uint32_t show_id = 0;
    int ret = db_add_show(movie_id, cinema_id, room_id, date, start_time, end_time, rows, cols, &show_id);
    
    if (ret == 0) {
        snprintf(resp, sizeof(resp),
                 "OK ADD_SHOW CREATED %u\n", show_id);
    } else {
        snprintf(resp, sizeof(resp),
                 "ERR ADD_SHOW FAILED Invalid_movie_id_or_database_error\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }
    
    send_line(session->sockfd, resp);
    log_msg("SEND", session->sockfd, resp);
    
    snprintf(resp, sizeof(resp), "END\n");
    send_line(session->sockfd, resp);
    log_msg("SEND", session->sockfd, resp);
}