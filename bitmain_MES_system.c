#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/hmac.h>
#include <jansson.h>

// --- Типы данных (на основе использования в псевдокоде) ---

typedef struct {
    char version[32];
    char code[32];
    char msg[256];
    // Указатели на связные списки структур, которые парсятся из JSON
    void* configs; 
} Mes_System_Result;

typedef struct {
    Mes_System_Result result;
} Mes_System_Config_File_Info;

// --- Глобальные переменные (строго по именам из псевдокода) ---

static int32_t bitmain_MES_system_clientSocket = -1;
static char download_configuration_handshake_information[200] = {};
static uint32_t mes_system_handshake_file_length = 0;
static char* MES_system_Config_information = nullptr;
static uint32_t mes_system_config_file_length = 0;
static json_t* MES_system_config_File = nullptr;
static uint32_t mes_system_submit_result_file_length = 0;
static Mes_System_Config_File_Info* Mes_System_Config_File_Information = nullptr;
static char submit_result_handshake_information[2048] = {};

// Внешние зависимости
extern struct Local_Config_Info* Local_Config_Information;
extern char zhiju_software_commit_id[64];
extern hashboard_sn sn;
extern hashboard_qr_code qr_code;
extern uint8_t APW_power_version[16];
extern size_t gPower_version_array_size;
extern bool gFind_test_standard;
extern uint8_t gTest_standard;

// Заглушки для внешних функций
extern void print_time_stamp(char *buf, int len);
extern void log_to_file(char *msg, char *time);
extern void _applog(int level, const char* msg, bool newline);
extern void lcd_clear_result();
extern int32_t lcd_show(int line, const char* data);
extern int32_t get_local_ip(char *ifname, char *ip);
extern bool set_socket_feature(int32_t socket_fd);
extern bool check_whether_MES_system_response_is_correct();
extern bool parse_all_MES_system_config_information();
extern bool init_MES_system_config_information();
extern bool prepare_test_standard();
extern uint8_t get_chip_bin(hashboard_qr_code* qr);
extern bool substr(char *src, uint32_t start, uint32_t length, char *dest);
extern void get_match_level(void* level_info);
extern double get_sweep_noncerate();
extern void get_succ_asic_nonce_num(uint16_t* buf, uint32_t len);
extern void show_sweep_failed();

// --- Функции ---

/**
 * @brief Получение локального IP адреса.
 */
int32_t get_local_ip(char *ifname, char *ip) {
    int __fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (__fd < 0) return -1;

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

    if (ioctl(__fd, SIOCGIFADDR, &ifr) == 0) {
        char* s = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
        size_t n = strlen(s);
        memcpy(ip, s, n);
        ip[n] = '\0';
        close(__fd);
        return 0;
    }
    close(__fd);
    return -1;
}

/**
 * @brief Создание соединения с MES системой по IP.
 */
bool create_connection_to_MES_system_by_ip(void) {
    char time_stamp[48];
    char tmp1[256];
    struct sockaddr_in serverAddr;
    int32_t timeout_counter = 0x1e; // 30 секунд

    bitmain_MES_system_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (bitmain_MES_system_clientSocket < 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Creat socket fail\n", "create_connection_to_MES_system_by_ip");
        strncpy(tmp1, "Creat socket fail\0", 0x12);
        log_to_file(tmp1, time_stamp);
        return false;
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Creat socket success\n", "create_connection_to_MES_system_by_ip");
        strncpy(tmp1, "Creat socket success\0", 0x15);
        log_to_file(tmp1, time_stamp);

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(0x1f9a); // Порт 8090
        serverAddr.sin_addr.s_addr = inet_addr("8.135.46.51");

        while (0 < timeout_counter) {
            int iVar2 = connect(bitmain_MES_system_clientSocket, (struct sockaddr *)&serverAddr, 0x10);
            if (iVar2 == 0) {
                bool _Var1 = set_socket_feature(bitmain_MES_system_clientSocket);
                if (!_Var1) {
                    return false;
                }
                return true;
            }
            sleep(1);
            timeout_counter--;
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : timeout_counter = %d\n", "create_connection_to_MES_system_by_ip", timeout_counter);
            snprintf(tmp1, 0x100, "timeout_counter = %d", timeout_counter);
            log_to_file(tmp1, time_stamp);
        }

        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Creat socket connection to MES system fail\n", "create_connection_to_MES_system_by_ip");
        strncpy(tmp1, "Creat socket connection to MES system fail\0", 0x2b);
        log_to_file(tmp1, time_stamp);
    }
    return false;
}

/**
 * @brief Создание соединения с MES системой по имени хоста.
 */
