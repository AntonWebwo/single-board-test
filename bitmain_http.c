#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <jansson.h>

// --- Глобальные переменные и константы (строго по псевдокоду) ---

static bool set_os_time = false;

static const char MES_HTTP_URL[] = "http://mes.bitmain.vip:51611/api/Default/GetMes";
static const char MES_KEY_PT1[] = "sC2^xZ5*pI9!lB0,";
static const char PORT_NAME_PT1[] = "PT1";
static const char USER_ID_PT1[] = "PM1";
static const char MES_KEY_PT2[] = "iY4,jI9!lS1?iM3_";
static const char PORT_NAME_PT2[] = "PT2";
static const char USER_ID_PT2[] = "PM2";

static const char PT1_PERMISSION[] = "[{\"sn\":\"%s\",\"type\":\"1\",\"code\":\"00000\",\"codedes\":\"good\",\"results\":\"ok\"}]";
static const char PT2_PERMISSION[] = "[{\"sn\":\"%s\",\"type\":\"1\",\"code\":\"00000\",\"codedes\":\"good\",\"results\":\"ok\"}]";
static const char PT1_SUBMISSION[] = "[{\"sn\":\"%s\",\"type\":\"2\",\"code\":\"%s\",\"codedes\":\"%s\",\"results\":\"%s\"}]";
static const char PT2_SUBMISSION[] = "[{\"sn\":\"%s\",\"type\":\"2\",\"code\":\"%s\",\"codedes\":\"%s\",\"results\":\"%s\"}]";
static const char PTX_SUBMISSION[] = "[{\"sn\":\"%s\",\"type\":\"2\",\"code\":\"%s\",\"codedes\":\"%s\",\"results\":\"%s\"}]";

typedef struct {
    int error_no;
    char error_code[10];
    char error_code_desc[20];
} pt_err_msg_t;

static const pt_err_msg_t err_msg_table[] = {
    {0,  "0000", "good"},
    {1,  "PT001", "power version err"},
    {2,  "PT002", "0 pattern test fail"},
    {3,  "PT003", "x asic not found"},
    {4,  "PT004", "EEPROM data err"},
    {5,  "PT005", "Sensor type err"},
    {6,  "PT006", "BA x err"},
    {7,  "PT007", "Lx test fail"},
    {8,  "PT008", "Power on fail"},
    {9,  "PT009", "PIC comm fail"},
    {10, "PT010", "Sensor comm fail"},
    {11, "PT011", "EEPROM write fail"},
    {12, "PT012", "Over temp protect"}
};

// --- Внешние зависимости (заглушки) ---
extern void print_time_stamp(char *datetime, int len);
extern void log_to_file(char *msg, char *time);
extern void hmac_sha256(uint8_t *out, const uint8_t *data, size_t data_len, const uint8_t *key, size_t key_len);
extern uint32_t get_abs_seconds(const char *date_str);

// --- Вспомогательные функции ---

void noblock_socket(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int32_t get_ip(char *ip) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return -1;

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    memcpy(ifr.ifr_name, "eth0", 4); // Предполагаем eth0 как в оригинале

    if (ioctl(sock, SIOCGIFADDR, &ifr) == 0) {
        char *s = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
        size_t n = strlen(s);
        memcpy(ip, s, n);
        close(sock);
        return 0;
    }
    close(sock);
    return -1;
}

bool sock_connecting(void) {
    return errno == EINPROGRESS;
}

bool interrupted(void) {
    return errno == EINTR;
}

bool set_socket_feature(int32_t socket_fd) {
    int tcp_one = 1;
    int tcp_keepidle = 45;  // 0x2d
    int tcp_keepintvl = 30; // 0x1e
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;

    char time_stamp[48];
    char tmp1[256];

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : socket_fd = %d\n", "set_socket_feature", socket_fd);
    snprintf(tmp1, 0x100, "socket_fd = %d", socket_fd);
    log_to_file(tmp1, time_stamp);

    if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Set socket send timeout fail, ret = %d\n", "set_socket_feature", errno);
        snprintf(tmp1, 0x100, "Set socket send timeout fail, ret = %d", errno);
        log_to_file(tmp1, time_stamp);
        return false;
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Set socket recv timeout fail, ret = %d\n", "set_socket_feature", errno);
        snprintf(tmp1, 0x100, "Set socket recv timeout fail, ret = %d", errno);
        log_to_file(tmp1, time_stamp);
        return false;
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_KEEPALIVE, &tcp_one, sizeof(tcp_one)) < 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Set socket keep alive fail, ret = %d\n", "set_socket_feature", errno);
        snprintf(tmp1, 0x100, "Set socket keep alive fail, ret = %d", errno);
        log_to_file(tmp1, time_stamp);
        return false;
    }

    if (setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &tcp_one, sizeof(tcp_one)) < 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Set socket no delay fail, ret = %d\n", "set_socket_feature", errno);
        snprintf(tmp1, 0x100, "Set socket no delay fail, ret = %d", errno);
        log_to_file(tmp1, time_stamp);
        return false;
    }

    if (setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPCNT, &tcp_one, sizeof(tcp_one)) < 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Set socket keep cnt fail, ret = %d\n", "set_socket_feature", errno);
        snprintf(tmp1, 0x100, "Set socket keep cnt fail, ret = %d", errno);
        log_to_file(tmp1, time_stamp);
        return false;
    }

    if (setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPIDLE, &tcp_keepidle, sizeof(tcp_keepidle)) < 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Set socket keep idle fail, ret = %d\n", "set_socket_feature", errno);
        snprintf(tmp1, 0x100, "Set socket keep idle fail, ret = %d", errno);
        log_to_file(tmp1, time_stamp);
        return false;
    }

    if (setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPINTVL, &tcp_keepintvl, sizeof(tcp_keepintvl)) < 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Set socket keep intvl fail, ret = %d\n", "set_socket_feature", errno);
        snprintf(tmp1, 0x100, "Set socket keep intvl fail, ret = %d", errno);
        log_to_file(tmp1, time_stamp);
        return false;
    }

    return true;
}

