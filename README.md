# Network Programming — Môi trường Ubuntu với Docker Compose

## Yêu cầu

- [Docker Desktop](https://www.docker.com/products/docker-desktop/) đã được cài đặt và đang chạy
- Terminal (macOS: Terminal / iTerm2, Windows: PowerShell / WSL)

---

## Cấu trúc dự án

```
network-programming/
├── Dockerfile          # Định nghĩa image Ubuntu với các công cụ lập trình mạng
├── docker-compose.yml  # Cấu hình container
├── README.md
└── (các file bài tập .c, .cpp, ... sẽ nằm ở đây)
```

> Thư mục hiện tại được mount vào `/workspace` bên trong container — mọi file bạn tạo trong container đều xuất hiện ngay trên máy thật và ngược lại.

---

## Lần đầu tiên: Build và khởi động container

```bash
# Di chuyển vào thư mục dự án
cd network-programming

# Build image và khởi động container (chạy nền)
docker compose up -d --build
```

Lệnh này sẽ:
1. Tải Ubuntu 24.04
2. Cài `gcc`, `g++`, `gdb`, `make`, `cmake`, `git`, `vim`, `tcpdump`, `net-tools`, ...
3. Khởi động container tên `network-programming`

> Lần đầu mất vài phút do tải image. Các lần sau dùng cache nên rất nhanh.

---

## Vào trong container để code

```bash
docker compose exec ubuntu bash
```

Bạn sẽ thấy dấu nhắc lệnh bên trong container:

```
root@<container-id>:/workspace#
```

---

## Thực hiện bài tập trong container

### Tạo file bài tập

```bash
# Bên trong container
vim hello.c
```

Ví dụ nội dung `hello.c`:

```c
#include <stdio.h>
int main() {
    printf("Hello from Ubuntu container!\n");
    return 0;
}
```

### Biên dịch và chạy

```bash
gcc hello.c -o hello
./hello
```

### Bài tập lập trình mạng (socket)

```bash
# Biên dịch với thư viện socket
gcc server.c -o server
gcc client.c -o client

# Mở terminal thứ nhất — chạy server
./server

# Mở terminal thứ hai — vào container và chạy client
docker compose exec ubuntu bash
./client
```

### Dùng gdb để debug

```bash
gcc -g server.c -o server
gdb ./server
```

---

## Các lệnh thường dùng

| Lệnh | Mô tả |
|------|-------|
| `docker compose up -d --build` | Build và khởi động container |
| `docker compose exec ubuntu bash` | Mở shell vào container |
| `docker compose ps` | Xem trạng thái container |
| `docker compose stop` | Dừng container (giữ data) |
| `docker compose down` | Xóa container (giữ image) |
| `docker compose down --rmi all` | Xóa cả container lẫn image |

---

## Mở nhiều terminal vào cùng một container

Mỗi terminal chạy lệnh sau để vào cùng container:

```bash
docker compose exec ubuntu bash
```

Hữu ích khi chạy server ở terminal 1 và client ở terminal 2.

---

## Ghi chú

- File tạo trong `/workspace` bên trong container = file trong thư mục `network-programming/` trên máy thật.
- Không cần rebuild image khi chỉ thay đổi file `.c`/`.cpp` — chỉ rebuild khi sửa `Dockerfile`.
- Nếu container bị dừng, dùng `docker compose start` để khởi động lại mà không cần build lại.