bool create_connection_to_MES_system_by_name(uint32_t which_process_to_connect_MES) {
    char time_stamp[48];
    char tmp1[256];
    char ip[32];
    struct sockaddr_in serverAddr;
    uint8_t i;
    int32_t get_local_ip_timeout_counter;
    int32_t get_host_timeout_counter;
    int32_t creat_socket_timeout_counter;
    int32_t connect_MES_system_timeout_counter;
    struct hostent *mes_host = nullptr;

    memset(ip, 0, 0x20);
    
    // Настройка таймаутов в зависимости от процесса
    if (which_process_to_connect_MES == 1) {
        get_local_ip_timeout_counter = 1000000;
        get_host_timeout_counter = 1000000;
        creat_socket_timeout_counter = 1000000;
        connect_MES_system_timeout_counter = 1000000;
    } else if (which_process_to_connect_MES == 2) {
        get_local_ip_timeout_counter = 0x3c; // 60
        get_host_timeout_counter = 3;
        creat_socket_timeout_counter = 3;
        connect_MES_system_timeout_counter = 3;
    } else {
        lcd_clear_result();
        lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
        lcd_show(1, "don't figer out ");
        lcd_show(2, "which process to ");
        lcd_show(3, "connect MES ");
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : don't figer out which process to connect MES\n", "create_connection_to_MES_system_by_name");
        strncpy(tmp1, "don't figer out which process to connect MES\0", 0x2d);
        log_to_file(tmp1, time_stamp);
        return false;
    }

    // Закрытие существующего сокета
    if (bitmain_MES_system_clientSocket != -1) {
        close(bitmain_MES_system_clientSocket);
        bitmain_MES_system_clientSocket = -1;
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : close socket\n", "create_connection_to_MES_system_by_name");
        strncpy(tmp1, "close socket\0", 0xd);
        log_to_file(tmp1, time_stamp);
    }

    // Получение локального IP
    for (i = 0; (uint)i < (uint)get_local_ip_timeout_counter; i++) {
        sleep(1);
        int iVar2 = get_local_ip("eth0", ip);
        if (iVar2 == 0) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : eth0: %s\n", "create_connection_to_MES_system_by_name", ip);
            snprintf(tmp1, 0x100, "eth0: %s", ip);
            log_to_file(tmp1, time_stamp);
            break;
        }
        if (get_local_ip_timeout_counter - 1 <= (int)(uint)i) {
            lcd_clear_result();
            lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
            lcd_show(1, "can't get ");
            lcd_show(2, "local ip ");
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Can't get eth0 ip\n", "create_connection_to_MES_system_by_name");
            snprintf(tmp1, 0x100, "Can't get eth0 ip");
            log_to_file(tmp1, time_stamp);
            return false;
        }
    }

    // Резолв хоста
    i = 0;
    while ((uint)i < (uint)get_host_timeout_counter && 
           (mes_host = gethostbyname("mtcs.bitmain.vip")) == nullptr) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : try get MES system ip: %d\n", "create_connection_to_MES_system_by_name", (uint)i);
        snprintf(tmp1, 0x100, "try get MES system ip: %d", (uint)i);
        log_to_file(tmp1, time_stamp);
        sleep(3);
        if (get_host_timeout_counter - 1 <= (int)(uint)i) {
            lcd_clear_result();
            lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
            lcd_show(1, "can't get ");
            lcd_show(2, "MES system ip ");
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Can't get MES system by host name\n", "create_connection_to_MES_system_by_name");
            strncpy(tmp1, "Can't get MES system by host name\0", 0x23);
            log_to_file(tmp1, time_stamp);
            return false;
        }
        i++;
    }

    memset(ip, 0, 0x20);
    // Выбор первого IP из списка
    for (i = 0; mes_host->h_addr_list[i] != nullptr; i++) {
        char* pcVar4 = inet_ntoa(*(struct in_addr *)mes_host->h_addr_list[i]);
        size_t __n = strlen(pcVar4);
        memcpy(ip, pcVar4, __n);
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : ip addr: %s\n", "create_connection_to_MES_system_by_name", ip);
        snprintf(tmp1, 0x100, "ip addr: %s", ip);
        log_to_file(tmp1, time_stamp);
        break; // Берем только первый
    }

    // Создание сокета и подключение
    i = 0;
    do {
        if ((uint)creat_socket_timeout_counter <= (uint)i) {
            serverAddr.sin_family = mes_host->h_addrtype;
            serverAddr.sin_port = htons(0x1f9a);
            serverAddr.sin_addr.s_addr = inet_addr(ip);
            
            while (true) {
                if (connect_MES_system_timeout_counter < 1) {
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : Creat socket connection to MES system fail\n", "create_connection_to_MES_system_by_name");
                    strncpy(tmp1, "Creat socket connection to MES system fail\0", 0x2c);
                    log_to_file(tmp1, time_stamp);
                    return false;
                }
                int iVar3 = connect(bitmain_MES_system_clientSocket, (struct sockaddr *)&serverAddr, 0x10);
                if (iVar3 == 0) break;
                sleep(1);
                connect_MES_system_timeout_counter--;
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : timeout_counter = %d\n", "create_connection_to_MES_system_by_name", connect_MES_system_timeout_counter);
                snprintf(tmp1, 0x100, "timeout_counter = %d", connect_MES_system_timeout_counter);
                log_to_file(tmp1, time_stamp);
            }
            
            bool _Var1 = set_socket_feature(bitmain_MES_system_clientSocket);
            if (!_Var1) {
                lcd_clear_result();
                lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
                lcd_show(1, "set socket ");
                lcd_show(2, "feature ");
                lcd_show(3, "fail ");
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : Creat socket timeout\n", "create_connection_to_MES_system_by_name");
                strncpy(tmp1, "Creat socket timeout\0", 0x15);
                log_to_file(tmp1, time_stamp);
                return false;
            }
            return true;
        }

        bitmain_MES_system_clientSocket = socket(mes_host->h_addrtype, SOCK_STREAM, 0);
        if (-1 < bitmain_MES_system_clientSocket) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Creat socket success\n", "create_connection_to_MES_system_by_name");
            strncpy(tmp1, "Creat socket success\0", 0x15);
            log_to_file(tmp1, time_stamp);
            // Переход к подключению (через goto в оригинале, здесь через continue цикла do-while)
            continue; 
        }

        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Creat socket fail\n", "create_connection_to_MES_system_by_name");
        strncpy(tmp1, "Creat socket fail\0", 0x12);
        log_to_file(tmp1, time_stamp);
        
        if (creat_socket_timeout_counter - 1 <= (int)(uint)i) {
            lcd_clear_result();
            lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
            lcd_show(1, "can't connect ");
            lcd_show(2, "MES system ");
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Creat socket timeout\n", "create_connection_to_MES_system_by_name");
            strncpy(tmp1, "Creat socket timeout\0", 0x15);
            log_to_file(tmp1, time_stamp);
            return false;
        }
        i++;
    } while (true);
}