int create_connection_to_server_by_ip(char *host, int port) {
    char time_stamp[48];
    char tmp1[256];
    struct sockaddr_in serverAddr;
    int32_t timeout_counter = 30; // 0x1e

    int sockd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockd < 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Creat socket fail\n", "create_connection_to_server_by_ip");
        strncpy(tmp1, "Creat socket fail\0", 0x12);
        log_to_file(tmp1, time_stamp);
        return -3;
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Creat socket success\n", "create_connection_to_server_by_ip");
        strncpy(tmp1, "Creat socket success\0", 0x15);
        log_to_file(tmp1, time_stamp);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons((uint16_t)port);
    serverAddr.sin_addr.s_addr = inet_addr(host);

    while (timeout_counter > 0) {
        int ret = connect(sockd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
        if (ret == 0) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Creat socket connection to server succeed\n", "create_connection_to_server_by_ip");
            strncpy(tmp1, "Creat socket connection to server succeed\0", 0x2a);
            log_to_file(tmp1, time_stamp);

            if (set_socket_feature(sockd)) {
                return sockd;
            }
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : set_socket_feature failed\n", "create_connection_to_server_by_ip");
            strncpy(tmp1, "set_socket_feature failed\0", 0x1c);
            log_to_file(tmp1, time_stamp);
            close(sockd);
            return -1;
        }
        sleep(1);
        timeout_counter--;
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : timeout_counter = %d\n", "create_connection_to_server_by_ip", timeout_counter);
        snprintf(tmp1, 0x100, "timeout_counter = %d", timeout_counter);
        log_to_file(tmp1, time_stamp);
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Creat socket connection to server failed\n", "create_connection_to_server_by_ip");
    strncpy(tmp1, "Creat socket connection to server failed\0", 0x2a);
    log_to_file(tmp1, time_stamp);
    close(sockd);
    return -2;
}

int create_connection_to_server_by_name(char *host, int port) {
    char time_stamp[48];
    char tmp1[256];
    char ip[32];
    struct hostent *mes_host = nullptr;
    int socket_id = -1;
    int32_t connect_MES_system_timeout_counter = 3;

    memset(ip, 0, 0x20);
    sleep(1);

    if (get_ip(ip) != 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Can't get %s ip\n", "create_connection_to_server_by_name", "eth0");
        snprintf(tmp1, 0x100, "Can't get %s ip", "eth0");
        log_to_file(tmp1, time_stamp);
        return -1;
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : %s: %s\n", "create_connection_to_server_by_name", "eth0", ip);
    snprintf(tmp1, 0x100, "%s: %s", "eth0", ip);
    log_to_file(tmp1, time_stamp);

    for (uint8_t i = 0; i < 3 && (mes_host = gethostbyname(host)) == nullptr; ++i) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : try get MES system ip: %d\n", "create_connection_to_server_by_name", i);
        snprintf(tmp1, 0x100, "try get MES system ip: %d", i);
        log_to_file(tmp1, time_stamp);
        sleep(3);
        if (i > 1) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Can't get MES system by host name\n", "create_connection_to_server_by_name");
            strncpy(tmp1, "Can't get MES system by host name\0", 0x23);
            log_to_file(tmp1, time_stamp);
            return -1;
        }
    }

    memset(ip, 0, 0x20);
    for (uint8_t i = 0; mes_host->h_addr_list[i] != nullptr; ++i) {
        char *src = inet_ntoa(*(struct in_addr *)mes_host->h_addr_list[i]);
        size_t n = strlen(src);
        memcpy(ip, src, n);
    }

    for (uint8_t i = 0; i < 3 && (socket_id = socket(mes_host->h_addrtype, SOCK_STREAM, 0)) < 0; ++i) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Creat socket fail\n", "create_connection_to_server_by_name");
        strncpy(tmp1, "Creat socket fail\0", 0x12);
        log_to_file(tmp1, time_stamp);
        if (i > 1) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Creat socket timeout\n", "create_connection_to_server_by_name");
            strncpy(tmp1, "Creat socket timeout\0", 0x15);
            log_to_file(tmp1, time_stamp);
            return -1;
        }
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = mes_host->h_addrtype;
    serverAddr.sin_port = htons((uint16_t)port);
    serverAddr.sin_addr.s_addr = inet_addr(ip);

    while (connect_MES_system_timeout_counter > 0) {
        int ret = connect(socket_id, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
        if (ret == 0) {
            if (set_socket_feature(socket_id)) {
                return socket_id;
            }
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Creat socket timeout\n", "create_connection_to_server_by_name");
            strncpy(tmp1, "Creat socket timeout\0", 0x15);
            log_to_file(tmp1, time_stamp);
            close(socket_id);
            return -1;
        }
        sleep(1);
        connect_MES_system_timeout_counter--;
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : timeout_counter = %d\n", "create_connection_to_server_by_name", connect_MES_system_timeout_counter);
        snprintf(tmp1, 0x100, "timeout_counter = %d", connect_MES_system_timeout_counter);
        log_to_file(tmp1, time_stamp);
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Creat socket connection to MES system fail\n", "create_connection_to_server_by_name");
    strncpy(tmp1, "Creat socket connection to MES system fail\0", 0x2b);
    log_to_file(tmp1, time_stamp);
    close(socket_id);
    return -1;
}

int http_tcpclient_create(char *host, int port, bool ver_1_0) {
    char time_stamp[48];
    char tmp1[256];
    char sockaddr_port[10];
    struct addrinfo hints, *servinfo, *p;
    int socket_id = -1;
    int count = 0;

    memset(sockaddr_port, 0, 10);
    sprintf(sockaddr_port, "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    printf("socket host:%s,port:%s\n", host, sockaddr_port);

    for (int i = 0; i < 5 && getaddrinfo(host, sockaddr_port, &hints, &servinfo) != 0; ++i) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Failed to getaddrinfo for %s:%s, sleep\n", "http_tcpclient_create", host, sockaddr_port);
        snprintf(tmp1, 0x100, "Failed to getaddrinfo for %s:%s, sleep", host, sockaddr_port);
        log_to_file(tmp1, time_stamp);
        res_init();
        sleep(5);
    }

    for (p = servinfo; p != nullptr; p = p->ai_next) {
        printf("socket domain:%d,type:%d,protocol:%d\n", p->ai_family, p->ai_socktype, p->ai_protocol);
        socket_id = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (socket_id == -1) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Failed socket\n", "http_tcpclient_create");
            strncpy(tmp1, "Failed socket\0", 0xe);
            log_to_file(tmp1, time_stamp);
            continue;
        }

        puts("log addr: ");
        for (int i = 0; i < 14; ++i) printf("%2X ", (uint8_t)p->ai_addr->sa_data[i]);
        putchar('\n');
        printf("log connect:%d family:%d,len:%d", socket_id, p->ai_addr->sa_family, p->ai_addrlen);

        if (connect(socket_id, p->ai_addr, p->ai_addrlen) != -1) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Succeeded immediate connect\n\n", "http_tcpclient_create");
            strncpy(tmp1, "Succeeded immediate connect\n\0", 0x1e);
            log_to_file(tmp1, time_stamp);
            noblock_socket(socket_id);
            freeaddrinfo(servinfo);
            return socket_id;
        }

        perror("socket connect err!\n");
        struct timeval tv_timeout = {1, 0};

        if (sock_connecting()) {
            do {
                count++;
                fd_set rd;
                memset(&rd, 0, sizeof(rd));
                FD_SET(socket_id, &rd);

                int selret = select(socket_id + 1, nullptr, &rd, nullptr, &tv_timeout);
                if (selret > 0 && FD_ISSET(socket_id, &rd)) {
                    int err;
                    socklen_t len = sizeof(err);
                    if (getsockopt(socket_id, SOL_SOCKET, SO_ERROR, &err, &len) == 0 && err == 0) {
                        print_time_stamp(time_stamp, 0x30);
                        printf("%s ", time_stamp);
                        printf("%s : Succeeded delayed connect, count:%d\n\n", "http_tcpclient_create", count);
                        snprintf(tmp1, 0x100, "Succeeded delayed connect, count:%d\n", count);
                        log_to_file(tmp1, time_stamp);
                        freeaddrinfo(servinfo);
                        return socket_id;
                    }
                }
            } while (selret < 0 && interrupted());

            close(socket_id);
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Select timeout/failed connect\n\n", "http_tcpclient_create");
            strncpy(tmp1, "Select timeout/failed connect\n\0", 0x1f);
            log_to_file(tmp1, time_stamp);
        } else {
            close(socket_id);
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Failed sock connect\n\n", "http_tcpclient_create");
            strncpy(tmp1, "Failed sock connect\n\0", 0x15);
            log_to_file(tmp1, time_stamp);
        }
    }

    if (p == nullptr) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Failed to connect to server on %s:%s\n\n", "http_tcpclient_create", host, sockaddr_port);
        snprintf(tmp1, 0x100, "Failed to connect to server on %s:%s\n", host, sockaddr_port);
        log_to_file(tmp1, time_stamp);
        socket_id = -2;
    }
    freeaddrinfo(servinfo);
    return socket_id;
}

