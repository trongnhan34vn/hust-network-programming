/* HW4 - Bai 2: TCP Server nhận file từ client
 * Protocol: client gửi [4-byte tên_file_len][tên_file][4-byte kích_thước][data]
 * Server lưu file vào thư mục "uploads/", báo lỗi nếu file đã tồn tại.
 * Kích thước file tối đa: 100MB (dùng uint32_t đủ ~4GB)
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

static int read_all(int fd, char *buf, int n) {
    int total = 0, r;
    while (total < n) {
        r = read(fd, buf + total, n - total);
        if (r <= 0) return total > 0 ? total : r;
        total += r;
    }
    return total;
}

static int write_all(int fd, const char *buf, int n) {
    int total = 0, w;
    while (total < n) {
        w = write(fd, buf + total, n - total);
        if (w <= 0) return w;
        total += w;
    }
    return total;
}

/* Nhận chuỗi có độ dài prefix 4 byte */
static int recv_str(int fd, char *buf, int maxlen) {
    uint32_t net_len;
    if (read_all(fd, (char *)&net_len, 4) != 4) return -1;
    int len = (int)ntohl(net_len);
    if (len >= maxlen) return -1;
    if (len > 0 && read_all(fd, buf, len) != len) return -1;
    buf[len] = '\0';
    return len;
}

/* Gửi chuỗi có độ dài prefix 4 byte */
static void send_str(int fd, const char *msg) {
    uint32_t net_len = htonl((uint32_t)strlen(msg));
    write_all(fd, (char *)&net_len, 4);
    write_all(fd, msg, (int)strlen(msg));
}

/* Trích tên file (basename) để tránh path traversal */
static const char *get_basename(const char *path) {
    const char *base = path;
    for (int i = 0; path[i]; i++) {
        if (path[i] == '/' || path[i] == '\\')
            base = path + i + 1;
    }
    return base;
}

static void handle_client(int cfd) {
    mkdir(UPLOAD_DIR, 0755);

    char filename[512];

    while (1) {
        /* Nhận tên file; chuỗi rỗng báo hiệu client kết thúc */
        int n = recv_str(cfd, filename, sizeof(filename));
        if (n <= 0) break;

        /* Nhận kích thước file (4 byte) */
        uint32_t net_size;
        if (read_all(cfd, (char *)&net_size, 4) != 4) break;
        long fsize = (long)ntohl(net_size);

        /* Xây dựng đường dẫn lưu file */
        char savepath[600];
        snprintf(savepath, sizeof(savepath), "%s/%s", UPLOAD_DIR, get_basename(filename));

        /* Kiểm tra file đã tồn tại chưa */
        struct stat st;
        if (stat(savepath, &st) == 0) {
            send_str(cfd, "Error: File is existent on server");
            /* Drain dữ liệu file để không làm lệch protocol */
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

        /* Nhận và lưu dữ liệu file */
        FILE *fp = fopen(savepath, "wb");
        if (!fp) {
            send_str(cfd, "Error: Cannot create file on server");
            continue;
        }

        char buf[BUF_SIZE];
        long rem = fsize;
        int ok = 1;
        while (rem > 0) {
            int chunk = rem < BUF_SIZE ? (int)rem : BUF_SIZE;
            int r = read(cfd, buf, chunk);
            if (r <= 0) { ok = 0; break; }
            fwrite(buf, 1, r, fp);
            rem -= r;
        }
        fclose(fp);

        if (ok) {
            send_str(cfd, "Successful transfering");
            printf("Saved: %s (%ld bytes)\n", savepath, fsize);
        } else {
            unlink(savepath); /* Xóa file chưa hoàn chỉnh */
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

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

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
