/* HW2 - Bai 1: DNS Resolver (command-line)
 * Cú pháp: ./resolver <IP_hoac_ten_mien>
 * - Nếu là IP hợp lệ → tra ngược ra tên miền
 * - Nếu là tên miền → tra xuôi ra địa chỉ IP
 * - Chuỗi chỉ gồm chữ số và dấu chấm nhưng không phải IPv4 hợp lệ → Invalid address
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* Trả về 1 nếu chuỗi chỉ gồm chữ số và dấu chấm (có dạng địa chỉ IP) */
static int looks_like_ip(const char *s) {
    for (int i = 0; s[i]; i++)
        if (!isdigit((unsigned char)s[i]) && s[i] != '.') return 0;
    return 1;
}

/* Tra ngược: địa chỉ IP → tên miền */
static void reverse_lookup(const char *ip) {
    struct in_addr addr;
    inet_pton(AF_INET, ip, &addr);

    struct hostent *h = gethostbyaddr(&addr, sizeof(addr), AF_INET);
    if (!h) {
        printf("Not found information\n");
        return;
    }
    printf("Official name: %s\n", h->h_name);
    if (h->h_aliases && h->h_aliases[0]) {
        printf("Alias name:\n");
        for (int i = 0; h->h_aliases[i]; i++)
            printf("%s\n", h->h_aliases[i]);
    }
}

/* Tra xuôi: tên miền → địa chỉ IP */
static void forward_lookup(const char *name) {
    struct hostent *h = gethostbyname(name);
    if (!h) {
        printf("Not found information\n");
        return;
    }
    struct in_addr **addrs = (struct in_addr **)h->h_addr_list;
    /* inet_ntoa dùng buffer tĩnh, cần printf ngay sau mỗi lần gọi */
    printf("Official IP: %s\n", inet_ntoa(*addrs[0]));
    if (addrs[1]) {
        printf("Alias IP:\n");
        for (int i = 1; addrs[i]; i++)
            printf("%s\n", inet_ntoa(*addrs[i]));
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <IP_or_domain>\n", argv[0]);
        return 1;
    }

    const char *param = argv[1];

    if (looks_like_ip(param)) {
        /* Kiểm tra IPv4 hợp lệ trước khi tra cứu */
        struct in_addr tmp;
        if (inet_pton(AF_INET, param, &tmp) != 1) {
            printf("Invalid address\n");
            return 1;
        }
        reverse_lookup(param);
    } else {
        forward_lookup(param);
    }

    return 0;
}