void http_tcpclient_close(int socket) {
    close(socket);
}

int http_parse_url(char *url, char *host, char *file, int *port) {
    if (!url || !host || !file || !port) return -1;

    char *ptr1;
    if (strncmp(url, "http://", 7) == 0) ptr1 = url + 7;
    else if (strncmp(url, "https://", 8) == 0) ptr1 = url + 8;
    else return -1;

    char *pcVar1 = strchr(ptr1, '/');
    if (!pcVar1) {
        size_t sVar2 = strlen(ptr1);
        memcpy(host, ptr1, sVar2);
        host[sVar2] = '\0';
    } else {
        size_t sVar2 = strlen(ptr1);
        size_t sVar3 = strlen(pcVar1);
        memcpy(host, ptr1, sVar2 - sVar3);
        host[sVar2 - sVar3] = '\0';
        if (pcVar1[1] != '\0') {
            sVar2 = strlen(pcVar1);
            memcpy(file, pcVar1 + 1, sVar2 - 1);
            file[sVar2 - 1] = '\0';
        }
    }

    pcVar1 = strchr(host, ':');
    if (!pcVar1) *port = 80;
    else {
        *pcVar1 = '\0';
        *port = atoi(pcVar1 + 1);
    }
    return 0;
}

int http_tcpclient_recv(int socket, char *lpbuff) {
    fd_set rd;
    memset(&rd, 0, sizeof(rd));
    FD_SET(socket, &rd);
    struct timeval timeout = {20, 0};

    int recvnum = 0;
    if (select(socket + 1, &rd, nullptr, nullptr, &timeout) > 0 && FD_ISSET(socket, &rd)) {
        char time_str[24];
        // get_format_time(time_str); // Заглушка
        printf("prepare recv:%s\n", time_str);
        ssize_t sVar1;
        while ((sVar1 = recv(socket, lpbuff + recvnum, 0x1fff - recvnum, 0)) > 0) {
            recvnum += sVar1;
        }
    }
    return recvnum;
}

