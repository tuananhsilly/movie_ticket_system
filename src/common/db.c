// file: src/common/db.c
#include "db.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>


static char g_users_path[256] = "data/users.csv";
static char g_bookings_path[256] = "data/bookings.csv";

//MOVIE STORAGE IN-MEMORY
#define MAX_MOVIES 1024
static Movie g_movies[MAX_MOVIES];
static int   g_movie_count = 0;

//SHOW STORAGE IN-MEMORY
#define MAX_SHOWS 2048
static Show  g_shows[MAX_SHOWS];
static int   g_show_count = 0;



int db_init(const char *users_path) {
    if (users_path) {
        strncpy(g_users_path, users_path, sizeof(g_users_path) - 1);
        g_users_path[sizeof(g_users_path) - 1] = '\0';
    }
    
    // Đảm bảo thư mục data tồn tại
    mkdir("data", 0755);
    mkdir("data/seats", 0755);
    
    // Nếu file chưa tồn tại, tạo file và ghi header đơn giản
    FILE *f = fopen(g_users_path, "r");
    if (!f) {
        f = fopen(g_users_path, "w");
        if (!f) {
            perror("db_init: cannot create users file");
            return -1;
        }
        // format: id,username,password,roles
        fprintf(f, "id,username,password,roles\n");
        fclose(f);
    } else {
        fclose(f);
    }
    
    // Khởi tạo bookings.csv nếu chưa có
    FILE *f_bookings = fopen(g_bookings_path, "r");
    if (!f_bookings) {
        f_bookings = fopen(g_bookings_path, "w");
        if (f_bookings) {
            // Format: id,user_id,show_id,seat_count,seat_list,status,booked_at
            fprintf(f_bookings, "id,user_id,show_id,seat_count,seat_list,status,booked_at\n");
            fclose(f_bookings);
        } else {
            perror("db_init: cannot create bookings file");
            // Không return error vì đây không phải critical
        }
    } else {
        fclose(f_bookings);
    }
    
    return 0;
}

//USER FUNCTIONS
static uint32_t db_get_next_user_id() {
    FILE *f = fopen(g_users_path, "r");
    if (!f) return 1;
    char line[256];
    uint32_t last_id = 0;
    // Bỏ dòng header
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return 1;
    }
    while (fgets(line, sizeof(line), f)) {
        char *token = strtok(line, ",");
        if (!token) continue;
        uint32_t id = (uint32_t)atoi(token);
        if (id > last_id) last_id = id;
    }
    fclose(f);
    return last_id + 1;
}

int db_find_user(const char *username, char *out_password, int pw_size, uint32_t *out_roles) {
    FILE *f = fopen(g_users_path, "r");
    if (!f) return -1;

    char line[256];
    // Bỏ header
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return -1;
    }

    while (fgets(line, sizeof(line), f)) {
        // id,username,password,roles
        char *id_str = strtok(line, ",");
        char *user_str = strtok(NULL, ",");
        char *pw_str = strtok(NULL, ",");
        char *roles_str = strtok(NULL, ",\n\r");

        if (!id_str || !user_str || !pw_str) continue;
        if (strcmp(user_str, username) == 0) {
            // copy password
            strncpy(out_password, pw_str, pw_size - 1);
            out_password[pw_size - 1] = '\0';
            if (out_roles && roles_str) {
                *out_roles = (uint32_t)strtoul(roles_str, NULL, 10);
            }
            fclose(f);
            return 0;
        }
    }

    fclose(f);
    return -1;
}

uint32_t db_get_user_id_by_username(const char *username) {
    FILE *f = fopen(g_users_path, "r");
    if (!f) return 0;

    char line[256];
    // Bỏ header
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return 0;
    }

    while (fgets(line, sizeof(line), f)) {
        // Tạo copy của line để strtok không modify buffer gốc
        char line_copy[256];
        strncpy(line_copy, line, sizeof(line_copy) - 1);
        line_copy[sizeof(line_copy) - 1] = '\0';
        
        // id,username,password,roles
        char *saveptr;
        char *id_str = strtok_r(line_copy, ",", &saveptr);
        char *user_str = strtok_r(NULL, ",", &saveptr);
        
        if (!id_str || !user_str) continue;
        if (strcmp(user_str, username) == 0) {
            uint32_t user_id = (uint32_t)atoi(id_str);
            fclose(f);
            return user_id;
        }
    }
    
    fclose(f);
    return 0;
}

