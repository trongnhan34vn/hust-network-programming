/* HW6 - Bai 2: TCP Client gửi/nhận thông điệp mã hóa Caesar
 * - Cả client và server đều biết trước khóa Caesar (CAESAR_KEY)
 * - Client mã hóa thông điệp trước khi gửi
 * - Client giải mã phản hồi nhận được từ server
 * - Gõ "q" hoặc "Q" để thoát
 * Cú pháp: ./client <server_ip> <port>
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

/* Mã hóa Caesar: dịch chuyển mỗi chữ cái Latin đi key vị trí,
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

/* Giải mã Caesar: dịch ngược lại key vị trí */
static void caesar_decrypt(const char *in, char *out, int key) {
    caesar_encrypt(in, out, 26 - (key % 26));
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
    printf("Connected to server %s:%s (Caesar key = %d)\n", argv[1], argv[2], CAESAR_KEY);

    char input[BUF_SIZE], encrypted[BUF_SIZE], enc_response[BUF_SIZE], decrypted[BUF_SIZE];

    while (1) {
        printf("Enter message (q/Q to quit): ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;

        input[strcspn(input, "\n")] = '\0';
        int len = (int)strlen(input);
        if (len == 0) continue;

        /* Mã hóa thông điệp trước khi gửi */
        caesar_encrypt(input, encrypted, CAESAR_KEY);
        printf("Sending encrypted: %s\n", encrypted);
        send_msg(sock, encrypted, (int)strlen(encrypted));

        /* Nếu lệnh thoát thì không chờ phản hồi */
        if (len == 1 && (input[0] == 'q' || input[0] == 'Q')) break;

        /* Nhận phản hồi đã mã hóa từ server rồi giải mã */
        int n = recv_msg(sock, enc_response, sizeof(enc_response));
        if (n < 0) { printf("Server disconnected\n"); break; }

        caesar_decrypt(enc_response, decrypted, CAESAR_KEY);
        printf("Received encrypted: %s\n", enc_response);
        printf("Decrypted response: %s\n\n", decrypted);
    }

    close(sock);
    return 0;
}
