// file: src/common/db.h
#ifndef DB_H
#define DB_H

#include "models.h"

/**
 * Khởi tạo DB.
 * users_path: path tới file data/users.csv
 */
int db_init(const char *users_path);

/**
 * Thêm user mới.
 * Trả 0 nếu success, -1 nếu lỗi (vd: user tồn tại).
 */
int db_add_user(const char *username, const char *password, uint32_t roles);

/**
 * Tìm user theo username.
 * Nếu tìm được:
 *    - copy password vào out_password (đảm bảo đủ size).
 *    - nếu out_roles != NULL: gán roles.
 *    - trả 0.
 * Nếu không thấy: trả -1.
 */
int db_find_user(const char *username, char *out_password, int pw_size, uint32_t *out_roles);

//Add load and search movie 
/**
 * Load danh sách phim từ file CSV.
 * movies_path: "data/movies.csv"
 * Trả 0 nếu OK, -1 nếu lỗi.
 */
 int db_load_movies(const char *movies_path);

 /**
  * Tìm phim theo keyword trong title (substring, không phân biệt hoa thường nếu bạn thích).
  * results: mảng để trả về
  * max_results: kích thước mảng
  * Trả số lượng phim tìm được (0..max_results).
  */
 int db_search_movies_by_title(const char *keyword, Movie *results, int max_results);
 
#endif
