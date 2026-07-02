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

    /* Tạo UDP socket (SOCK_DGRAM).
     * Khác TCP, UDP không cần listen/accept vì không có kết nối; mỗi datagram độc lập. */
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    /* Cấu hình địa chỉ lắng nghe: chấp nhận datagram từ mọi interface */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;

    /* Gắn socket vào địa chỉ; với UDP không cần listen() sau bind() */
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(sock); return 1;
    }
    printf("UDP Server listening on port %s\n", argv[1]);

    char buf[BUF_SIZE], response[BUF_SIZE * 2];
    struct sockaddr_in client;
    socklen_t clen = sizeof(client);

    /* Vòng lặp chính: nhận datagram, xử lý, gửi phản hồi */
    while (1) {
        /* recvfrom trả về nội dung datagram và lưu địa chỉ người gửi vào client */
        int n = recvfrom(sock, buf, sizeof(buf) - 1, 0,
                         (struct sockaddr *)&client, &clen);
        if (n < 0) { perror("recvfrom"); continue; }
        buf[n] = '\0';  /* Kết thúc chuỗi nhận được */
        printf("Received from %s: %s\n", inet_ntoa(client.sin_addr), buf);

        /* Kiểm tra từng ký tự: chỉ chấp nhận chữ cái (a-z, A-Z) và chữ số (0-9) */
        int valid = 1;
        for (int i = 0; buf[i]; i++) {
            if (!isalnum((unsigned char)buf[i])) { valid = 0; break; }
        }

        if (!valid) {
            /* Chuỗi chứa ký tự không hợp lệ → gửi thông báo lỗi */
            snprintf(response, sizeof(response), "Error");
        } else {
            /* Duyệt một lần, phân loại từng ký tự vào digits hoặc letters */
            char digits[BUF_SIZE] = "", letters[BUF_SIZE] = "";
            int di = 0, li = 0;
            for (int i = 0; buf[i]; i++) {
                if (isdigit((unsigned char)buf[i])) digits[di++]  = buf[i];
                else                                 letters[li++] = buf[i];
            }
            digits[di] = letters[li] = '\0';
            /* Định dạng phản hồi: chữ số trên dòng đầu, chữ cái dòng thứ hai */
            snprintf(response, sizeof(response), "%s\n%s", digits, letters);
        }

        /* Gửi phản hồi về đúng địa chỉ client vừa nhận được */
        sendto(sock, response, strlen(response), 0,
               (struct sockaddr *)&client, clen);
    }

    close(sock);
    return 0;
}