int db_add_user(const char *username, const char *password, uint32_t roles) {
    char pw_buf[PASSWORD_MAX_LEN];

    // Kiểm tra trùng username
    if (db_find_user(username, pw_buf, sizeof(pw_buf), NULL) == 0) {
        // đã tồn tại
        return -1;
    }

    FILE *f_check = fopen(g_users_path, "r");
    if (f_check) {
        // Di chuyển đến cuối file
        fseek(f_check, 0, SEEK_END);
        long file_size = ftell(f_check);
        
        if (file_size > 0) {
            // Đọc ký tự cuối cùng
            fseek(f_check, file_size - 1, SEEK_SET);
            char last_char = fgetc(f_check);
            fclose(f_check);
            
            // Nếu ký tự cuối không phải \n, cần thêm \n
            if (last_char != '\n') {
                FILE *f_append_nl = fopen(g_users_path, "a");
                if (f_append_nl) {
                    fprintf(f_append_nl, "\n");
                    fclose(f_append_nl);
                }
            }
        } else {
            fclose(f_check);
        }
    }

    FILE *f = fopen(g_users_path, "a");
    if (!f) return -1;

    uint32_t new_id = db_get_next_user_id();

    // Ghi 1 dòng: id,username,password,roles
    fprintf(f, "%u,%s,%s,%u\n", new_id, username, password, roles);
    fclose(f);
    return 0;
}

// Movie functions 
int db_load_movies(const char *movies_path) {
    g_movie_count = 0;
    FILE *f = fopen(movies_path, "r");
    if (!f) {
        perror("db_load_movies: cannot open movies file");
        return -1;
    }

    char line[512];
    // Giả sử có header: id,title,genre,duration
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return 0;
    }

    while (fgets(line, sizeof(line), f)) {
        if (g_movie_count >= MAX_MOVIES) break;
        // id,title,genre,duration
        char *id_str     = strtok(line, ",");
        char *title_str  = strtok(NULL, ",");
        char *genre_str  = strtok(NULL, ",");
        char *dur_str    = strtok(NULL, ",");
        char *desc_str   = strtok(NULL, ",\n\r");
        if (!id_str || !title_str || !genre_str || !dur_str) continue;

        Movie m;
        m.id = (uint32_t)atoi(id_str);

        // copy title, genre, duration
        strncpy(m.title, title_str, sizeof(m.title) - 1);
        m.title[sizeof(m.title) - 1] = '\0';

        strncpy(m.genre, genre_str, sizeof(m.genre) - 1);
        m.genre[sizeof(m.genre) - 1] = '\0';

        m.duration_min = atoi(dur_str);

        if (desc_str) {
            strncpy(m.description, desc_str, sizeof(m.description) - 1);
            m.description[sizeof(m.description) - 1] = '\0';
        } else {
            m.description[0] = '\0';
        }

        g_movies[g_movie_count++] = m;
    }

    fclose(f);
    return 0;
}

static int title_contains_keyword(const char *title, const char *keyword) {
    // Đơn giản: substring, phân biệt hoa/thường.
    // Nếu muốn không phân biệt, có thể chuyển về lower trước.
    return (strstr(title, keyword) != NULL);
}

int db_search_movies_by_title(const char *keyword, Movie *results, int max_results) {
    int count = 0;
    for (int i = 0; i < g_movie_count && count < max_results; i++) {
        if (title_contains_keyword(g_movies[i].title, keyword)) {
            results[count++] = g_movies[i];
        }
    }
    return count;
}

