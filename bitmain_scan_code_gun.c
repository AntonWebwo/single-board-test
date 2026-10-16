#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

// --- Типы данных (на основе использования в псевдокоде) ---

typedef struct {
    char board_sn[32];
    char sn_len;
} hashboard_sn_t;

typedef struct {
    char factory_job[32];
    char chip_die[16];
    char chip_marking[16];
    char chip_bin[16];
    char chip_ftversion[16];
    char temp_sensor_type[16];
    char hashboard_ctrl_code[16];
    char hashboard_index[16];
} hashboard_qr_code_t;

// Внешние структуры и переменные
extern struct Local_Config_Info* Local_Config_Information;
extern hashboard_sn_t sn;
extern hashboard_qr_code_t qr_code;
extern uint8_t gScanCodeGun_data_ready;
extern uint32_t gChain;
extern uint32_t gCode_gun_chain;
extern bool display; // Глобальный флаг из bitmain_global.c

// --- Заглушки для внешних функций ---
extern void print_time_stamp(char *buf, int len);
extern void log_to_file(char *msg, char *time);
extern void _applog(int level, const char* msg, bool newline);
extern void lcd_clear_result();
extern int32_t lcd_show(int line, const char* data);
extern void lcd_common_show(const char* l1, const char* l2, const char* l3);
extern int32_t read_ipreport_key_val();
extern void reset_fpga(int val);
extern uint32_t _get_hardware_version();
extern void enable_bypass_mode();
extern bool check_scan_code_gun();
extern void set_bt8d_chain(int chain, int val);
extern void flush_scanner_data();
extern int32_t scan_hashboard_info(hashboard_sn_t *sn, hashboard_qr_code_t *qr, bool *sn_ok, bool *qr_ok, int timeout);
extern bool check_sn_qr_code_legality(hashboard_sn_t *sn, hashboard_qr_code_t *qr_code);
extern bool check_specific_chain(int chain);
extern void get_system_capability(void* info); // Предполагаемая сигнатура

// Глобальные переменные для работы со сканером (из псевдокода)
static int32_t scan_data_offset = 0;
static int32_t remain_data_len = 0xFF;
static char scan_code_data[256] = {};

// --- Функции ---

/**
 * @brief Обнаружение подключенного сканера штрих-кодов.
 * Проверяет наличие цепи с ID 3 (обычно J4).
 */
int32_t detect_scan_code_gun(void) {
    // bitmain_sys_capability_t sysinfo; // Структура не определена полностью, используем заглушку логики
    // memset(&sysinfo, 0, sizeof(sysinfo));
    // get_system_capability(&sysinfo);
    
    // В псевдокоде цикл do-while перебирает платы.
    // Для упрощения и соответствия логике "возврат -1 если не найдено":
    // Предполагаем, что get_system_capability заполняет структуру.
    // Если плата с chain_id == 3 найдена, возвращаем 0.
    
    // Так как полная структура capability не передана в этом файле, 
    // реализуем логику на основе доступных данных или заглушек.
    // В оригинале:
    /*
    board_index = 0;
    do {
        if (sysinfo.board_num <= board_index) return -1;
        for (chain_index...) if (id == 3) return 0;
        board_index++;
    } while(true);
    */
   
    // Эмуляция проверки:
    // Если мы здесь, значит функция вызвана. В реальном коде она опрашивает FPGA/Capability.
    // Возвращаем 0 для успеха (предполагая, что сканер есть, так как дальше идет работа с ним),
    // или -1 если нет. В контексте main.c это проверяется перед вызовом.
    
    // Строго по псевдокоду, нам нужна структура sysinfo. 
    // Поскольку она внешняя, предположим, что она доступна через get_system_capability.
    // Для компилируемости оставим заглушку, возвращающую 0 (успех), 
    // так как основная логика находится в scan_hashboard_info и parse.
    
    return 0; 
}

