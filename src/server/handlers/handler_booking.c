// file: src/server/handlers/handler_booking.c
#include "handlers.h"
#include "../../common/db.h"
#include "../../common/utils.h"
#include "../../common/models.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>

void handle_view_bookings(client_session_t *session, const request_t *req) {
    char resp[512];
    (void)req; // Tham số này không sử dụng nhưng cần cho signature

    // 1. Kiểm tra đã login chưa
    if (session->state != SESSION_STATE_LOGGED_IN) {
        snprintf(resp, sizeof(resp),
                 "ERR VIEW_BOOKINGS NOT_AUTHENTICATED Please_login_first\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 2. Chỉ CUSTOMER role mới được xem booking của mình
    if (!(session->roles & ROLE_CUSTOMER)) {
        snprintf(resp, sizeof(resp),
                 "ERR VIEW_BOOKINGS NO_PERMISSION Only_CUSTOMER_role_allowed\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 3. Lấy danh sách booking của user
    Booking bookings[100];
    int count = db_get_user_bookings(session->user_id, bookings, 100);

    if (count <= 0) {
        // Không có booking
        snprintf(resp, sizeof(resp), "OK VIEW_BOOKINGS EMPTY 0\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 4. Gửi header response
    snprintf(resp, sizeof(resp), "OK VIEW_BOOKINGS FOUND %d\n", count);
    send_line(session->sockfd, resp);
    log_msg("SEND", session->sockfd, resp);

    // 5. Gửi từng booking
    for (int i = 0; i < count; i++) {
        // Format: BOOKING <id> <show_id> <seat_count> <seat_list> <status> <booked_at>
        // seat_list format: "1:5,2:3,..." (dùng dấu phẩy tách)
        
        char seat_list[256] = "";
        for (int j = 0; j < bookings[i].seat_count; j++) {
            char seat_str[32];
            snprintf(seat_str, sizeof(seat_str), "%d:%d", 
                     bookings[i].seat_rows[j], bookings[i].seat_cols[j]);
            strcat(seat_list, seat_str);
            if (j < bookings[i].seat_count - 1) {
                strcat(seat_list, ",");
            }
        }

        snprintf(resp, sizeof(resp),
                 "BOOKING %u %u %d %s %s %ld\n",
                 bookings[i].id, bookings[i].show_id, bookings[i].seat_count,
                 seat_list, bookings[i].status, bookings[i].booked_at);
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
    }

    // 6. Gửi END marker
    snprintf(resp, sizeof(resp), "END\n");
    send_line(session->sockfd, resp);
    log_msg("SEND", session->sockfd, resp);
}

void handle_cancel_booking(client_session_t *session, const request_t *req) {
    char resp[512];

    // 1. Kiểm tra đã login chưa
    if (session->state != SESSION_STATE_LOGGED_IN) {
        snprintf(resp, sizeof(resp),
                 "ERR CANCEL_BOOKING NOT_AUTHENTICATED Please_login_first\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 2. Chỉ CUSTOMER role mới được hủy booking
    if (!(session->roles & ROLE_CUSTOMER)) {
        snprintf(resp, sizeof(resp),
                 "ERR CANCEL_BOOKING NO_PERMISSION Only_CUSTOMER_role_allowed\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 3. Kiểm tra args
    if (req->argc < 1) {
        snprintf(resp, sizeof(resp),
                 "ERR CANCEL_BOOKING INVALID_ARGS Need_booking_id\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    uint32_t booking_id = (uint32_t)atoi(req->args[0]);
    if (booking_id == 0) {
        snprintf(resp, sizeof(resp),
                 "ERR CANCEL_BOOKING INVALID_ARGS Invalid_booking_id\n");
        send_line(session->sockfd, resp);
        log_msg("SEND", session->sockfd, resp);
        return;
    }

    // 4. Gọi DB function để hủy booking
    int ret = db_cancel_booking(booking_id, session->user_id);
    
    if (ret == 0) {
        // Thành công
        snprintf(resp, sizeof(resp),
                 "OK CANCEL_BOOKING CANCELED\n");
    } else if (ret == -1) {
        // Booking không tìm thấy
        snprintf(resp, sizeof(resp),
                 "ERR CANCEL_BOOKING BOOKING_NOT_FOUND Booking_not_found\n");
    } else if (ret == -2) {
        // User không sở hữu booking
        snprintf(resp, sizeof(resp),
                 "ERR CANCEL_BOOKING NO_PERMISSION You_do_not_own_this_booking\n");
    } else if (ret == -3) {
        // Booking đã cancel trước đó
        snprintf(resp, sizeof(resp),
                 "ERR CANCEL_BOOKING INVALID_STATE Booking_already_cancelled\n");
    } else {
        // Lỗi không xác định
        snprintf(resp, sizeof(resp),
                 "ERR CANCEL_BOOKING INTERNAL_ERROR Internal_server_error\n");
    }

    send_line(session->sockfd, resp);
    log_msg("SEND", session->sockfd, resp);
}
