// file: src/client/main_client.c
#include "client.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include "../common/models.h"

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

// Add this helper function at the top of main_client.c, after the helper functions

/**
 * Print user-friendly message based on server response
 * Returns 1 if message was handled, 0 if not (should print raw response)
 */
 static int print_friendly_message(const char *response) {
    if (!response) return 0;
    
    // Trim response
    char resp_copy[512];
    strncpy(resp_copy, response, sizeof(resp_copy) - 1);
    resp_copy[sizeof(resp_copy) - 1] = '\0';
    
    // Remove trailing newlines
    size_t len = strlen(resp_copy);
    while (len > 0 && (resp_copy[len-1] == '\n' || resp_copy[len-1] == '\r')) {
        resp_copy[--len] = '\0';
    }
    
    // REGISTER responses
    if (strstr(resp_copy, "OK REGISTER USER_CREATED") != NULL) {
        printf("✓ Registration successful! You can now login.\n");
        return 1;
    }
    if (strstr(resp_copy, "ERR REGISTER USER_EXISTS") != NULL) {
        printf("✗ Registration failed: Username already exists.\n");
        return 1;
    }
    if (strstr(resp_copy, "ERR REGISTER") != NULL) {
        if (strstr(resp_copy, "INVALID_ARGS") != NULL) {
            printf("✗ Registration failed: Invalid username or password.\n");
        } else {
            printf("✗ Registration failed. Please try again.\n");
        }
        return 1;
    }
    
    // LOGIN responses
    if (strstr(resp_copy, "OK LOGIN LOGIN_OK") != NULL) {
        printf("✓ Login successful! Welcome!\n");
        return 1;
    }
    if (strstr(resp_copy, "ERR LOGIN INVALID_CREDENTIALS") != NULL) {
        printf("✗ Login failed: Invalid username or password.\n");
        return 1;
    }
    if (strstr(resp_copy, "ERR LOGIN") != NULL) {
        printf("✗ Login failed. Please try again.\n");
        return 1;
    }
    
    // ADD_MOVIE responses
    if (strstr(resp_copy, "OK ADD_MOVIE CREATED") != NULL) {
        uint32_t movie_id = 0;
        if (sscanf(resp_copy, "OK ADD_MOVIE CREATED %u", &movie_id) == 1) {
            printf("✓ Movie added successfully! Movie ID: %u\n", movie_id);
        } else {
            printf("✓ Movie added successfully!\n");
        }
        return 1;
    }
    if (strstr(resp_copy, "ERR ADD_MOVIE") != NULL) {
        if (strstr(resp_copy, "NOT_AUTHENTICATED") != NULL) {
            printf("✗ Please login first.\n");
        } else if (strstr(resp_copy, "NO_PERMISSION") != NULL) {
            printf("✗ Permission denied: Only Manager or Admin can add movies.\n");
        } else {
            printf("✗ Failed to add movie. Please check your input.\n");
        }
        return 1;
    }
    
    // ADD_SHOW responses
    if (strstr(resp_copy, "OK ADD_SHOW CREATED") != NULL) {
        uint32_t show_id = 0;
        if (sscanf(resp_copy, "OK ADD_SHOW CREATED %u", &show_id) == 1) {
            printf("✓ Show added successfully! Show ID: %u\n", show_id);
        } else {
            printf("✓ Show added successfully!\n");
        }
        return 1;
    }
    if (strstr(resp_copy, "ERR ADD_SHOW") != NULL) {
        if (strstr(resp_copy, "NOT_AUTHENTICATED") != NULL) {
            printf("✗ Please login first.\n");
        } else if (strstr(resp_copy, "NO_PERMISSION") != NULL) {
            printf("✗ Permission denied: Only Admin can add shows.\n");
        } else if (strstr(resp_copy, "SHOW_NOT_FOUND") != NULL) {
            printf("✗ Show not found.\n");
        } else {
            printf("✗ Failed to add show. Please check your input.\n");
        }
        return 1;
    }
    
    // UPDATE_SHOW responses
    if (strstr(resp_copy, "OK UPDATE_SHOW UPDATED") != NULL) {
        printf("✓ Show updated successfully!\n");
        return 1;
    }
    if (strstr(resp_copy, "ERR UPDATE_SHOW") != NULL) {
        if (strstr(resp_copy, "NOT_AUTHENTICATED") != NULL) {
            printf("✗ Please login first.\n");
        } else if (strstr(resp_copy, "NO_PERMISSION") != NULL) {
            printf("✗ Permission denied: Only Manager or Admin can update shows.\n");
        } else if (strstr(resp_copy, "SHOW_NOT_FOUND") != NULL) {
            printf("✗ Show not found.\n");
        } else if (strstr(resp_copy, "INVALID_ARGS") != NULL) {
            printf("✗ Invalid input. Please check date and time format (YYYY-MM-DD HH:MM-HH:MM).\n");
        } else {
            printf("✗ Failed to update show.\n");
        }
        return 1;
    }
    
    // CANCEL_SHOW responses
    if (strstr(resp_copy, "OK CANCEL_SHOW CANCELED") != NULL) {
        printf("✓ Show canceled successfully!\n");
        return 1;
    }
    if (strstr(resp_copy, "ERR CANCEL_SHOW") != NULL) {
        if (strstr(resp_copy, "NOT_AUTHENTICATED") != NULL) {
            printf("✗ Please login first.\n");
        } else if (strstr(resp_copy, "NO_PERMISSION") != NULL) {
            printf("✗ Permission denied: Only Manager or Admin can cancel shows.\n");
        } else if (strstr(resp_copy, "SHOW_NOT_FOUND") != NULL) {
            printf("✗ Show not found.\n");
        } else if (strstr(resp_copy, "HAS_BOOKINGS") != NULL) {
            printf("✗ Cannot cancel show: Show has existing bookings.\n");
        } else {
            printf("✗ Failed to cancel show.\n");
        }
        return 1;
    }
    
    // CREATE_USER responses
    if (strstr(resp_copy, "OK CREATE_USER USER_CREATED") != NULL) {
        printf("✓ User created successfully!\n");
        return 1;
    }
    if (strstr(resp_copy, "ERR CREATE_USER") != NULL) {
        if (strstr(resp_copy, "NOT_AUTHENTICATED") != NULL) {
            printf("✗ Please login first.\n");
        } else if (strstr(resp_copy, "NO_PERMISSION") != NULL) {
            printf("✗ Permission denied: Only Admin can create users.\n");
        } else if (strstr(resp_copy, "USER_EXISTS") != NULL) {
            printf("✗ Username already exists. Please choose another username.\n");
        } else {
            printf("✗ Failed to create user.\n");
        }
        return 1;
    }
    
    // GRANT_ROLE responses
    if (strstr(resp_copy, "OK GRANT_ROLE SUCCESS") != NULL) {
        printf("✓ Role granted successfully!\n");
        return 1;
    }
    if (strstr(resp_copy, "ERR GRANT_ROLE") != NULL) {
        if (strstr(resp_copy, "NOT_AUTHENTICATED") != NULL) {
            printf("✗ Please login first.\n");
        } else if (strstr(resp_copy, "NO_PERMISSION") != NULL) {
            printf("✗ Permission denied: Only Admin can grant roles.\n");
        } else if (strstr(resp_copy, "USER_NOT_FOUND") != NULL) {
            printf("✗ User not found.\n");
        } else if (strstr(resp_copy, "ROLE_INVALID") != NULL) {
            printf("✗ Invalid role. Role must be CUSTOMER, MANAGER, or ADMIN.\n");
        } else {
            printf("✗ Failed to grant role.\n");
        }
        return 1;
    }
    
    // REVOKE_ROLE responses
    if (strstr(resp_copy, "OK REVOKE_ROLE SUCCESS") != NULL) {
        printf("✓ Role revoked successfully!\n");
        return 1;
    }
    if (strstr(resp_copy, "ERR REVOKE_ROLE") != NULL) {
        if (strstr(resp_copy, "NOT_AUTHENTICATED") != NULL) {
            printf("✗ Please login first.\n");
        } else if (strstr(resp_copy, "NO_PERMISSION") != NULL) {
            printf("✗ Permission denied: Only Admin can revoke roles.\n");
        } else if (strstr(resp_copy, "USER_NOT_FOUND") != NULL) {
            printf("✗ User not found.\n");
        } else if (strstr(resp_copy, "ROLE_INVALID") != NULL) {
            printf("✗ Invalid role. Role must be CUSTOMER, MANAGER, or ADMIN.\n");
        } else {
            printf("✗ Failed to revoke role.\n");
        }
        return 1;
    }
    
    // BOOK_SEATS responses
    if (strstr(resp_copy, "OK BOOK_SEATS BOOKED") != NULL) {
        char *ticket_ids = strstr(resp_copy, "BOOKED");
        if (ticket_ids) {
            ticket_ids += 7; // Skip "BOOKED "
            printf("✓ Booking successful! Ticket ID(s): %s\n", ticket_ids);
        } else {
            printf("✓ Booking successful!\n");
        }
        return 1;
    }
    if (strstr(resp_copy, "ERR BOOK_SEATS") != NULL) {
        if (strstr(resp_copy, "NOT_AUTHENTICATED") != NULL) {
            printf("✗ Please login first.\n");
        } else if (strstr(resp_copy, "NO_PERMISSION") != NULL) {
            printf("✗ Permission denied: Only Customer can book seats.\n");
        } else if (strstr(resp_copy, "SHOW_NOT_FOUND") != NULL) {
            printf("✗ Show not found.\n");
        } else if (strstr(resp_copy, "SEAT_INVALID") != NULL) {
            printf("✗ Invalid seat. Row or column out of range.\n");
        } else if (strstr(resp_copy, "SEAT_TAKEN") != NULL) {
            printf("✗ Seat already booked. Please choose another seat.\n");
        } else if (strstr(resp_copy, "INVALID_ARGS") != NULL) {
            printf("✗ Invalid input. Please check your seat selection.\n");
        } else {
            printf("✗ Failed to book seats. Please try again.\n");
        }
        return 1;
    }
    
    // CANCEL_BOOKING responses
    if (strstr(resp_copy, "OK CANCEL_BOOKING CANCELED") != NULL) {
        printf("✓ Booking cancelled successfully!\n");
        return 1;
    }
    if (strstr(resp_copy, "ERR CANCEL_BOOKING") != NULL) {
        if (strstr(resp_copy, "NOT_AUTHENTICATED") != NULL) {
            printf("✗ Please login first.\n");
        } else if (strstr(resp_copy, "BOOKING_NOT_FOUND") != NULL) {
            printf("✗ Booking not found.\n");
        } else if (strstr(resp_copy, "NO_PERMISSION") != NULL) {
            printf("✗ You don't have permission to cancel this booking.\n");
        } else {
            printf("✗ Failed to cancel booking.\n");
        }
        return 1;
    }
    
    // SEARCH_MOVIE / LIST_MOVIE / LIST_SHOW - these are handled separately
    // as they have multi-line responses
    
    // GET_SEATS - handled separately as it has multi-line response
    
    // VIEW_BOOKINGS responses
    if (strstr(resp_copy, "OK VIEW_BOOKINGS EMPTY") != NULL) {
        printf("You have no bookings.\n");
        return 1;
    }
    if (strstr(resp_copy, "ERR VIEW_BOOKINGS") != NULL) {
        if (strstr(resp_copy, "NOT_AUTHENTICATED") != NULL) {
            printf("✗ Please login first.\n");
        } else {
            printf("✗ Failed to retrieve bookings.\n");
        }
        return 1;
    }
    
    return 0; // Not handled, print raw response
}

