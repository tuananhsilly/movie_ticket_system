// file: src/common/models.h
#ifndef MODELS_H
#define MODELS_H

#include <stdint.h>
#include <time.h>

#define USERNAME_MAX_LEN 32
#define PASSWORD_MAX_LEN 64
#define ROLE_STRING_MAX_LEN 64

// Bitmask role
#define ROLE_CUSTOMER  (1u << 0)
#define ROLE_MANAGER   (1u << 1)
#define ROLE_ADMIN     (1u << 2)

typedef struct {
    uint32_t id;
    char username[USERNAME_MAX_LEN];
    char password[PASSWORD_MAX_LEN]; // xâu thông thường, không hashed 
    uint32_t roles; // bitmask
} User;


//   Chuyển bitmask roles -> chuỗi role_list, ví dụ "CUSTOMER,MANAGER"

void roles_to_string(uint32_t roles, char *out, int out_size);

// Parse chuỗi role list thành bitmask 
uint32_t string_to_roles(const char *role_str);

// Model Movie
#define TITLE_MAX_LEN   128
#define GENRE_MAX_LEN   64

typedef struct {
    uint32_t id;
    char title[TITLE_MAX_LEN];    // dùng '_' thay ' ' khi gửi qua giao thức
    char genre[GENRE_MAX_LEN];
    int duration_min;
} Movie;

//Model movie show (xuất chiếu)
#define CINEMA_ID_MAX_LEN 32
#define ROOM_ID_MAX_LEN 32
#define DATE_STR_LEN 16  // YYYY-MM-DD
#define TIME_STR_LEN 16   // HH:MM

typedef struct {
    uint32_t id;
    uint32_t movie_id;
    char cinema_id[CINEMA_ID_MAX_LEN];
    char room_id[ROOM_ID_MAX_LEN];
    char date[DATE_STR_LEN];
    char start_time[TIME_STR_LEN];
    char end_time[TIME_STR_LEN];
} Show;

#endif