int http_tcpclient_send(int socket, char *buff, int size) {
    int sent = 0;
    while (sent < size) {
        ssize_t sVar1 = send(socket, buff + sent, size - sent, 0);
        if (sVar1 == -1) return -1;
        sent += sVar1;
    }
    return sent;
}

char *http_parse_result(char *lpbuf, bool ver1_0, bool raw) {
    char *p_start = strchr(lpbuf, '{');
    char *p_end = strrchr(lpbuf, '}');
    if (!p_start || !p_end) {
        puts("resp not find {...}");
        return nullptr;
    }
    size_t len = p_end - p_start + 2;
    char *response = (char *)malloc(len);
    memset(response, 0, len);
    strncpy(response, p_start, len - 1);
    return response;
}

char *http_get(char *url, bool ver1_0, bool raw) {
    char time_stamp[48];
    char tmp1[256];
    char host_addr[2048], file[2048], http_buf[8192];
    int port = 0;

    memset(host_addr, 0, 0x800);
    memset(file, 0, 0x800);
    memset(http_buf, 0, 0x2000);

    if (!url) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : url is null!\n", "http_get");
        strncpy(tmp1, "url is null!\0", 0xd);
        log_to_file(tmp1, time_stamp);
        return nullptr;
    }

    if (http_parse_url(url, host_addr, file, &port) != 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : parse url failed!\n", "http_get");
        strncpy(tmp1, "parse url failed!\0", 0x12);
        log_to_file(tmp1, time_stamp);
        return nullptr;
    }

    int socket_fd = http_tcpclient_create(host_addr, port, ver1_0);
    if (socket_fd < 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : tcp client create failed\n", "http_get");
        strncpy(tmp1, "tcp client create failed\0", 0x19);
        log_to_file(tmp1, time_stamp);
        return nullptr;
    }

    if (ver1_0)
        sprintf(http_buf, "GET /%s HTTP/1.0\r\nHost: %s:%d\r\nAccept: */*\r\n\r\n", file, host_addr, port);
    else
        sprintf(http_buf, "GET /%s HTTP/1.1\r\nHOST: %s:%d\r\nAccept: */*\r\n\r\n", file, host_addr, port);

    if (http_tcpclient_send(socket_fd, http_buf, strlen(http_buf)) < 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : tcp client send failed\n", "http_get");
        strncpy(tmp1, "tcp client send failed\0", 0x18);
        log_to_file(tmp1, time_stamp);
        http_tcpclient_close(socket_fd);
        return nullptr;
    }

    memset(http_buf, 0, 0x2000);
    if (http_tcpclient_recv(socket_fd, http_buf) < 1) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : tcp client recv failed\n", "http_get");
        strncpy(tmp1, "tcp client recv failed\0", 0x18);
        log_to_file(tmp1, time_stamp);
        http_tcpclient_close(socket_fd);
        return nullptr;
    }

    http_tcpclient_close(socket_fd);
    return http_parse_result(http_buf, ver1_0, raw);
}

