// file: src/common/utils.h
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * Ghi log đơn giản: direction = "RECV" hoặc "SEND" hoặc "INFO"...
 * client_id: có thể là socket fd hoặc index session.
 */
void log_msg(const char *direction, int client_id, const char *line);

/**
 * Cắt bỏ \r\n ở cuối chuỗi (nếu có).
 */
void trim_newline(char *s);

/**
 * Lấy timestamp dạng "YYYY-MM-DD HH:MM:SS" vào buffer out.
 * out_size >= 20.
 */
void current_timestamp(char *out, size_t out_size);

/**
 * Send complete line to socket, ensuring all data is sent.
 * Returns 0 on success, -1 on error.
 */
 int send_line(int sockfd, const char *line);

#endif