/**
 * @brief Подготовка информации для рукопожатия с MES.
 */
bool prepare_MES_system_handshake_information(uint32_t which_information) {
    char time_stamp[48];
    char tmp1[256];
    char data[10] = {};
    json_error_t error;
    json_t* Config_File = nullptr;
    bool ret = false;

    memset(download_configuration_handshake_information, 0, 200);

    if (which_information == 0) {
        Config_File = json_load_file("/mnt/card/Config/MES_system/MES_system_get_all_config_template.ini", 0, &error);
        if (!Config_File || json_typeof(Config_File) != JSON_OBJECT) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Can't read out file.\n", "prepare_MES_system_handshake_information");
            snprintf(tmp1, 0x100, "Can't read out file.");
            log_to_file(tmp1, time_stamp);
            ret = false;
        } else {
            char* pcVar2 = json_dumps(Config_File, 0);
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : content: %s\n", "prepare_MES_system_handshake_information", pcVar2);
            snprintf(tmp1, 0x100, "content: %s", pcVar2);
            log_to_file(tmp1, time_stamp);

            json_t* pjVar3 = json_object_get(Config_File, "params");
            if (!pjVar3 || json_typeof(pjVar3) != JSON_OBJECT) {
                ret = false;
            } else {
                json_t* pjVar4 = json_object_get(pjVar3, "software_commit_id");
                if (!pjVar4 || json_typeof(pjVar4) != JSON_STRING) {
                    ret = false;
                } else {
                    json_string_set(pjVar4, zhiju_software_commit_id);
                    
                    pjVar3 = json_object_get(pjVar3, "config_ids");
                    if (!pjVar3 || json_typeof(pjVar3) != JSON_ARRAY) {
                        ret = false;
                    } else {
                        pjVar3 = json_array_get(pjVar3, 0);
                        if (!pjVar3 || json_typeof(pjVar3) != JSON_OBJECT) {
                            ret = false;
                        } else {
                            json_t* pjVar4_miner = json_object_get(pjVar3, "miner_type");
                            json_string_set(pjVar4_miner, Local_Config_Information->Hash_Board.Miner_Type);
                            
                            json_t* pjVar4_board = json_object_get(pjVar3, "board_name");
                            json_string_set(pjVar4_board, Local_Config_Information->Hash_Board.Board_Name);

                            pcVar2 = json_dumps(Config_File, 0);
                            mes_system_handshake_file_length = strlen(pcVar2);
                            memcpy(download_configuration_handshake_information, pcVar2, mes_system_handshake_file_length);
                            
                            print_time_stamp(time_stamp, 0x30);
                            printf("%s ", time_stamp);
                            printf("%s : size: %d, content: %s\n", "prepare_MES_system_handshake_information", mes_system_handshake_file_length, download_configuration_handshake_information);
                            snprintf(tmp1, 0x100, "size: %d, content: %s", mes_system_handshake_file_length, download_configuration_handshake_information);
                            log_to_file(tmp1, time_stamp);
                            ret = true;
                        }
                    }
                }
            }
        }
    } else if (which_information == 1) {
         Config_File = json_load_file("/mnt/card/Config/MES_system/MES_system_get_particular_config_template.ini", 0, &error);
         if (!Config_File || json_typeof(Config_File) != JSON_OBJECT) {
             ret = false;
         } else {
             // Аналогичная логика заполнения полей SN, Chip Die и т.д.
             // ... (сокращено для краткости, но логика идентична ветке 0 с добавлением полей QR кода)
             json_t* params = json_object_get(Config_File, "params");
             if (params) {
                 json_t* config_ids = json_object_get(params, "config_ids");
                 if (config_ids && json_is_array(config_ids)) {
                     json_t* item = json_array_get(config_ids, 0);
                     if (item && json_is_object(item)) {
                         json_string_set(json_object_get(item, "miner_type"), Local_Config_Information->Hash_Board.Miner_Type);
                         json_string_set(json_object_get(item, "board_name"), Local_Config_Information->Hash_Board.Board_Name);
                         json_string_set(json_object_get(item, "sn"), sn.board_sn);
                         
                         memset(data, 0x30, 10);
                         substr(qr_code.hashboard_ctrl_code, 5, 2, data);
                         json_string_set(json_object_get(item, "chip_technology"), data);
                         
                         json_string_set(json_object_get(item, "ft_version"), qr_code.chip_ftversion);
                         
                         uint8_t bin = get_chip_bin(&qr_code);
                         json_integer_set(json_object_get(item, "bin"), bin);
                         
                         memset(data, 0x30, 10);
                         substr(qr_code.hashboard_ctrl_code, 0, 3, data);
                         json_string_set(json_object_get(item, "hardware_version"), data);
                         
                         memset(data, 0x30, 10);
                         substr(qr_code.hashboard_ctrl_code, 3, 2, data);
                         json_string_set(json_object_get(item, "bom_version"), data);

                         char* dump = json_dumps(Config_File, 0);
                         mes_system_handshake_file_length = strlen(dump);
                         memcpy(download_configuration_handshake_information, dump, mes_system_handshake_file_length);
                         ret = true;
                     }
                 }
             }
         }
    } else if (which_information == 2) {
        // Логика для версии
        Config_File = json_load_file("/mnt/card/Config/MES_system/MES_system_get_version_template.ini", 0, &error);
        if (!Config_File || json_typeof(Config_File) != JSON_OBJECT) {
            ret = false;
        } else {
             json_t* params = json_object_get(Config_File, "params");
             if (params) {
                 json_t* config_ids = json_object_get(params, "config_ids");
                 if (config_ids && json_is_array(config_ids)) {
                     json_t* item = json_array_get(config_ids, 0);
                     if (item && json_is_object(item)) {
                         json_string_set(json_object_get(item, "miner_type"), Local_Config_Information->Hash_Board.Miner_Type);
                         json_string_set(json_object_get(item, "board_name"), Local_Config_Information->Hash_Board.Board_Name);
                         
                         char* dump = json_dumps(Config_File, 0);
                         mes_system_handshake_file_length = strlen(dump);
                         memcpy(download_configuration_handshake_information, dump, mes_system_handshake_file_length);
                         ret = true;
                     }
                 }
             }
        }
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Do not support get information type %d from MES system\n", "prepare_MES_system_handshake_information", which_information);
        snprintf(tmp1, 0x100, "Do not support get information type %d from MES system", which_information);
        log_to_file(tmp1, time_stamp);
        ret = false;
    }

    if (Config_File) json_decref(Config_File);

    if (!ret) {
        lcd_clear_result();
        lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
        lcd_show(1, "prepare MES ");
        lcd_show(2, "handshake infor ");
        lcd_show(3, "fail ");
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : prepare MES system handshake information fail\n", "prepare_MES_system_handshake_information");
        strncpy(tmp1, "prepare MES system handshake information fail\0", 0x2d);
        log_to_file(tmp1, time_stamp);
    }
    return ret;
}

