# Cập nhật LIST_SHOW và GET_SEAT cho Admin/Manager

## Tóm tắt thay đổi

Đã thêm khả năng cho role **ADMIN** và **MANAGER** quản lý hệ thống dễ dàng hơn thông qua hai lệnh mới:

### 1. **LIST_SHOW - Xem toàn bộ suất chiếu**

#### Cho Admin/Manager (không cần movie_id):
```
LIST_SHOW
```

**Phản hồi:**
```
OK LIST_SHOW FOUND <N>
SHOW <id> <movie_id> <cinema_id> <room_id> <date> <start_time>
SHOW <id> <movie_id> <cinema_id> <room_id> <date> <start_time>
...
END
```

#### Cho Customer (cần movie_id):
```
LIST_SHOW <movie_id>
LIST_SHOW <movie_id> <date>
```

### 2. **GET_SEATS - Xem chi tiết ghế của suất chiếu**

Admin/Manager có thể xem ghế của bất kỳ show nào:
```
GET_SEATS <show_id>
```

**Phản hồi:**
```
OK GET_SEATS <show_id> <rows> <cols> <movie_id>
SEAT <row> <col> <status>
SEAT <row> <col> <status>
...
END
```

Trạng thái ghế:
- `FREE`: Ghế trống, sẵn sàng đặt
- `BOOKED`: Ghế đã được đặt
- `HELD`: Ghế đang được giữ

---

## Chi tiết triển khai

### A. Backend Changes

#### 1. `src/common/db.h`
- Thêm hàm: `int db_get_all_shows(const char *date, const char *cinema_id, Show *results, int max_results);`
- Cho phép lấy toàn bộ shows với bộ lọc optional theo date và cinema_id

#### 2. `src/common/db.c`
- Triển khai `db_get_all_shows()` để duyệt toàn bộ shows từ database
- Hỗ trợ bộ lọc flexible

#### 3. `src/server/handlers/handler_show.c`

**handle_list_show():**
- Nếu request không có args và user là ADMIN/MANAGER: gọi `db_get_all_shows()` để lấy toàn bộ shows
- Nếu request có movie_id hoặc user là CUSTOMER: gọi `db_list_shows_by_movie()` (hành vi cũ)
- Response bây giờ gồm 6 trường: `SHOW <id> <movie_id> <cinema_id> <room_id> <date> <start_time>`

**handle_get_seats():**
- Thêm movie_id vào response header: `OK GET_SEATS <show_id> <rows> <cols> <movie_id>`
- Admin/Manager vẫn có thể xem seats của bất kỳ show nào

### B. Client Changes

#### `src/client/main_client.c`

**LIST_SHOW Section:**
- User có thể bỏ qua movie_id nếu là ADMIN/MANAGER
- Hiển thị thêm cột Movie ID trong bảng kết quả
- Format bảng: `Show ID | Movie ID | Cinema | Room | Date | Time`

**GET_SEATS Section:**
- Cập nhật parse response để nhận thêm movie_id
- Hiển thị: "Seat Map for Show X (Movie: Y, NxM)"

---

## Ví dụ sử dụng

### Admin xem toàn bộ shows:
```
LOGIN admin admin1
LIST_SHOW
```

Output:
```
=== Found 5 show(s) ===
Show ID  Movie ID Cinema       Room         Date         Time
-----------------------------------------------------------------------------------
101      10       CGV_01       ROOM_01      2024-05-01   19:30
102      11       CGV_01       ROOM_02      2024-05-01   21:30
103      12       LOTTE_01     ROOM_A       2024-05-02   20:00
104      11       36           abc          2025-6-11    19:30
105      14       366          xyz          2025-6-22    10:00
-----------------------------------------------------------------------------------
```

### Admin xem chi tiết ghế của show 101:
```
GET_SEATS 101
```

Output:
```
=== Seat Map for Show 101 (Movie: 10, 10x15) ===
```

### Customer vẫn phải cung cấp movie_id:
```
LIST_SHOW 10
```

---

## Testing

Đã test với:
- Admin user (role = 4) login và chạy `LIST_SHOW` → ✓ Lấy được toàn bộ 5 shows
- Admin user chạy `GET_SEATS 101` → ✓ Lấy được chi tiết 150 ghế (10x15)
- Response format đúng: movie_id được thêm vào các response

---

## Build & Run

```bash
cd movie_ticket_system
make clean
make
./build/server &
./build/client
```

Command test:
```bash
echo -e "LOGIN admin admin1\nLIST_SHOW\nQUIT" | nc localhost 9000
```
