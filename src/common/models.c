// file: src/common/models.c
#include "models.h"
#include <string.h>
#include <stdio.h>

void roles_to_string(uint32_t roles, char *out, int out_size) {
    out[0] = '\0';
    int first = 1;

    if (roles & ROLE_CUSTOMER) {
        strncat(out, "CUSTOMER", out_size - strlen(out) - 1);
        first = 0;
    }
    if (roles & ROLE_MANAGER) {
        if (!first) strncat(out, ",", out_size - strlen(out) - 1);
        strncat(out, "MANAGER", out_size - strlen(out) - 1);
        first = 0;
    }
    if (roles & ROLE_ADMIN) {
        if (!first) strncat(out, ",", out_size - strlen(out) - 1);
        strncat(out, "ADMIN", out_size - strlen(out) - 1);
    }
    if (out[0] == '\0') {
        // Không có role nào, fallback:
        strncat(out, "NONE", out_size - 1);
    }
}

uint32_t string_to_roles(const char *role_str) {
    uint32_t roles = 0;
    // Đơn giản: kiểm tra substring
    if (strstr(role_str, "CUSTOMER") != NULL) roles |= ROLE_CUSTOMER;
    if (strstr(role_str, "MANAGER")  != NULL) roles |= ROLE_MANAGER;
    if (strstr(role_str, "ADMIN")    != NULL) roles |= ROLE_ADMIN;
    return roles;
}