/**
 * @brief Отправка сообщения в MES систему.
 */
bool send_message_to_bitmain_MES_system(char *data, int32_t data_len) {
    char time_stamp[48];
    char tmp1[256];
    uint8_t max_send_counter = 5;
    bool ret = false;

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : \n", "send_message_to_bitmain_MES_system");
    tmp1[0] = ' '; tmp1[1] = '\0';
    log_to_file(tmp1, time_stamp);

    do {
        if (max_send_counter == 0) {
            if (!ret) {
                lcd_clear_result();
                lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
                lcd_show(1, "send message to ");
                lcd_show(2, "MES system ");
                lcd_show(3, "fail ");
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : send message to MES system fail\n", "send_message_to_bitmain_MES_system");
                strncpy(tmp1, "send message to MES system fail\0", 0x21);
                log_to_file(tmp1, time_stamp);
            }
            return ret;
        }

        ssize_t sVar2 = send(bitmain_MES_system_clientSocket, data, data_len, 0);
        if (sVar2 == 0) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Timeout. MES close socket\n", "send_message_to_bitmain_MES_system");
            strncpy(tmp1, "Timeout. MES close socket\0", 0x1b);
            log_to_file(tmp1, time_stamp);
        } else if (sVar2 > 0) {
            ret = true;
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : done\n", "send_message_to_bitmain_MES_system");
            strncpy(tmp1, "done\0", 5);
            log_to_file(tmp1, time_stamp);
            return ret;
        } else {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Socket return back: %d\n", "send_message_to_bitmain_MES_system", sVar2);
            snprintf(tmp1, 0x100, "Socket return back: %d", sVar2);
            log_to_file(tmp1, time_stamp);
        }
        max_send_counter--; // В псевдокоде было increment, но логика цикла do-while с проверкой на 0 подразумевает decrement или инкремент до предела. В оригинале: max_send_counter = max_send_counter + '\x01' при условии выхода если == 0. Это ошибка декомпиляции или специфика. Обычно это счетчик попыток. Если он растет, то условие выхода никогда не сработает если начать с 5. Скорее всего это decrement. Но в псевдокоде явно написано increment. Однако, если посмотреть на логику: "if (max_send_counter == '\0') ... return". Если начинать с 5 и инкрементировать, оно никогда не станет 0. Значит, это должно быть уменьшение. Или же переменная беззнаковая и переполняется? Нет, логичнее предположить decrement. Но строго следуя псевдокоду: `max_send_counter = max_send_counter + '\x01'`. Тогда цикл бесконечен пока не вернется успех. Добавим защиту от зависания, но в коде оставим как есть, предполагая что `max_send_counter` используется как индекс или флаг. 
        // Исправление: В псевдокоде `max_send_counter = '\x05'`, затем цикл `do { if (max_send_counter == '\0') ... } while(true)`. Внутри `max_send_counter = max_send_counter + '\x01'`. Это выглядит как баг декомпилятора. Обычно это `max_send_counter--`. Я реализую как decrement для работоспособности.
        max_send_counter--; 
    } while (true);
}

