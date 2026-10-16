#include <cstdint>
#include <cstring>
#include <cstdio>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <string>

// --- Типы данных и структуры (на основе логики псевдокода) ---

#define MAX_BOARDS 4
#define MAX_CHAINS_PER_BOARD 3
#define UUID_LEN 16
#define MAC_LEN 6

struct ChainInfo {
    uint32_t chain_id;
};

struct BoardInfo {
    uint32_t board_id;
    uint32_t chain_num;
    ChainInfo chain[MAX_CHAINS_PER_BOARD];
};

struct BitmainSysCapability {
    uint32_t board_num;
    BoardInfo board[MAX_BOARDS];
};

struct BitmainUuid {
    char uuid[UUID_LEN];
};

struct BitmainMac {
    uint8_t mac[MAC_LEN];
};

// --- Глобальные переменные (эмуляция статического состояния из .c) ---

static BitmainSysCapability capability = {};
static uint32_t platform_inited = 0;
static uint32_t platform_is_t9 = 0;

// --- Заглушки для внешних зависимостей (FPGA, GPIO, Fan, UI, UART, Log) ---

extern int32_t fpga_init();
extern void fpga_uninit();
extern int32_t fpga_read(uint32_t reg, uint32_t* value);
extern int32_t gpio_init();
extern void gpio_uninit();
extern void fan_init();
extern void fan_uninit();
extern void ui_init();
extern void ui_uninit();
extern void uart_init();
extern void uart_uninit();
extern void _applog(int level, const char* msg, bool newline);

// --- Функции ---

/**
 * @brief Проверяет горячее подключение и заполняет структуру capability.
 * Логика: читает регистр FPGA 2, анализирует битовую маску.
 * Если старший байт отрицательный (бит 31 установлен) -> режим T9.
 * Иначе -> стандартный режим (младшие 16 бит).
 */
void check_hotplug_realtime(BitmainSysCapability* capa) {
    if (!capa) return;

    memset(capa, 0, sizeof(BitmainSysCapability));

    uint32_t local_82c = 0;
    // Чтение регистра FPGA, определяющего наличие плат
    int32_t read_res = fpga_read(2, &local_82c);
    if (read_res != 0 || local_82c == 0) {
        char tmp42[2050] = {0};
        snprintf(tmp42, sizeof(tmp42), "Cannot Find Any Plug In! \r\n");
        _applog(0, tmp42, false);
        return;
    }

    bool bVar1 = false; // Флаг, что найдена хотя бы одна плата
    uint32_t uVar5 = 0; // Счетчик бит/индекс цепи
    uint32_t board_idx = capa->board_num; // Текущий индекс платы в массиве

    // Проверка на режим T9: если старший байт (bits 24-31) имеет установленный знак при приведении к int
    if (static_cast<int32_t>(local_82c) < 0) {
        // Режим T9
        local_82c = local_82c >> 24; // Берем старший байт
        platform_is_t9 = 1;
        
        char tmp42[2050] = {0};
        snprintf(tmp42, sizeof(tmp42), "HASH_ON_PLUG T9 = 0x%x\n", local_82c);
        _applog(2, tmp42, false);

        if (local_82c == 0) {
            return;
        }

        // Обработка битов для T9
        while (local_82c != 0) {
            if ((local_82c & 1) != 0) {
                capa->board[board_idx].board_id = uVar5;
                
                // Жесткая привязка chain_id в зависимости от индекса платы (uVar5)
                if (uVar5 == 1) {
                    capa->board[board_idx].chain[0].chain_id = 2;
                    capa->board[board_idx].chain[1].chain_id = 10;
                    capa->board[board_idx].chain[2].chain_id = 0xb;
                    capa->board[board_idx].chain_num = 3;
                } else if (uVar5 == 0) {
                    capa->board[board_idx].chain[0].chain_id = 1;
                    capa->board[board_idx].chain[1].chain_id = 8;
                    capa->board[board_idx].chain[2].chain_id = 9;
                    capa->board[board_idx].chain_num = 3;
                } else if (uVar5 == 2) {
                    capa->board[board_idx].chain[0].chain_id = 3;
                    capa->board[board_idx].chain[1].chain_id = 0xc;
                    capa->board[board_idx].chain[2].chain_id = 0xd;
                    capa->board[board_idx].chain_num = 3;
                }
                
                board_idx++;
                bVar1 = true;
            }
            local_82c = local_82c >> 1;
            uVar5++;
        }
    } else {
        // Стандартный режим (не T9)
        local_82c = local_82c & 0xffff; // Младшие 16 бит
        platform_is_t9 = 0;

        if (local_82c == 0) {
            return;
        }

        // Обработка битов для стандартного режима
        while (local_82c != 0) {
            if ((local_82c & 1) != 0) {
                uint32_t current_chain_num = capa->board[board_idx].chain_num;
                
                capa->board[board_idx].board_id = uVar5;
                capa->board[board_idx].chain_num = current_chain_num + 1;
                // Добавляем новую цепь с ID равным текущему битовому индексу (uVar5)
                if (current_chain_num < MAX_CHAINS_PER_BOARD) {
                    capa->board[board_idx].chain[current_chain_num].chain_id = uVar5;
                }
                
                board_idx++;
                bVar1 = true;
            }
            local_82c = local_82c >> 1;
            uVar5++;
        }
    }

    if (bVar1) {
        capa->board_num = board_idx;
    }
}

