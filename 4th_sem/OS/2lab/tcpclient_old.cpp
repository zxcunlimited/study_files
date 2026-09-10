#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

static int send_all(int s, const void *buf, size_t len)
{
    const char *p = (const char *)buf;
    size_t sent = 0;

    while (sent < len) {
        ssize_t n = send(s, p + sent, len - sent, MSG_NOSIGNAL);
        if (n <= 0) {
            return -1;
        }
        sent += (size_t)n;
    }

    return 0;
}

static int recv_all(int s, void *buf, size_t len)
{
    char *p = (char *)buf;
    size_t received = 0;

    while (received < len) {
        ssize_t n = recv(s, p + received, len - received, 0);
        if (n <= 0) {
            return -1;
        }
        received += (size_t)n;
    }

    return 0;
}

static int parse_date(const char *s, uint32_t *value)
{
    if (s == NULL || value == NULL) {
        return -1;
    }

    if (strlen(s) < 10) {
        return -1;
    }

    for (int i = 0; i < 10; ++i) {
        if (i == 2 || i == 5) {
            if (s[i] != '.') {
                return -1;
            }
        } else {
            if (s[i] < '0' || s[i] > '9') {
                return -1;
            }
        }
    }

    int day = (s[0] - '0') * 10 + (s[1] - '0');
    int month = (s[3] - '0') * 10 + (s[4] - '0');
    int year = (s[6] - '0') * 1000 + (s[7] - '0') * 100 + (s[8] - '0') * 10 + (s[9] - '0');

    if (day < 1 || day > 31 || month < 1 || month > 12) {
        return -1;
    }

    *value = (uint32_t)(year * 10000 + month * 100 + day);
    return 10;
}

static int parse_time(const char *s, uint32_t *value)
{
    if (s == NULL || value == NULL) {
        return -1;
    }

    if (strlen(s) < 8) {
        return -1;
    }

    for (int i = 0; i < 8; ++i) {
        if (i == 2 || i == 5) {
            if (s[i] != ':') {
                return -1;
            }
        } else {
            if (s[i] < '0' || s[i] > '9') {
                return -1;
            }
        }
    }

    int hour = (s[0] - '0') * 10 + (s[1] - '0');
    int minute = (s[3] - '0') * 10 + (s[4] - '0');
    int second = (s[6] - '0') * 10 + (s[7] - '0');

    if (hour > 23 || minute > 59 || second > 59) {
        return -1;
    }

    *value = (uint32_t)(hour * 10000 + minute * 100 + second);
    return 8;
}

static int parse_address(const char *arg, struct sockaddr_in *addr, char *ip_buf, size_t ip_buf_size)
{
    char buffer[256];
    char *colon;
    long port;

    if (arg == NULL || addr == NULL || ip_buf == NULL || ip_buf_size == 0) {
        return -1;
    }

    strncpy(buffer, arg, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    colon = strrchr(buffer, ':');
    if (colon == NULL) {
        return -1;
    }

    *colon = '\0';
    ++colon;

    port = strtol(colon, NULL, 10);
    if (port <= 0 || port > 65535) {
        return -1;
    }

    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons((uint16_t)port);

    if (inet_aton(buffer, &addr->sin_addr) == 0) {
        return -1;
    }

    strncpy(ip_buf, buffer, ip_buf_size - 1);
    ip_buf[ip_buf_size - 1] = '\0';

    return (int)port;
}

static int parse_line(const char *line, uint32_t *date, uint32_t *time1, uint32_t *time2, const char **message)
{
    const char *p;
    int n;

    if (line == NULL || date == NULL || time1 == NULL || time2 == NULL || message == NULL) {
        return -1;
    }

    p = line;

    n = parse_date(p, date);
    if (n < 0) {
        return -1;
    }
    p += n;

    if (*p != ' ') {
        return -1;
    }
    ++p;

    n = parse_time(p, time1);
    if (n < 0) {
        return -1;
    }
    p += n;

    if (*p != ' ') {
        return -1;
    }
    ++p;

    n = parse_time(p, time2);
    if (n < 0) {
        return -1;
    }
    p += n;

    if (*p != ' ') {
        return -1;
    }
    ++p;

    *message = p;
    return 0;
}

int main(int argc, char *argv[])
{
    struct sockaddr_in server_addr;
    char server_ip[64];
    int server_port;
    int sock = -1;
    FILE *file;
    char line[512 * 1024]; // увеличено кол-во Кб 
    uint32_t index = 0;

    if (argc != 3) {
        printf("Usage: %s IP:PORT input.txt\n", argv[0]);
        return 1;
    }

    server_port = parse_address(argv[1], &server_addr, server_ip, sizeof(server_ip));
    if (server_port < 0) {
        printf("Error: invalid server address\n");
        return 1;
    }

    file = fopen(argv[2], "r");
    if (file == NULL) {
        printf("Error: cannot open file '%s'\n", argv[2]);
        return 1;
    }

    for (int attempt = 0; attempt < 10; ++attempt) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            printf("Error: socket() failed\n");
            fclose(file);
            return 1;
        }

        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0) {
            break;
        }

        close(sock);
        sock = -1;
        usleep(100000);
    }

    if (sock < 0) {
        printf("Error: could not connect to %s:%d\n", server_ip, server_port);
        fclose(file);
        return 1;
    }

    if (send_all(sock, "put", 3) < 0) {
        printf("Error: failed to send request header\n");
        fclose(file);
        close(sock);
        return 1;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        uint32_t date;
        uint32_t time1;
        uint32_t time2;
        const char *message;
        uint32_t message_len;
        uint32_t net_index;
        uint32_t net_date;
        uint32_t net_time1;
        uint32_t net_time2;
        uint32_t net_message_len;
        char reply[2];
        size_t len = strlen(line);

        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }

        if (len == 0) {
            continue;
        }

        if (parse_line(line, &date, &time1, &time2, &message) < 0) {
            continue; // пропуск плохой строки вместо завершения работы клиента
        }

        message_len = (uint32_t)strlen(message);

        net_index = htonl(index);
        net_date = htonl(date);
        net_time1 = htonl(time1);
        net_time2 = htonl(time2);
        net_message_len = htonl(message_len);

        if (send_all(sock, &net_index, sizeof(net_index)) < 0 ||
            send_all(sock, &net_date, sizeof(net_date)) < 0 ||
            send_all(sock, &net_time1, sizeof(net_time1)) < 0 ||
            send_all(sock, &net_time2, sizeof(net_time2)) < 0 ||
            send_all(sock, &net_message_len, sizeof(net_message_len)) < 0 ||
            (message_len > 0 && send_all(sock, message, message_len) < 0)) {
            printf("Error: send failed\n");
            fclose(file);
            close(sock);
            return 1;
        }

        if (recv_all(sock, reply, sizeof(reply)) < 0) {
            printf("Error: connection lost while waiting for confirmation\n");
            fclose(file);
            close(sock);
            return 1;
        }

        if (reply[0] != 'o' || reply[1] != 'k') {
            printf("Error: unexpected server response\n");
            fclose(file);
            close(sock);
            return 1;
        }

        ++index;
    }

    fclose(file);
    close(sock);
    return 0;
}