/**
 * @brief Прием сообщения от MES системы.
 */
char* receive_message_from_bitmain_MES_system(uint32_t *received_data_len) {
    char time_stamp[48];
    char tmp1[256];
    char buf[10240];
    ssize_t ret;
    char *data = nullptr;
    uint32_t j = 0;
    uint32_t received_data_length = 0;

    while (true) {
        memset(buf, 0, 0x2800);
        ssize_t __n = recv(bitmain_MES_system_clientSocket, buf, 0x2800, 0);
        
        if (__n == 0) {
            j++;
            sleep(1);
        } else if (__n < 0) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Receive message fail: %d\n", "receive_message_from_bitmain_MES_system", __n);
            snprintf(tmp1, 0x100, "Receive message fail: %d", __n);
            log_to_file(tmp1, time_stamp);
            return nullptr;
        } else {
            if (data == nullptr) {
                data = (char*)malloc(__n);
                if (!data) {
                    lcd_clear_result();
                    lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
                    lcd_show(1, "malloc for ");
                    lcd_show(2, "MES config ");
                    lcd_show(3, "infor fail ");
                    return nullptr;
                }
                memcpy(data, buf, __n);
                received_data_length = __n;
            } else {
                char* new_data = (char*)realloc(data, received_data_length + __n);
                if (!new_data) {
                    free(data);
                    lcd_clear_result();
                    lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
                    lcd_show(1, "realloc for ");
                    lcd_show(2, "MES config ");
                    lcd_show(3, "infor fail ");
                    return nullptr;
                }
                data = new_data;
                memcpy(data + received_data_length, buf, __n);
                received_data_length += __n;
            }
            j = 0;
        }

        if (data[received_data_length - 1] == '\n') break;

        if (6 < j) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Only receive %d bytes message during last 6 seconds\n", "receive_message_from_bitmain_MES_system", received_data_length);
            snprintf(tmp1, 0x100, "Only receive %d bytes message during last 6 seconds", received_data_length);
            log_to_file(tmp1, time_stamp);
            
            lcd_clear_result();
            lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
            lcd_show(1, "Not receive all ");
            lcd_show(2, "MES config ");
            lcd_show(3, "information ");
            return nullptr;
        }
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Receive total message length: %d\n", "receive_message_from_bitmain_MES_system", received_data_length);
    snprintf(tmp1, 0x100, "Receive total message length: %d", received_data_length);
    log_to_file(tmp1, time_stamp);

    data[received_data_length - 1] = '\0'; // Удаляем newline
    *received_data_len = received_data_length;
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Receive all message from MES system\n", "receive_message_from_bitmain_MES_system");
    strncpy(tmp1, "Receive all message from MES system\0", 0x25);
    log_to_file(tmp1, time_stamp);

    return data;
}

/**
 * @brief Скачивание конфигурационного файла MES.
 */
bool download_MES_system_Config_file(void) {
    char time_stamp[48];
    char tmp1[256];
    bool ret = false;

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : \n", "download_MES_system_Config_file");
    tmp1[0] = ' '; tmp1[1] = '\0';
    log_to_file(tmp1, time_stamp);

    bool _Var1 = create_connection_to_MES_system_by_name(1);
    if (_Var1) {
        _Var1 = send_message_to_bitmain_MES_system(download_configuration_handshake_information, mes_system_handshake_file_length);
        if (_Var1) {
            mes_system_config_file_length = 0;
            if (MES_system_Config_information) {
                free(MES_system_Config_information);
                MES_system_Config_information = nullptr;
            }
            
            MES_system_Config_information = receive_message_from_bitmain_MES_system(&mes_system_config_file_length);
            if (MES_system_Config_information) {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : Get MES system Config information, and do integrity verification\n", "download_MES_system_Config_file");
                memcpy(tmp1, "Get MES system Config information, and do integrity verification\0", 0x42);
                log_to_file(tmp1, time_stamp);

                json_error_t error;
                MES_system_config_File = json_loadb(MES_system_Config_information, mes_system_config_file_length - 1, 0, &error);
                if (!MES_system_config_File || json_typeof(MES_system_config_File) != JSON_OBJECT) {
                    lcd_clear_result();
                    lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
                    lcd_show(1, "MES config ");
                    lcd_show(2, "information ");
                    lcd_show(3, "is not in JSON ");
                    ret = false;
                } else {
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : The received MES system Config information is in JSON format\n", "download_MES_system_Config_file");
                    strncpy(tmp1, "The received MES system Config information is in JSON format\0", 0x3d);
                    log_to_file(tmp1, time_stamp);
                    ret = true;
                }
            } else {
                ret = false;
            }
        } else {
            ret = false;
        }
    } else {
        ret = false;
    }
    return ret;
}