// SHOW FUNCTIONS
int db_load_shows(const char *shows_path) {
    g_show_count = 0;
    FILE *f = fopen(shows_path, "r");
    if (!f) {
        perror("db_load_shows: cannot open shows file");
        return -1;
    }
    char line[512];
    //header: id,movie_id,cinema_id,room_id,date,time
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return 0;
    }
    while (fgets(line, sizeof(line), f)) {
        if (g_show_count >= MAX_SHOWS) break;

        // id,movie_id,cinema_id,room_id,date,start_time,end_time
        char *id_str = strtok(line, ",");
        char *movie_id_str = strtok(NULL, ",");
        char *cinema_str = strtok(NULL, ",");
        char *room_str = strtok(NULL, ",");
        char *date_str = strtok(NULL, ",");
        char *start_time_str = strtok(NULL, ",");
        char *end_time_str = strtok(NULL, ",\n\r");

        if (!id_str || !movie_id_str || !cinema_str || !room_str || 
            !date_str || !start_time_str || !end_time_str) continue;

        Show s;
        s.id = (uint32_t)atoi(id_str);
        s.movie_id = (uint32_t)atoi(movie_id_str);
    
            strncpy(s.cinema_id, cinema_str, sizeof(s.cinema_id) - 1);
        s.cinema_id[sizeof(s.cinema_id) - 1] = '\0';
    
        strncpy(s.room_id, room_str, sizeof(s.room_id) - 1);
        s.room_id[sizeof(s.room_id) - 1] = '\0';
    
        strncpy(s.date, date_str, sizeof(s.date) - 1);
        s.date[sizeof(s.date) - 1] = '\0';
    
        strncpy(s.start_time, start_time_str, sizeof(s.start_time) - 1);
        s.start_time[sizeof(s.start_time) - 1] = '\0';
    
        strncpy(s.end_time, end_time_str, sizeof(s.end_time) - 1);
        s.end_time[sizeof(s.end_time) - 1] = '\0';
    
            // Mặc định rows và cols (có thể đọc từ CSV sau nếu cần)
        s.rows = 10;  // Mặc định 10 hàng
        s.cols = 15;  // Mặc định 15 cột
    
        g_shows[g_show_count++] = s;
    }

    fclose(f);
    return 0;
}



static Movie *find_movie_by_id(uint32_t movie_id) {
    for (int i = 0; i < g_movie_count; i++) {
        if (g_movies[i].id == movie_id) {
            return &g_movies[i];
        }
    }
    return NULL;
}

//Tránh trùng lặp khi duyệt theo genre, cinema, timeslot

static void add_movie_if_not_exists(Movie *results, int *pcount, int max_results, const Movie *m) {
    if (*pcount >= max_results) return;
    for (int i = 0; i < *pcount; i++) {
        if (results[i].id == m->id) return;
    }
    results[*pcount] = *m;
    (*pcount)++;
}

//LIST MOVIE SHOW FUNCTIONS
int db_list_movies_by_genre(const char *genre, Movie *results, int max_results) {
    int count = 0;
    for (int i = 0; i < g_movie_count && count < max_results; i++) {
        if (strcmp(g_movies[i].genre, genre) == 0) {
            results[count++] = g_movies[i];
        }
    }
    return count;
}

int db_list_movies_by_cinema(const char *cinema_id, Movie *results, int max_results) {
    int count = 0;
    for (int i = 0; i < g_show_count; i++) {
        if (strcmp(g_shows[i].cinema_id, cinema_id) == 0) {
            Movie *m = find_movie_by_id(g_shows[i].movie_id);
            if (m) {
                add_movie_if_not_exists(results, &count, max_results, m);
            }
        }
    }
    return count;
}

//Kiểm tra overlap giữa 2 khoảng thời gian
static int timeslot_overlaps(const char *show_start, const char *show_end, 
    const char *req_from, const char *req_to) {
return (strcmp(show_start, req_to) <= 0 && strcmp(show_end, req_from) >= 0);
}

int db_list_movies_by_timeslot(const char *date, const char *from_time, const char *to_time,
      Movie *results, int max_results) {
    int count = 0;
    for (int i = 0; i < g_show_count; i++) {
        if (strcmp(g_shows[i].date, date) == 0 &&
            timeslot_overlaps(g_shows[i].start_time, g_shows[i].end_time, from_time, to_time)) {
        Movie *m = find_movie_by_id(g_shows[i].movie_id);
        if (m) {
            add_movie_if_not_exists(results, &count, max_results, m);
            }
        }
    }
    return count;
}

