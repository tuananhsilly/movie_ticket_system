# Nguyên tắc chung của giao thức

## 0.1. Kiểu giao thức và định dạng bản tin

- **Kiểu giao thức:** client/server, request/response, chạy trên **TCP**.

- **Kiểu bản tin:** custom text-based.

- **Mỗi bản tin là 1 dòng ASCII**, kết thúc bằng `\n`.

- **Các trường cách nhau bởi đúng 1 dấu cách**.

- **Không dùng khoảng trắng trong field** → thay bằng `_` (underscore) trong dữ liệu text:

    `Avengers: Endgame` → `Avengers:_Endgame`.

## Định dạng bản tin tổng quát

### Client → Server (request)

```
<COMMAND> [arg1] [arg2] ...\n
```

Ví dụ:

```
LOGIN alice mypass
SEARCH_MOVIE Avengers
BOOK_SEATS 101 3 2 5 2 6 2 7
```

### Server → Client (response)

- Thành công:

```
OK <COMMAND> [STATUS] [data...]\n
```

- Thất bại:

```
ERR <COMMAND> <ERROR_CODE> [message...]\n
```

- Trường hợp trả **danh sách nhiều dòng** (movies, shows, seats):

```
OK <COMMAND> [STATUS/N]\n
<DATA_LINE_1>\n
<DATA_LINE_2>\n
...
END\n
```

Trong đó mỗi `DATA_LINE` có prefix như `MOVIE`, `SHOW`, `SEAT`, `USER`,…

> Server PHẢI log mọi bản tin:
> 
> - Khi nhận: `RECV <client_id> <raw_line>`
> - Khi gửi: `SEND <client_id> <raw_line>`

## 0.2. Các trạng thái session & role

Server lưu trạng thái cho mỗi client:

- `CONNECTED_NOT_LOGGED_IN`
- `LOGGED_IN_CUSTOMER`
- `LOGGED_IN_MANAGER`
- `LOGGED_IN_ADMIN`

(hoặc dạng bitmask roles)

`roles` có thể gồm:

- `CUSTOMER`
- `MANAGER` (người quản lý bán hàng)
- `ADMIN` (quản trị hệ thống)

Một user có thể có nhiều role (vd: `ADMIN` + `MANAGER`).

Server **kiểm tra role** trước khi xử lý các lệnh nhạy cảm.

## 0.3. Mã trạng thái & mã lỗi

### Một số `STATUS` thành công thường dùng

- `SUCCESS`
- `USER_CREATED`
- `LOGIN_OK`
- `FOUND`
- `EMPTY`
- `CREATED`
- `UPDATED`
- `CANCELED`
- `BOOKED`

### `ERROR_CODE` dùng trong bản tin `ERR`

- `INVALID_COMMAND` – lệnh không tồn tại / sai format.
- `NOT_AUTHENTICATED` – chưa đăng nhập mà gọi usecase cần login.
- `NO_PERMISSION` – không đủ quyền (vd: customer gọi ADD_MOVIE).
- `INVALID_ARGS` – sai số lượng/định dạng tham số.
- `USER_EXISTS` – username đã tồn tại.
- `USER_NOT_FOUND`
- `INVALID_CREDENTIAL` – sai username hoặc password.
- `MOVIE_NOT_FOUND`
- `SHOW_NOT_FOUND`
- `NO_MOVIES` – không tìm được phim phù hợp.
- `NO_SHOWS` – không có suất chiếu phù hợp.
- `SEAT_INVALID` – ghế không tồn tại.
- `SEAT_TAKEN` – ghế đã có người đặt.
- `ROLE_INVALID`
- `INTERNAL_ERROR` – lỗi server.
