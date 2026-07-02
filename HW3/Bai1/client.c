/* HW3 - Bai 1: UDP Client xử lý chuỗi ký tự
 * Nhập chuỗi từ bàn phím, gửi lên server, nhận và hiển thị kết quả.
 * Cú pháp: ./client <server_ip> <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }

    /* Tạo UDP socket; không cần connect() vì UDP là connectionless */
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    /* Cấu hình địa chỉ server để gửi datagram đến */
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port   = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server.sin_addr); /* Chuyển chuỗi IP sang dạng nhị phân */

    char input[BUF_SIZE], response[BUF_SIZE * 2];

    /* Vòng lặp nhập liệu: mỗi lần gửi một datagram và nhận một datagram phản hồi */
    while (1) {
        printf("Enter string (empty to quit): ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;  /* EOF (Ctrl+D) thì thoát */

        /* Xóa newline do fgets đọc vào; nếu dòng rỗng thì thoát */
        input[strcspn(input, "\n")] = '\0';
        if (input[0] == '\0') break;

        /* Gửi datagram đến server (không cần kết nối trước) */
        sendto(sock, input, strlen(input), 0,
               (struct sockaddr *)&server, sizeof(server));

        /* Nhận datagram phản hồi; slen dùng để truyền/nhận kích thước địa chỉ */
        socklen_t slen = sizeof(server);
        int n = recvfrom(sock, response, sizeof(response) - 1, 0,
                         (struct sockaddr *)&server, &slen);
        if (n < 0) { perror("recvfrom"); continue; }
        response[n] = '\0';
        printf("Result:\n%s\n", response);
    }

    close(sock);
    return 0;
}
