/* HW6 - Bai 2: TCP Server gửi/nhận thông điệp mã hóa Caesar
 * - Cả client và server đều biết trước khóa Caesar (CAESAR_KEY)
 * - Server nhận thông điệp đã mã hóa → giải mã → hiển thị
 * - Server mã hóa phản hồi → gửi về client
 * - Gõ "q" hoặc "Q" để kết thúc phiên (client gửi lệnh thoát)
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

#define BUF_SIZE    1024
#define CAESAR_KEY  3       /* Khóa Caesar dùng chung giữa client và server */

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

/* Mã hóa Caesar: dịch chuyển mỗi chữ cái Latin đi key vị trí (theo chiều vòng),
 * ký tự không phải chữ cái thì giữ nguyên. */
static void caesar_encrypt(const char *in, char *out, int key) {
    for (int i = 0; in[i]; i++) {
        char c = in[i];
        if (isupper((unsigned char)c))
            out[i] = (char)('A' + (c - 'A' + key) % 26);
        else if (islower((unsigned char)c))
            out[i] = (char)('a' + (c - 'a' + key) % 26);
        else
            out[i] = c;
    }
    out[strlen(in)] = '\0';
}

/* Giải mã Caesar: dịch ngược lại key vị trí (cộng thêm 26 để tránh số âm khi mod) */
static void caesar_decrypt(const char *in, char *out, int key) {
    caesar_encrypt(in, out, 26 - (key % 26));
}

/* Xử lý một client:
 * - Nhận thông điệp đã mã hóa, giải mã và hiển thị
 * - Mã hóa phản hồi rồi gửi về
 * - Thoát khi nhận "q"/"Q" */
static void handle_client(int cfd, const char *client_ip) {
    char encrypted[BUF_SIZE], decrypted[BUF_SIZE], response[BUF_SIZE], enc_response[BUF_SIZE];

    printf("[%s] Session started (Caesar key = %d)\n", client_ip, CAESAR_KEY);

    while (1) {
        /* Nhận thông điệp đã mã hóa từ client */
        int n = recv_msg(cfd, encrypted, sizeof(encrypted));
        if (n < 0) break;

        /* Giải mã để đọc nội dung thực */
        caesar_decrypt(encrypted, decrypted, CAESAR_KEY);
        printf("[%s] Encrypted  : %s\n", client_ip, encrypted);
        printf("[%s] Decrypted  : %s\n", client_ip, decrypted);

        /* Nếu client gửi lệnh thoát thì kết thúc phiên */
        if (n == 1 && (decrypted[0] == 'q' || decrypted[0] == 'Q')) {
            printf("[%s] Client requested quit\n", client_ip);
            break;
        }

        /* Tạo phản hồi, mã hóa rồi gửi về */
        snprintf(response, sizeof(response), "Echo: %s", decrypted);
        caesar_encrypt(response, enc_response, CAESAR_KEY);
        send_msg(cfd, enc_response, (int)strlen(enc_response));
    }

    printf("[%s] Session ended\n", client_ip);
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
    printf("Caesar Cipher Server listening on port %s (key=%d)\n", argv[1], CAESAR_KEY);

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
