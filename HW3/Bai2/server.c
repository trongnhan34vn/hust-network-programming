/* HW3 - Bai 2: UDP Server phân giải tên miền
 * Nhận tên miền hoặc địa chỉ IP từ client, tra cứu DNS rồi gửi kết quả về.
 * Cú pháp: ./server <port>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

#define BUF_SIZE 2048

/* Trả về 1 nếu chuỗi chỉ gồm chữ số và dấu chấm → có dạng địa chỉ IPv4.
 * Dùng để quyết định tra ngược (IP→hostname) hay tra xuôi (hostname→IP). */
static int looks_like_ip(const char *s) {
    for (int i = 0; s[i]; i++)
        if (!isdigit((unsigned char)s[i]) && s[i] != '.') return 0;
    return 1;
}

/* Thực hiện tra cứu DNS và ghi kết quả dạng chuỗi vào result.
 * result được gửi thẳng về client qua UDP nên tất cả thông tin gộp vào một chuỗi.
 * Dùng offset (off) để nối chuỗi an toàn và tránh tràn buffer. */
static void do_lookup(const char *query, char *result, int rsize) {
    if (looks_like_ip(query)) {
        /* Tra ngược: IP → hostname dùng gethostbyaddr() */
        struct in_addr addr;
        if (inet_pton(AF_INET, query, &addr) != 1) {
            snprintf(result, rsize, "IP Address is invalid");
            return;
        }
        struct hostent *h = gethostbyaddr(&addr, sizeof(addr), AF_INET);
        if (!h) {
            snprintf(result, rsize, "Not found information");
        } else {
            /* Ghép tên chính và các alias vào một chuỗi kết quả */
            int off = snprintf(result, rsize, "Official name: %s", h->h_name);
            if (h->h_aliases && h->h_aliases[0]) {
                off += snprintf(result + off, rsize - off, "\nAlias name:");
                for (int i = 0; h->h_aliases[i] && off < rsize - 1; i++)
                    off += snprintf(result + off, rsize - off, "\n%s", h->h_aliases[i]);
            }
        }
    } else {
        /* Tra xuôi: hostname → IP dùng gethostbyname() */
        struct hostent *h = gethostbyname(query);
        if (!h) {
            snprintf(result, rsize, "Not found information");
        } else {
            struct in_addr **addrs = (struct in_addr **)h->h_addr_list;
            /* IP đầu tiên là official, các IP sau là alias */
            int off = snprintf(result, rsize, "Official IP: %s", inet_ntoa(*addrs[0]));
            if (addrs[1]) {
                off += snprintf(result + off, rsize - off, "\nAlias IP:");
                for (int i = 1; addrs[i] && off < rsize - 1; i++)
                    off += snprintf(result + off, rsize - off, "\n%s", inet_ntoa(*addrs[i]));
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    /* Tạo UDP socket; mỗi datagram từ client là một yêu cầu tra cứu độc lập */
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    /* Cấu hình và bind địa chỉ lắng nghe */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(atoi(argv[1]));
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(sock); return 1;
    }
    printf("DNS UDP Server listening on port %s\n", argv[1]);

    char buf[BUF_SIZE], result[BUF_SIZE];
    struct sockaddr_in client;
    socklen_t clen = sizeof(client);

    /* Vòng lặp chính: nhận query, tra cứu DNS, gửi kết quả về client */
    while (1) {
        int n = recvfrom(sock, buf, sizeof(buf) - 1, 0,
                         (struct sockaddr *)&client, &clen);
        if (n < 0) { perror("recvfrom"); continue; }
        buf[n] = '\0';
        printf("Query from %s: %s\n", inet_ntoa(client.sin_addr), buf);

        /* Tra cứu và gửi kết quả về đúng địa chỉ client đã gửi query */
        do_lookup(buf, result, sizeof(result));
        sendto(sock, result, strlen(result), 0,
               (struct sockaddr *)&client, clen);
    }

    close(sock);
    return 0;
}
