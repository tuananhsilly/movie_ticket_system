// file: src/client/main_client.c
#include "client.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// ... existing includes ...

#define SERVER_PORT 9000

static void menu_not_logged_in() {
    printf("=== Movie Ticket Client ===\n");
    printf("1. REGISTER\n");
    printf("2. LOGIN\n");
    printf("Choose: ");
}

static void menu_logged_in() {
    printf("=== Movie Ticket Client ===\n");
    printf("1. SEARCH_MOVIE\n");
    printf("2. LIST_MOVIE\n");
    printf("3. QUIT\n");
    printf("Choose: ");
}

int main(int argc, char *argv[]) {
    char server_ip[64] = "127.0.0.1"; // Default localhost
    
    if (argc >= 2) {
        // IP từ command line: ./client 192.168.1.100
        strncpy(server_ip, argv[1], sizeof(server_ip) - 1);
        server_ip[sizeof(server_ip) - 1] = '\0';
    } else {
        // Hoặc prompt user nhập IP
        printf("Enter server IP address (default: 127.0.0.1): ");
        char input[64];
        if (fgets(input, sizeof(input), stdin)) {
            input[strcspn(input, "\r\n")] = '\0';
            if (strlen(input) > 0) {
                strncpy(server_ip, input, sizeof(server_ip) - 1);
                server_ip[sizeof(server_ip) - 1] = '\0';
            }
        }
    }
    
    printf("Connecting to server at %s:%d...\n", server_ip, SERVER_PORT);
    
    int sockfd = client_connect(server_ip, SERVER_PORT);
    if (sockfd < 0) {
        printf("Cannot connect to server at %s:%d\n", server_ip, SERVER_PORT);
        return 1;
    }
    
    printf("Connected successfully!\n\n");

    int choice;
    char username[64];
    char password[64];
    char keyword[128];
    char line[256];
    char resp[512];

    int logged_in = 0; // client-side flag đơn giản

    while (1) {
        // Hiển thị menu phù hợp với trạng thái
        if (logged_in) {
            menu_logged_in();
        } else {
            menu_not_logged_in();
        }
        
        if (scanf("%d", &choice) != 1) break;
        while (getchar() != '\n'); // clear stdin

        if (!logged_in) {
            // Menu khi chưa login
            if (choice == 1) {
                printf("Username: ");
                fgets(username, sizeof(username), stdin);
                printf("Password: ");
                fgets(password, sizeof(password), stdin);

                username[strcspn(username, "\r\n")] = '\0';
                password[strcspn(password, "\r\n")] = '\0';

                snprintf(line, sizeof(line), "REGISTER %s %s\n", username, password);
                client_send_line(sockfd, line);
                if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                    printf("Server closed connection\n");
                    break;
                }
                printf("SERVER: %s", resp);
            } else if (choice == 2) {
                printf("Username: ");
                fgets(username, sizeof(username), stdin);
                printf("Password: ");
                fgets(password, sizeof(password), stdin);

                username[strcspn(username, "\r\n")] = '\0';
                password[strcspn(password, "\r\n")] = '\0';

                snprintf(line, sizeof(line), "LOGIN %s %s\n", username, password);
                client_send_line(sockfd, line);
                if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                    printf("Server closed connection\n");
                    break;
                }
                printf("SERVER: %s", resp);
                if (strstr(resp, "OK LOGIN LOGIN_OK") != NULL) {
                    logged_in = 1;
                    printf("Login successful! Welcome!\n");
                } else {
                    logged_in = 0;
                }
            } else {
                printf("Invalid choice. Please choose 1 or 2.\n");
            }
        } else {
            // Menu khi đã login
            if (choice == 1) {
                // SEARCH_MOVIE
                printf("Keyword: ");
                fgets(keyword, sizeof(keyword), stdin);
                keyword[strcspn(keyword, "\r\n")] = '\0';
            
                // Gửi request SEARCH_MOVIE
                snprintf(line, sizeof(line), "SEARCH_MOVIE %s\n", keyword);
                client_send_line(sockfd, line);
            
                // Đọc dòng đầu tiên
                if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                    printf("Server closed connection\n");
                    break;
                }
            
                // Trim response
                char *trimmed_resp = resp;
                while (*trimmed_resp == ' ' || *trimmed_resp == '\t') trimmed_resp++;
                size_t len = strlen(trimmed_resp);
                while (len > 0 && (trimmed_resp[len-1] == ' ' || trimmed_resp[len-1] == '\t' || 
                                   trimmed_resp[len-1] == '\r' || trimmed_resp[len-1] == '\n')) {
                    trimmed_resp[--len] = '\0';
                }
            
                // Luôn in ra response đầu tiên
                printf("SERVER: %s\n", trimmed_resp);
                fflush(stdout);
            
                // 1) Không đăng nhập (không nên xảy ra vì đã login)
                if (strncmp(trimmed_resp, "ERR SEARCH_MOVIE NOT_AUTHENTICATED", 34) == 0) {
                    printf("You must login first.\n");
                    logged_in = 0; // Reset login status
                    continue;
                }
            
                // 2) Lỗi khác (NO_PERMISSION, INVALID_ARGS, ...)
                if (strncmp(trimmed_resp, "ERR SEARCH_MOVIE", 17) == 0) {
                    continue;
                }
            
                // 3) Không có phim
                if (strstr(trimmed_resp, "OK SEARCH_MOVIE EMPTY") != NULL) {
                    // Server sẽ còn gửi "END\n" => phải đọc để không bị sót
                    if (client_recv_line(sockfd, resp, sizeof(resp)) > 0) {
                        printf("SERVER: %s", resp);
                    }
                    printf("No movie found.\n");
                    continue;
                }
            
                // 4) Có phim: FOUND N
                if (strstr(trimmed_resp, "OK SEARCH_MOVIE FOUND") != NULL) {
                    // Parse số lượng phim
                    int movie_count = 0;
                    sscanf(trimmed_resp, "OK SEARCH_MOVIE FOUND %d", &movie_count);
                    
                    // Đọc và hiển thị tất cả MOVIE lines ngay lập tức
                    while (1) {
                        if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                            printf("Server closed connection\n");
                            break;
                        }
                
                        // Khi server gửi "END\n" thì kết thúc list
                        if (strncmp(resp, "END", 3) == 0) {
                            printf("SERVER: %s", resp);
                            break;
                        }
                
                        // resp dạng: MOVIE <id> <title> <genre> <duration>
                        printf("SERVER: %s", resp);
                        fflush(stdout);
                    }
                }
            } else if (choice == 2) {
                // LIST_MOVIE
                int sub;
                char genre[64];
                char cinema[64];
                char date[16], from[16], to[16];
                char param[128];

                printf("Filter type:\n");
                printf(" 1. GENRE\n");
                printf(" 2. CINEMA\n");
                printf(" 3. TIMESLOT\n");
                printf("Choose: ");
                if (scanf("%d", &sub) != 1) break;
                while (getchar() != '\n');

                if (sub == 1) {
                    printf("Genre: ");
                    fgets(genre, sizeof(genre), stdin);
                    genre[strcspn(genre, "\r\n")] = '\0';
                    snprintf(param, sizeof(param), "GENRE %s", genre);
                } else if (sub == 2) {
                    printf("Cinema ID: ");
                    fgets(cinema, sizeof(cinema), stdin);
                    cinema[strcspn(cinema, "\r\n")] = '\0';
                    snprintf(param, sizeof(param), "CINEMA %s", cinema);
                } else if (sub == 3) {
                    printf("Date (YYYY-MM-DD): ");
                    fgets(date, sizeof(date), stdin);
                    date[strcspn(date, "\r\n")] = '\0';
                    printf("From time (HH:MM): ");
                    fgets(from, sizeof(from), stdin);
                    from[strcspn(from, "\r\n")] = '\0';
                    printf("To time (HH:MM): ");
                    fgets(to, sizeof(to), stdin);
                    to[strcspn(to, "\r\n")] = '\0';
                    // format: YYYY-MM-DD_HH:MM-HH:MM
                    snprintf(param, sizeof(param), "TIMESLOT %s_%s-%s", date, from, to);
                } else {
                    printf("Invalid filter type\n");
                    continue;
                }

                // Gửi: LIST_MOVIE <FILTER> <value>
                snprintf(line, sizeof(line), "LIST_MOVIE %s\n", param);
                client_send_line(sockfd, line);

                // Đọc dòng đầu
                if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                    printf("Server closed connection\n");
                    break;
                }
                
                // Trim response
                char *trimmed_resp = resp;
                while (*trimmed_resp == ' ' || *trimmed_resp == '\t') trimmed_resp++;
                size_t len = strlen(trimmed_resp);
                while (len > 0 && (trimmed_resp[len-1] == ' ' || trimmed_resp[len-1] == '\t' || 
                                   trimmed_resp[len-1] == '\r' || trimmed_resp[len-1] == '\n')) {
                    trimmed_resp[--len] = '\0';
                }
                
                // Luôn in ra response đầu tiên
                printf("SERVER: %s\n", trimmed_resp);
                fflush(stdout);

                // 1) Không đăng nhập (không nên xảy ra)
                if (strncmp(trimmed_resp, "ERR LIST_MOVIE NOT_AUTHENTICATED", 33) == 0) {
                    printf("You must login first.\n");
                    logged_in = 0; // Reset login status
                    continue;
                }
                
                // 2) Lỗi khác (INVALID_ARGS, NO_PERMISSION, ...)
                if (strncmp(trimmed_resp, "ERR LIST_MOVIE", 14) == 0) {
                    continue;
                }
                
                // 3) Không có phim
                if (strstr(trimmed_resp, "OK LIST_MOVIE EMPTY") != NULL) {
                    // Đọc END
                    if (client_recv_line(sockfd, resp, sizeof(resp)) > 0) {
                        printf("SERVER: %s", resp);
                    }
                    printf("No movies for this filter.\n");
                    continue;
                }
                
                // 4) Có phim: FOUND N
                if (strstr(trimmed_resp, "OK LIST_MOVIE FOUND") != NULL) {
                    // Parse số lượng phim
                    int movie_count = 0;
                    sscanf(trimmed_resp, "OK LIST_MOVIE FOUND %d", &movie_count);
                    
                    // Đọc và hiển thị tất cả MOVIE lines ngay lập tức
                    while (1) {
                        if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                            printf("Server closed connection\n");
                            break;
                        }
                        
                        // Khi server gửi "END\n" thì kết thúc list
                        if (strncmp(resp, "END", 3) == 0) {
                            printf("SERVER: %s", resp);
                            break;
                        }
                        
                        // resp dạng: MOVIE <id> <title> <genre> <duration>
                        printf("SERVER: %s", resp);
                        fflush(stdout);
                    }
                }
                // Nếu nhận được MOVIE line mà không có header, có thể là lỗi protocol
                else if (strncmp(trimmed_resp, "MOVIE", 5) == 0) {
                    // Đã nhận MOVIE line nhưng không có header FOUND trước đó
                    printf("(Received MOVIE line without header - possible protocol error)\n");
                    // Vẫn tiếp tục đọc các dòng còn lại để không làm hỏng protocol
                    while (1) {
                        if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                            break;
                        }
                        if (strncmp(resp, "END", 3) == 0) {
                            printf("SERVER: %s", resp);
                            break;
                        }
                        printf("SERVER: %s", resp);
                    }
                }
            } else if (choice == 3) {
                // QUIT
                snprintf(line, sizeof(line), "QUIT\n");
                client_send_line(sockfd, line);
                if (client_recv_line(sockfd, resp, sizeof(resp)) > 0) {
                    printf("SERVER: %s", resp);
                }
                break;
            } else {
                printf("Invalid choice. Please choose 1, 2, or 3.\n");
            }
        }
    }

    close(sockfd);
    return 0;
}