int db_list_shows_by_movie(uint32_t movie_id, const char *date, Show *results, int max_results) {
    int count = 0;
    for (int i = 0; i < g_show_count && count < max_results; i++) {
        // Kiểm tra movie_id khớp
        if (g_shows[i].movie_id != movie_id) continue;
        
        // Nếu có date filter, kiểm tra date
        if (date != NULL && strlen(date) > 0) {
            if (strcmp(g_shows[i].date, date) != 0) continue;
        }
        
        // Thêm vào results
        results[count++] = g_shows[i];
    }
    return count;
}

// SEAT FUNCTIONS
Show* db_find_show_by_id(uint32_t show_id) {
    for (int i = 0; i < g_show_count; i++) {
        if (g_shows[i].id == show_id) {
            return &g_shows[i];
        }
    }
    return NULL;
}

static void get_seats_file_path(uint32_t show_id, char *path, size_t path_size) {
    snprintf(path, path_size, "data/seats/show_%u_seats.csv", show_id);
}

int db_init_seats_for_show(uint32_t show_id, int rows, int cols) {
    char path[256];
    get_seats_file_path(show_id, path, sizeof(path));
    
    // Kiểm tra file đã tồn tại chưa
    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return 0; // Đã tồn tại, không cần tạo lại
    }
    
    // Tạo file mới với header
    f = fopen(path, "w");
    if (!f) {
        perror("db_init_seats_for_show: cannot create seats file");
        return -1;
    }
    
    fprintf(f, "row,col,status,booking_id\n");
    
    // Tạo tất cả ghế với status FREE
    for (int row = 1; row <= rows; row++) {
        for (int col = 1; col <= cols; col++) {
            fprintf(f, "%d,%d,FREE,0\n", row, col);
        }
    }
    
    fclose(f);
    return 0;
}

int db_load_seats_for_show(uint32_t show_id, Seat *seats, int max_seats, int *out_rows, int *out_cols) {
    char path[256];
    get_seats_file_path(show_id, path, sizeof(path));
    
    FILE *f = fopen(path, "r");
    if (!f) {
        // File không tồn tại, có thể cần khởi tạo
        return -1;
    }
    
    char line[256];
    // Bỏ header
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return 0;
    }
    
    int count = 0;
    int max_row = 0, max_col = 0;
    
    while (fgets(line, sizeof(line), f) && count < max_seats) {
        // Format: row,col,status,booking_id
        char *row_str = strtok(line, ",");
        char *col_str = strtok(NULL, ",");
        char *status_str = strtok(NULL, ",");
        char *booking_id_str = strtok(NULL, ",\n\r");
        
        if (!row_str || !col_str || !status_str) continue;
        
        Seat s;
        s.show_id = show_id;
        s.row = atoi(row_str);
        s.col = atoi(col_str);
        strncpy(s.status, status_str, sizeof(s.status) - 1);
        s.status[sizeof(s.status) - 1] = '\0';
        s.booking_id = booking_id_str ? (uint32_t)atoi(booking_id_str) : 0;
        
        if (s.row > max_row) max_row = s.row;
        if (s.col > max_col) max_col = s.col;
        
        seats[count++] = s;
    }
    
    fclose(f);
    
    if (out_rows) *out_rows = max_row;
    if (out_cols) *out_cols = max_col;
    
    return count;
}

int db_get_seats_for_show(uint32_t show_id, Seat *results, int max_results, int *out_rows, int *out_cols) {
    // Tìm show để lấy rows và cols mặc định
    Show *show = db_find_show_by_id(show_id);
    if (!show) {
        return -1; // Show không tồn tại
    }
    
    // Nếu show chưa có rows/cols, dùng giá trị mặc định
    int rows = show->rows > 0 ? show->rows : 10;
    int cols = show->cols > 0 ? show->cols : 15;
    
    // Khởi tạo seats nếu chưa có file
    db_init_seats_for_show(show_id, rows, cols);
    
    // Load seats từ file
    int count = db_load_seats_for_show(show_id, results, max_results, out_rows, out_cols);
    
    if (count < 0) {
        // File không tồn tại, trả về seats mặc định (tất cả FREE)
        count = 0;
        for (int row = 1; row <= rows && count < max_results; row++) {
            for (int col = 1; col <= cols && count < max_results; col++) {
                results[count].show_id = show_id;
                results[count].row = row;
                results[count].col = col;
                strcpy(results[count].status, "FREE");
                results[count].booking_id = 0;
                count++;
            }
        }
        if (out_rows) *out_rows = rows;
        if (out_cols) *out_cols = cols;
    }
    
    return count;
}

