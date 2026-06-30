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

static int read_all(int fd, char *buf, int n) {
    int total = 0, r;
    while (total < n) {
        r = read(fd, buf + total, n - total);
        if (r <= 0) return r;
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

static int recv_msg(int fd, char *buf, int maxlen) {
    uint32_t net_len;
    if (read_all(fd, (char *)&net_len, 4) != 4) return -1;
    int len = (int)ntohl(net_len);
    if (len >= maxlen) return -1;
    if (read_all(fd, buf, len) != len) return -1;
    buf[len] = '\0';
    return len;
}

static int send_msg(int fd, const char *buf, int len) {
    uint32_t net_len = htonl((uint32_t)len);
    if (write_all(fd, (char *)&net_len, 4) != 4) return -1;
    return write_all(fd, buf, len);
}

int main(void) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port   = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server.sin_addr);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect - cannot connect to server");
        close(sock);
        return 1;
    }
    printf("Connected to server %s:%d\n", SERVER_IP, SERVER_PORT);

    char input[BUF_SIZE], response[BUF_SIZE];
    long long bytes_sent = 0;

    while (1) {
        printf("Enter message (q/Q to quit): ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;

        input[strcspn(input, "\n")] = '\0';
        int len = (int)strlen(input);

        send_msg(sock, input, len);
        bytes_sent += len;

        /* Thoát nếu người dùng gửi "q" hoặc "Q" */
        if (len == 1 && (input[0] == 'q' || input[0] == 'Q')) break;

        int n = recv_msg(sock, response, sizeof(response));
        if (n < 0) { printf("Server disconnected\n"); break; }
        printf("Server: %s\n", response);
    }

    printf("Total bytes sent to server: %lld\n", bytes_sent);
    close(sock);
    return 0;
}
