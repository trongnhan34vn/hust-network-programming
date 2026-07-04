/* HW6 - Bai 1: TCP Server nhận file từ client
 * - Lắng nghe kết nối TCP trên cổng chỉ định
 * - Nhận tên file và nội dung từ client
 * - Lưu file vào thư mục hiện tại với tiền tố "received_"
 * - Gửi thông báo xác nhận (ACK) về cho client
 * Cú pháp: ./server <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>

#define BUF_SIZE     4096
#define FNAME_MAX    256

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
 *   [4 byte độ dài (big-endian)] [nội dung]
 * Trả về số byte nội dung, hoặc -1 nếu lỗi */
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

/* Nhận file từ một client:
 * Giao thức: [tên file (length-prefix)] [kích thước file (4 byte)] [dữ liệu file]
 * Sau khi nhận xong gửi ACK về client. */
static void handle_client(int cfd, const char *client_ip) {
    char fname_buf[FNAME_MAX];

    /* Bước 1: nhận tên file từ client */
    int fname_len = recv_msg(cfd, fname_buf, sizeof(fname_buf));
    if (fname_len <= 0) {
        printf("[%s] Failed to receive filename\n", client_ip);
        close(cfd);
        return;
    }

    /* Tạo đường dẫn lưu file với tiền tố "received_" */
    char save_path[FNAME_MAX + 16];
    snprintf(save_path, sizeof(save_path), "received_%s", fname_buf);

    /* Bước 2: nhận kích thước file (4 byte big-endian) */
    uint32_t net_fsize;
    if (read_all(cfd, (char *)&net_fsize, 4) != 4) {
        printf("[%s] Failed to receive file size\n", client_ip);
        close(cfd);
        return;
    }
    uint32_t fsize = ntohl(net_fsize);
    printf("[%s] Receiving file: %s (%u bytes)\n", client_ip, fname_buf, fsize);

    /* Mở file để ghi */
    FILE *fp = fopen(save_path, "wb");
    if (!fp) {
        perror("fopen");
        close(cfd);
        return;
    }

    /* Bước 3: nhận dữ liệu file theo từng chunk */
    char buf[BUF_SIZE];
    uint32_t received = 0;
    while (received < fsize) {
        int to_read = (fsize - received < (uint32_t)BUF_SIZE)
                      ? (int)(fsize - received) : BUF_SIZE;
        int r = read(cfd, buf, to_read);
        if (r <= 0) break;
        fwrite(buf, 1, r, fp);
        received += r;
    }
    fclose(fp);

    if (received == fsize) {
        printf("[%s] File saved as: %s\n", client_ip, save_path);
        send_msg(cfd, "OK: File received successfully", 30);
    } else {
        printf("[%s] Incomplete transfer: %u/%u bytes\n", client_ip, received, fsize);
        send_msg(cfd, "ERR: Incomplete transfer", 24);
    }

    close(cfd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    setbuf(stdout, NULL);   /* unbuffered stdout so logs appear immediately */

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
    printf("File Transfer Server listening on port %s\n", argv[1]);

    while (1) {
        struct sockaddr_in client;
        socklen_t clen = sizeof(client);
        int cfd = accept(sock, (struct sockaddr *)&client, &clen);
        if (cfd < 0) { perror("accept"); continue; }

        char *ip = inet_ntoa(client.sin_addr);
        printf("Client connected: %s\n", ip);
        handle_client(cfd, ip);
        printf("Client disconnected: %s\n", ip);
    }

    close(sock);
    return 0;
}