// BOOKING FUNCTIONS

// static char g_bookings_path[256] = "data/bookings.csv";

static uint32_t db_get_next_booking_id() {
    FILE *f = fopen(g_bookings_path, "r");
    if (!f) return 5001; // Bắt đầu từ 5001
    
    char line[512];
    uint32_t last_id = 5000;
    
    // Bỏ header
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return 5001;
    }
    
    while (fgets(line, sizeof(line), f)) {
        char *id_str = strtok(line, ",");
        if (!id_str) continue;
        uint32_t id = (uint32_t)atoi(id_str);
        if (id > last_id) last_id = id;
    }
    
    fclose(f);
    return last_id + 1;
}

int db_check_seat_available(uint32_t show_id, int row, int col) {
    Seat seats[200];
    int rows = 0, cols = 0;
    int count = db_get_seats_for_show(show_id, seats, 200, &rows, &cols);
    
    if (count < 0) return -1;
    
    // Kiểm tra row và col có hợp lệ không
    if (row < 1 || row > rows || col < 1 || col > cols) {
        return -1; // Out of range
    }
    
    // Tìm seat
    for (int i = 0; i < count; i++) {
        if (seats[i].row == row && seats[i].col == col) {
            if (strcmp(seats[i].status, "FREE") == 0) {
                return 0; // Available
            } else {
                return -1; // Already booked
            }
        }
    }
    
    return -1; // Not found
}

int db_update_seat_status(uint32_t show_id, int row, int col, const char *status, uint32_t booking_id) {
    char path[256];
    get_seats_file_path(show_id, path, sizeof(path));
    
    // Đọc toàn bộ file vào memory
    Seat seats[200];
    int rows = 0, cols = 0;
    int count = db_get_seats_for_show(show_id, seats, 200, &rows, &cols);
    
    if (count < 0) return -1;
    
    // Update seat trong memory
    int found = 0;
    for (int i = 0; i < count; i++) {
        if (seats[i].row == row && seats[i].col == col) {
            strncpy(seats[i].status, status, sizeof(seats[i].status) - 1);
            seats[i].status[sizeof(seats[i].status) - 1] = '\0';
            seats[i].booking_id = booking_id;
            found = 1;
            break;
        }
    }
    
    if (!found) return -1;
    
    // Ghi lại toàn bộ file
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    
    fprintf(f, "row,col,status,booking_id\n");
    for (int i = 0; i < count; i++) {
        fprintf(f, "%d,%d,%s,%u\n",
                seats[i].row, seats[i].col, seats[i].status, seats[i].booking_id);
    }
    
    fclose(f);
    return 0;
}

int db_create_booking(uint32_t user_id, uint32_t show_id,
                     const int *seat_rows, const int *seat_cols, int seat_count,
                     uint32_t *booking_id_out) {
    // 1. Kiểm tra tất cả ghế có available không
    for (int i = 0; i < seat_count; i++) {
        if (db_check_seat_available(show_id, seat_rows[i], seat_cols[i]) != 0) {
            return -1; // Có ghế không available
        }
    }
    
    // 2. Tạo booking ID mới
    uint32_t booking_id = db_get_next_booking_id();
    
    // 3. Update tất cả seats thành BOOKED
    for (int i = 0; i < seat_count; i++) {
        if (db_update_seat_status(show_id, seat_rows[i], seat_cols[i], "BOOKED", booking_id) != 0) {
            // Rollback: đặt lại các ghế đã update về FREE
            for (int j = 0; j < i; j++) {
                db_update_seat_status(show_id, seat_rows[j], seat_cols[j], "FREE", 0);
            }
            return -1;
        }
    }
    
    // 4. Ghi booking vào file bookings.csv
    FILE *f = fopen(g_bookings_path, "a");
    if (!f) {
        // Rollback seats
        for (int i = 0; i < seat_count; i++) {
            db_update_seat_status(show_id, seat_rows[i], seat_cols[i], "FREE", 0);
        }
        return -1;
    }
    
    // Format: id,user_id,show_id,seat_count,seat_list,status,booked_at
    // seat_list: "row1:col1,row2:col2,..."
    fprintf(f, "%u,%u,%u,%d,", booking_id, user_id, show_id, seat_count);
    
    for (int i = 0; i < seat_count; i++) {
        fprintf(f, "%d:%d", seat_rows[i], seat_cols[i]);
        if (i < seat_count - 1) fprintf(f, ",");
    }
    
    fprintf(f, ",confirmed,%ld\n", (long)time(NULL));
    fclose(f);
    
    if (booking_id_out) *booking_id_out = booking_id;
    return 0;
}