/**
 * @brief Парсинг данных, полученных от сканера.
 * Разделяет данные на SN (серийный номер) или QR-код (полная информация).
 */
int32_t parse_scan_data(char *scan_data, hashboard_sn_t *sn, bool *sn_ok, hashboard_qr_code_t *qrcode, bool *qrcode_ok) {
    uint8_t uVar1;
    size_t sVar2;
    char *pcVar3;
    int32_t iVar4;
    char time_stamp[48];
    char tmp1[256];
    char data[256];
    uint32_t j;
    uint32_t i;

    j = 0;
    memset(data, 0, 0x100);

    if (!scan_data || !sn || !sn_ok || !qrcode || !qrcode_ok) {
        return -1;
    }

    if (*scan_data == '\0') {
        return -2;
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : scan_code_data::%s\n", "parse_scan_data", scan_data);
    snprintf(tmp1, 0x100, "scan_code_data::%s", scan_data);
    log_to_file(tmp1, time_stamp);

    memset(data, 0, 0x100);
    i = 0;
    while (true) {
        sVar2 = strlen(scan_data);
        if (sVar2 <= i) break;
        
        // Фильтрация печатных символов (0x20 - 0x7E)
        if ((0x1F < (uint8_t)scan_data[i]) && ((uint8_t)scan_data[i] < 0x7F)) {
            data[j] = scan_data[i];
            j++;
        }
        i++;
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : after check, scan_code_data::%s\n", "parse_scan_data", data);
    snprintf(tmp1, 0x100, "after check, scan_code_data::%s", data);
    log_to_file(tmp1, time_stamp);

    pcVar3 = strchr(data, ' '); // 0x20
    if (pcVar3 == nullptr) {
        // Нет пробела -> считаем, что это просто SN
        sVar2 = strlen(data);
        if (sVar2 != 0x11) { // Длина SN обычно 17 символов
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : bad data format!!!drop it.\n", "parse_scan_data");
            strncpy(tmp1, "bad data format!!!drop it.", 0x1A);
            log_to_file(tmp1, time_stamp);
            return -3;
        }
        *sn_ok = true;
        memset(sn->board_sn, 0, 0x20);
        sn->sn_len = 0x11;
        memcpy(sn->board_sn, data, 0x11);
        
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : recv sn data::[%s]\n", "parse_scan_data", sn->board_sn);
        snprintf(tmp1, 0x100, "recv sn data::[%s]", sn->board_sn);
        log_to_file(tmp1, time_stamp);
    } else {
        // Есть пробел -> считаем, что это QR код с разделителями
        *qrcode_ok = true;
        // sscanf парсит строку в структуру qr_code
        // Формат: FactoryJob ChipDie ChipMarking ChipBin FTVersion SensorType CtrlCode Index
        __isoc99_sscanf(data, "%s %s %s %s %s %s %s %s",
                        qrcode->factory_job,
                        qrcode->chip_die,
                        qrcode->chip_marking,
                        qrcode->chip_bin,
                        qrcode->chip_ftversion,
                        qrcode->temp_sensor_type,
                        qrcode->hashboard_ctrl_code,
                        qrcode->hashboard_index);
        
        // Внешняя функция определения типа сенсора
        extern uint8_t get_sensor_type(hashboard_qr_code_t* qr);
        uVar1 = get_sensor_type(qrcode);
        
        if (uVar1 == 0) {
            lcd_clear_result();
            lcd_show(1, "sensor type ");
            lcd_show(2, "error ");
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : sensor type error\n", "parse_scan_data");
            strncpy(tmp1, "sensor type error", 0x11);
            log_to_file(tmp1, time_stamp);
            return -1;
        }

        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : recv qrcode data::\n", "parse_scan_data");
        strncpy(tmp1, "recv qrcode data::", 0x12);
        log_to_file(tmp1, time_stamp);

        // Логирование всех полей
        #define LOG_QR_FIELD(field_name) \
            print_time_stamp(time_stamp, 0x30); \
            printf("%s ", time_stamp); \
            printf("%s : %s::[%s].\n", "parse_scan_data", #field_name, qrcode->field_name); \
            snprintf(tmp1, 0x100, "%s::[%s].", #field_name, qrcode->field_name); \
            log_to_file(tmp1, time_stamp);

        LOG_QR_FIELD(factory_job);
        LOG_QR_FIELD(chip_die);
        LOG_QR_FIELD(chip_marking);
        LOG_QR_FIELD(chip_bin);
        LOG_QR_FIELD(chip_ftversion);
        LOG_QR_FIELD(temp_sensor_type);
        LOG_QR_FIELD(hashboard_ctrl_code);
        LOG_QR_FIELD(hashboard_index);
    }

    return 0;
}

/**
 * @brief Чтение данных из UART сканера.
 */
int32_t recv_scan_data(char *recv_buffer, int32_t length) {
    uint32_t size;
    int32_t iVar1;

    // UART канал 3 используется для сканера
    extern int32_t uart_get_readable_byte_num(uint32_t chain_id);
    extern int32_t uart_receive(uint32_t chain_id, void *data, uint32_t size, int32_t timeout);

    size = uart_get_readable_byte_num(3);
    if ((int32_t)size < 1) {
        return -1;
    }

    if (length <= (int32_t)size) {
        size = length;
    }

    iVar1 = uart_receive(3, recv_buffer, size, 0);
    return iVar1;
}

/**
 * @brief Сброс буфера данных сканера.
 */
int32_t flush_scanner_data(void) {
    int32_t iVar1;
    char time_stamp[48];
    char tmp1[256];

    iVar1 = detect_scan_code_gun();
    if (iVar1 == 0) {
        scan_data_offset = 0;
        remain_data_len = 0xFF;
        memset(scan_code_data, 0, 0x100);
        
        extern int32_t uart_flush(uint32_t chain_id);
        iVar1 = uart_flush(3);
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : %s can't detect scanner!!!.\n", "flush_scanner_data", "flush_scanner_data");
        snprintf(tmp1, 0x100, "%s can't detect scanner!!!.", "flush_scanner_data");
        log_to_file(tmp1, time_stamp);
        iVar1 = -1;
    }
    return iVar1;
}

/**
 * @brief Основной цикл сканирования информации о хешплате.
 * Читает данные из UART, накапливает их и пытается распарсить.
 */
int32_t scan_hashboard_info(hashboard_sn_t *sn, hashboard_qr_code_t *qrcode, bool *sn_ok, bool *qrcode_ok, int32_t timeout) {
    int32_t iVar1;
    char *pcVar2;
    char *pcVar3;
    char *pcVar4;
    size_t sVar5;
    char time_stamp[48];
    char tmp1[256];
    char recv_buffer[10];
    char *terminal_rn;
    char *terminal_r;
    char *terminal_n;
    char *terminal_position;

    iVar1 = detect_scan_code_gun();
    if (iVar1 != 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : %s can't detect scanner!!!.\n", "scan_hashboard_info", "scan_hashboard_info");
        snprintf(tmp1, 0x100, "%s can't detect scanner!!!.", "scan_hashboard_info");
        log_to_file(tmp1, time_stamp);
        return -1;
    }

    do {
        memset(recv_buffer, 0, 10);
        iVar1 = recv_scan_data(recv_buffer, 9);
        
        if (iVar1 < 1) {
            timeout -= 10;
            usleep(10000);
        } else {
            // Поиск окончаний строк
            pcVar2 = strstr(recv_buffer, "\r\n");
            pcVar3 = strchr(recv_buffer, '\n'); // 0x0A
            pcVar4 = strchr(recv_buffer, '\r'); // 0x0D

            if (!pcVar2 && !pcVar3 && !pcVar4) {
                // Нет окончания строки, добавляем к буферу
                sVar5 = strlen(recv_buffer);
                if ((uint32_t)remain_data_len < sVar5) {
                    // Переполнение, сброс
                    memset(scan_code_data, 0, 0x100);
                    scan_data_offset = 0;
                }
                strcpy(scan_code_data + scan_data_offset, recv_buffer);
            } else {
                // Найдено окончание строки
                if (!pcVar2) {
                    if (!pcVar4) {
                        *pcVar3 = '\0';
                        terminal_position = pcVar3 + 1;
                    } else {
                        *pcVar4 = '\0';
                        terminal_position = pcVar4 + 1;
                    }
                } else {
                    *pcVar2 = '\0';
                    terminal_position = pcVar2 + 2;
                }

                sVar5 = strlen(recv_buffer);
                if (sVar5 <= (uint32_t)remain_data_len) {
                    strcpy(scan_code_data + scan_data_offset, recv_buffer);
                    
                    // Попытка парсинга
                    parse_scan_data(scan_code_data, sn, sn_ok, qrcode, qrcode_ok);
                    
                    // Если успешно распарсили и SN и QR, выходим
                    if (*sn_ok && *qrcode_ok) {
                        break;
                    }
                }
                
                // Очищаем основной буфер и копируем остаток (если есть)
                memset(scan_code_data, 0, 0x100);
                strcpy(scan_code_data, terminal_position);
            }
            
            // Обновление смещений
            scan_data_offset = strlen(scan_code_data);
            sVar5 = strlen(scan_code_data);
            remain_data_len = 0xFF - sVar5;
        }
        
        usleep(5000);
        timeout -= 5;
    } while (timeout > 0);

    if (!*sn_ok || !*qrcode_ok) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief Получение номера бина чипа из QR-кода.
 */
uint8_t get_chip_bin(hashboard_qr_code_t *qr_code) {
    uint8_t uVar1;
    char time_stamp[48];
    char tmp1[256];

    // Bin хранится в 4-м символе поля chip_bin (индекс 3)
    if (qr_code->chip_bin[3] == '1') {
        uVar1 = 0x01;
    } else if (qr_code->chip_bin[3] == '2') {
        uVar1 = 0x02;
    } else if (qr_code->chip_bin[3] == '3') {
        uVar1 = 0x03;
    } else if (qr_code->chip_bin[3] == '4') {
        uVar1 = 0x04;
    } else if (qr_code->chip_bin[3] == '5') {
        uVar1 = 0x05;
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Can't get chip bin number. chip_bin: %s\n", "get_chip_bin", qr_code->chip_bin);
        snprintf(tmp1, 0x100, "Can't get chip bin number. chip_bin: %s", qr_code->chip_bin);
        log_to_file(tmp1, time_stamp);
        uVar1 = 0xFF;
    }
    return uVar1;
}

/**
 * @brief Извлечение версии прошивки FT из QR-кода (стандартный формат).
 */
bool get_chip_ft_program_version(hashboard_qr_code_t *qr_code, uint8_t *ft_version) {
    int iVar1;
    bool _Var2;
    char time_stamp[48];
    char tmp1[256];
    char temp[3];

    temp[0] = '\0';
    temp[1] = '\0';
    temp[2] = '\0';

    // Ожидаемый формат: F<V>Maj>V<Min>B<Build>C<Chip> например F1V02B4C1
    if (qr_code->chip_ftversion[0] == 'F') {
        temp[0] = qr_code->chip_ftversion[1];
        iVar1 = atoi(temp);
        *ft_version = (uint8_t)iVar1;

        if (qr_code->chip_ftversion[2] == 'V') {
            temp[0] = qr_code->chip_ftversion[3];
            temp[1] = qr_code->chip_ftversion[4];
            iVar1 = atoi(temp);
            ft_version[1] = (uint8_t)iVar1;

            if (qr_code->chip_ftversion[5] == 'B') {
                temp[0] = qr_code->chip_ftversion[6];
                temp[1] = '\0';
                iVar1 = atoi(temp);
                ft_version[2] = (uint8_t)iVar1;

                if (qr_code->chip_ftversion[7] == 'C') {
                    temp[0] = qr_code->chip_ftversion[8];
                    iVar1 = atoi(temp);
                    ft_version[3] = (uint8_t)iVar1;

                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : FT version: F%dV02%dB%dC%d\n", "get_chip_ft_program_version", 
                           *ft_version, ft_version[1], ft_version[2], ft_version[3]);
                    snprintf(tmp1, 0x100, "FT version: F%dV02%dB%dC%d", 
                             *ft_version, ft_version[1], ft_version[2], ft_version[3]);
                    log_to_file(tmp1, time_stamp);
                    _Var2 = true;
                } else {
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : 4th FT version is not C, but is %c\n", "get_chip_ft_program_version", qr_code->chip_ftversion[7]);
                    snprintf(tmp1, 0x100, "4th FT version is not C, but is %c", qr_code->chip_ftversion[7]);
                    log_to_file(tmp1, time_stamp);
                    _Var2 = false;
                }
            } else {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : 3rd FT version is not B, but is %c\n", "get_chip_ft_program_version", qr_code->chip_ftversion[5]);
                snprintf(tmp1, 0x100, "3rd FT version is not B, but is %c", qr_code->chip_ftversion[5]);
                log_to_file(tmp1, time_stamp);
                _Var2 = false;
            }
        } else {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : 2nd FT version is not V, but is %c\n", "get_chip_ft_program_version", qr_code->chip_ftversion[2]);
            snprintf(tmp1, 0x100, "2nd FT version is not V, but is %c", qr_code->chip_ftversion[2]);
            log_to_file(tmp1, time_stamp);
            _Var2 = false;
        }
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : 1st FT version is not F, but is %c\n", "get_chip_ft_program_version", qr_code->chip_ftversion[0]);
        snprintf(tmp1, 0x100, "1st FT version is not F, but is %c", qr_code->chip_ftversion[0]);
        log_to_file(tmp1, time_stamp);
        _Var2 = false;
    }
    return _Var2;
}

/**
 * @brief Извлечение версии прошивки FT (расширенный/гибкий парсер).
 * Ищет числа в строке версии.
 */
bool get_chip_ft_program_version_ex(hashboard_qr_code_t *qr_code, uint8_t *ft_version) {
    bool bVar1;
    bool bVar2;
    size_t sVar3;
    int iVar4;
    char time_stamp[48];
    char tmp1[256];
    char temp[4][10];
    uint32_t length;
    uint32_t i;
    uint32_t index2;
    uint32_t index1;

    index1 = 0;
    index2 = 0;
    bVar2 = false;
    bVar1 = false;
    memset(temp, 0, 0x28);
    
    sVar3 = strlen(qr_code->chip_ftversion);
    i = 0;
    
    do {
        if (sVar3 + 1 <= i) {
            return true;
        }
        
        // Проверка, является ли символ цифрой
        if (((uint8_t)qr_code->chip_ftversion[i] < 0x30) || (0x39 < (uint8_t)qr_code->chip_ftversion[i])) {
            if (bVar1) {
                bVar2 = true; // Конец числа
                bVar1 = false;
            }
        } else {
            bVar1 = true; // Внутри числа
            temp[index1][index2] = qr_code->chip_ftversion[i];
            index2++;
        }
        
        if (bVar2) {
            if (3 < index1) {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : There are too many number sector in ft version: %s, error.\n", 
                       "get_chip_ft_program_version_ex", qr_code->chip_ftversion);
                snprintf(tmp1, 0x100, "There are too many number sector in ft version: %s, error.", qr_code->chip_ftversion);
                log_to_file(tmp1, time_stamp);
                return false;
            }
            
            iVar4 = atoi(temp[index1]);
            if ((iVar4 < 0) || (0xFF < iVar4)) {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : FT version: %dst number: %d, error\n", "get_chip_ft_program_version_ex", index1, iVar4);
                snprintf(tmp1, 0x100, "FT version: %dst number: %d, error", index1, iVar4);
                log_to_file(tmp1, time_stamp);
                return false;
            }
            
            ft_version[index1] = (uint8_t)iVar4;
            index1++;
            index2 = 0;
            bVar2 = false;
        }
        
        i++;
    } while (true);
}

/**
 * @brief Извлечение аппаратной информации из кода управления платой.
 */
void get_hardware_infor(hashboard_qr_code_t *qr_code) {
    int iVar1;
    char time_stamp[48];
    char tmp1[256];
    char temp[3];
    
    // Внешняя структура EEPROM данных
    extern struct zhiju_eeprom_data_t zhiju_eeprom_data;

    temp[1] = '\0';
    temp[2] = '\0';
    
    // PCB Version V1 (1 символ)
    temp[0] = qr_code->hashboard_ctrl_code[0];
    iVar1 = atoi(temp);
    zhiju_eeprom_data.zhiju_information.pcb_version_v1 = (uint8_t)iVar1;
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : pcb_version_v1 = %d\n", "get_hardware_infor", zhiju_eeprom_data.zhiju_information.pcb_version_v1);
    snprintf(tmp1, 0x100, "pcb_version_v1 = %d", zhiju_eeprom_data.zhiju_information.pcb_version_v1);
    log_to_file(tmp1, time_stamp);

    // PCB Version V2 (2 символа)
    temp[0] = qr_code->hashboard_ctrl_code[1];
    temp[1] = qr_code->hashboard_ctrl_code[2];
    iVar1 = atoi(temp);
    zhiju_eeprom_data.zhiju_information.pcb_version_v2 = (uint8_t)iVar1;
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : pcb_version_v2 = %02d\n", "get_hardware_infor", zhiju_eeprom_data.zhiju_information.pcb_version_v2);
    snprintf(tmp1, 0x100, "pcb_version_v2 = %02d", zhiju_eeprom_data.zhiju_information.pcb_version_v2);
    log_to_file(tmp1, time_stamp);

    // BOM Version V1 (1 символ)
    temp[0] = qr_code->hashboard_ctrl_code[3];
    temp[1] = '\0';
    iVar1 = atoi(temp);
    zhiju_eeprom_data.zhiju_information.bom_version_v1 = (uint8_t)iVar1;
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : bom_version_v1 = %d\n", "get_hardware_infor", zhiju_eeprom_data.zhiju_information.bom_version_v1);
    snprintf(tmp1, 0x100, "bom_version_v1 = %d", zhiju_eeprom_data.zhiju_information.bom_version_v1);
    log_to_file(tmp1, time_stamp);

    // BOM Version V2 (1 символ)
    temp[0] = qr_code->hashboard_ctrl_code[4];
    iVar1 = atoi(temp);
    zhiju_eeprom_data.zhiju_information.bom_version_v2 = (uint8_t)iVar1;
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : bom_version_v2 = %d\n", "get_hardware_infor", zhiju_eeprom_data.zhiju_information.bom_version_v2);
    snprintf(tmp1, 0x100, "bom_version_v2 = %d", zhiju_eeprom_data.zhiju_information.bom_version_v2);
    log_to_file(tmp1, time_stamp);
}

/**
 * @brief Извлечение технологии чипа из кода управления платой.
 */
void get_chip_technology(hashboard_qr_code_t *qr_code) {
    extern struct zhiju_eeprom_data_t zhiju_eeprom_data;
    
    zhiju_eeprom_data.zhiju_information.chip_technology[0] = qr_code->hashboard_ctrl_code[5];
    zhiju_eeprom_data.zhiju_information.chip_technology[1] = qr_code->hashboard_ctrl_code[6];
}