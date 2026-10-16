#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>

// --- Типы данных (на основе использования в псевдокоде) ---

typedef struct {
    uint32_t v[9]; // Массив для хранения байтов команды
    union {
        uint8_t _REGDATA[4];
        uint8_t _ADDR;
        uint8_t _REGADDR;
        uint8_t _Length;
    } u;
} set_config_t;

typedef struct {
    uint32_t v[5];
    union {
        uint8_t _ADDR;
        uint8_t _Length;
    } u;
} set_address_t;

typedef struct {
    uint32_t v[5];
    union {
        uint8_t _ADDR;
        uint8_t _REGADDR;
        uint8_t _Length;
    } u;
} get_status_t;

typedef struct {
    uint32_t v[5];
    union {
        uint8_t _Length;
    } u;
} chain_inactive_t;

// Внешние функции
extern uint8_t ASIC_CRC5(uint8_t *data, uint32_t len);
extern void send_BC_command(uint32_t *cmd, uint32_t chain_id);

// Заглушки для логирования
extern void print_time_stamp(char *buf, int len);
extern void log_to_file(char *msg, char *time);
extern void _applog(int level, const char* msg, bool newline);

// --- Функции ---

/**
 * @brief Генерация команды установки конфигурации регистра.
 */
int32_t generate_set_config_command(
    uint8_t mode, 
    uint32_t which_asic_address, 
    uint32_t register_address,
    uint32_t register_data, 
    set_config_t *command
) {
    byte bVar1;
    int32_t iVar2;

    if (command == nullptr) {
        iVar2 = -1;
    } else {
        iVar2 = 0;
        memset(command->v, 0, sizeof(command->v)); // Инициализация нулями
        
        // Заполнение полей команды согласно битовым маскам из псевдокода
        command->u._REGDATA[3] = (uint8_t)register_data;       // Младший байт данных
        command->u._ADDR = (uint8_t)which_asic_address;        // Адрес ASIC
        command->v[0] = ((mode != 0) << 4) | 0x41;             // Заголовок команды (0x41 или 0x51)
        
        command->u._REGDATA[0] = (uint8_t)(register_data >> 24); // Старший байт данных
        command->v[8] = 0;                                     // Резерв/CRC место
        command->u._REGADDR = (uint8_t)register_address;       // Адрес регистра
        command->u._REGDATA[1] = (uint8_t)(register_data >> 16); // Средний старший байт
        command->u._REGDATA[2] = (uint8_t)(register_data >> 8);  // Средний младший байт
        command->u._Length = 9;                                // Длина пакета

        // Расчет CRC5
        bVar1 = ASIC_CRC5((uint8_t*)command->v, 0x40); // 0x40 бит = 5 байт? Нет, длина в битах или байтах зависит от реализации ASIC_CRC5. В псевдокоде 0x40 (64 decimal) скорее всего ошибка декомпиляции размера, обычно CRC считается по первым N байтам. Судя по структуре v[9], это 36 байт. Но в других функциях используется 0x20 (32 бита/4 байта). 
        // Внимательно смотрим на псевдокод: ASIC_CRC5(command->v, 0x40). Если 0x40 это биты, то это 5 байт. Если байты, то много. 
        // Обычно заголовок + адрес + регистр + данные = 5 байт. 5 * 8 = 40 бит = 0x28. 
        // В псевдокоде часто встречаются магические числа. Оставим как есть, предполагая, что ASIC_CRC5 принимает размер в битах или специфичным образом.
        // Однако, в generate_set_address_command используется 0x20 (32 бита = 4 байта).
        // Для set_config: Header(1) + Addr(1) + RegAddr(1) + Data(4) = 7 байт? Или структура упакована иначе.
        // Следуем строго псевдокоду:
        
        command->v[8] = (command->v[8] & 0xE0) | (bVar1 & 0x1F); // Установка младших 5 бит последнего байта как CRC
    }
    return iVar2;
}

/**
 * @brief Генерация команды установки адреса ASIC.
 */
int32_t generate_set_address_command(uint32_t which_asic_address, set_address_t *command) {
    byte bVar1;
    int32_t iVar2;

    if (command == nullptr) {
        iVar2 = -1;
    } else {
        iVar2 = 0;
        memset(command->v, 0, sizeof(command->v));
        
        command->v[4] = 0;
        command->u._ADDR = (uint8_t)which_asic_address;
        command->u._Length = 5;
        command->v[0] = '@'; // 0x40 - Код команды установки адреса
        
        bVar1 = ASIC_CRC5((uint8_t*)command->v, 0x20); // CRC по первым 4 байтам (32 бита)
        command->v[4] = (command->v[4] & 0xE0) | (bVar1 & 0x1F);
    }
    return iVar2;
}