static void menu_not_logged_in() {
    printf("=== Movie Ticket Client ===\n");
    printf("1. REGISTER\n");
    printf("2. LOGIN\n");
    printf("Choose: ");
}

static void menu_manager_admin(){
    printf("=== Movie Ticket Client(Manager/Admin) ===\n");
    printf("1. LIST_MOVIE\n");
    printf("2. ADD_MOVIE\n");
    printf("3. ADD_SHOW\n");
    printf("4. UPDATE_SHOW\n");
    printf("5. CANCEL_SHOW\n");
    printf("6. LIST_SHOW\n");
    printf("7. GET_SEATS\n");
    printf("8. CREATE_USER\n");
    printf("9. GRANT_ROLE\n");
    printf("10. REVOKE_ROLE\n");
    printf("11. QUIT\n");
    printf("Choose: ");
}

static void menu_customer() {
    printf("=== Movie Ticket Client(Customer) ===\n");
    printf("1. SEARCH_MOVIE\n");
    printf("2. LIST_MOVIE\n");
    printf("3. LIST_SHOW\n");
    printf("4. GET_SEATS\n");
    printf("5. BOOK_SEATS\n");
    printf("6. VIEW_BOOKINGS\n");
    printf("7. CANCEL_BOOKING\n");
    printf("8. QUIT\n");
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
    uint32_t client_roles = 0;

    while (1) {
        // Hiển thị menu phù hợp với trạng thái
        if (logged_in) {
            // Kiểm tra role để hiển thị menu phù hợp
            if (client_roles & (ROLE_MANAGER | ROLE_ADMIN)) {
                menu_manager_admin();
            } else {
                menu_customer();
            }
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
                if (!print_friendly_message(resp)) {
                    printf("SERVER: %s", resp);
                }
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
                if (!print_friendly_message(resp)) {
                    printf("SERVER: %s", resp);
                }
                if (strstr(resp, "OK LOGIN LOGIN_OK") != NULL) {
                    logged_in = 1;
                    // Parse role từ response: "OK LOGIN LOGIN_OK CUSTOMER,MANAGER"
                    char *role_str = strstr(resp, "OK LOGIN LOGIN_OK");
                    if (role_str) {
                        role_str += strlen("OK LOGIN LOGIN_OK ");
                        // Trim whitespace
                        while (*role_str == ' ') role_str++;
                        client_roles = string_to_roles(role_str);
                    }
                } else {
                    logged_in = 0;
                    client_roles = 0;
                }
            } else {
                printf("Invalid choice. Please choose 1 or 2.\n");
            }
        } else {
            // Menu khi đã login
            if (client_roles & (ROLE_MANAGER | ROLE_ADMIN)) {
                // Menu manager/admin
                if (choice == 1) {
                    // LIST_MOVIE
                    char param[128] = "";
                    
                    printf("Press Enter for filtering by all: ");
                    fgets(param, sizeof(param), stdin);
                    param[strcspn(param, "\r\n")] = '\0';
                    
                    // Gửi: LIST_MOVIE <FILTER> [value]
                    snprintf(line, sizeof(line), "LIST_MOVIE %s\n", param);
                    client_send_line(sockfd, line);
                    
                    if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                        printf("Server closed connection\n");
                        break;
                    }
                    
                    char *trimmed_resp = resp;
                    while (*trimmed_resp == ' ' || *trimmed_resp == '\t') trimmed_resp++;
                    size_t len = strlen(trimmed_resp);
                    while (len > 0 && (trimmed_resp[len-1] == ' ' || trimmed_resp[len-1] == '\t' || 
                                       trimmed_resp[len-1] == '\r' || trimmed_resp[len-1] == '\n')) {
                        trimmed_resp[--len] = '\0';
                    }
                    
                    if (strncmp(trimmed_resp, "ERR LIST_MOVIE", 14) == 0) {
                        printf("✗ Failed to list movies.\n");
                        continue;
                    }
                    
                    if (strstr(trimmed_resp, "OK LIST_MOVIE EMPTY") != NULL) {
                        if (client_recv_line(sockfd, resp, sizeof(resp)) > 0) {
                            // Consume END
                        }
                        printf("No movies found.\n");
                        continue;
                    }
                    
                    if (strstr(trimmed_resp, "OK LIST_MOVIE FOUND") != NULL) {
                        int movie_count = 0;
                        sscanf(trimmed_resp, "OK LIST_MOVIE FOUND %d", &movie_count);
                        
                        printf("\n=== Found %d movie(s) ===\n", movie_count);
                        printf("%-6s %-30s %-15s %-5s \n", "ID", "Title", "Genre", "Duration");
                        printf("---------------------------------------------------------------------------------------------------\n");
                        
                        while (1) {
                            if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                                printf("Server closed connection\n");
                                break;
                            }
                            
                            if (strncmp(resp, "END", 3) == 0) {
                                break;
                            }
                            
                            unsigned int movie_id;
                            char title[128], genre[64], desc[256] = "";
                            int duration;
                            
                            if (sscanf(resp, "MOVIE %u %127s %63s %d[^\n]", 
                                       &movie_id, title, genre, &duration) >= 3) {
                                for (int i = 0; title[i]; i++) {
                                    if (title[i] == '_') title[i] = ' ';
                                }
                                for (int i = 0; genre[i]; i++) {
                                    if (genre[i] == '_') genre[i] = ' ';
                                }
                                printf("%-6u %-30.30s %-15s %-5d\n", 
                                       movie_id, title, genre, duration);
                            }
                        }
                        printf("---------------------------------------------------------------------------------------------------\n\n");
                    }
                } else if (choice == 3) {
                    // ADD_MOVIE
                    char title[128], genre[64], description[512];
                    int duration_min;
                    
                    printf("Title: ");
                    fgets(title, sizeof(title), stdin);
                    title[strcspn(title, "\r\n")] = '\0';
                    
                    printf("Genre: ");
                    fgets(genre, sizeof(genre), stdin);
                    genre[strcspn(genre, "\r\n")] = '\0';
                    
                    printf("Duration (minutes): ");
                    scanf("%d", &duration_min);
                    while (getchar() != '\n'); // clear stdin
                    
                    printf("Description: ");
                    fgets(description, sizeof(description), stdin);
                    description[strcspn(description, "\r\n")] = '\0';
                    
                    // Thay khoảng trắng bằng '_'
                    for (int i = 0; title[i]; i++) {
                        if (title[i] == ' ') title[i] = '_';
                    }
                    for (int i = 0; genre[i]; i++) {
                        if (genre[i] == ' ') genre[i] = '_';
                    }
                    for (int i = 0; description[i]; i++) {
                        if (description[i] == ' ') description[i] = '_';
                    }
                    
                    snprintf(line, sizeof(line), "ADD_MOVIE %s %s %d %s\n", 
                             title, genre, duration_min, description);
                    client_send_line(sockfd, line);
                    
                    if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                        printf("Server closed connection\n");
                        break;
                    }
                    if (!print_friendly_message(resp) && !strstr(resp, "END")) {
                        printf("SERVER: %s", resp);
                    } else if (strstr(resp, "END")) {
                        printf("Add movie successful!\n");
                        continue;
                    }
                } else if (choice == 2) {
                    // ADD_SHOW
                    uint32_t movie_id;
                    char cinema_id[64], room_id[64], date[32], start_time[32], end_time[32];
                    int rows, cols;
                    
                    printf("Movie ID: ");
                    scanf("%u", &movie_id);
                    while (getchar() != '\n'); // clear stdin
                    
                    printf("Cinema ID: ");
                    fgets(cinema_id, sizeof(cinema_id), stdin);
                    cinema_id[strcspn(cinema_id, "\r\n")] = '\0';
                    
                    printf("Room ID: ");
                    fgets(room_id, sizeof(room_id), stdin);
                    room_id[strcspn(room_id, "\r\n")] = '\0';
                    
                    printf("Date (YYYY-MM-DD): ");
                    fgets(date, sizeof(date), stdin);
                    date[strcspn(date, "\r\n")] = '\0';
                    
                    printf("Start Time (HH:MM): ");
                    fgets(start_time, sizeof(start_time), stdin);
                    start_time[strcspn(start_time, "\r\n")] = '\0';
                    
                    printf("End Time (HH:MM): ");
                    fgets(end_time, sizeof(end_time), stdin);
                    end_time[strcspn(end_time, "\r\n")] = '\0';
                    
                    printf("Rows: ");
                    scanf("%d", &rows);
                    printf("Cols: ");
                    scanf("%d", &cols);
                    while (getchar() != '\n'); // clear stdin
                    
                    snprintf(line, sizeof(line), "ADD_SHOW %u %s %s %s %s %s %d %d\n", 
                             movie_id, cinema_id, room_id, date, start_time, end_time, rows, cols);
                    client_send_line(sockfd, line);
                    
                    if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                        printf("Server closed connection\n");
                        break;
                    }
                    if (!print_friendly_message(resp)) {
                        printf("SERVER: %s", resp);
                    }
                    
                    // Server gửi thêm dòng "END" sau response, cần đọc và bỏ qua
                    if (client_recv_line(sockfd, resp, sizeof(resp)) > 0) {
                        // Silently consume END line
                    }
                } else if (choice == 3) {
                    // UPDATE_SHOW
                    char show_id_str[16], date[16], time_str[32];
                    
                    printf("Show ID: ");
                    fgets(show_id_str, sizeof(show_id_str), stdin);
                    show_id_str[strcspn(show_id_str, "\r\n")] = '\0';
                    
                    printf("New date (YYYY-MM-DD): ");
                    fgets(date, sizeof(date), stdin);
                    date[strcspn(date, "\r\n")] = '\0';
                    
                    printf("New time (HH:MM-HH:MM): ");
                    fgets(time_str, sizeof(time_str), stdin);
                    time_str[strcspn(time_str, "\r\n")] = '\0';
                    
                    snprintf(line, sizeof(line), "UPDATE_SHOW %s %s %s\n", 
                            show_id_str, date, time_str);
                    client_send_line(sockfd, line);
                    
                    if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                        printf("Server closed connection\n");
                        break;
                    }
                    if (!print_friendly_message(resp)) {
                        printf("SERVER: %s", resp);
                    }

                } else if (choice == 4) {
                    // CANCEL_SHOW  
                    char show_id_str[16];
                    
                    printf("Show ID: ");
                    fgets(show_id_str, sizeof(show_id_str), stdin);
                    show_id_str[strcspn(show_id_str, "\r\n")] = '\0';
                    
                    snprintf(line, sizeof(line), "CANCEL_SHOW %s\n", show_id_str);
                    client_send_line(sockfd, line);
                    
                    if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                        printf("Server closed connection\n");
                        break;
                    }
                    if (!print_friendly_message(resp)) {
                        printf("SERVER: %s", resp);
                    }
                } else if (choice == 5) {
                    // LIST_SHOW - Admin/Manager view all shows
                    char movie_id_str[16] = "";
                    char date_str[16] = "";
                    
                    printf("Movie ID (leave empty to view all shows): ");
                    fgets(movie_id_str, sizeof(movie_id_str), stdin);
                    movie_id_str[strcspn(movie_id_str, "\r\n")] = '\0';
                    
                    printf("Date (YYYY-MM-DD, optional, press Enter to skip): ");
                    fgets(date_str, sizeof(date_str), stdin);
                    date_str[strcspn(date_str, "\r\n")] = '\0';
                    
                    // Gửi request LIST_SHOW
                    if (strlen(movie_id_str) > 0) {
                        if (strlen(date_str) > 0) {
                            snprintf(line, sizeof(line), "LIST_SHOW %s %s\n", movie_id_str, date_str);
                        } else {
                            snprintf(line, sizeof(line), "LIST_SHOW %s\n", movie_id_str);
                        }
                    } else {
                        snprintf(line, sizeof(line), "LIST_SHOW\n");
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
                
                    // Handle errors friendly
                    if (strncmp(trimmed_resp, "ERR LIST_SHOW", 13) == 0) {
                        if (strstr(trimmed_resp, "NOT_AUTHENTICATED") != NULL) {
                            printf("✗ Please login first.\n");
                            logged_in = 0;
                            continue;
                        } else {
                            printf("✗ Failed to list shows.\n");
                            continue;
                        }
                    }
                    
                    // Không có suất chiếu
                    if (strstr(trimmed_resp, "OK LIST_SHOW EMPTY") != NULL) {
                        if (client_recv_line(sockfd, resp, sizeof(resp)) > 0) {
                            // Consume END
                        }
                        printf("No shows found.\n");
                        continue;
                    }
                    
                    // Có suất chiếu: FOUND N
                    if (strstr(trimmed_resp, "OK LIST_SHOW FOUND") != NULL) {
                        int show_count = 0;
                        sscanf(trimmed_resp, "OK LIST_SHOW FOUND %d", &show_count);
                        
                        printf("\n=== Found %d show(s) ===\n", show_count);
                        printf("%-8s %-10s %-12s %-12s %-12s %-8s\n", "Show ID", "Movie ID", "Cinema", "Room", "Date", "Time");
                        printf("-----------------------------------------------------------------------------------\n");
                        
                        while (1) {
                            if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                                printf("Server closed connection\n");
                                break;
                            }
                            
                            if (strncmp(resp, "END", 3) == 0) {
                                break;
                            }
                            
                            unsigned int show_id, movie_id;
                            char cinema_id[64];
                            char room_id[64];
                            char date[16];
                            char time[16];
                            
                            if (sscanf(resp, "SHOW %u %u %63s %63s %15s %15s", 
                                       &show_id, &movie_id, cinema_id, room_id, date, time) == 6) {
                                for (int i = 0; cinema_id[i]; i++) {
                                    if (cinema_id[i] == '_') cinema_id[i] = ' ';
                                }
                                for (int i = 0; room_id[i]; i++) {
                                    if (room_id[i] == '_') room_id[i] = ' ';
                                }
                                printf("%-8u %-10u %-12s %-12s %-12s %-8s\n", 
                                       show_id, movie_id, cinema_id, room_id, date, time);
                            }
                            fflush(stdout);
                        }
                        printf("-----------------------------------------------------------------------------------\n\n");
                    }
                } else if (choice == 6) {
                    // GET_SEATS
                    char show_id_str[16];
                    
                    printf("Show ID: ");
                    fgets(show_id_str, sizeof(show_id_str), stdin);
                    show_id_str[strcspn(show_id_str, "\r\n")] = '\0';
                    
                    snprintf(line, sizeof(line), "GET_SEATS %s\n", show_id_str);
                    client_send_line(sockfd, line);
                
                    if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                        printf("Server closed connection\n");
                        break;
                    }
                
                    char *trimmed_resp = resp;
                    while (*trimmed_resp == ' ' || *trimmed_resp == '\t') trimmed_resp++;
                    size_t len = strlen(trimmed_resp);
                    while (len > 0 && (trimmed_resp[len-1] == ' ' || trimmed_resp[len-1] == '\t' || 
                                       trimmed_resp[len-1] == '\r' || trimmed_resp[len-1] == '\n')) {
                        trimmed_resp[--len] = '\0';
                    }
                
                    if (strncmp(trimmed_resp, "ERR GET_SEATS", 13) == 0) {
                        if (strstr(trimmed_resp, "NOT_AUTHENTICATED") != NULL) {
                            printf("✗ Please login first.\n");
                            logged_in = 0;
                            continue;
                        } else if (strstr(trimmed_resp, "SHOW_NOT_FOUND") != NULL) {
                            printf("✗ Show not found.\n");
                            continue;
                        } else {
                            printf("✗ Failed to get seat map.\n");
                            continue;
                        }
                    }
                    
                    if (strstr(trimmed_resp, "OK GET_SEATS") != NULL) {
                        unsigned int show_id, movie_id = 0;
                        int rows, cols;
                        if (sscanf(trimmed_resp, "OK GET_SEATS %u %d %d %u", &show_id, &rows, &cols, &movie_id) >= 3) {
                            if (movie_id > 0) {
                                printf("\n=== Seat Map for Show %u (Movie: %u, %dx%d) ===\n", show_id, movie_id, rows, cols);
                            } else {
                                printf("\n=== Seat Map for Show %u (%dx%d) ===\n", show_id, rows, cols);
                            }
                        }
                        
                        char seat_map[20][20];
                        for (int r = 0; r < rows; r++) {
                            for (int c = 0; c < cols; c++) {
                                seat_map[r][c] = '?';
                            }
                        }
                        
                        while (1) {
                            if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                                printf("Server closed connection\n");
                                break;
                            }
                            
                            if (strncmp(resp, "END", 3) == 0) {
                                break;
                            }
                            
                            int row, col;
                            char status[16];
                            if (sscanf(resp, "SEAT %d %d %15s", &row, &col, status) == 3) {
                                if (row >= 1 && row <= rows && col >= 1 && col <= cols) {
                                    if (strcmp(status, "FREE") == 0) {
                                        seat_map[row - 1][col - 1] = 'O';
                                    } else if (strcmp(status, "BOOKED") == 0) {
                                        seat_map[row - 1][col - 1] = 'X';
                                    } else if (strcmp(status, "HELD") == 0) {
                                        seat_map[row - 1][col - 1] = 'H';
                                    } else {
                                        seat_map[row - 1][col - 1] = '?';
                                    }
                                }
                            }
                        }
                        
                        printf("\nSeat Layout (O=Free, X=Booked, H=Held):\n");
                        printf("   ");
                        for (int c = 1; c <= cols; c++) printf("%3d", c);
                        printf("\n");
                        
                        for (int r = 0; r < rows; r++) {
                            char row_label[4];
                            if (r + 1 <= 26) {
                                snprintf(row_label, sizeof(row_label), "%c", 'A' + r);
                            } else {
                                snprintf(row_label, sizeof(row_label), "%c%c", 'A' + r / 26 - 1, 'A' + r % 26);
                            }
                            printf("%2s |", row_label);
                            for (int c = 0; c < cols; c++) {
                                printf("%3c", seat_map[r][c]);
                            }
                            printf("|\n");
                        }
                        printf("\n");
                    }
                } else if (choice == 9) {
                    // CREATE_USER
                    char new_username[64], new_password[64];
                    
                    printf("New username: ");
                    fgets(new_username, sizeof(new_username), stdin);
                    new_username[strcspn(new_username, "\r\n")] = '\0';
                    
                    printf("New password: ");
                    fgets(new_password, sizeof(new_password), stdin);
                    new_password[strcspn(new_password, "\r\n")] = '\0';
                    
                    snprintf(line, sizeof(line), "CREATE_USER %s %s\n", new_username, new_password);
                    client_send_line(sockfd, line);
                    
                    if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                        printf("Server closed connection\n");
                        break;
                    }
                    if (!print_friendly_message(resp)) {
                        printf("SERVER: %s", resp);
                    }
                    
                } else if (choice == 9) {
                    // CREATE_USER
                    char target_username[64], role[32];
                    
                    printf("Username: ");
                    fgets(target_username, sizeof(target_username), stdin);
                    target_username[strcspn(target_username, "\r\n")] = '\0';
                    
                    printf("Role (CUSTOMER/MANAGER/ADMIN): ");
                    fgets(role, sizeof(role), stdin);
                    role[strcspn(role, "\r\n")] = '\0';
                    
                    snprintf(line, sizeof(line), "GRANT_ROLE %s %s\n", target_username, role);
                    client_send_line(sockfd, line);
                    
                    if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                        printf("Server closed connection\n");
                        break;
                    }
                    if (!print_friendly_message(resp)) {
                        printf("SERVER: %s", resp);
                    }
                    
                } else if (choice == 10) {
                    // GRANT_ROLE
                    char target_username[64], role[32];
                    
                    printf("Username: ");
                    fgets(target_username, sizeof(target_username), stdin);
                    target_username[strcspn(target_username, "\r\n")] = '\0';
                    
                    printf("Role (CUSTOMER/MANAGER/ADMIN): ");
                    fgets(role, sizeof(role), stdin);
                    role[strcspn(role, "\r\n")] = '\0';
                    
                    snprintf(line, sizeof(line), "GRANT_ROLE %s %s\n", target_username, role);
                    client_send_line(sockfd, line);
                    
                    if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                        printf("Server closed connection\n");
                        break;
                    }
                    if (!print_friendly_message(resp)) {
                        printf("SERVER: %s", resp);
                    }
                    
                } else if (choice == 11) {
                    // REVOKE_ROLE
                    char target_username[64], role[32];
                    
                    printf("Username: ");
                    fgets(target_username, sizeof(target_username), stdin);
                    target_username[strcspn(target_username, "\r\n")] = '\0';
                    
                    printf("Role (CUSTOMER/MANAGER/ADMIN): ");
                    fgets(role, sizeof(role), stdin);
                    role[strcspn(role, "\r\n")] = '\0';
                    
                    snprintf(line, sizeof(line), "REVOKE_ROLE %s %s\n", target_username, role);
                    client_send_line(sockfd, line);
                    
                    if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                        printf("Server closed connection\n");
                        break;
                    }
                    if (!print_friendly_message(resp)) {
                        printf("SERVER: %s", resp);
                    }
                    
                } else if (choice == 12) {
                    // QUIT
                } else {
                    printf("Invalid choice.\n");
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
            
                // Handle errors friendly
                if (strncmp(trimmed_resp, "ERR SEARCH_MOVIE", 17) == 0) {
                    if (strstr(trimmed_resp, "NOT_AUTHENTICATED") != NULL) {
                        printf("✗ Please login first.\n");
                        logged_in = 0;
                        continue;
                    } else if (strstr(trimmed_resp, "INVALID_ARGS") != NULL) {
                        printf("✗ Invalid search keyword.\n");
                        continue;
                    } else {
                        printf("✗ Search failed. Please try again.\n");
                        continue;
                    }
                }
            
                // 3) Không có phim
                if (strstr(trimmed_resp, "OK SEARCH_MOVIE EMPTY") != NULL) {
                    // Server sẽ còn gửi "END\n" => phải đọc để không bị sót
                    if (client_recv_line(sockfd, resp, sizeof(resp)) > 0) {
                        // Silently consume END
                    }
                    printf("No movies found matching your search.\n");
                    continue;
                }
            
                // 4) Có phim: FOUND N
                if (strstr(trimmed_resp, "OK SEARCH_MOVIE FOUND") != NULL) {
                    // Parse số lượng phim
                    int movie_count = 0;
                    sscanf(trimmed_resp, "OK SEARCH_MOVIE FOUND %d", &movie_count);
                    
                    printf("\n=== Found %d movie(s) ===\n", movie_count);
                    printf("%-6s %-30s %-12s %-6s %-40s\n", "ID", "Title", "Genre", "Min", "Description");
                    printf("----------------------------------------------------------------------------------------------------\n");
                    
                    // Đọc và hiển thị tất cả MOVIE lines
                    while (1) {
                        if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                            printf("Server closed connection\n");
                            break;
                        }
                
                        // Khi server gửi "END\n" thì kết thúc list
                        if (strncmp(resp, "END", 3) == 0) {
                            break; // Don't print END
                        }
                
                        // Parse MOVIE line: MOVIE <id> <title> <genre> <duration> <description>
                        unsigned int movie_id;
                        char title[128];
                        char genre[64];
                        char desc[256] = "";
                        int duration;
                        
                        if (sscanf(resp, "MOVIE %u %127s %63s %d %255s", &movie_id, title, genre, &duration, desc) >= 4) {
                            // Replace underscores with spaces for display
                            for (int i = 0; title[i]; i++) {
                                if (title[i] == '_') title[i] = ' ';
                            }
                            for (int i = 0; genre[i]; i++) {
                                if (genre[i] == '_') genre[i] = ' ';
                            }
                            for (int i = 0; desc[i]; i++) {
                                if (desc[i] == '_') desc[i] = ' ';
                            }
                            printf("%-6u %-30.30s %-12s %-6d %.40s\n", movie_id, title, genre, duration, desc);
                        }
                        fflush(stdout);
                    }
                    printf("----------------------------------------------------------------------------------------------------\n\n");
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
                printf(" 4. ALL\n");
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
                } else if (sub == 4) {
                    param[0] = '\0';
                }else {
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
                
                // Handle errors friendly
                if (strncmp(trimmed_resp, "ERR LIST_MOVIE", 14) == 0) {
                    if (strstr(trimmed_resp, "NOT_AUTHENTICATED") != NULL) {
                        printf("✗ Please login first.\n");
                        logged_in = 0;
                        continue;
                    } else if (strstr(trimmed_resp, "INVALID_ARGS") != NULL) {
                        printf("✗ Invalid filter parameters.\n");
                        continue;
                    } else {
                        printf("✗ Failed to list movies.\n");
                        continue;
                    }
                }
                
                // 3) Không có phim
                if (strstr(trimmed_resp, "OK LIST_MOVIE EMPTY") != NULL) {
                    // Đọc END
                    if (client_recv_line(sockfd, resp, sizeof(resp)) > 0) {
                        // Silently consume END
                    }
                    printf("No movies found for this filter.\n");
                    continue;
                }
                
                // 4) Có phim: FOUND N
                if (strstr(trimmed_resp, "OK LIST_MOVIE FOUND") != NULL) {
                    // Parse số lượng phim
                    int movie_count = 0;
                    sscanf(trimmed_resp, "OK LIST_MOVIE FOUND %d", &movie_count);
                    
                    printf("\n=== Found %d movie(s) ===\n", movie_count);
                    printf("%-6s %-30s %-12s %-6s %-40s\n", "ID", "Title", "Genre", "Min", "Description");
                    printf("----------------------------------------------------------------------------------------------------\n");
                    
                    // Đọc và hiển thị tất cả MOVIE lines
                    while (1) {
                        if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                            printf("Server closed connection\n");
                            break;
                        }
                        
                        // Khi server gửi "END\n" thì kết thúc list
                        if (strncmp(resp, "END", 3) == 0) {
                            break; // Don't print END
                        }
                        
                        // Parse MOVIE line: MOVIE <id> <title> <genre> <duration> <description>
                        unsigned int movie_id;
                        char title[128];
                        char genre[64];
                        char desc[256] = "";
                        int duration;
                        
                        if (sscanf(resp, "MOVIE %u %127s %63s %d %255s", &movie_id, title, genre, &duration, desc) >= 4) {
                            // Replace underscores with spaces for display
                            for (int i = 0; title[i]; i++) {
                                if (title[i] == '_') title[i] = ' ';
                            }
                            for (int i = 0; genre[i]; i++) {
                                if (genre[i] == '_') genre[i] = ' ';
                            }
                            for (int i = 0; desc[i]; i++) {
                                if (desc[i] == '_') desc[i] = ' ';
                            }
                            printf("%-6u %-30.30s %-12s %-6d %.40s\n", movie_id, title, genre, duration, desc);
                        }
                        fflush(stdout);
                    }
                    printf("----------------------------------------------------------------------------------------------------\n\n");
                }
            } else if (choice == 3) {
                // LIST_SHOW
                char movie_id_str[16] = "";
                char date_str[16] = "";
                
                printf("Movie ID: ");
                fgets(movie_id_str, sizeof(movie_id_str), stdin);
                movie_id_str[strcspn(movie_id_str, "\r\n")] = '\0';
                
                printf("Date (YYYY-MM-DD, optional, press Enter to skip): ");
                fgets(date_str, sizeof(date_str), stdin);
                date_str[strcspn(date_str, "\r\n")] = '\0';
                
                // Gửi request LIST_SHOW
                if (strlen(movie_id_str) > 0) {
                    // Customer: movie_id là bắt buộc
                    if (strlen(date_str) > 0) {
                        snprintf(line, sizeof(line), "LIST_SHOW %s %s\n", movie_id_str, date_str);
                    } else {
                        snprintf(line, sizeof(line), "LIST_SHOW %s\n", movie_id_str);
                    }
                } else {
                    // Admin/Manager: không cần movie_id, gửi lệnh trống
                    snprintf(line, sizeof(line), "LIST_SHOW\n");
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
            
                // Handle errors friendly
                if (strncmp(trimmed_resp, "ERR LIST_SHOW", 13) == 0) {
                    if (strstr(trimmed_resp, "NOT_AUTHENTICATED") != NULL) {
                        printf("✗ Please login first.\n");
                        logged_in = 0;
                        continue;
                    } else if (strstr(trimmed_resp, "INVALID_ARGS") != NULL) {
                        printf("✗ Invalid movie ID or date.\n");
                        continue;
                    } else {
                        printf("✗ Failed to list shows.\n");
                        continue;
                    }
                }
                
                // 3) Không có suất chiếu
                if (strstr(trimmed_resp, "OK LIST_SHOW EMPTY") != NULL) {
                    // Đọc END
                    if (client_recv_line(sockfd, resp, sizeof(resp)) > 0) {
                        // Silently consume END
                    }
                    printf("No shows found.\n");
                    continue;
                }
                
                // 4) Có suất chiếu: FOUND N
                if (strstr(trimmed_resp, "OK LIST_SHOW FOUND") != NULL) {
                    // Parse số lượng shows
                    int show_count = 0;
                    sscanf(trimmed_resp, "OK LIST_SHOW FOUND %d", &show_count);
                    
                    printf("\n=== Found %d show(s) ===\n", show_count);
                    printf("%-8s %-10s %-12s %-12s %-12s %-8s\n", "Show ID", "Movie ID", "Cinema", "Room", "Date", "Time");
                    printf("-----------------------------------------------------------------------------------\n");
                    
                    // Đọc và hiển thị tất cả SHOW lines
                    while (1) {
                        if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                            printf("Server closed connection\n");
                            break;
                        }
                        
                        // Khi server gửi "END\n" thì kết thúc list
                        if (strncmp(resp, "END", 3) == 0) {
                            break; // Don't print END
                        }
                        
                        // Parse SHOW line: SHOW <id> <movie_id> <cinema_id> <room_id> <date> <time>
                        unsigned int show_id, movie_id;
                        char cinema_id[64];
                        char room_id[64];
                        char date[16];
                        char time[16];
                        
                        if (sscanf(resp, "SHOW %u %u %63s %63s %15s %15s", 
                                   &show_id, &movie_id, cinema_id, room_id, date, time) == 6) {
                            // Replace underscores with spaces for display
                            for (int i = 0; cinema_id[i]; i++) {
                                if (cinema_id[i] == '_') cinema_id[i] = ' ';
                            }
                            for (int i = 0; room_id[i]; i++) {
                                if (room_id[i] == '_') room_id[i] = ' ';
                            }
                            printf("%-8u %-10u %-12s %-12s %-12s %-8s\n", 
                                   show_id, movie_id, cinema_id, room_id, date, time);
                        }
                        fflush(stdout);
                    }
                    printf("-----------------------------------------------------------------------------------\n\n");
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
            
                // Handle errors friendly
                if (strncmp(trimmed_resp, "ERR GET_SEATS", 13) == 0) {
                    if (strstr(trimmed_resp, "NOT_AUTHENTICATED") != NULL) {
                        printf("✗ Please login first.\n");
                        logged_in = 0;
                        continue;
                    } else if (strstr(trimmed_resp, "SHOW_NOT_FOUND") != NULL) {
                        printf("✗ Show not found.\n");
                        continue;
                    } else if (strstr(trimmed_resp, "INVALID_ARGS") != NULL) {
                        printf("✗ Invalid show ID.\n");
                        continue;
                    } else {
                        printf("✗ Failed to get seat map.\n");
                        continue;
                    }
                }
                
                // 3) Thành công: OK GET_SEATS <show_id> <rows> <cols> [movie_id]
                if (strstr(trimmed_resp, "OK GET_SEATS") != NULL) {
                    unsigned int show_id, movie_id = 0;
                    int rows, cols;
                    if (sscanf(trimmed_resp, "OK GET_SEATS %u %d %d %u", &show_id, &rows, &cols, &movie_id) >= 3) {
                        if (movie_id > 0) {
                            printf("\n=== Seat Map for Show %u (Movie: %u, %dx%d) ===\n", show_id, movie_id, rows, cols);
                        } else {
                            printf("\n=== Seat Map for Show %u (%dx%d) ===\n", show_id, rows, cols);
                        }
                        
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
                            printf("%2d ", c);
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
                        
                        printf("\nLegend: O = Available, X = Booked\n");
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
            
                if (!print_friendly_message(trimmed_resp)) {
                    printf("SERVER: %s\n", trimmed_resp);
                }
                fflush(stdout);
            } else if (choice == 6) {
                // VIEW_BOOKINGS
                snprintf(line, sizeof(line), "VIEW_BOOKINGS\n");
                client_send_line(sockfd, line);
            
                if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                    printf("Server closed connection\n");
                    break;
                }
            
                char *trimmed_resp = resp;
                while (*trimmed_resp == ' ' || *trimmed_resp == '\t') trimmed_resp++;
                size_t len = strlen(trimmed_resp);
                while (len > 0 && (trimmed_resp[len-1] == ' ' || trimmed_resp[len-1] == '\t' || 
                                   trimmed_resp[len-1] == '\r' || trimmed_resp[len-1] == '\n')) {
                    trimmed_resp[--len] = '\0';
                }
            
                if (!print_friendly_message(trimmed_resp)) {
                    printf("SERVER: %s\n", trimmed_resp);
                }
                
                // Kiểm tra nếu EMPTY
                if (strstr(trimmed_resp, "EMPTY") != NULL) {
                    printf("You have no bookings.\n");
                    continue;
                }
                
                // Đọc danh sách booking cho đến khi gặp "END"
                if (strstr(trimmed_resp, "FOUND") != NULL) {
                    int booking_count = 0;
                    sscanf(trimmed_resp, "OK VIEW_BOOKINGS FOUND %d", &booking_count);
                    printf("\n=== Your Bookings (%d) ===\n", booking_count);
                    printf("------------------------------------------------------------\n");
                    
                    while (1) {
                        if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                            printf("Server closed connection\n");
                            break;
                        }
                        
                        trimmed_resp = resp;
                        while (*trimmed_resp == ' ' || *trimmed_resp == '\t') trimmed_resp++;
                        len = strlen(trimmed_resp);
                        while (len > 0 && (trimmed_resp[len-1] == ' ' || trimmed_resp[len-1] == '\t' || 
                                          trimmed_resp[len-1] == '\r' || trimmed_resp[len-1] == '\n')) {
                            trimmed_resp[--len] = '\0';
                        }
                        
                        if (strcmp(trimmed_resp, "END") == 0) {
                            break; // Don't print END
                        }
                        
                        if (strncmp(trimmed_resp, "BOOKING", 7) == 0) {
                            // Parse: BOOKING <id> <show_id> <seat_count> <seat_list> <status> <booked_at>
                            unsigned int booking_id, show_id;
                            int seat_count;
                            char seat_list[256];
                            char status[16];
                            
                            if (sscanf(trimmed_resp, "BOOKING %u %u %d %255s %15s", 
                                      &booking_id, &show_id, &seat_count, seat_list, status) >= 4) {
                                printf("  Booking ID: %u | Show: %u | Seats: %d | Status: %s\n", 
                                       booking_id, show_id, seat_count, status);
                            } else {
                                printf("  %s\n", trimmed_resp);
                            }
                        }
                    }
                    printf("------------------------------------------------------------\n\n");
                }
            } else if (choice == 7) {
                // CANCEL_BOOKING
                char booking_id_str[16];
                
                printf("Booking ID: ");
                fgets(booking_id_str, sizeof(booking_id_str), stdin);
                booking_id_str[strcspn(booking_id_str, "\r\n")] = '\0';
                
                snprintf(line, sizeof(line), "CANCEL_BOOKING %s\n", booking_id_str);
                client_send_line(sockfd, line);
            
                if (client_recv_line(sockfd, resp, sizeof(resp)) <= 0) {
                    printf("Server closed connection\n");
                    break;
                }
            
                char *trimmed_resp = resp;
                while (*trimmed_resp == ' ' || *trimmed_resp == '\t') trimmed_resp++;
                size_t len = strlen(trimmed_resp);
                while (len > 0 && (trimmed_resp[len-1] == ' ' || trimmed_resp[len-1] == '\t' || 
                                   trimmed_resp[len-1] == '\r' || trimmed_resp[len-1] == '\n')) {
                    trimmed_resp[--len] = '\0';
                }
            
                if (!print_friendly_message(trimmed_resp)) {
                    printf("SERVER: %s\n", trimmed_resp);
                }
            } else if (choice == 8) {
                // QUIT
                snprintf(line, sizeof(line), "QUIT\n");
                client_send_line(sockfd, line);
                if (client_recv_line(sockfd, resp, sizeof(resp)) > 0) {
                    // Silently handle QUIT response
                }
                printf("Goodbye! Thank you for using Movie Ticket System.\n");
                break;
            } else {
                printf("Invalid choice. Please choose 1-8.\n");
            }
            }
        }
    }

    close(sockfd);
    return 0;
}