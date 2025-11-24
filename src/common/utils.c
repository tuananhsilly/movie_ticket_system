// file: src/common/utils.c
#include "utils.h"
#include <sys/socket.h>
#include <errno.h>

void current_timestamp(char *out, size_t out_size) {
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    if (!tm_now) {
        snprintf(out, out_size, "0000-00-00 00:00:00");
        return;
    }
    strftime(out, out_size, "%Y-%m-%d %H:%M:%S", tm_now);
}

void log_msg(const char *direction, int client_id, const char *line) {
    char ts[32];
    current_timestamp(ts, sizeof(ts));

    // In ra stdout
    printf("[%s] %s client=%d %s\n", ts, direction, client_id, line);

    // Ghi file logs/server.log
    FILE *f = fopen("logs/server.log", "a");
    if (f) {
        fprintf(f, "[%s] %s client=%d %s\n", ts, direction, client_id, line);
        fclose(f);
    }
}

void trim_newline(char *s) {
    if (!s) return;
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';
        len--;
    }
}


int send_line(int sockfd, const char *line) {
    if (!line) return -1;
    
    size_t len = strlen(line);
    size_t sent = 0;
    
    while (sent < len) {
        ssize_t n = send(sockfd, line + sent, len - sent, 0);
        if (n < 0) {
            // Retry on interrupt (EINTR) or temporary unavailability (EAGAIN/EWOULDBLOCK)
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue; // Retry
            }
            return -1; // Error
        }
        if (n == 0) {
            return -1; // Connection closed
        }
        sent += n;
    }
    
    return 0;
}