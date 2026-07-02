/* HW4 - Bai 2: TCP Server nhận file từ client
 * Protocol: client gửi [4-byte tên_file_len][tên_file][4-byte kích_thước][data]
 * Server lưu file vào thư mục "uploads/", báo lỗi nếu file đã tồn tại.
 * Kích thước file tối đa: ~4GB (dùng uint32_t)
 * Cú pháp: ./server <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>

#define UPLOAD_DIR "uploads"
#define BUF_SIZE   4096

/* Đọc đúng n byte từ fd, xử lý partial read của TCP */
static int read_all(int fd, char *buf, int n) {
    int total = 0, r;
    while (total < n) {
        r = read(fd, buf + total, n - total);
        if (r <= 0) return total > 0 ? total : r;  /* Trả về số đã đọc nếu đang đọc dở */
        total += r;
    }
    return total;
}

/* Ghi đúng n byte vào fd, xử lý partial write của TCP */
static int write_all(int fd, const char *buf, int n) {
    int total = 0, w;
    while (total < n) {
        w = write(fd, buf + total, n - total);
        if (w <= 0) return w;
        total += w;
    }
    return total;
}

/* Nhận chuỗi theo giao thức length-prefix 4 byte.
 * Dùng cho tên file (chuỗi ngắn có thể rỗng để báo hiệu kết thúc phiên). */
static int recv_str(int fd, char *buf, int maxlen) {
    uint32_t net_len;
    if (read_all(fd, (char *)&net_len, 4) != 4) return -1;
    int len = (int)ntohl(net_len);
    if (len >= maxlen) return -1;
    /* len == 0 là tín hiệu kết thúc từ client; không cần đọc thêm byte nào */
    if (len > 0 && read_all(fd, buf, len) != len) return -1;
    buf[len] = '\0';
    return len;
}

/* Gửi chuỗi thông báo theo giao thức length-prefix 4 byte */
static void send_str(int fd, const char *msg) {
    uint32_t net_len = htonl((uint32_t)strlen(msg));
    write_all(fd, (char *)&net_len, 4);
    write_all(fd, msg, (int)strlen(msg));
}

/* Lấy phần basename của đường dẫn để tránh path traversal attack.
 * Ví dụ: "../../etc/passwd" → "passwd"
 * Hỗ trợ cả dấu '/' (Unix) lẫn '\\' (Windows). */
static const char *get_basename(const char *path) {
    const char *base = path;
    for (int i = 0; path[i]; i++) {
        if (path[i] == '/' || path[i] == '\\')
            base = path + i + 1;
    }
    return base;
}

/* Xử lý một phiên gửi file từ client.
 * Mỗi lần lặp nhận một file; chuỗi tên file rỗng báo hiệu client kết thúc. */
static void handle_client(int cfd) {
    /* Tạo thư mục uploads nếu chưa tồn tại (0755 = rwxr-xr-x) */
    mkdir(UPLOAD_DIR, 0755);

    char filename[512];

    while (1) {
        /* Nhận tên file; chuỗi rỗng (len==0) báo hiệu client kết thúc phiên */
        int n = recv_str(cfd, filename, sizeof(filename));
        if (n <= 0) break;

        /* Nhận kích thước file (4 byte, big-endian) */
        uint32_t net_size;
        if (read_all(cfd, (char *)&net_size, 4) != 4) break;
        long fsize = (long)ntohl(net_size);

        /* Xây dựng đường dẫn lưu file; chỉ dùng basename để ngăn path traversal */
        char savepath[600];
        snprintf(savepath, sizeof(savepath), "%s/%s", UPLOAD_DIR, get_basename(filename));

        /* Kiểm tra file đã tồn tại chưa trước khi mở để ghi */
        struct stat st;
        if (stat(savepath, &st) == 0) {
            send_str(cfd, "Error: File is existent on server");
            /* Phải drain (đọc bỏ) đúng số byte dữ liệu file để không làm lệch protocol.
             * Nếu không drain, lần nhận tiếp theo sẽ đọc nhầm data của file này thành header. */
            char drain[BUF_SIZE];
            long rem = fsize;
            while (rem > 0) {
                int chunk = rem < BUF_SIZE ? (int)rem : BUF_SIZE;
                int r = read(cfd, drain, chunk);
                if (r <= 0) break;
                rem -= r;
            }
            continue;
        }

        /* Mở file để ghi nhị phân */
        FILE *fp = fopen(savepath, "wb");
        if (!fp) {
            send_str(cfd, "Error: Cannot create file on server");
            continue;
        }

        /* Nhận dữ liệu file theo từng chunk và ghi vào đĩa */
        char buf[BUF_SIZE];
        long rem = fsize;
        int ok = 1;
        while (rem > 0) {
            int chunk = rem < BUF_SIZE ? (int)rem : BUF_SIZE;
            int r = read(cfd, buf, chunk);
            if (r <= 0) { ok = 0; break; }     /* Kết nối bị ngắt giữa chừng */
            fwrite(buf, 1, r, fp);
            rem -= r;
        }
        fclose(fp);

        if (ok) {
            send_str(cfd, "Successful transfering");
            printf("Saved: %s (%ld bytes)\n", savepath, fsize);
        } else {
            /* Xóa file chưa hoàn chỉnh để tránh lưu file bị hỏng */
            unlink(savepath);
            send_str(cfd, "Error: File transfering is interrupted");
        }
    }

    close(cfd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    /* Tạo TCP socket */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    /* Cho phép tái sử dụng địa chỉ ngay sau khi server tắt */
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(sock); return 1;
    }
    listen(sock, 5);
    printf("File server listening on port %s (saving to ./%s/)\n", argv[1], UPLOAD_DIR);

    /* Vòng lặp chính: chấp nhận từng client và xử lý tuần tự */
    while (1) {
        struct sockaddr_in client;
        socklen_t clen = sizeof(client);
        int cfd = accept(sock, (struct sockaddr *)&client, &clen);
        if (cfd < 0) { perror("accept"); continue; }
        printf("Client connected: %s\n", inet_ntoa(client.sin_addr));
        handle_client(cfd);
        printf("Client disconnected\n");
    }

    close(sock);
    return 0;
}
