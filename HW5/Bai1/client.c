/* HW5 - Bai 1: TCP Client chuyển chữ hoa
 * - Kết nối đến server 127.0.0.1:5500
 * - Nhập thông điệp, gửi cho server nhận về dạng chữ hoa
 * - Nhập "q" hoặc "Q" để đóng kết nối
 * - Hiển thị tổng số byte đã gửi trước khi thoát
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 5500
#define BUF_SIZE    1024

/* Đọc đúng n byte từ fd, xử lý partial read của TCP */
static int read_all(int fd, char *buf, int n) {
    int total = 0, r;
    while (total < n) {
        r = read(fd, buf + total, n - total);
        if (r <= 0) return r;   /* Lỗi hoặc kết nối đóng */
        total += r;
    }
    return total;
}

/* Ghi đúng n byte vào fd, xử lý partial write của TCP */
static int write_all(int fd, const char *buf, int n) {
    int total = 0, w;
    while (total < n) {
        w = write(fd, buf + total, n - total);
        if (w <= 0) return w;   /* Lỗi hoặc kết nối đóng */
        total += w;
    }
    return total;
}

/* Nhận một tin nhắn theo giao thức length-prefix:
 *   [4 byte độ dài (big-endian)] [nội dung]
 * Trả về số byte nội dung, hoặc -1 nếu lỗi */
static int recv_msg(int fd, char *buf, int maxlen) {
    uint32_t net_len;
    /* Đọc 4 byte header để biết độ dài phần nội dung */
    if (read_all(fd, (char *)&net_len, 4) != 4) return -1;
    int len = (int)ntohl(net_len);      /* Chuyển từ network byte order sang host */
    if (len >= maxlen) return -1;       /* Tránh tràn buffer */
    if (len > 0 && read_all(fd, buf, len) != len) return -1;
    buf[len] = '\0';
    return len;
}

/* Gửi một tin nhắn theo giao thức length-prefix:
 *   [4 byte độ dài (big-endian)] [nội dung]
 * Trả về số byte nội dung đã gửi, hoặc giá trị âm nếu lỗi */
static int send_msg(int fd, const char *buf, int len) {
    uint32_t net_len = htonl((uint32_t)len);    /* Chuyển sang network byte order */
    if (write_all(fd, (char *)&net_len, 4) != 4) return -1;
    return write_all(fd, buf, len);
}

int main(void) {
    /* Tạo TCP socket */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    /* Cấu hình địa chỉ server cần kết nối đến */
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port   = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server.sin_addr);

    /* Thực hiện kết nối TCP 3-way handshake đến server */
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect - cannot connect to server");
        close(sock);
        return 1;
    }
    printf("Connected to server %s:%d\n", SERVER_IP, SERVER_PORT);

    char input[BUF_SIZE], response[BUF_SIZE];
    long long bytes_sent = 0;   /* Đếm tổng số byte nội dung đã gửi (không tính header) */

    /* Vòng lặp nhập liệu: mỗi lần đọc một dòng, gửi lên server và nhận kết quả */
    while (1) {
        printf("Enter message (q/Q to quit): ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;  /* EOF (Ctrl+D) thì thoát */

        /* Xóa ký tự newline do fgets đọc vào */
        input[strcspn(input, "\n")] = '\0';
        int len = (int)strlen(input);

        /* Gửi tin nhắn lên server và cộng dồn vào bộ đếm */
        send_msg(sock, input, len);
        bytes_sent += len;

        /* Nếu người dùng gửi lệnh thoát thì không chờ phản hồi, thoát luôn */
        if (len == 1 && (input[0] == 'q' || input[0] == 'Q')) break;

        /* Nhận phản hồi (chuỗi chữ hoa) từ server và in ra màn hình */
        int n = recv_msg(sock, response, sizeof(response));
        if (n < 0) { printf("Server disconnected\n"); break; }
        printf("Server: %s\n", response);
    }

    /* In tổng số byte đã gửi trước khi đóng socket */
    printf("Total bytes sent to server: %lld\n", bytes_sent);
    close(sock);
    return 0;
}
