// file: src/common/models.h
#ifndef MODELS_H
#define MODELS_H

#include <stdint.h>

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
    char password[PASSWORD_MAX_LEN]; // để đơn giản, không hash
    uint32_t roles; // bitmask
} User;

/**
 * Chuyển bitmask roles -> chuỗi role_list, ví dụ "CUSTOMER,MANAGER"
 */
void roles_to_string(uint32_t roles, char *out, int out_size);

/**
 * Parse chuỗi role list thành bitmask (không thực sự cần cho REGISTER/LOGIN,
 * nhưng để sẵn).
 */
uint32_t string_to_roles(const char *role_str);

// Model Movie
#define TITLE_MAX_LEN   128
#define GENRE_MAX_LEN   64

typedef struct {
    uint32_t id;
    char title[TITLE_MAX_LEN];    // dùng '_' thay ' ' khi gửi qua wire
    char genre[GENRE_MAX_LEN];
    int duration_min;
} Movie;

#endif