// ADD MOVIE FUNCTIONS
static uint32_t db_get_next_movie_id() {
    // Tìm movie ID lớn nhất trong g_movies[]
    uint32_t max_id = 0;
    for (int i = 0; i < g_movie_count; i++) {
        if (g_movies[i].id > max_id) {
            max_id = g_movies[i].id;
        }
    }
    return max_id + 1;
}

int db_add_movie(const char *title, const char *genre, int duration_min,
                 const char *description, uint32_t *movie_id_out) {
    // 1. Validate input
    if (!title || strlen(title) == 0 || !genre || duration_min <= 0) {
        return -1;
    }
    
    // 2. Tạo movie ID mới
    uint32_t new_id = db_get_next_movie_id();
    
    // 3. Thêm vào in-memory array
    if (g_movie_count >= MAX_MOVIES) {
        return -1; // Array đầy
    }
    
    Movie m;
    m.id = new_id;
    strncpy(m.title, title, sizeof(m.title) - 1);
    m.title[sizeof(m.title) - 1] = '\0';
    
    strncpy(m.genre, genre, sizeof(m.genre) - 1);
    m.genre[sizeof(m.genre) - 1] = '\0';
    
    m.duration_min = duration_min;
    
    if (description) {
        strncpy(m.description, description, sizeof(m.description) - 1);
        m.description[sizeof(m.description) - 1] = '\0';
    } else {
        m.description[0] = '\0';
    }
    
    g_movies[g_movie_count++] = m;
    
    // 4. Ghi vào file CSV
    FILE *f = fopen("data/movies.csv", "a");  // Append mode
    if (!f) {
        // Rollback: xóa khỏi array
        g_movie_count--;
        return -1;
    }
    
    // Format: id,title,genre,duration,description
    fprintf(f, "%u,%s,%s,%d,%s\n", m.id, m.title, m.genre, m.duration_min, m.description);
    fclose(f);
    
    if (movie_id_out) *movie_id_out = new_id;
    return 0;
}

// Thêm các hàm này vào db.c

int db_update_user_role(const char *username, uint32_t new_roles) {
    // Đọc toàn bộ file users.csv vào memory
    FILE *f = fopen(g_users_path, "r");
    if (!f) return -1;
    
    char lines[1000][256]; // Giả sử tối đa 1000 users
    int line_count = 0;
    int found = 0;
    
    // Đọc header
    if (!fgets(lines[line_count++], sizeof(lines[0]), f)) {
        fclose(f);
        return -1;
    }
    
    // Đọc tất cả các dòng
    while (fgets(lines[line_count], sizeof(lines[0]), f) && line_count < 1000) {
        char line_copy[256];
        strncpy(line_copy, lines[line_count], sizeof(line_copy) - 1);
        line_copy[sizeof(line_copy) - 1] = '\0';
        
        char *saveptr;
        char *id_str = strtok_r(line_copy, ",", &saveptr);
        char *user_str = strtok_r(NULL, ",", &saveptr);
        
        if (user_str && strcmp(user_str, username) == 0) {
            // Tìm thấy user, update role
            char *pw_str = strtok_r(NULL, ",", &saveptr);
            if (id_str && pw_str) {
                snprintf(lines[line_count], sizeof(lines[0]), 
                        "%s,%s,%s,%u\n", id_str, user_str, pw_str, new_roles);
                found = 1;
            }
        }
        line_count++;
    }
    fclose(f);
    
    if (!found) return -1;
    
    // Ghi lại toàn bộ file
    f = fopen(g_users_path, "w");
    if (!f) return -1;
    
    for (int i = 0; i < line_count; i++) {
        fprintf(f, "%s", lines[i]);
    }
    fclose(f);
    
    return 0;
}