char *http_post(char *url, char *post_str) {
    char time_stamp[48];
    char tmp1[256];
    char host_addr[2048], file[2048], lpbuf[8192];
    int port = 0;

    puts("http request");
    memset(lpbuf, 0, 0x2000);
    memset(host_addr, 0, 0x800);
    memset(file, 0, 0x800);

    if (!url || !post_str) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : http_post parameter check failed!\n", "http_post");
        strncpy(tmp1, "http_post parameter check failed!\0", 0x22);
        log_to_file(tmp1, time_stamp);
        return nullptr;
    }

    if (http_parse_url(url, host_addr, file, &port) != 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : http_parse_url failed!\n", "http_post");
        strncpy(tmp1, "http_parse_url failed!\0", 0x18);
        log_to_file(tmp1, time_stamp);
        return nullptr;
    }

    printf("log parse url:%s addr:%s file:%s port:%d \n", url, host_addr, file, port);
    int socket = create_connection_to_server_by_name(host_addr, port);
    if (socket < 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : create socket failed\n", "http_post");
        strncpy(tmp1, "create socket failed\0", 0x15);
        log_to_file(tmp1, time_stamp);
        return nullptr;
    }

    size_t sVar2 = strlen(post_str);
    snprintf(lpbuf, 0x2000,
             "POST /%s HTTP/1.1\r\nHOST: %s:%d\r\nAccept: */*\r\nContent-Type:application/json\r\nConnection:keep-alive\r\nContent-Length: %zu\r\n\r\n",
             file, host_addr, port, sVar2);

    if (http_tcpclient_send(socket, lpbuf, strlen(lpbuf)) < 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : http_tcpclient_send failed..\n", "http_post");
        strncpy(tmp1, "http_tcpclient_send failed..\0", 0x1e);
        log_to_file(tmp1, time_stamp);
        http_tcpclient_close(socket);
        return nullptr;
    }

    if (http_tcpclient_send(socket, post_str, sVar2) < 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : http_tcpclient_send failed..\n", "http_post");
        strncpy(tmp1, "http_tcpclient_send failed..\0", 0x1e);
        log_to_file(tmp1, time_stamp);
        http_tcpclient_close(socket);
        return nullptr;
    }

    char time_str[24];
    // get_format_time(time_str);
    printf("send over time:%s\n", time_str);

    memset(lpbuf, 0, 0x2000);
    if (http_tcpclient_recv(socket, lpbuf) < 1) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : http_tcpclient_recv failed\n", "http_post");
        strncpy(tmp1, "http_tcpclient_recv failed\0", 0x1c);
        log_to_file(tmp1, time_stamp);
        http_tcpclient_close(socket);
        return nullptr;
    }

    http_tcpclient_close(socket);
    // get_format_time(time_str);
    printf("recv over time:%s\n", time_str);
    return http_parse_result(lpbuf, false, false);
}

char *ssl_request(char *url, char *post_str) {
    char time_stamp[48];
    char tmp1[256];
    char host_addr[2048], file[2048], lpbuf[8192];
    int port = 0;
    char *resp = nullptr;

    puts("https request");
    memset(lpbuf, 0, 0x2000);
    memset(host_addr, 0, 0x800);
    memset(file, 0, 0x800);

    if (!url || !post_str) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : http_post parameter check failed!\n", "ssl_request");
        strncpy(tmp1, "http_post parameter check failed!\0", 0x22);
        log_to_file(tmp1, time_stamp);
        return nullptr;
    }

    if (http_parse_url(url, host_addr, file, &port) != 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : http_parse_url failed!\n", "ssl_request");
        strncpy(tmp1, "http_parse_url failed!\0", 0x18);
        log_to_file(tmp1, time_stamp);
        return nullptr;
    }

    printf("host:%s,file:%s,port:%d\n", host_addr, file, port);
    printf("log parse url:%s addr:%s file:%s port:%d \n", url, host_addr, file, port);

    int socket_id = create_connection_to_server_by_name(host_addr, port);
    OPENSSL_init_ssl(0, nullptr);
    OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, nullptr);
    OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, nullptr);

    const SSL_METHOD *meth = TLS_client_method();
    SSL_CTX *ctx = SSL_CTX_new(meth);
    if (!ctx) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    SSL *s = SSL_new(ctx);
    SSL_set_fd(s, socket_id);
    if (SSL_connect(s) == -1) {
        ERR_print_errors_fp(stderr);
    } else {
        const SSL_CIPHER *c = SSL_get_current_cipher(s);
        printf("Connected with %s encryption\n", SSL_CIPHER_get_name(c));
    }

    size_t sVar3 = strlen(post_str);
    snprintf(lpbuf, 0x2000,
             "POST /%s HTTP/1.1\r\nHOST: %s:%d\r\nAccept: */*\r\nContent-Type:application/json\r\nConnection:keep-alive\r\nContent-Length: %zu\r\n\r\n",
             file, host_addr, port, sVar3);

    if (SSL_write(s, lpbuf, strlen(lpbuf)) > -1 && SSL_write(s, post_str, sVar3) > -1) {
        memset(lpbuf, 0, 0x2000);
        int read_bytes = SSL_read(s, lpbuf, 0x2000);
        if (read_bytes < 0) {
            int err = errno;
            printf("msg recv fail,errcode:%d,err msg:%s\n", err, strerror(err));
        } else {
            resp = http_parse_result(lpbuf, false, false);
        }
    }

    SSL_shutdown(s);
    SSL_free(s);
    close(socket_id);
    SSL_CTX_free(ctx);
    return resp;
}

