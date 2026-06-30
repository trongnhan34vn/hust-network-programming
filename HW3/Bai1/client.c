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

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port   = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server.sin_addr);

    char input[BUF_SIZE], response[BUF_SIZE * 2];

    while (1) {
        printf("Enter string (empty to quit): ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;

        input[strcspn(input, "\n")] = '\0';
        if (input[0] == '\0') break;

        sendto(sock, input, strlen(input), 0,
               (struct sockaddr *)&server, sizeof(server));

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
