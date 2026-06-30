/* HW5 - Bai 1: Multi-thread TCP Server chuyển chữ hoa
 * - Lắng nghe trên 127.0.0.1:5500
 * - Mỗi client kết nối được xử lý bởi một thread riêng
 * - Nhận chuỗi, chuyển thành chữ hoa, gửi lại
 * - Đóng kết nối với client nếu nhận được "q" hoặc "Q"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
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

/* Nhận tin nhắn với 4-byte length prefix */
static int recv_msg(int fd, char *buf, int maxlen) {
    uint32_t net_len;
    if (read_all(fd, (char *)&net_len, 4) != 4) return -1;
    int len = (int)ntohl(net_len);
    if (len >= maxlen) return -1;
    if (read_all(fd, buf, len) != len) return -1;
    buf[len] = '\0';
    return len;
}

/* Gửi tin nhắn với 4-byte length prefix */
static int send_msg(int fd, const char *buf, int len) {
    uint32_t net_len = htonl((uint32_t)len);
    if (write_all(fd, (char *)&net_len, 4) != 4) return -1;
    return write_all(fd, buf, len);
}

/* Thread xử lý từng client */
static void *handle_client(void *arg) {
    int cfd = *(int *)arg;
    free(arg);

    char buf[BUF_SIZE];
    while (1) {
        int n = recv_msg(cfd, buf, sizeof(buf));
        if (n < 0) break; /* Kết nối bị đóng */

        /* Thoát nếu client gửi "q" hoặc "Q" */
        if (n == 1 && (buf[0] == 'q' || buf[0] == 'Q')) {
            printf("Client sent quit signal\n");
            break;
        }

        /* Chuyển toàn bộ chuỗi thành chữ hoa */
        for (int i = 0; i < n; i++)
            buf[i] = toupper((unsigned char)buf[i]);

        send_msg(cfd, buf, n);
    }

    close(cfd);
    printf("Client thread exiting\n");
    return NULL;
}

int main(void) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(sock); return 1;
    }
    listen(sock, 10);
    printf("Multi-thread server listening on %s:%d\n", SERVER_IP, SERVER_PORT);

    while (1) {
        struct sockaddr_in client;
        socklen_t clen = sizeof(client);
        int cfd = accept(sock, (struct sockaddr *)&client, &clen);
        if (cfd < 0) { perror("accept"); continue; }
        printf("New client: %s\n", inet_ntoa(client.sin_addr));

        /* Cấp phát bộ nhớ cho fd để truyền vào thread an toàn */
        int *pcfd = malloc(sizeof(int));
        if (!pcfd) { close(cfd); continue; }
        *pcfd = cfd;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, pcfd);
        pthread_detach(tid); /* Thread tự dọn dẹp khi xong */
    }

    close(sock);
    return 0;
}