char *post_request(char *url, char *post_str) {
    if (strstr(url, "https") == url) {
        return ssl_request(url, post_str);
    }
    return http_post(url, post_str);
}

int hmac_get_url_message(char *port_name, char *user_id, char *key, char *payload, char *response_msg) {
    char time_stamp[48];
    char tmp1[256];
    char nonce[20], signature_msg[100], get_url[1000];
    uint8_t signature[32];
    char signature_hex[65];

    memset(nonce, 0, 20);
    memset(signature_msg, 0, 100);
    memset(signature, 0, 32);
    memset(signature_hex, 0, 65);
    memset(get_url, 0, 1000);

    struct timeval tv_now;
    gettimeofday(&tv_now, nullptr);
    sprintf(nonce, "%ld", tv_now.tv_sec);

    strncat(signature_msg, port_name, 99);
    strncat(signature_msg, user_id, 99 - strlen(signature_msg));
    strncat(signature_msg, nonce, 99 - strlen(signature_msg));

    hmac_sha256(signature, (uint8_t *)signature_msg, strlen(signature_msg), (uint8_t *)key, strlen(key));

    for (int i = 0; i < 32; ++i) {
        sprintf(signature_hex + i * 2, "%02x", signature[i]);
    }

    snprintf(get_url, 1000, "%s/1?PortName=%s&Userid=%s&Nonce=%s&Signature=%s&json=%s",
             MES_HTTP_URL, port_name, user_id, nonce, signature_hex, payload);

    char *pcVar2 = http_get(get_url, false, false);
    if (!pcVar2) {
        if (response_msg) strncpy(response_msg, "mes disconnect", 100);
        return -1;
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : response messge:%s\n", "hmac_get_url_message", pcVar2);
    snprintf(tmp1, 0x100, "response messge:%s", pcVar2);
    log_to_file(tmp1, time_stamp);

    json_error_t err;
    json_t *json = json_loads(pcVar2, 0, &err);
    free(pcVar2);

    if (!json) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : JSON decode failed(%d): %s\n", "hmac_get_url_message", err.line, err.text);
        snprintf(tmp1, 0x100, "JSON decode failed(%d): %s", err.line, err.text);
        log_to_file(tmp1, time_stamp);
        return 1;
    }

    int ret = 1;
    json_t *arr_entry = json_array_get(json, 0);
    if (json_is_array(json) && arr_entry && json_is_object(arr_entry)) {
        json_t *results = json_object_get(arr_entry, "results");
        if (results && json_is_string(results)) {
            const char *res_str = json_string_value(results);
            if (strncasecmp(res_str, "ok", 2) == 0) {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : result ok\n", "hmac_get_url_message");
                strncpy(tmp1, "result ok\0", 0xa);
                log_to_file(tmp1, time_stamp);
                json_decref(json);
                return 0;
            }
            if (strncasecmp(res_str, "ng", 2) == 0) {
                json_t *offset = json_object_get(arr_entry, "offset");
                if (offset && json_is_string(offset)) {
                    double dVar6 = atof(json_string_value(offset));
                    struct timeval stime;
                    gettimeofday(&stime, nullptr);
                    stime.tv_sec += (long long)dVar6;
                    settimeofday(&stime, nullptr);
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : result ng and set new time offset\n", "hmac_get_url_message");
                    strncpy(tmp1, "result ng and set new time offset\0", 0x22);
                    log_to_file(tmp1, time_stamp);
                    json_decref(json);
                    return (int)dVar6;
                }
            }
        }
        json_t *msg = json_object_get(arr_entry, "msg");
        if (msg && json_is_string(msg)) {
            if (response_msg) strncpy(response_msg, json_string_value(msg), 100);
            json_decref(json);
            return -2;
        }
    }
    json_decref(json);
    return ret;
}

