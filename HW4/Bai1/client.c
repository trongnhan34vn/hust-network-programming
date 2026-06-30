/* HW4 - Bai 1: TCP Client xử lý chuỗi ký tự
 * Nhập chuỗi từ bàn phím, gửi lên server, nhận kết quả tách chữ số/chữ cái.
 * Dùng length-prefix (4 byte) để xử lý byte-stream của TCP.
 * Cú pháp: ./client <server_ip> <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>

#define BUF_SIZE 1024

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

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port   = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server.sin_addr);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect"); close(sock); return 1;
    }
    printf("Connected to server %s:%s\n", argv[1], argv[2]);

    char input[BUF_SIZE], response[BUF_SIZE * 2];

    while (1) {
        printf("Enter string (empty to quit): ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;

        input[strcspn(input, "\n")] = '\0';
        if (input[0] == '\0') break;

        send_msg(sock, input, (int)strlen(input));

        int n = recv_msg(sock, response, sizeof(response));
        if (n < 0) { printf("Server disconnected\n"); break; }
        printf("Result:\n%s\n", response);
    }

    close(sock);
    return 0;
}
