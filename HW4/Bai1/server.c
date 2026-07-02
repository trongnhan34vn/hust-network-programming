/* HW4 - Bai 1: TCP Server xử lý chuỗi ký tự
 * Nhận chuỗi từ client, tách thành chữ số và chữ cái, gửi lại.
 * Dùng length-prefix (4 byte) để xử lý vấn đề byte-stream của TCP.
 * Cú pháp: ./server <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>

#define BUF_SIZE 1024

/* Đọc đúng n byte từ fd, xử lý partial read của TCP.
 * TCP có thể trả về ít byte hơn yêu cầu trong một lần read() (vì là byte-stream),
 * nên cần loop cho đến khi đủ n byte. */
static int read_all(int fd, char *buf, int n) {
    int total = 0, r;
    while (total < n) {
        r = read(fd, buf + total, n - total);
        if (r <= 0) return r;   /* Lỗi hoặc kết nối bị đóng */
        total += r;
    }
    return total;
}

/* Ghi đúng n byte vào fd, xử lý partial write của TCP.
 * write() cũng có thể ghi ít hơn yêu cầu nên cần loop tương tự. */
static int write_all(int fd, const char *buf, int n) {
    int total = 0, w;
    while (total < n) {
        w = write(fd, buf + total, n - total);
        if (w <= 0) return w;   /* Lỗi hoặc kết nối bị đóng */
        total += w;
    }
    return total;
}

/* Nhận một tin nhắn theo giao thức length-prefix:
 *   [4 byte độ dài (big-endian network byte order)] [nội dung]
 * Cách này giải quyết vấn đề TCP không bảo toàn ranh giới tin nhắn (message boundary). */
static int recv_msg(int fd, char *buf, int maxlen) {
    uint32_t net_len;
    /* Bước 1: đọc 4 byte header để biết độ dài phần nội dung */
    if (read_all(fd, (char *)&net_len, 4) != 4) return -1;
    int len = (int)ntohl(net_len);  /* Chuyển từ network (big-endian) sang host byte order */
    if (len >= maxlen) return -1;   /* Từ chối nếu vượt kích thước buffer */
    /* Bước 2: đọc đúng len byte nội dung */
    if (read_all(fd, buf, len) != len) return -1;
    buf[len] = '\0';                /* Null-terminate để dùng như chuỗi C */
    return len;
}

/* Gửi một tin nhắn theo giao thức length-prefix:
 *   [4 byte độ dài (big-endian)] [nội dung] */
static int send_msg(int fd, const char *buf, int len) {
    uint32_t net_len = htonl((uint32_t)len);    /* Chuyển sang network byte order */
    if (write_all(fd, (char *)&net_len, 4) != 4) return -1;
    return write_all(fd, buf, len);
}

/* Xử lý một client: nhận chuỗi, kiểm tra, tách ký tự, gửi phản hồi.
 * Chạy trong vòng lặp cho đến khi client đóng kết nối. */
static void handle_client(int cfd) {
    char buf[BUF_SIZE], response[BUF_SIZE * 2];

    while (1) {
        int n = recv_msg(cfd, buf, sizeof(buf));
        if (n <= 0) break;  /* Client đóng kết nối hoặc lỗi */

        /* Kiểm tra toàn bộ chuỗi: chỉ chấp nhận chữ cái và chữ số */
        int valid = 1;
        for (int i = 0; buf[i]; i++) {
            if (!isalnum((unsigned char)buf[i])) { valid = 0; break; }
        }

        if (!valid) {
            snprintf(response, sizeof(response), "Error");
        } else {
            /* Duyệt một lần và phân loại từng ký tự vào digits hoặc letters */
            char digits[BUF_SIZE] = "", letters[BUF_SIZE] = "";
            int di = 0, li = 0;
            for (int i = 0; buf[i]; i++) {
                if (isdigit((unsigned char)buf[i])) digits[di++]  = buf[i];
                else                                 letters[li++] = buf[i];
            }
            digits[di] = letters[li] = '\0';
            /* Định dạng: chữ số dòng đầu, chữ cái dòng thứ hai */
            snprintf(response, sizeof(response), "%s\n%s", digits, letters);
        }

        send_msg(cfd, response, (int)strlen(response));
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

    /* Cho phép tái sử dụng địa chỉ ngay sau khi server tắt (tránh "Address already in use") */
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* Cấu hình địa chỉ lắng nghe: chấp nhận kết nối từ mọi interface */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(sock); return 1;
    }
    listen(sock, 5);    /* Hàng chờ tối đa 5 kết nối đang chờ được accept */
    printf("TCP Server listening on port %s\n", argv[1]);

    /* Vòng lặp chính: chấp nhận từng client và xử lý tuần tự (single-thread) */
    while (1) {
        struct sockaddr_in client;
        socklen_t clen = sizeof(client);
        int cfd = accept(sock, (struct sockaddr *)&client, &clen);
        if (cfd < 0) { perror("accept"); continue; }
        printf("Client connected: %s\n", inet_ntoa(client.sin_addr));
        handle_client(cfd);     /* Chặn ở đây cho đến khi client ngắt kết nối */
        printf("Client disconnected\n");
    }

    close(sock);
    return 0;
}