int db_grant_role(const char *username, uint32_t role_to_add) {
    char pw_buf[PASSWORD_MAX_LEN];
    uint32_t current_roles = 0;
    
    // Lấy roles hiện tại
    if (db_find_user(username, pw_buf, sizeof(pw_buf), &current_roles) != 0) {
        return -1; // User không tồn tại
    }
    
    // Thêm role mới (bitwise OR)
    uint32_t new_roles = current_roles | role_to_add;
    
    return db_update_user_role(username, new_roles);
}

int db_revoke_role(const char *username, uint32_t role_to_remove) {
    char pw_buf[PASSWORD_MAX_LEN];
    uint32_t current_roles = 0;
    
    // Lấy roles hiện tại
    if (db_find_user(username, pw_buf, sizeof(pw_buf), &current_roles) != 0) {
        return -1; // User không tồn tại
    }
    
    // Gỡ role (bitwise AND với NOT)
    uint32_t new_roles = current_roles & (~role_to_remove);
    
    // Đảm bảo user luôn có ít nhất ROLE_CUSTOMER
    if (new_roles == 0) {
        new_roles = ROLE_CUSTOMER;
    }
    
    return db_update_user_role(username, new_roles);
}

// ADD SHOW FUNCTIONS
static uint32_t db_get_next_show_id() {
    // Tìm show ID lớn nhất trong g_shows[]
    uint32_t max_id = 0;
    for (int i = 0; i < g_show_count; i++) {
        if (g_shows[i].id > max_id) {
            max_id = g_shows[i].id;
        }
    }
    return max_id + 1;
}

int db_add_show(uint32_t movie_id, const char *cinema_id, const char *room_id, 
                const char *date, const char *start_time, const char *end_time,
                int rows, int cols, uint32_t *show_id_out) {
    // 1. Validate input
    if (movie_id == 0 || !cinema_id || strlen(cinema_id) == 0 || 
        !room_id || strlen(room_id) == 0 || !date || strlen(date) == 0 ||
        !start_time || strlen(start_time) == 0 || !end_time || strlen(end_time) == 0 ||
        rows <= 0 || cols <= 0) {
        return -1;
    }
    
    // 2. Kiểm tra movie có tồn tại không
    Movie *movie = find_movie_by_id(movie_id);
    if (!movie) {
        return -1; // Movie không tồn tại
    }
    
    // 3. Tạo show ID mới
    uint32_t new_id = db_get_next_show_id();
    
    // 4. Thêm vào in-memory array
    if (g_show_count >= MAX_SHOWS) {
        return -1; // Array đầy
    }
    
    Show s;
    s.id = new_id;
    s.movie_id = movie_id;
    strncpy(s.cinema_id, cinema_id, sizeof(s.cinema_id) - 1);
    s.cinema_id[sizeof(s.cinema_id) - 1] = '\0';
    
    strncpy(s.room_id, room_id, sizeof(s.room_id) - 1);
    s.room_id[sizeof(s.room_id) - 1] = '\0';
    
    strncpy(s.date, date, sizeof(s.date) - 1);
    s.date[sizeof(s.date) - 1] = '\0';
    
    strncpy(s.start_time, start_time, sizeof(s.start_time) - 1);
    s.start_time[sizeof(s.start_time) - 1] = '\0';
    
    strncpy(s.end_time, end_time, sizeof(s.end_time) - 1);
    s.end_time[sizeof(s.end_time) - 1] = '\0';
    
    s.rows = rows;
    s.cols = cols;
    
    g_shows[g_show_count++] = s;
    
    // 5. Ghi vào file CSV
    FILE *f = fopen("data/shows.csv", "a");  // Append mode
    if (!f) {
        // Rollback: xóa khỏi array
        g_show_count--;
        return -1;
    }
    
    // Format: id,movie_id,cinema_id,room_id,date,start_time,end_time
    fprintf(f, "%u,%u,%s,%s,%s,%s,%s\n", s.id, s.movie_id, s.cinema_id, s.room_id, 
            s.date, s.start_time, s.end_time);
    fclose(f);
    
    // 6. Khởi tạo seats cho show này
    db_init_seats_for_show(new_id, rows, cols);
    
    if (show_id_out) *show_id_out = new_id;
    return 0;
}