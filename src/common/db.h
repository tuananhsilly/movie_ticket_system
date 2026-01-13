// file: src/common/db.h
#ifndef DB_H
#define DB_H

#include "models.h"


// USER 

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
 
 uint32_t db_get_user_id_by_username(const char *username);

 //MOVIE FUNCTIONS
 int db_load_movies(const char *movies_path);

 /**
  * Tìm phim theo keyword trong title (substring, không phân biệt hoa thường nếu bạn thích).
  * results: mảng để trả về
  * max_results: kích thước mảng
  * Trả số lượng phim tìm được (0..max_results).
  */
 int db_search_movies_by_title(const char *keyword, Movie *results, int max_results);

 /**
  * Lấy tất cả phim (dùng cho admin/manager xem danh sách).
  * results: mảng để trả về
  * max_results: kích thước mảng
  * Trả số lượng phim.
  */
 int db_get_all_movies(Movie *results, int max_results);

 //LIST MOVIE SHOW FUNCTIONS

 int db_load_shows(const char *shows_path);

 // Duyệt theo thể loại
int db_list_movies_by_genre(const char *genre, Movie *results, int max_results);
 
//Duyệt theo rạp: cinema_id: Trả về các movie có ít nhất 1 show ở cinema đó 
int db_list_movies_by_cinema(const char *cinema_id, Movie *results, int max_results);

/* Duyệt phim theo khung giờ: date + [HH:MM, HH:MM]
Return về các movie có ít nhất 1 show trong khoảng thời gian đó */
int db_list_movies_by_timeslot(const char *date, const char *from_time, const char *to_time, Movie *results, int max_results);

//LIST SHOW FUNCTIONS
int db_list_shows_by_movie(uint32_t movie_id, const char *date, Show *results, int max_results);

/**
 * Lấy toàn bộ shows từ database (dùng cho admin/manager).
 * Optional filtering by date and cinema_id (NULL để bỏ qua).
 * results: mảng để trả về
 * max_results: kích thước mảng
 * Trả số lượng shows tìm được (0..max_results).
 */
int db_get_all_shows(const char *date, const char *cinema_id, Show *results, int max_results);

// Find show by id, return pointer to Show struct if found, NULL otherwise
Show* db_find_show_by_id(uint32_t show_id); 

//Seat functions
// Load seats from csv file
// File path: data/seats/show_<show_id>_seats.csv
// Format: row,col,status,booking_id
// Return 1 if success, 0 if failed
int db_load_seats_for_show(uint32_t show_id, Seat *seats, int max_seats, int *out_rows, int *out_cols);

/**
 * Khởi tạo seats mặc định cho một show (nếu chưa có file).
 * Tạo file CSV với tất cả ghế là FREE.
 */

 int db_init_seats_for_show(uint32_t show_id, int rows, int cols);

/**
 * Lấy danh sách seats của một show.
 * results: mảng để trả về
 * max_results: kích thước mảng
 * out_rows, out_cols: số hàng và cột
 * Trả số lượng seats.
 */
int db_get_seats_for_show(uint32_t show_id, Seat *results, int max_results, int *out_rows, int *out_cols);

// BOOKING FUNCTIONS

/**
 * Tạo booking mới và update seats.
 * user_id: ID của user đặt vé
 * show_id: ID của show
 * seat_rows, seat_cols: mảng các hàng và cột ghế
 * seat_count: số lượng ghế
 * booking_id_out: trả về ID của booking mới tạo
 * Trả 0 nếu thành công, -1 nếu lỗi.
 */
 int db_create_booking(uint32_t user_id, uint32_t show_id, 
    const int *seat_rows, const int *seat_cols, int seat_count,
    uint32_t *booking_id_out);

/**
* Kiểm tra ghế có available không.
* Trả 0 nếu available, -1 nếu không available hoặc không tồn tại.
*/
int db_check_seat_available(uint32_t show_id, int row, int col);

/**
* Update seat status và booking_id.
* Trả 0 nếu thành công, -1 nếu lỗi.
*/
int db_update_seat_status(uint32_t show_id, int row, int col, const char *status, uint32_t booking_id);


// ADD MOVIE FUNCTIONS
int db_add_movie(const char *title, const char *genre, int duration_min, const char *description, uint32_t *movie_id_out);

// ADD SHOW FUNCTIONS
int db_add_show(uint32_t movie_id, const char *cinema_id, const char *room_id, 
                const char *date, const char *start_time, const char *end_time,
                int rows, int cols, uint32_t *show_id_out);

// ADMIN FUNCTIONS

/**
 * Cập nhật role của một user.
 * username: username của user cần update
 * new_roles: bitmask roles mới
 * Trả 0 nếu thành công, -1 nếu user không tồn tại hoặc lỗi.
 */
 int db_update_user_role(const char *username, uint32_t new_roles);

 /**
  * Thêm role vào user hiện có (không ghi đè, chỉ thêm).
  * username: username của user
  * role_to_add: role cần thêm (ROLE_CUSTOMER, ROLE_MANAGER, hoặc ROLE_ADMIN)
  * Trả 0 nếu thành công, -1 nếu lỗi.
  */
 int db_grant_role(const char *username, uint32_t role_to_add);
 
 /**
  * Gỡ role khỏi user hiện có.
  * username: username của user
  * role_to_remove: role cần gỡ (ROLE_CUSTOMER, ROLE_MANAGER, hoặc ROLE_ADMIN)
  * Trả 0 nếu thành công, -1 nếu lỗi.
  */
 int db_revoke_role(const char *username, uint32_t role_to_remove);

// UC5: VIEW_BOOKINGS & CANCEL_BOOKING

/**
 * Lấy danh sách booking của user.
 * user_id: ID của user
 * bookings: mảng Booking để trả về
 * max_bookings: kích thước mảng
 * Trả số lượng booking tìm được (0 nếu không có).
 */
int db_get_user_bookings(uint32_t user_id, Booking *bookings, int max_bookings);

/**
 * Hủy booking (đặt status = "cancelled" và đánh dấu ghế FREE).
 * booking_id: ID của booking cần hủy
 * user_id: ID của user (để kiểm tra ownership)
 * Trả 0 nếu thành công
 * Trả -1 nếu booking không tìm thấy
 * Trả -2 nếu user không sở hữu booking
 * Trả -3 nếu booking đã cancelled trước đó
 * Trả -4 nếu lỗi khác
 */
int db_cancel_booking(uint32_t booking_id, uint32_t user_id);

//UPDATE SHOW FUNCTIONS
int db_update_show_date_time(uint32_t show_id, const char *new_date, 
    const char *start_time, const char *end_time);

/**
* Check if a show has any confirmed bookings.
* Returns 1 if has bookings, 0 if no bookings, -1 on error.
*/
int db_has_show_bookings(uint32_t show_id);

/**
* Cancel (delete) a show.
* Only works if show has no bookings.
* Returns 0 on success, -1 on error.
*/
int db_cancel_show(uint32_t show_id);
#endif
