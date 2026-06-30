/* HW4 - Bai 2: TCP Client gửi file lên server
 * Nhập đường dẫn file từ bàn phím, gửi file lên server, hiển thị kết quả.
 * Protocol: [4-byte tên_file_len][tên_file][4-byte kích_thước][data]
 * Cú pháp: ./client <server_ip> <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>

#define BUF_SIZE 4096

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

/* Gửi chuỗi với prefix 4 byte độ dài */
static void send_str(int fd, const char *s) {
    uint32_t net_len = htonl((uint32_t)strlen(s));
    write_all(fd, (char *)&net_len, 4);
    write_all(fd, s, (int)strlen(s));
}

/* Nhận chuỗi với prefix 4 byte độ dài */
static int recv_str(int fd, char *buf, int maxlen) {
    uint32_t net_len;
    if (read_all(fd, (char *)&net_len, 4) != 4) return -1;
    int len = (int)ntohl(net_len);
    if (len >= maxlen) return -1;
    if (len > 0 && read_all(fd, buf, len) != len) return -1;
    buf[len] = '\0';
    return len;
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
    printf("Connected to file server %s:%s\n", argv[1], argv[2]);

    char filepath[512], response[256];

    while (1) {
        printf("Enter file path (empty to quit): ");
        fflush(stdout);
        if (!fgets(filepath, sizeof(filepath), stdin)) break;

        filepath[strcspn(filepath, "\n")] = '\0';
        if (filepath[0] == '\0') {
            send_str(sock, ""); /* Báo hiệu kết thúc */
            break;
        }

        /* Kiểm tra file tồn tại và lấy kích thước */
        struct stat st;
        if (stat(filepath, &st) != 0) {
            printf("Error: File not found\n");
            continue;
        }

        FILE *fp = fopen(filepath, "rb");
        if (!fp) {
            printf("Error: Cannot open file\n");
            continue;
        }

        /* Gửi tên file */
        send_str(sock, filepath);

        /* Gửi kích thước file (4 byte) */
        uint32_t net_size = htonl((uint32_t)st.st_size);
        write_all(sock, (char *)&net_size, 4);

        /* Gửi nội dung file */
        char buf[BUF_SIZE];
        long rem = (long)st.st_size;
        int ok = 1;
        while (rem > 0) {
            int chunk = rem < BUF_SIZE ? (int)rem : BUF_SIZE;
            int r = (int)fread(buf, 1, chunk, fp);
            if (r <= 0) { ok = 0; break; }
            write_all(sock, buf, r);
            rem -= r;
        }
        fclose(fp);

        if (!ok) { printf("Error reading file\n"); break; }

        /* Nhận phản hồi từ server */
        if (recv_str(sock, response, sizeof(response)) < 0) {
            printf("Server disconnected\n");
            break;
        }
        printf("%s\n", response);
    }

    close(sock);
    return 0;
}
