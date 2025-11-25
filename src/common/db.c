// file: src/common/db.c
#include "db.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static char g_users_path[256] = "data/users.csv";

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

int db_add_user(const char *username, const char *password, uint32_t roles) {
    char pw_buf[PASSWORD_MAX_LEN];

    // Kiểm tra trùng username
    if (db_find_user(username, pw_buf, sizeof(pw_buf), NULL) == 0) {
        // đã tồn tại
        return -1;
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
        char *dur_str    = strtok(NULL, ",\n\r");

        if (!id_str || !title_str || !genre_str || !dur_str) continue;

        Movie m;
        m.id = (uint32_t)atoi(id_str);

        // copy title, genre, duration
        strncpy(m.title, title_str, sizeof(m.title) - 1);
        m.title[sizeof(m.title) - 1] = '\0';

        strncpy(m.genre, genre_str, sizeof(m.genre) - 1);
        m.genre[sizeof(m.genre) - 1] = '\0';

        m.duration_min = atoi(dur_str);

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