/**
 * @brief Генерация команды получения статуса.
 */
int32_t generate_get_status_command(
    uint8_t mode, 
    uint32_t which_asic_address, 
    uint32_t register_address,
    get_status_t *command
) {
    byte bVar1;
    int32_t iVar2;

    if (command == nullptr) {
        iVar2 = -1;
    } else {
        iVar2 = 0;
        memset(command->v, 0, sizeof(command->v));
        
        command->u._ADDR = (uint8_t)which_asic_address;
        command->v[4] = 0;
        command->u._REGADDR = (uint8_t)register_address;
        command->u._Length = 5;
        command->v[0] = ((mode != 0) << 4) | 0x42; // 0x42 или 0x52 - Код команды чтения
        
        bVar1 = ASIC_CRC5((uint8_t*)command->v, 0x20);
        command->v[4] = (command->v[4] & 0xE0) | (bVar1 & 0x1F);
    }
    return iVar2;
}

/**
 * @brief Генерация команды деактивации цепи.
 */
int32_t generate_chain_inactive_command(chain_inactive_t *command) {
    byte bVar1;
    int32_t iVar2;

    if (command == nullptr) {
        iVar2 = -1;
    } else {
        iVar2 = 0;
        memset(command->v, 0, sizeof(command->v));
        
        command->v[4] = 0;
        command->u._Length = 5;
        command->v[0] = 'S'; // 0x53 - Код команды Stop/Inactive
        
        bVar1 = ASIC_CRC5((uint8_t*)command->v, 0x20);
        command->v[4] = (command->v[4] & 0xE0) | (bVar1 & 0x1F);
    }
    return iVar2;
}

/**
 * @brief Отправка команды через UART/FPGA.
 * Преобразует массив байтов команды в 32-битные слова для отправки.
 */
int32_t send_command(uint8_t which_chain, uint8_t *command, int32_t length) {
    int iVar1;
    byte *pbVar2;
    uint32_t *puVar3;
    uint uVar4;
    uint uVar5;
    uint uVar6;
    int iVar7;
    uint32_t bc_command[3]; // Буфер для формирования 32-битных слов (макс 12 байт, но команды обычно 5 или 9 байт)
    
    uVar4 = 0;
    bc_command[0] = 0;
    bc_command[1] = 0;
    bc_command[2] = 0;
    puVar3 = bc_command;

    if (length != 0) {
LAB_000d09aa:
        do {
            iVar1 = 0;
            pbVar2 = command + uVar4;
            iVar7 = 1;
            uVar5 = uVar4;
            
            // Цикл упаковки байтов в 32-битное слово (Big Endian / Network Byte Order logic)
            while (uVar5 < (uint)length) {
                uVar6 = (uint)*pbVar2;
                uVar5 = *puVar3; // Текущее значение аккумулятора
                
                if (iVar1 == 0) {
                    *puVar3 = uVar5 | (uVar6 << 24); // Первый байт в старшую позицию
                } else if (iVar1 == 1) {
                    *puVar3 = uVar5 | (uVar6 << 16);
                } else if (iVar1 == 2) {
                    *puVar3 = uVar5 | (uVar6 << 8);
                } else {
                    *puVar3 = uVar5 | uVar6; // Четвертый байт
                    
                    // Если собрали 4 байта (iVar7 == 4), переходим к следующему слову
                    if (iVar7 == 4) {
                        uVar4 = uVar4 + 4;
                        puVar3 = puVar3 + 1;
                        
                        // Проверка выхода за пределы длины
                        if ((uint)length <= uVar4) goto LAB_000d09dc;
                        goto LAB_000d09aa; // Перезапуск внешнего цикла для нового слова
                    }
                }
                
                iVar1 = iVar1 + 1;
                iVar7 = iVar7 + 1;
                pbVar2 = pbVar2 + 1;
                uVar5 = uVar4 + iVar1; // Корректировка индекса для проверки условия цикла
            }
            
            // Обработка остатка, если длина не кратна 4
            uVar4 = uVar4 + 4;
            puVar3 = puVar3 + 1;
        } while (uVar4 < (uint)length);
    }

LAB_000d09dc:
    // Отправка сформированных 32-битных слов в FPGA/UART
    send_BC_command(bc_command, (uint)which_chain);
    
    return 0;
}