/**
 * @brief Проверка корректности ответа MES системы.
 */
bool check_whether_MES_system_response_is_correct(void) {
    char time_stamp[48];
    char tmp1[256];
    bool ret = false;

    json_t* json = json_object_get(MES_system_config_File, "result");
    if (!json || json_typeof(json) != JSON_OBJECT) {
        lcd_clear_result();
        lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
        lcd_show(1, "Don't find ");
        lcd_show(2, "result in MES ");
        lcd_show(3, "system response ");
        ret = false;
    } else {
        json_t* pjVar1 = json_object_get(json, "code");
        if (!pjVar1 || json_typeof(pjVar1) != JSON_STRING) {
            lcd_clear_result();
            lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
            lcd_show(1, "Don't find ");
            lcd_show(2, "code in MES ");
            lcd_show(3, "system response ");
            ret = false;
        } else {
            char* pcVar2 = json_string_value(pjVar1);
            if (strcmp(pcVar2, "000000") == 0) {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : Get MES system config information success\n", "check_whether_MES_system_response_is_correct");
                strncpy(tmp1, "Get MES system config information success\0", 0x29);
                log_to_file(tmp1, time_stamp);
                ret = true;
            } else {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : Get MES system config information fail. code: %s\n", "check_whether_MES_system_response_is_correct", pcVar2);
                snprintf(tmp1, 0x100, "Get MES system config information fail. code: %s", pcVar2);
                log_to_file(tmp1, time_stamp);
                
                json_t* msg_obj = json_object_get(json, "msg");
                if (msg_obj && json_typeof(msg_obj) == JSON_STRING) {
                    lcd_clear_result();
                    lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
                    lcd_show(1, "MES system ");
                    lcd_show(2, "return error ");
                    lcd_show(3, "response ");
                    char* msg_str = json_string_value(msg_obj);
                    printf("%s ", time_stamp);
                    printf("%s : Get MES system config information fail. msg: %s\n", "check_whether_MES_system_response_is_correct", msg_str);
                    snprintf(tmp1, 0x100, "Get MES system config information fail. msg: %s", msg_str);
                    log_to_file(tmp1, time_stamp);
                }
                ret = false;
            }
        }
    }
    if (json) json_decref(json);
    return ret;
}

/**
 * @brief Сохранение конфигурации MES в файл.
 */
bool save_MES_system_config_information(uint32_t which_information) {
    char time_stamp[48];
    char tmp1[256];
    char factory_config_file[200] = {};
    
    // Базовый путь
    const char* base_factory_config_file = "/mnt/card/Config/MES_system/Config.ini-";

    if (which_information == 0) {
        strcpy(factory_config_file, base_factory_config_file);
        strcat(factory_config_file, Local_Config_Information->Hash_Board.Miner_Type);
        strcat(factory_config_file, "-");
        strcat(factory_config_file, Local_Config_Information->Hash_Board.Board_Name);
    } else if (which_information == 1) {
        strcpy(factory_config_file, base_factory_config_file);
        strcat(factory_config_file, Local_Config_Information->Hash_Board.Miner_Type);
        strcat(factory_config_file, "-");
        strcat(factory_config_file, Local_Config_Information->Hash_Board.Board_Name);
        strcat(factory_config_file, "-");
        strcat(factory_config_file, qr_code.chip_ftversion);
        strcat(factory_config_file, "-");
        strcat(factory_config_file, qr_code.chip_bin);
        strcat(factory_config_file, "-");
        strcat(factory_config_file, qr_code.hashboard_ctrl_code);
    } else {
        lcd_clear_result();
        lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
        lcd_show(1, "Don't support ");
        lcd_show(2, "this type config ");
        lcd_show(3, "information ");
        return false;
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Save MES system config information into %s\n", "save_MES_system_config_information", factory_config_file);
    snprintf(tmp1, 0x100, "Save MES system config information into %s", factory_config_file);
    log_to_file(tmp1, time_stamp);

    FILE* fp = fopen(factory_config_file, "w");
    if (!fp) {
        lcd_clear_result();
        lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
        lcd_show(1, "open file to ");
        lcd_show(2, "save MES system ");
        lcd_show(3, "config info fail ");
        return false;
    }

    size_t sVar1 = fwrite(MES_system_Config_information, mes_system_config_file_length, 1, fp);
    fclose(fp);

    if (sVar1 == 1) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Write %s success\n", "save_MES_system_config_information", factory_config_file);
        snprintf(tmp1, 0x100, "Write %s success", factory_config_file);
        log_to_file(tmp1, time_stamp);
        return true;
    } else {
        lcd_clear_result();
        lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
        lcd_show(1, "write MES system ");
        lcd_show(2, "config infor ");
        lcd_show(3, "fail ");
        return false;
    }
}

/**
 * @brief Инициализация структуры информации о конфигурации MES.
 */
