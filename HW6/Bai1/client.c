/* HW6 - Bai 1: TCP Client gửi file lên server
 * - Kết nối đến server TCP
 * - Đọc file từ đường dẫn chỉ định, gửi tên file và nội dung lên server
 * - Nhận thông báo xác nhận (ACK) từ server
 * Cú pháp: ./client <server_ip> <port> <filepath>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdint.h>
#include <libgen.h>

#define BUF_SIZE  4096
#define FNAME_MAX 256

/* Đọc đúng n byte từ fd, xử lý partial read của TCP */
static int read_all(int fd, char *buf, int n) {
    int total = 0, r;
    while (total < n) {
        r = read(fd, buf + total, n - total);
        if (r <= 0) return r;
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

/* Nhận một tin nhắn theo giao thức length-prefix:
 *   [4 byte độ dài (big-endian)] [nội dung] */
static int recv_msg(int fd, char *buf, int maxlen) {
    uint32_t net_len;
    if (read_all(fd, (char *)&net_len, 4) != 4) return -1;
    int len = (int)ntohl(net_len);
    if (len <= 0 || len >= maxlen) return -1;
    if (read_all(fd, buf, len) != len) return -1;
    buf[len] = '\0';
    return len;
}

/* Gửi một tin nhắn theo giao thức length-prefix:
 *   [4 byte độ dài (big-endian)] [nội dung] */
static int send_msg(int fd, const char *buf, int len) {
    uint32_t net_len = htonl((uint32_t)len);
    if (write_all(fd, (char *)&net_len, 4) != 4) return -1;
    return write_all(fd, buf, len);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server_ip> <port> <filepath>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int         port      = atoi(argv[2]);
    const char *filepath  = argv[3];

    /* Lấy tên file (không lấy đường dẫn thư mục) */
    char path_copy[FNAME_MAX];
    strncpy(path_copy, filepath, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';
    char *fname = basename(path_copy);

    /* Mở file để đọc */
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    /* Lấy kích thước file */
    struct stat st;
    if (stat(filepath, &st) < 0) {
        perror("stat"); fclose(fp); return 1;
    }
    uint32_t fsize = (uint32_t)st.st_size;

    /* Tạo TCP socket và kết nối tới server */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); fclose(fp); return 1; }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port   = htons(port);
    inet_pton(AF_INET, server_ip, &server.sin_addr);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect"); fclose(fp); close(sock); return 1;
    }
    printf("Connected to server %s:%d\n", server_ip, port);
    printf("Sending file: %s (%u bytes)\n", fname, fsize);

    /* Bước 1: gửi tên file */
    if (send_msg(sock, fname, (int)strlen(fname)) < 0) {
        fprintf(stderr, "Failed to send filename\n");
        fclose(fp); close(sock); return 1;
    }

    /* Bước 2: gửi kích thước file (4 byte big-endian) */
    uint32_t net_fsize = htonl(fsize);
    if (write_all(sock, (char *)&net_fsize, 4) != 4) {
        fprintf(stderr, "Failed to send file size\n");
        fclose(fp); close(sock); return 1;
    }

    /* Bước 3: gửi dữ liệu file theo từng chunk */
    char buf[BUF_SIZE];
    uint32_t sent = 0;
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        if (write_all(sock, buf, (int)n) != (int)n) {
            fprintf(stderr, "Error sending file data\n");
            break;
        }
        sent += (uint32_t)n;
        printf("\rProgress: %u/%u bytes (%.1f%%)", sent, fsize,
               fsize > 0 ? (float)sent / fsize * 100.0f : 100.0f);
        fflush(stdout);
    }
    fclose(fp);
    printf("\n");

    /* Bước 4: chờ xác nhận từ server */
    char ack[256];
    int ack_len = recv_msg(sock, ack, sizeof(ack));
    if (ack_len > 0)
        printf("Server response: %s\n", ack);
    else
        printf("No response from server\n");

    close(sock);
    return 0;
}
