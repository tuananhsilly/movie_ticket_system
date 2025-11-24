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
