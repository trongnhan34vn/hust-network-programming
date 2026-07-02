/* HW3 - Bai 2: UDP Client phân giải tên miền
 * Nhập tên miền hoặc địa chỉ IP, gửi lên server, hiển thị kết quả DNS.
 * Cú pháp: ./client <server_ip> <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 2048

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }

    /* Tạo UDP socket; mỗi lần hỏi là một datagram độc lập, không cần connect() */
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    /* Cấu hình địa chỉ server DNS */
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port   = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &server.sin_addr);

    char input[256], response[BUF_SIZE];

    /* Vòng lặp: mỗi lần nhập một tên miền/IP, gửi đi và chờ nhận kết quả */
    while (1) {
        printf("Enter domain or IP (empty to quit): ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;  /* EOF thì thoát */

        /* Xóa newline; dòng rỗng → thoát */
        input[strcspn(input, "\n")] = '\0';
        if (input[0] == '\0') break;

        /* Gửi query đến server DNS */
        sendto(sock, input, strlen(input), 0,
               (struct sockaddr *)&server, sizeof(server));

        /* Nhận và hiển thị kết quả DNS từ server */
        socklen_t slen = sizeof(server);
        int n = recvfrom(sock, response, sizeof(response) - 1, 0,
                         (struct sockaddr *)&server, &slen);
        if (n < 0) { perror("recvfrom"); continue; }
        response[n] = '\0';
        printf("%s\n", response);
    }

    close(sock);
    return 0;
}