int compose_error_msg(char *sn, bool ok, int error_no, int error_info, char *send_msg) {
    char time_stamp[48];
    char tmp1[256];
    char error_dec[20];
    memset(error_dec, 0, 20);

    if (error_no >= 0 && error_no < 13) {
        if (error_no == 6) snprintf(error_dec, 0x14, "BA %d", error_info);
        else if (error_no == 7) snprintf(error_dec, 0x14, "L%d", error_info);
        else if (error_no == 3) snprintf(error_dec, 0x14, "%d asic", error_info);
        else {
            if (err_msg_table[error_no].error_no != error_no) {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : compose_error_msg, error_no doesn't match:%d\n", "compose_error_msg", error_no);
                snprintf(tmp1, 0x100, "compose_error_msg, error_no doesn't match:%d", error_no);
                log_to_file(tmp1, time_stamp);
                return 2;
            }
            memcpy(error_dec, err_msg_table[error_no].error_code_desc, 20);
        }

        const char *result_str = ok ? "ok" : "ng";
        snprintf(send_msg, 100, PTX_SUBMISSION, sn, err_msg_table[error_no].error_code, error_dec, result_str);
        return 0;
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : compose_error_msg, invalid error_no:%d\n", "compose_error_msg", error_no);
    snprintf(tmp1, 0x100, "compose_error_msg, invalid error_no:%d", error_no);
    log_to_file(tmp1, time_stamp);
    return 2;
}

int get_pt1_test_permission(char *sn, char *response_msg) {
    // В псевдокоде эта функция просто возвращает 0, предполагая, что разрешение получено
    // или используется локальный шаблон PT1_PERMISSION.
    // Для строгого соответствия возвращаем 0.
    return 0;
}

int send_pt1_test_result(char *sn, bool ok, int error_no, int error_info, char *send_msg, char *response_msg) {
    // В псевдокоде эта функция возвращает 0x1234 (маркер заглушки или специфический код)
    return 0x1234;
}

int get_pt2_test_permission(char *sn, char *response_msg) {
    return 0;
}

int send_pt2_test_result(char *sn, bool ok, int error_no, int error_info, char *send_msg, char *response_msg) {
    return 0x1234;
}

bool set_time_based_notify_ntime(char *ntime) {
    char time_stamp[48];
    char tmp1[256];
    uint32_t ntime_value = strtol(ntime, nullptr, 16);

    if (set_os_time || ntime_value == 0) return false;

    struct timeval stime;
    gettimeofday(&stime, nullptr);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : ntime_value:%d\n", "set_time_based_notify_ntime", ntime_value);
    snprintf(tmp1, 0x100, "ntime_value:%d", ntime_value);
    log_to_file(tmp1, time_stamp);

    if ((uint32_t)stime.tv_sec < ntime_value) {
        stime.tv_sec = ntime_value;
        settimeofday(&stime, nullptr);
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : settime notify.ntime successfuly, ntimeValue:%d\n", "set_time_based_notify_ntime", ntime_value);
        snprintf(tmp1, 0x100, "settime notify.ntime successfuly, ntimeValue:%d", ntime_value);
        log_to_file(tmp1, time_stamp);
        set_os_time = true;
        return true;
    }
    return false;
}

const char *__json_array_string(json_t *val, uint entry) {
    if (!val || !json_is_array(val)) return nullptr;
    if (entry >= json_array_size(val)) return nullptr;
    json_t *arr_entry = json_array_get(val, entry);
    if (!arr_entry || !json_is_string(arr_entry)) return nullptr;
    return json_string_value(arr_entry);
}

int get_time_from_pool(char *server_name, int server_port) {
    char time_stamp[48];
    char tmp1[256];
    char http_buf[8192];
    memset(http_buf, 0, 0x2000);

    int socket = http_tcpclient_create(server_name, server_port, false);
    if (socket < 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : tcp client create failed\n", "get_time_from_pool");
        strncpy(tmp1, "tcp client create failed\0", 0x19);
        log_to_file(tmp1, time_stamp);
        return -1;
    }

    memcpy(http_buf, "{\"id\":1, \"method\": \"mining.subscribe\", \"params\": [\"gominer/1.0.0\"]}\n", 0x45);
    if (http_tcpclient_send(socket, http_buf, strlen(http_buf)) != strlen(http_buf)) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : tcp client send failed\n", "get_time_from_pool");
        strncpy(tmp1, "tcp client send failed\0", 0x18);
        log_to_file(tmp1, time_stamp);
        http_tcpclient_close(socket);
        return -2;
    }

    memcpy(http_buf, "{\"id\":2, \"method\": \"mining.authorize\", \"params\": [\"user\", \"123\"]}\n", 0x42);
    if (http_tcpclient_send(socket, http_buf, strlen(http_buf)) != strlen(http_buf)) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : tcp client send failed\n", "get_time_from_pool");
        strncpy(tmp1, "tcp client send failed\0", 0x18);
        log_to_file(tmp1, time_stamp);
        http_tcpclient_close(socket);
        return -3;
    }

    if (http_tcpclient_recv(socket, http_buf) < 1) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : tcp client recv failed\n", "get_time_from_pool");
        strncpy(tmp1, "tcp client recv failed\0", 0x18);
        log_to_file(tmp1, time_stamp);
        http_tcpclient_close(socket);
        return -4;
    }

    char *json_start_msg = http_buf;
    char *pcVar4;
    while ((pcVar4 = strchr(json_start_msg, '\n')) != nullptr) {
        size_t len = pcVar4 - json_start_msg;
        char *string = (char *)malloc(len + 1);
        memcpy(string, json_start_msg, len);
        string[len] = '\0';

        json_error_t err;
        json_t *json = json_loads(string, 0, &err);
        free(string);
        json_start_msg = pcVar4 + 1;

        if (!json) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : JSON decode failed(%d): %s\n", "get_time_from_pool", err.line, err.text);
            snprintf(tmp1, 0x100, "JSON decode failed(%d): %s", err.line, err.text);
            log_to_file(tmp1, time_stamp);
            continue;
        }

        json_t *method = json_object_get(json, "method");
        json_t *params = json_object_get(json, "params");
        const char *method_str = method ? json_string_value(method) : nullptr;

        if (method_str && strncasecmp(method_str, "mining.notify", 13) == 0) {
            const char *ntime_str = __json_array_string(params, 7);
            if (ntime_str) {
                set_time_based_notify_ntime((char *)ntime_str);
                json_decref(json);
                goto LAB_EXIT;
            }
        }
        json_decref(json);
    }

    http_tcpclient_close(socket);
    return -5;