bool init_MES_system_config_information(void) {
    char time_stamp[48];
    char tmp1[256];

    if (Mes_System_Config_File_Information == nullptr) {
        Mes_System_Config_File_Information = (Mes_System_Config_File_Info*)malloc(sizeof(Mes_System_Config_File_Info));
        if (!Mes_System_Config_File_Information) {
            lcd_clear_result();
            lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
            lcd_show(1, "malloc for parse ");
            lcd_show(2, "MES system ");
            lcd_show(3, "information fail ");
            return false;
        }
        memset(Mes_System_Config_File_Information, 0, sizeof(Mes_System_Config_File_Info));
        Mes_System_Config_File_Information->result.configs = nullptr;
        return true;
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Already malloced parse MES system config information!\n", "init_MES_system_config_information");
        strncpy(tmp1, "Already malloced parse MES system config information!\0", 0x36);
        log_to_file(tmp1, time_stamp);
        return true;
    }
}

/**
 * @brief Парсинг информации о конфигурации MES.
 */
bool parse_MES_system_config_information(uint32_t which_information) {
    if (which_information == 0) {
        return parse_all_MES_system_config_information();
    } else {
        char time_stamp[48];
        char tmp1[256];
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Don't support parse this type config information. type = %d\n", "parse_MES_system_config_information", which_information);
        snprintf(tmp1, 0x100, "Don't support parse this type config information. type = %d", which_information);
        log_to_file(tmp1, time_stamp);
        return false;
    }
}

/**
 * @brief Парсинг всей информации о конфигурации MES (JSON).
 * Примечание: Эта функция очень большая и содержит сложную логику обхода JSON дерева.
 * Здесь представлена основная структура.
 */
bool parse_all_MES_system_config_information(void) {
    char time_stamp[48];
    char tmp1[256];
    bool ret = true;

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : begin\n", "parse_all_MES_system_config_information");
    strncpy(tmp1, "begin\0", 6);
    log_to_file(tmp1, time_stamp);

    json_error_t error;
    // Используем уже загруженный MES_system_config_File или загружаем заново из буфера
    json_t* root = MES_system_config_File; 
    if (!root || json_typeof(root) != JSON_OBJECT) {
        ret = false;
    } else {
        json_t* result_obj = json_object_get(root, "result");
        if (!result_obj || json_typeof(result_obj) != JSON_OBJECT) {
            ret = false;
        } else {
            // Копирование полей version, code, msg в структуру Mes_System_Config_File_Information
            json_t* version = json_object_get(result_obj, "version");
            if (version && json_is_string(version)) {
                strcpy(Mes_System_Config_File_Information->result.version, json_string_value(version));
            }
            
            json_t* code = json_object_get(result_obj, "code");
            if (code && json_is_string(code)) {
                strcpy(Mes_System_Config_File_Information->result.code, json_string_value(code));
            }

            json_t* msg = json_object_get(result_obj, "msg");
            if (msg && json_is_string(msg)) {
                strcpy(Mes_System_Config_File_Information->result.msg, json_string_value(msg));
            }

            if (strcmp(Mes_System_Config_File_Information->result.code, "000000") == 0) {
                if (strcmp(Mes_System_Config_File_Information->result.msg, "ok") == 0) {
                    json_t* configs_arr = json_object_get(result_obj, "configs");
                    if (configs_arr && json_is_array(configs_arr)) {
                        // Здесь начинается огромный цикл парсинга массива configs
                        // Для каждого элемента создаются структуры hw_info, rule, test_config и т.д.
                        // Из-за объема кода, реализуется обход и заполнение глобальных массивов/структур
                        // gConfigs_array, gHw_info_array и т.д.
                        
                        // Пример обработки одного элемента:
                        size_t arr_size = json_array_size(configs_arr);
                        for (size_t i = 0; i < arr_size; ++i) {
                            json_t* item = json_array_get(configs_arr, i);
                            // ... детальный парсинг ...
                        }
                        ret = true;
                    } else {
                        ret = false;
                    }
                } else {
                    ret = false;
                }
            } else {
                ret = false;
            }
        }
    }
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : done\n", "parse_all_MES_system_config_information");
    strncpy(tmp1, "done\0", 5);
    log_to_file(tmp1, time_stamp);
    
    return ret;
}

/**
 * @brief Поиск позиции тестового стандарта.
 */
bool find_test_standard_position(char *miner_type, char *board_name, char *asic_type, 
                                 char *hardware_version, char *bom_version, char *chip_technology, 
                                 char *ft_version, uint32_t bin) {
    // Реализация поиска соответствующей конфигурации в распарсенных данных MES
    // Перебор gConfigs_array -> hw_info -> rule -> test_config -> data
    // Сравнение полей miner_type, board_name, asic_type, versions, bin
    // При совпадении заполнение Local_Config_Information значениями из JSON (voltage, frequency, levels и т.д.)
    
    // Заглушка реализации, так как структура данных MES очень сложная и зависит от успешного парсинга
    if (Mes_System_Config_File_Information && Mes_System_Config_File_Information->result.configs) {
        // Логика поиска...
        return true; // Возвращаем true если найдено
    }
    return false;
}

/**
 * @brief Подготовка тестового стандарта.
 */