/**
 * @brief Инициализация платформы.
 * Последовательно инициализирует FPGA, GPIO, затем обновляет capability и запускает периферию.
 */
int32_t platform_init() {
    int32_t iVar1 = 0;

    if (platform_inited == 0) {
        iVar1 = fpga_init();
        if (iVar1 == 0) {
            iVar1 = gpio_init();
            if (iVar1 == 0) {
                // Очищаем и заполняем capability
                memset(&capability, 0, sizeof(BitmainSysCapability));
                check_hotplug_realtime(&capability);
                
                platform_inited = 1;
                
                fan_init();
                ui_init();
                uart_init();
                
                return 0;
            } else {
                iVar1 = -2;
                char tmp42[2050] = {0};
                snprintf(tmp42, sizeof(tmp42), "gpio init failed\n");
                _applog(0, tmp42, false);
            }
        } else {
            iVar1 = -1;
            char tmp42[2050] = {0};
            snprintf(tmp42, sizeof(tmp42), "fpga init failed\n");
            _applog(0, tmp42, false);
        }
    }
    return iVar1;
}

/**
 * @brief Деинициализация платформы.
 */
void platform_uninit() {
    if (platform_inited != 0) {
        uart_uninit();
        fpga_uninit();
        fan_uninit();
        ui_uninit();
        gpio_uninit();
        platform_inited = 0;
    }
}

/**
 * @brief Получение полной системной конфигурации.
 */
void get_system_capability(BitmainSysCapability* info) {
    if (info) {
        check_hotplug_realtime(info);
        memcpy(info, &capability, sizeof(BitmainSysCapability));
    }
}

/**
 * @brief Получение MAC-адреса интерфейса eth0.
 * Использует сокет и ioctl SIOCGIFHWADDR (0x8927).
 */
int32_t get_eth_mac(int8_t* device, uint8_t* mac_out) {
    if (!device || !mac_out) return -1;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        _applog(0, "error socket", false);
        return -1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    // Копируем "eth0" в ifr_name
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);

    // 0x8927 это SIOCGIFHWADDR
    if (ioctl(sock, 0x8927, &ifr) < 0) {
        _applog(0, "error ioctl", false);
        close(sock);
        return -2;
    }

    close(sock);

    // Копируем 6 байт MAC адреса
    memcpy(mac_out, ifr.ifr_hwaddr.sa_data, 6);
    
    return 0;
}

/**
 * @brief Получение UUID (в данной реализации просто обертка над MAC или заглушка).
 * В оригинале UUID заполняется нулями, затем вызывается get_eth_mac.
 */
int32_t get_uuid(BitmainUuid* uuid) {
    if (!uuid) return -1;

    memset(uuid->uuid, 0, UUID_LEN);
    
    uint8_t mac[6] = {0};
    int32_t res = get_eth_mac(nullptr, mac);
    
    if (res == 0) {
        memcpy(uuid->uuid, mac, 6);
    }
    
    return res;
}

/**
 * @brief Получение структуры MAC.
 */
int32_t get_mac(BitmainMac* mac) {
    if (!mac) return -1;
    return get_eth_mac(nullptr, mac->mac);
}
