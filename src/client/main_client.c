// file: src/client/main_client.c
#include "client.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
// ... existing includes ...

#define SERVER_PORT 9000

// Helper function: Convert số thành chữ cái (1->A, 2->B, ..., 26->Z, 27->AA, ...)
static void row_to_letter(int row, char *out, size_t out_size) {
    if (row < 1) {
        strncpy(out, "?", out_size - 1);
        out[out_size - 1] = '\0';
        return;
    }
    
    // Simple: A-Z (1-26)
    if (row <= 26) {
        out[0] = 'A' + (row - 1);
        out[1] = '\0';
    } else {
        // AA, AB, ... (27+)
        int first = (row - 1) / 26;
        int second = (row - 1) % 26;
        snprintf(out, out_size, "%c%c", 'A' + first - 1, 'A' + second);
    }
}

// Helper function: Convert chữ cái thành số (A->1, B->2, ...)
static int letter_to_row(const char *letter) {
    if (!letter || strlen(letter) == 0) return 0;
    
    if (strlen(letter) == 1) {
        return letter[0] - 'A' + 1;
    } else {
        // AA, AB, ...
        int first = letter[0] - 'A' + 1;
        int second = letter[1] - 'A' + 1;
        return first * 26 + second;
    }
}

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
    printf("3. LIST_SHOW\n");
    printf("4. GET_SEATS\n");
    printf("5. BOOK_SEATS\n");
    printf("6. QUIT\n");
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
                // LIST_SHOW
                char movie_id_str[16];
                char date_str[16] = "";
                
                printf("Movie ID: ");
                fgets(movie_id_str, sizeof(movie_id_str), stdin);
                movie_id_str[strcspn(movie_id_str, "\r\n")] = '\0';
                
                printf("Date (YYYY-MM-DD, optional, press Enter to skip): ");
                fgets(date_str, sizeof(date_str), stdin);
                date_str[strcspn(date_str, "\r\n")] = '\0';
                
                // Gửi request LIST_SHOW
                if (strlen(date_str) > 0) {
                    snprintf(line, sizeof(line), "LIST_SHOW %s %s\n", movie_id_str, date_str);
                } else {
                    snprintf(line, sizeof(line), "LIST_SHOW %s\n", movie_id_str);
                }
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
            
                // 1) Không đăng nhập
                if (strncmp(trimmed_resp, "ERR LIST_SHOW NOT_AUTHENTICATED", 31) == 0) {
                    printf("You must login first.\n");
                    logged_in = 0;
                    continue;
                }
                
                // 2) Lỗi khác (INVALID_ARGS, ...)
                if (strncmp(trimmed_resp, "ERR LIST_SHOW", 13) == 0) {
                    continue;
                }
                
                // 3) Không có suất chiếu
                if (strstr(trimmed_resp, "OK LIST_SHOW EMPTY") != NULL) {
                    // Đọc END
                    if (client_recv_line(sockfd, resp, sizeof(resp)) > 0) {
                        printf("SERVER: %s", resp);
                    }
                    printf("No shows found for this movie.\n");
                    continue;
                }
                
                // 4) Có suất chiếu: FOUND N
                if (strstr(trimmed_resp, "OK LIST_SHOW FOUND") != NULL) {
                    // Parse số lượng shows
                    int show_count = 0;
                    sscanf(trimmed_resp, "OK LIST_SHOW FOUND %d", &show_count);
                    
                    printf("\n=== Found %d show(s) ===\n", show_count);
                    printf("%-8s %-12s %-12s %-12s %-8s\n", "Show ID", "Cinema", "Room", "Date", "Time");
                    printf("------------------------------------------------------------\n");
                    
                    // Đọc và hiển thị tất cả SHOW lines
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
                        
                        // Parse SHOW line: SHOW <id> <cinema_id> <room_id> <date> <time>
                        unsigned int show_id;
                        char cinema_id[64];
                        char room_id[64];
                        char date[16];
                        char time[16];
                        
                        if (sscanf(resp, "SHOW %u %63s %63s %15s %15s", 
                                   &show_id, cinema_id, room_id, date, time) == 5) {
                            // Replace underscores with spaces for display
                            for (int i = 0; cinema_id[i]; i++) {
                                if (cinema_id[i] == '_') cinema_id[i] = ' ';
                            }
                            for (int i = 0; room_id[i]; i++) {
                                if (room_id[i] == '_') room_id[i] = ' ';
                            }
                            printf("%-8u %-12s %-12s %-12s %-8s\n", 
                                   show_id, cinema_id, room_id, date, time);
                        } else {
                            printf("SERVER: %s", resp);
                        }
                        fflush(stdout);
                    }
                    printf("------------------------------------------------------------\n\n");
                }
            } else if (choice == 4) {
                // GET_SEATS
                char show_id_str[16];
                
                printf("Show ID: ");
                fgets(show_id_str, sizeof(show_id_str), stdin);
                show_id_str[strcspn(show_id_str, "\r\n")] = '\0';
                
                // Gửi request GET_SEATS
                snprintf(line, sizeof(line), "GET_SEATS %s\n", show_id_str);
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
            
                // 1) Không đăng nhập
                if (strncmp(trimmed_resp, "ERR GET_SEATS NOT_AUTHENTICATED", 31) == 0) {
                    printf("You must login first.\n");
                    logged_in = 0;
                    continue;
                }
                
                // 2) Lỗi khác (SHOW_NOT_FOUND, INVALID_ARGS, ...)
                if (strncmp(trimmed_resp, "ERR GET_SEATS", 13) == 0) {
                    continue;
                }
                
                // 3) Thành công: OK GET_SEATS <show_id> <rows> <cols>
                if (strstr(trimmed_resp, "OK GET_SEATS") != NULL) {
                    unsigned int show_id;
                    int rows, cols;
                    if (sscanf(trimmed_resp, "OK GET_SEATS %u %d %d", &show_id, &rows, &cols) == 3) {
                        printf("\n=== Seat Map for Show %u (%dx%d) ===\n", show_id, rows, cols);
                        
                        // Tạo mảng 2D để lưu seats
                        char seat_map[20][20]; // Max 20x20
                        // Khởi tạo với '?' (unknown)
                        for (int r = 0; r < rows; r++) {
                            for (int c = 0; c < cols; c++) {
                                seat_map[r][c] = '?';
                            }
                        }
                        
                        // Đọc tất cả SEAT lines
                        while (1) {
                            if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                                printf("Server closed connection\n");
                                break;
                            }
                            
                            // Khi server gửi "END\n" thì kết thúc list
                            if (strncmp(resp, "END", 3) == 0) {
                                break;
                            }
                            
                            // Parse SEAT line: SEAT <row> <col> <status>
                            int row, col;
                            char status[16];
                            if (sscanf(resp, "SEAT %d %d %15s", &row, &col, status) == 3) {
                                // Lưu vào map (row và col là 1-based)
                                if (row >= 1 && row <= rows && col >= 1 && col <= cols) {
                                    if (strcmp(status, "FREE") == 0) {
                                        seat_map[row - 1][col - 1] = 'O'; // O = Available
                                    } else if (strcmp(status, "BOOKED") == 0) {
                                        seat_map[row - 1][col - 1] = 'X'; // X = Booked
                                    } else if (strcmp(status, "HELD") == 0) {
                                        seat_map[row - 1][col - 1] = 'H'; // H = Held
                                    } else {
                                        seat_map[row - 1][col - 1] = '?';
                                    }
                                }
                            }
                        }
                        
                        // Hiển thị bản đồ ghế với row dạng chữ cái
                        printf("\n   ");
                        for (int c = 1; c <= cols; c++) {
                            printf("%3d ", c);
                        }
                        printf("\n");
                        
                        for (int r = 0; r < rows; r++) {
                            char row_letter[4];
                            row_to_letter(r + 1, row_letter, sizeof(row_letter));
                            printf("%-3s ", row_letter);
                            for (int c = 0; c < cols; c++) {
                                printf(" %c ", seat_map[r][c]);
                            }
                            printf("\n");
                        }
                        
                        printf("\nLegend: O = Available, X = Booked, H = Held\n");
                        printf("------------------------------------------------------------\n\n");
                    }
                }
            } else if (choice == 5) {
                // BOOK_SEATS
                char show_id_str[16];
                char seat_count_str[8];
                
                printf("Show ID: ");
                fgets(show_id_str, sizeof(show_id_str), stdin);
                show_id_str[strcspn(show_id_str, "\r\n")] = '\0';
                
                printf("Number of seats: ");
                fgets(seat_count_str, sizeof(seat_count_str), stdin);
                seat_count_str[strcspn(seat_count_str, "\r\n")] = '\0';
                
                int seat_count = atoi(seat_count_str);
                if (seat_count <= 0 || seat_count > 20) {
                    printf("Invalid seat count (1-20)\n");
                    continue;
                }
                
                // Nhập các ghế
                int seat_rows[20];
                int seat_cols[20];
                
                printf("Enter seats (format: Row Col, e.g., A 5):\n");
                for (int i = 0; i < seat_count; i++) {
                    char input[32];
                    char row_letter[8];
                    int col;
                    
                    printf("Seat %d: ", i + 1);
                    if (fgets(input, sizeof(input), stdin)) {
                        if (sscanf(input, "%7s %d", row_letter, &col) == 2) {
                            seat_rows[i] = letter_to_row(row_letter);
                            seat_cols[i] = col;
                            
                            if (seat_rows[i] <= 0 || seat_cols[i] <= 0) {
                                printf("Invalid seat format. Use format: A 5\n");
                                i--; // Retry
                                continue;
                            }
                        } else {
                            printf("Invalid format. Use: Row Col (e.g., A 5)\n");
                            i--; // Retry
                            continue;
                        }
                    }
                }
                
                // Gửi request BOOK_SEATS
                snprintf(line, sizeof(line), "BOOK_SEATS %s %d", show_id_str, seat_count);
                for (int i = 0; i < seat_count; i++) {
                    char temp[32];
                    snprintf(temp, sizeof(temp), " %d %d", seat_rows[i], seat_cols[i]);
                    strncat(line, temp, sizeof(line) - strlen(line) - 1);
                }
                strncat(line, "\n", sizeof(line) - strlen(line) - 1);
                
                client_send_line(sockfd, line);
            
                // Đọc response
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
            
                printf("SERVER: %s\n", trimmed_resp);
                fflush(stdout);
            
                // Xử lý response
                if (strncmp(trimmed_resp, "ERR BOOK_SEATS", 14) == 0) {
                    // Error đã được in ra
                    continue;
                }
                
                if (strstr(trimmed_resp, "OK BOOK_SEATS BOOKED") != NULL) {
                    // Parse ticket IDs
                    char *ticket_ids = strstr(trimmed_resp, "BOOKED");
                    if (ticket_ids) {
                        ticket_ids += 7; // Skip "BOOKED "
                        printf("Booking successful! Ticket IDs: %s\n", ticket_ids);
                    }
                }
            } else if (choice == 6) {
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