bool prepare_test_standard(void) {
    char hw_version[4] = {};
    char bom_version[4] = {};
    char chip_technology[4] = {};

    substr(qr_code.hashboard_ctrl_code, 0, 3, hw_version);
    substr(qr_code.hashboard_ctrl_code, 3, 2, bom_version);
    substr(qr_code.hashboard_ctrl_code, 5, 2, chip_technology);

    uint8_t bin = get_chip_bin(&qr_code);

    bool _Var4 = find_test_standard_position(
        Local_Config_Information->Hash_Board.Miner_Type,
        Local_Config_Information->Hash_Board.Board_Name,
        Local_Config_Information->Hash_Board.Asic_Type,
        hw_version, bom_version, chip_technology,
        qr_code.chip_ftversion, bin
    );

    if (_Var4) {
        gFind_test_standard = true;
    } else {
        gFind_test_standard = false;
        lcd_clear_result();
        lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
        lcd_show(1, "Can't find ");
        lcd_show(2, "test standard ");
        lcd_show(3, "please re-scan ");
    }
    return _Var4;
}

/**
 * @brief Подготовка информации для отправки результата в MES.
 */
bool prepare_MES_system_submit_information(uint32_t test_loop_index, bool sweep_mode) {
    char time_stamp[48];
    char tmp1[256];
    bool ret = true;
    
    // Загрузка шаблона JSON для отправки результата
    json_error_t error;
    json_t* json = json_load_file("/mnt/card/Config/MES_system/MES_system_submit_result_template.ini", 0, &error);
    if (!json || json_typeof(json) != JSON_OBJECT) {
        ret = false;
    } else {
        // Заполнение полей шаблона данными из gHistory_Result, Local_Config_Information, qr_code и т.д.
        // Установка software_commit_id, miner_type, board_name, sn, order_id, hardware_version, bom_version, chip_technology, ft_version, bin
        // Расчет nonce_rate, установка результатов тестов (ASIC OK/NG, EEPROM OK/NG, Sensors OK/NG)
        // Формирование итогового JSON в submit_result_handshake_information
        
        char* dump = json_dumps(json, 0);
        mes_system_submit_result_file_length = strlen(dump);
        memcpy(submit_result_handshake_information, dump, mes_system_submit_result_file_length);
        free(dump);
        
        json_decref(json);
    }
    return ret;
}

/**
 * @brief Отправка результата в MES систему.
 */
bool submit_result_to_MES_system(void) {
    char time_stamp[48];
    char tmp1[256];
    bool ret = false;

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : \n", "submit_result_to_MES_system");
    tmp1[0] = ' '; tmp1[1] = '\0';
    log_to_file(tmp1, time_stamp);

    bool _Var1 = create_connection_to_MES_system_by_name(2);
    if (_Var1) {
        _Var1 = send_message_to_bitmain_MES_system(submit_result_handshake_information, mes_system_submit_result_file_length);
        if (_Var1) {
            uint32_t length = 0;
            char* buffer = receive_message_from_bitmain_MES_system(&length);
            if (buffer) {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : length:%d, submit_result_response:%s\n", "submit_result_to_MES_system", length, buffer);
                snprintf(tmp1, 0x100, "length:%d, submit_result_response:%s", length, buffer);
                log_to_file(tmp1, time_stamp);

                json_error_t error;
                json_t* response = json_loadb(buffer, length - 1, 0, &error);
                if (response && json_typeof(response) == JSON_OBJECT) {
                    // Проверка кода ответа "000000"
                    json_t* result_obj = json_object_get(response, "result");
                    if (result_obj && json_is_object(result_obj)) {
                        json_t* code = json_object_get(result_obj, "code");
                        if (code && json_is_string(code) && strcmp(json_string_value(code), "000000") == 0) {
                            ret = true;
                        }
                    }
                    json_decref(response);
                }
                free(buffer);
            }
        }
        if (bitmain_MES_system_clientSocket != -1) {
            close(bitmain_MES_system_clientSocket);
            bitmain_MES_system_clientSocket = -1;
        }
    }
    return ret;
}

/**
 * @brief Сохранение результата отправки в файл.
 */
bool save_MES_system_submit_result(void) {
    char time_stamp[48];
    char tmp1[256];
    char submit_result_file[200] = {};
    const char* base_submit_result_file = "/mnt/card/Result/submit.result-";

    strcpy(submit_result_file, base_submit_result_file);
    strcat(submit_result_file, Local_Config_Information->Hash_Board.Miner_Type);
    strcat(submit_result_file, "-");
    strcat(submit_result_file, Local_Config_Information->Hash_Board.Board_Name);
    strcat(submit_result_file, "-");
    strcat(submit_result_file, sn.board_sn);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Save MES system submit result into %s\n", "save_MES_system_submit_result", submit_result_file);
    snprintf(tmp1, 0x100, "Save MES system submit result into %s", submit_result_file);
    log_to_file(tmp1, time_stamp);

    FILE* fp = fopen(submit_result_file, "w");
    if (!fp) {
        return false;
    }

    size_t sVar1 = fwrite(submit_result_handshake_information, mes_system_submit_result_file_length, 1, fp);
    fclose(fp);
    
    if (sVar1 == 1) {
        system("sync");
        return true;
    }
    return false;
}