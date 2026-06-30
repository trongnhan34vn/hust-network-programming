/* HW2 - Bai 2: DNS Resolver nâng cao
 * - Chế độ interactive: nhập từ bàn phím, thoát khi nhập dòng rỗng
 * - Chế độ batch: ./resolver list.txt (đọc từng dòng trong file)
 * - Hỗ trợ IPv4, IPv6
 * - Cảnh báo địa chỉ đặc biệt (loopback, private, multicast, link-local)
 * - Ghi log vào resolver.log
 * - Nhiều địa chỉ trên cùng một dòng (cách nhau bằng khoảng trắng)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define LOG_FILE "resolver.log"
#define BUF_SIZE 1024

static FILE *log_fp;

/* Ghi log với timestamp */
static void log_write(const char *query, const char *result) {
    if (!log_fp) return;
    time_t t = time(NULL);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&t));
    fprintf(log_fp, "[%s] %s -> %s\n", ts, query, result);
    fflush(log_fp);
}

/* Trả về 1 nếu chuỗi có dạng địa chỉ IPv4 (chỉ chữ số và dấu chấm) */
static int looks_like_ipv4(const char *s) {
    for (int i = 0; s[i]; i++)
        if (!isdigit((unsigned char)s[i]) && s[i] != '.') return 0;
    return 1;
}

/* Trả về 1 nếu chuỗi có dạng IPv6 (chứa dấu hai chấm) */
static int looks_like_ipv6(const char *s) {
    return strchr(s, ':') != NULL;
}

/* Cảnh báo nếu địa chỉ IPv4 thuộc nhóm đặc biệt */
static void warn_special_ipv4(const char *ip) {
    struct in_addr addr;
    inet_pton(AF_INET, ip, &addr);
    uint32_t n = ntohl(addr.s_addr);

    int special = ((n >> 24) == 127)       ||  /* loopback 127.x.x.x */
                  ((n >> 24) == 10)        ||  /* private 10.x.x.x */
                  ((n >> 16) == 0xC0A8)   ||  /* private 192.168.x.x */
                  ((n >> 20) == 0xAC1)    ||  /* private 172.16-31.x.x */
                  ((n >> 24) >= 224)      ||  /* multicast/broadcast */
                  ((n >> 16) == 0xA9FE);      /* link-local 169.254.x.x */
    if (special)
        printf("Warning: special IP address — may not have DNS record\n");
}

/* Tra cứu một địa chỉ hoặc tên miền */
static void resolve(const char *query) {
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    char result_log[512] = "unknown";

    if (looks_like_ipv6(query)) {
        /* Tra ngược IPv6 → hostname */
        struct in6_addr addr6;
        if (inet_pton(AF_INET6, query, &addr6) != 1) {
            printf("Invalid address\n");
            log_write(query, "Invalid address");
            return;
        }
        struct sockaddr_in6 sa6;
        memset(&sa6, 0, sizeof(sa6));
        sa6.sin6_family = AF_INET6;
        sa6.sin6_addr = addr6;
        char host[NI_MAXHOST];
        if (getnameinfo((struct sockaddr *)&sa6, sizeof(sa6), host, sizeof(host),
                        NULL, 0, NI_NAMEREQD) != 0) {
            printf("Not found information\n");
            snprintf(result_log, sizeof(result_log), "Not found");
        } else {
            printf("Official name: %s\n", host);
            snprintf(result_log, sizeof(result_log), "Official name: %s", host);
        }

    } else if (looks_like_ipv4(query)) {
        /* Tra ngược IPv4 → hostname */
        struct in_addr addr;
        if (inet_pton(AF_INET, query, &addr) != 1) {
            printf("Invalid address\n");
            log_write(query, "Invalid address");
            return;
        }
        warn_special_ipv4(query);
        struct hostent *h = gethostbyaddr(&addr, sizeof(addr), AF_INET);
        if (!h) {
            printf("Not found information\n");
            snprintf(result_log, sizeof(result_log), "Not found");
        } else {
            printf("Official name: %s\n", h->h_name);
            snprintf(result_log, sizeof(result_log), "Official name: %s", h->h_name);
            if (h->h_aliases && h->h_aliases[0]) {
                printf("Alias name:\n");
                for (int i = 0; h->h_aliases[i]; i++)
                    printf("%s\n", h->h_aliases[i]);
            }
        }

    } else {
        /* Tra xuôi: tên miền → địa chỉ IP (hỗ trợ cả IPv4 và IPv6) */
        struct addrinfo hints, *res, *p;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(query, NULL, &hints, &res) != 0) {
            printf("Not found information\n");
            snprintf(result_log, sizeof(result_log), "Not found");
        } else {
            int first = 1, alias_header = 0;
            char ipbuf[INET6_ADDRSTRLEN];
            for (p = res; p; p = p->ai_next) {
                void *sin_addr = (p->ai_family == AF_INET)
                    ? (void *)&((struct sockaddr_in *)p->ai_addr)->sin_addr
                    : (void *)&((struct sockaddr_in6 *)p->ai_addr)->sin6_addr;
                inet_ntop(p->ai_family, sin_addr, ipbuf, sizeof(ipbuf));
                if (first) {
                    printf("Official IP: %s\n", ipbuf);
                    snprintf(result_log, sizeof(result_log), "Official IP: %s", ipbuf);
                    first = 0;
                } else {
                    if (!alias_header) { printf("Alias IP:\n"); alias_header = 1; }
                    printf("%s\n", ipbuf);
                }
            }
            freeaddrinfo(res);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
    printf("Query time: %.2f ms\n", ms);
    log_write(query, result_log);
}

/* Xử lý một dòng: tách từng token (địa chỉ/tên miền) và tra cứu */
static void process_line(char *line) {
    char *tok = strtok(line, " \t\r\n");
    while (tok) {
        resolve(tok);
        tok = strtok(NULL, " \t\r\n");
    }
}

int main(int argc, char *argv[]) {
    log_fp = fopen(LOG_FILE, "a");

    char line[BUF_SIZE];

    if (argc == 2) {
        /* Thử mở như file batch */
        FILE *fp = fopen(argv[1], "r");
        if (fp) {
            while (fgets(line, sizeof(line), fp))
                process_line(line);
            fclose(fp);
        } else {
            /* Không phải file → tra cứu trực tiếp tham số */
            resolve(argv[1]);
        }
    } else {
        /* Chế độ interactive */
        printf("Enter IP or domain (empty line to quit):\n");
        while (fgets(line, sizeof(line), stdin)) {
            line[strcspn(line, "\n")] = '\0';
            if (line[0] == '\0') break;
            process_line(line);
        }
    }

    if (log_fp) fclose(log_fp);
    return 0;
}
