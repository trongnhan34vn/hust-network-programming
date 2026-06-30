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

/* Đọc đúng n byte từ fd (xử lý partial read của TCP) */
static int read_all(int fd, char *buf, int n) {
    int total = 0, r;
    while (total < n) {
        r = read(fd, buf + total, n - total);
        if (r <= 0) return r;
        total += r;
    }
    return total;
}

/* Ghi đúng n byte vào fd */
static int write_all(int fd, const char *buf, int n) {
    int total = 0, w;
    while (total < n) {
        w = write(fd, buf + total, n - total);
        if (w <= 0) return w;
        total += w;
    }
    return total;
}

/* Nhận tin nhắn: đọc 4 byte độ dài rồi đọc nội dung */
static int recv_msg(int fd, char *buf, int maxlen) {
    uint32_t net_len;
    if (read_all(fd, (char *)&net_len, 4) != 4) return -1;
    int len = (int)ntohl(net_len);
    if (len >= maxlen) return -1;
    if (read_all(fd, buf, len) != len) return -1;
    buf[len] = '\0';
    return len;
}

/* Gửi tin nhắn: gửi 4 byte độ dài rồi gửi nội dung */
static int send_msg(int fd, const char *buf, int len) {
    uint32_t net_len = htonl((uint32_t)len);
    if (write_all(fd, (char *)&net_len, 4) != 4) return -1;
    return write_all(fd, buf, len);
}

static void handle_client(int cfd) {
    char buf[BUF_SIZE], response[BUF_SIZE * 2];

    while (1) {
        int n = recv_msg(cfd, buf, sizeof(buf));
        if (n <= 0) break; /* client đóng kết nối */

        /* Kiểm tra: chỉ cho phép alphanumeric */
        int valid = 1;
        for (int i = 0; buf[i]; i++) {
            if (!isalnum((unsigned char)buf[i])) { valid = 0; break; }
        }

        if (!valid) {
            snprintf(response, sizeof(response), "Error");
        } else {
            char digits[BUF_SIZE] = "", letters[BUF_SIZE] = "";
            int di = 0, li = 0;
            for (int i = 0; buf[i]; i++) {
                if (isdigit((unsigned char)buf[i])) digits[di++]  = buf[i];
                else                                 letters[li++] = buf[i];
            }
            digits[di] = letters[li] = '\0';
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
    printf("TCP Server listening on port %s\n", argv[1]);

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
