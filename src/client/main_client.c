// file: src/client/main_client.c
#include "client.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 9000

static void menu() {
    printf("=== Movie Ticket Client ===\n");
    printf("1. REGISTER\n");
    printf("2. LOGIN\n");
    printf("3. SEARCH_MOVIE (require login)\n");
    printf("4. QUIT\n");
    printf("Choose: ");
}

int main() {
    int sockfd = client_connect("127.0.0.1", SERVER_PORT);
    if (sockfd < 0) {
        printf("Cannot connect to server\n");
        return 1;
    }

    int choice;
    char username[64];
    char password[64];
    char keyword[128];
    char line[256];
    char resp[512];

    int logged_in = 0; // client-side flag đơn giản

    while (1) {
        menu();
        if (scanf("%d", &choice) != 1) break;
        while (getchar() != '\n'); // clear stdin

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
            if (strncmp(resp, "OK LOGIN LOGIN_OK", 17) == 0) {
                logged_in = 1;
            } else {
                logged_in = 0;
            }
        } else if (choice == 3) {
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
        
            // Luôn in ra response đầu tiên để debug
            printf("SERVER: %s", resp);
            fflush(stdout);
        
            // 1) Không đăng nhập
            if (strncmp(resp, "ERR SEARCH_MOVIE NOT_AUTHENTICATED", 34) == 0) {
                printf("You must login first.\n");
                continue;
            }
        
            // 2) Lỗi khác (NO_PERMISSION, INVALID_ARGS, ...)
            if (strncmp(resp, "ERR SEARCH_MOVIE", 17) == 0) {
                continue;
            }
        
            // 3) Không có phim
            if (strncmp(resp, "OK SEARCH_MOVIE EMPTY", 22) == 0) {
                // Server sẽ còn gửi "END\n" => phải đọc để không bị sót
                if (client_recv_line(sockfd, resp, sizeof(resp)) > 0) {
                    printf("SERVER: %s", resp);
                }
                printf("No movie found.\n");
                continue;
            }
        
            // 4) Có phim: FOUND N - kiểm tra bằng strstr thay vì strncmp
            if (strstr(resp, "OK SEARCH_MOVIE FOUND") != NULL) {
                // Parse số lượng phim
                int movie_count = 0;
                sscanf(resp, "OK SEARCH_MOVIE FOUND %d", &movie_count);
                
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
                    fflush(stdout); // Flush sau mỗi dòng để đảm bảo hiển thị ngay
                }
            } else {
                // Trường hợp không khớp với bất kỳ điều kiện nào - có thể là response lạ
                printf("(Unexpected response format)\n");
            }
        } else if (choice == 4) {
            snprintf(line, sizeof(line), "QUIT\n");
            client_send_line(sockfd, line);
            if (client_recv_line(sockfd, resp, sizeof(resp)) > 0) {
                printf("SERVER: %s", resp);
            }
            break;
        } else {
            printf("Invalid choice\n");
        }
    }

    close(sockfd);
    return 0; 
}