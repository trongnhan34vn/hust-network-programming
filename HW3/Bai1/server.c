/* HW3 - Bai 1: UDP Server xử lý chuỗi ký tự
 * Nhận chuỗi từ client, tách thành phần chữ số và chữ cái.
 * Nếu chuỗi chứa ký tự không phải chữ số/chữ cái → gửi "Error"
 * Cú pháp: ./server <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(sock); return 1;
    }
    printf("UDP Server listening on port %s\n", argv[1]);

    char buf[BUF_SIZE], response[BUF_SIZE * 2];
    struct sockaddr_in client;
    socklen_t clen = sizeof(client);

    while (1) {
        int n = recvfrom(sock, buf, sizeof(buf) - 1, 0,
                         (struct sockaddr *)&client, &clen);
        if (n < 0) { perror("recvfrom"); continue; }
        buf[n] = '\0';
        printf("Received from %s: %s\n", inet_ntoa(client.sin_addr), buf);

        /* Kiểm tra: chỉ cho phép chữ cái và chữ số */
        int valid = 1;
        for (int i = 0; buf[i]; i++) {
            if (!isalnum((unsigned char)buf[i])) { valid = 0; break; }
        }

        if (!valid) {
            snprintf(response, sizeof(response), "Error");
        } else {
            /* Tách chữ số và chữ cái */
            char digits[BUF_SIZE] = "", letters[BUF_SIZE] = "";
            int di = 0, li = 0;
            for (int i = 0; buf[i]; i++) {
                if (isdigit((unsigned char)buf[i])) digits[di++]  = buf[i];
                else                                 letters[li++] = buf[i];
            }
            digits[di] = letters[li] = '\0';
            snprintf(response, sizeof(response), "%s\n%s", digits, letters);
        }

        sendto(sock, response, strlen(response), 0,
               (struct sockaddr *)&client, clen);
    }

    close(sock);
    return 0;
}