LAB_EXIT:
    http_tcpclient_close(socket);
    return 0;
}

int get_time_from_web(char *url) {
    char time_stamp[48];
    char tmp1[256];
    char real_date[100];
    memset(real_date, 0, 100);

    char *response = http_get(url, false, true);
    if (!response) return -1;

    char *date_start = strstr(response, "Date: ");
    char *date_end = date_start ? strstr(date_start, "\r\n") : nullptr;

    if (date_end) {
        memcpy(real_date, date_start, date_end - date_start);
    }

    uint32_t time_seconds = get_abs_seconds(real_date + 11);
    struct timeval stime;
    gettimeofday(&stime, nullptr);

    if ((uint32_t)stime.tv_sec < time_seconds) {
        stime.tv_sec = time_seconds;
        settimeofday(&stime, nullptr);
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : set time successfuly, time_seconds:%d\n", "get_time_from_web", time_seconds);
        snprintf(tmp1, 0x100, "set time successfuly, time_seconds:%d", time_seconds);
        log_to_file(tmp1, time_stamp);
    }

    free(response);
    return 0;
}

int32_t get_power_from_server(char *server_ip, int32_t port, uint32_t *power_result) {
    char time_stamp[48];
    char tmp1[256];
    char url[128];
    memset(url, 0, 0x80);
    sprintf(url, "http://%s:%d/api/v1/power", server_ip, port);

    char *string = http_get(url, true, false);
    if (!string) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : http get failed\n", "get_power_from_server");
        strncpy(tmp1, "http get failed\0", 0x11);
        log_to_file(tmp1, time_stamp);
        return -1;
    }

    json_error_t err;
    json_t *json = json_loads(string, 0, &err);
    free(string);

    if (!json) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : JSON decode failed(%d): %s\n", "get_power_from_server", err.line, err.text);
        snprintf(tmp1, 0x100, "JSON decode failed(%d): %s", err.line, err.text);
        log_to_file(tmp1, time_stamp);
        return -1;
    }

    json_t *power_val = json_object_get(json, "power");
    if (!power_val) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : no json object [power] found\n", "get_power_from_server");
        strncpy(tmp1, "no json object [power] found\0", 0x1d);
        log_to_file(tmp1, time_stamp);
        json_decref(json);
        return -1;
    }

    json_int_t jVar1 = json_integer_value(power_val);
    if (jVar1 < 1) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : IP not found.\n", "get_power_from_server");
        strncpy(tmp1, "IP not found.\0", 0xe);
        log_to_file(tmp1, time_stamp);
        json_decref(json);
        return -1;
    }

    *power_result = (uint32_t)jVar1;
    json_decref(json);
    return 0;
}

int32_t sync_time_from_network(void) {
    int ret = get_time_from_web("http://www.baidu.com");
    if (ret == 0 ||
        (ret = get_time_from_web("http://www.taobao.com")) == 0 ||
        (ret = get_time_from_web("http://www.tencent.com")) == 0 ||
        (ret = get_time_from_pool("ss.antpool.com", 3333)) == 0 ||
        (ret = get_time_from_pool("us.ss.btc.com", 1800)) == 0 ||
        (ret = get_time_from_pool("btc.ss.poolin.com", 443)) == 0 ||
        (ret = get_time_from_pool("btc.f2pool.com", 1314)) == 0 ||
        (ret = get_time_from_pool("eu.ss.btc.com", 1800)) == 0) {
        return 0;
    }
    return 1;
}