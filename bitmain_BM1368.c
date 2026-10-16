#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// --- Типы данных (на основе использования в псевдокоде) ---

typedef struct {
    uint32_t nonce;
    uint32_t blk_version;
    // Другие поля структуры nonce/format
} return_nonce_128bit_t;

typedef struct {
    uint8_t midstate[32];
    uint8_t data[12]; // Примерный размер, зависит от формата работы
    // Другие поля
} work_format;

typedef struct {
    uint32_t pattern;
    uint32_t nonce_repeat;
    // Другие поля паттерна
} pattern_work;

// Внешние глобальные переменные и структуры
extern struct Local_Config_Info* Local_Config_Information;
extern uint32_t gAddress_interval;
extern uint32_t gChain;
extern uint32_t err_nonce_resp; // Глобальный счетчик ошибок nonce
extern pattern_work* pattern_info_new_works[]; // Массив работ из pattern_info_new
extern uint32_t gMax_Nonce;
extern uint32_t gTimeout_Clock;

// Заглушки для внешних функций
extern void print_time_stamp(char *buf, int len);
extern void log_to_file(char *msg, char *time);
extern void _applog(int level, const char* msg, bool newline);
extern void lcd_clear_result();
extern int32_t lcd_show(int line, const char* data);
extern uint8_t get_asic_index_by_nonce(uint32_t nonce, uint32_t interval);
extern uint8_t get_coreid_by_nonce(uint32_t nonce);
extern int create_socket();
extern void send_nonce(int fd, char *buf, int len);
extern int g_sock;
extern FILE *fd_log;
extern char *bin2hex(unsigned char *data, int len);
extern void free(void *ptr);

// Глобальная переменная сокета (из псевдокода)
static int g_sock = 0;

// --- Функции ---

/**
 * @brief Проверка конфигурационной информации для BM1368.
 * Валидирует параметры теста, количество ядер и интервалы адресации.
 */
bool BTC_check_config_information(char *asic_type) {
    int iVar1;
    char *pcVar2;
    char *pcVar3;
    char *pcVar5;
    bool _Var4;
    char time_stamp[48];
    char tmp1[256];
    char lcd_data[16];
    uint8_t i;

    iVar1 = strcmp(asic_type, "BM1368");
    if (iVar1 == 0) {
        // Проверка типа теста
        iVar1 = strcmp(Local_Config_Information->Test_Process, "PT1");
        if (((((iVar1 == 0) ||
              (iVar1 = strcmp(Local_Config_Information->Test_Process, "PT1new"), iVar1 == 0)) ||
             (iVar1 = strcmp(Local_Config_Information->Test_Process, "PT1+"), iVar1 == 0)) ||
            ((iVar1 = strcmp(Local_Config_Information->Test_Process, "PT2"), iVar1 == 0 ||
              (iVar1 = strcmp(Local_Config_Information->Test_Process, "Reset_Low"), iVar1 == 0)))) ||
           ((iVar1 = strcmp(Local_Config_Information->Test_Process, "PT3"), iVar1 == 0 ||
             (iVar1 = strcmp(Local_Config_Information->Test_Process, "SWEEP"), iVar1 == 0 ||
              (iVar1 = strcmp(Local_Config_Information->Test_Process, "DropLevel"), iVar1 == 0))))) {
            
            // Подсчет активных методов тестирования
            i = (Local_Config_Information->Test_Info).Test_Method.Software_Pattern != false;
            if ((Local_Config_Information->Test_Info).Test_Method.Hardware_Pattern != false) {
                i = i + 1;
            }
            if ((Local_Config_Information->Test_Info).Test_Method.Autogen_Pattern != false) {
                i = i + 1;
            }

            // Если активен только один метод
            if (i == 1) {
                if ((Local_Config_Information->Test_Info).Test_Method.Software_Pattern != false) {
                    // Проверка количества midstate для программного паттерна
                    if ((Local_Config_Information->Test_Info).Test_Method.Midstate_Number != 0x10) {
                        lcd_clear_result();
                        lcd_show(0, (Local_Config_Information->Hash_Board).Board_Name);
                        lcd_show(1, "midstate number ");
                        lcd_show(2, "error ");
                        print_time_stamp(time_stamp, 0x30);
                        printf("%s ", time_stamp);
                        printf("%s : In software pattern mode, midstate number is %d, error\n",
                               "BTC_check_config_information",
                               (Local_Config_Information->Test_Info).Test_Method.Midstate_Number);
                        snprintf(tmp1, 0x100, "In software pattern mode, midstate number is %d, error ",
                                 (Local_Config_Information->Test_Info).Test_Method.Midstate_Number);
                        log_to_file(tmp1, time_stamp);
                    }
                    // Специфичное требование для BM1368: Pattern_Number % 2 == 0 при 16 midstate
                    if (((Local_Config_Information->Test_Info).Test_Method.Midstate_Number == 0x10) &&
                        (((Local_Config_Information->Test_Info).Test_Standard.Pattern_Number & 3) != 0)) {
                        lcd_clear_result();
                        lcd_show(0, (Local_Config_Information->Hash_Board).Board_Name);
                        lcd_show(1, "Pattern Number ");
                        lcd_show(2, "midstate number ");
                        lcd_show(3, "error ");
                        print_time_stamp(time_stamp, 0x30);
                        printf("%s ", time_stamp);
                        printf("%s : When BM1368 test 16 midstate, Pattern_Number %% 2 must be equal 0. Now Pattern_Number: %d\n",
                               "BTC_check_config_information",
                               (Local_Config_Information->Test_Info).Test_Standard.Pattern_Number);
                        snprintf(tmp1, 0x100,
                                 "When BM1368 test 16 midstate, Pattern_Number %% 2 must be equal 0. Now Pattern_Number: %d ",
                                 (Local_Config_Information->Test_Info).Test_Standard.Pattern_Number);
                        log_to_file(tmp1, time_stamp);
                        return false;
                    }
                }
                
                // Настройка параметров для Hardware Pattern
                if ((Local_Config_Information->Test_Info).Test_Method.Hardware_Pattern != false) {
                    (Local_Config_Information->Test_Info).Test_Method.Midstate_Number = 1;
                    gMax_Nonce = 0x376400;
                    gTimeout_Clock = 0x14d;
                }
                
                // Настройка параметров для Autogen Pattern
                if ((Local_Config_Information->Test_Info).Test_Method.Autogen_Pattern != false) {
                    (Local_Config_Information->Test_Info).Test_Method.Midstate_Number = 0x10;
                }

                // Установка параметров платы для BM1368
                (Local_Config_Information->Hash_Board).Big_Core_Num = 0x50;      // 80
                (Local_Config_Information->Hash_Board).Small_Core_Num = 0x4fc;   // 1276
                (Local_Config_Information->Hash_Board).Small_Core_Num_In_Big_Core = 0x10; // 16

                // Расчет интервала адресации в зависимости от количества ASIC
                if ((Local_Config_Information->Hash_Board).Asic_Num < 0x81) { // < 129
                    if ((Local_Config_Information->Hash_Board).Asic_Num < 0x41) { // < 65
                        if ((Local_Config_Information->Hash_Board).Asic_Num < 0x21) { // < 33
                            lcd_clear_result();
                            lcd_show(0, (Local_Config_Information->Hash_Board).Board_Name);
                            lcd_show(1, "Asic_Num  < 32 ");
                            lcd_show(3, "error ");
                            print_time_stamp(time_stamp, 0x30);
                            printf("%s ", time_stamp);
                            printf("%s : ERROR: Asic_Num == %d\n", "BTC_check_config_information",
                                   (Local_Config_Information->Hash_Board).Asic_Num);
                            snprintf(tmp1, 0x100, "ERROR: Asic_Num == %d ",
                                     (Local_Config_Information->Hash_Board).Asic_Num);
                            log_to_file(tmp1, time_stamp);
                            return false;
                        }
                        gAddress_interval = 4;
                    } else {
                        gAddress_interval = 2;
                    }
                } else {
                    gAddress_interval = 1;
                }
                
                // Установка длины EEPROM
                // Примечание: в псевдокоде присваивается 0x100, но тип переменной может быть uint32_t
                // В C++ это просто присваивание
                extern uint32_t gEEPROM_length; 
                gEEPROM_length = 0x100;
                
                _Var4 = true;
            } else {
                // Конфликт методов тестирования
                lcd_clear_result();
                lcd_show(0, (Local_Config_Information->Hash_Board).Board_Name);
                lcd_show(1, "test method ");
                lcd_show(2, "conflict ");
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                
                pcVar3 = (Local_Config_Information->Test_Info).Test_Method.Software_Pattern ? "true" : "false";
                pcVar2 = (Local_Config_Information->Test_Info).Test_Method.Hardware_Pattern ? "true" : "false";
                pcVar5 = (Local_Config_Information->Test_Info).Test_Method.Autogen_Pattern ? "true" : "false";
                
                printf("%s : test method conflict. Software_Pattern: %s, Hardware_Pattern: %s, Autogen_Pattern: %s\n",
                       "BTC_check_config_information", pcVar3, pcVar2, pcVar5);
                
                snprintf(tmp1, 0x100,
                         "test method conflict. Software_Pattern: %s, Hardware_Pattern: %s, Autogen_Pattern: %s ",
                         pcVar3, pcVar2, pcVar5);
                log_to_file(tmp1, time_stamp);
                _Var4 = false;
            }
        } else {
            // Ошибка процесса теста
            lcd_clear_result();
            lcd_show(0, (Local_Config_Information->Hash_Board).Board_Name);
            lcd_show(1, "test process ");
            lcd_show(2, "error ");
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : test process (%s) error.\n", "BTC_check_config_information",
                   Local_Config_Information->Test_Process); // Исправлено на доступ к полю
            snprintf(tmp1, 0x100, "test process (%s) error. ", Local_Config_Information->Test_Process);
            log_to_file(tmp1, time_stamp);
            _Var4 = false;
        }
    } else {
        // Неподдерживаемый тип ASIC
        memset(lcd_data, 0, 0x10);
        lcd_clear_result();
        lcd_show(0, (Local_Config_Information->Hash_Board).Board_Name);
        lcd_show(1, "Don't support ");
        strcpy(lcd_data, asic_type);
        lcd_show(2, lcd_data);
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Don't support %s. asic should be BM1368\n", "BTC_check_config_information",
               asic_type);
        snprintf(tmp1, 0x100, "Don't support %s. asic should be BM1368 ", asic_type);
        log_to_file(tmp1, time_stamp);
        _Var4 = false;
    }
    return _Var4;
}

/**
 * @brief Обработка nonce для режима Software Pattern.
 * Проверяет соответствие nonce отправленным работам.
 */
void BTC_software_pattern_check_nonce(return_nonce_128bit_t *nonce_buf) {
    byte bVar1;
    uint uVar3;
    int iVar4;
    uint uVar5;
    uint uVar6;
    pattern_work *pwVar7;
    char time_stamp[48];
    char tmp1[256];
    uint32_t which_pattern;
    uint32_t which_small_core;
    uint32_t which_big_core;
    uint32_t which_asic;
    uint uVar2;

    if (nonce_buf == nullptr) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : nonce_buf is null\n", "BTC_software_pattern_check_nonce");
        strncpy(tmp1, "nonce_buf is nul", 0x10);
        tmp1[0x10] = 'l';
        tmp1[0x11] = '\0';
        log_to_file(tmp1, time_stamp);
        return;
    }

    // Проверка ID цепи в первом байте
    if ((*(byte *)nonce_buf & 0xf) == gChain) {
        bVar1 = get_asic_index_by_nonce(nonce_buf->nonce, gAddress_interval);
        uVar2 = (uint)bVar1;
        
        bVar1 = get_coreid_by_nonce(nonce_buf->nonce);
        uVar3 = (uint)bVar1;
        
        uVar5 = nonce_buf->blk_version & 0xf;
        uVar6 = (int)((ushort *)&nonce_buf->reg.chip_address & 0x7fff) >> 7;

        if (uVar2 < (Local_Config_Information->Hash_Board).Asic_Num) {
            if (uVar3 < 0x50) { // Big Core Num check
                // Проверка аномального ядра (последнее большое ядро?)
                if (uVar3 == 0x4f) {
                    if (uVar5 < 4) {
                        err_nonce_resp = err_nonce_resp + 1;
                        return;
                    }
                } else if (0xf < uVar5) { // Small Core Num In Big Core check
                    err_nonce_resp = err_nonce_resp + 1;
                    return;
                }

                // Проверка номера паттерна
                if (uVar6 < (Local_Config_Information->Test_Info).Test_Standard.Pattern_Number) {
                    // Расчет указателя на работу
                    pwVar7 = pattern_info_new_works[uVar2] +
                             uVar6 +
                             (Local_Config_Information->Test_Info).Test_Standard.Pattern_Number *
                             (uVar3 * 0x10 + uVar5);

                    // Сравнение nonce
                    if (nonce_buf->nonce == pwVar7->pattern) {
                        // Логика для 4 midstate или общего случая
                        if ((Local_Config_Information->Test_Info).Test_Method.Midstate_Number == 4) {
                            if (pwVar7->nonce_repeat == 0) {
                                pwVar7->nonce_repeat = pwVar7->nonce_repeat + 1;
                                extern uint32_t gHistory_Result_index; // Предполагаемый индекс
                                extern struct HistoryResult gHistory_Result[];
                                
                                gHistory_Result[gHistory_Result_index].valid_nonce_num =
                                    gHistory_Result[gHistory_Result_index].valid_nonce_num + 1;
                                
                                iVar4 = uVar3 * 0x10 + uVar5;
                                gHistory_Result[gHistory_Result_index].asic_core_nonce_num[uVar2][iVar4] =
                                    gHistory_Result[gHistory_Result_index].asic_core_nonce_num[uVar2][iVar4] + 1;
                                
                                gHistory_Result[gHistory_Result_index].every_asic_got_nonce_num[uVar2] =
                                    gHistory_Result[gHistory_Result_index].every_asic_got_nonce_num[uVar2] + 1;
                            } else {
                                gHistory_Result[gHistory_Result_index].repeat_nonce_number =
                                    gHistory_Result[gHistory_Result_index].repeat_nonce_number + 1;
                                pwVar7->nonce_repeat = pwVar7->nonce_repeat + 1;
                            }
                        } else {
                            // Общий случай
                            if (pwVar7->nonce_repeat == 0) {
                                pwVar7->nonce_repeat = pwVar7->nonce_repeat + 1;
                                extern uint32_t gHistory_Result_index;
                                extern struct HistoryResult gHistory_Result[];

                                gHistory_Result[gHistory_Result_index].valid_nonce_num =
                                    gHistory_Result[gHistory_Result_index].valid_nonce_num + 1;
                                
                                iVar4 = uVar3 * 0x10 + uVar5;
                                gHistory_Result[gHistory_Result_index].asic_core_nonce_num[uVar2][iVar4] =
                                    gHistory_Result[gHistory_Result_index].asic_core_nonce_num[uVar2][iVar4] + 1;
                                
                                gHistory_Result[gHistory_Result_index].every_asic_got_nonce_num[uVar2] =
                                    gHistory_Result[gHistory_Result_index].every_asic_got_nonce_num[uVar2] + 1;
                            } else {
                                gHistory_Result[gHistory_Result_index].repeat_nonce_number =
                                    gHistory_Result[gHistory_Result_index].repeat_nonce_number + 1;
                                pwVar7->nonce_repeat = pwVar7->nonce_repeat + 1;
                            }
                        }
                    }
                } else {
                    err_nonce_resp = err_nonce_resp + 1;
                }
            } else {
                err_nonce_resp = err_nonce_resp + 1;
            }
        } else {
            err_nonce_resp = err_nonce_resp + 1;
        }
    } else {
        err_nonce_resp = err_nonce_resp + 1;
    }
    return;
}

/**
 * @brief Обработка nonce для режима Mid Auto Gen.
 * Аналогична software pattern, но с другой логикой извлечения полей из nonce.
 */
void BTC_mid_auto_gen_check_nonce(return_nonce_128bit_t *nonce_buf) {
    byte bVar1;
    uint uVar3;
    int iVar4;
    uint uVar5;
    uint uVar6;
    pattern_work *pwVar7;
    char time_stamp[48];
    char tmp1[256];
    uint32_t which_pattern;
    uint32_t which_small_core;
    uint32_t which_big_core;
    uint32_t which_asic;
    uint uVar2;

    if ((*(byte *)nonce_buf & 0xf) == gChain) {
        bVar1 = get_asic_index_by_nonce(nonce_buf->nonce, gAddress_interval);
        uVar2 = (uint)bVar1;
        
        bVar1 = get_coreid_by_nonce(nonce_buf->nonce);
        uVar3 = (uint)bVar1;
        
        // Извлечение small core из blk_version (сдвиг на 8)
        uVar5 = (nonce_buf->blk_version >> 8) & 0xf;
        uVar6 = (int)((ushort *)&nonce_buf->reg.chip_address & 0x7fff) >> 7;

        if (uVar2 < (Local_Config_Information->Hash_Board).Asic_Num) {
            if (uVar3 < 0x50) {
                if (uVar3 == 0x4f) {
                    if (uVar5 < 4) {
                        print_time_stamp(time_stamp, 0x30);
                        printf("%s ", time_stamp);
                        printf("%s : receive nonce from Aisc: %d, big_core %d, small_core %d, but the most Asic small_core number is: %d\n",
                               "BTC_mid_auto_gen_check_nonce", uVar2, 0x4f, uVar5, 0x10);
                        snprintf(tmp1, 0x100,
                                 "receive nonce from Aisc: %d, big_core %d, small_core %d, but the most Asic small_core number is: %d ",
                                 uVar2, 0x4f, uVar5, 0x10);
                        log_to_file(tmp1, time_stamp);
                        return;
                    }
                } else if (0xf < uVar5) {
                    print_time_stamp(tmp1, 0x30); // Опечатка в оригинале? Используем tmp1 как буфер времени
                    printf("%s ", tmp1);
                    printf("%s : receive nonce from Aisc: %d, big_core %d, small_core %d, but the most Asic small_core number is: %d\n",
                           "BTC_mid_auto_gen_check_nonce", uVar2, uVar3, uVar5, 0x10);
                    return;
                }

                if (uVar6 < (Local_Config_Information->Test_Info).Test_Standard.Pattern_Number) {
                    pwVar7 = pattern_info_new_works[uVar2] +
                             uVar6 +
                             (Local_Config_Information->Test_Info).Test_Standard.Pattern_Number *
                             (uVar3 * 0x10 + uVar5);

                    if (nonce_buf->nonce == pwVar7->pattern) {
                        if (pwVar7->nonce_repeat == 0) {
                            pwVar7->nonce_repeat = pwVar7->nonce_repeat + 1;
                            extern uint32_t gHistory_Result_index;
                            extern struct HistoryResult gHistory_Result[];

                            gHistory_Result[gHistory_Result_index].valid_nonce_num =
                                gHistory_Result[gHistory_Result_index].valid_nonce_num + 1;
                            
                            iVar4 = uVar3 * 0x10 + uVar5;
                            gHistory_Result[gHistory_Result_index].asic_core_nonce_num[uVar2][iVar4] =
                                gHistory_Result[gHistory_Result_index].asic_core_nonce_num[uVar2][iVar4] + 1;
                            
                            gHistory_Result[gHistory_Result_index].every_asic_got_nonce_num[uVar2] =
                                gHistory_Result[gHistory_Result_index].every_asic_got_nonce_num[uVar2] + 1;
                        } else {
                            gHistory_Result[gHistory_Result_index].repeat_nonce_number =
                                gHistory_Result[gHistory_Result_index].repeat_nonce_number + 1;
                            pwVar7->nonce_repeat = pwVar7->nonce_repeat + 1;
                        }
                    }
                } else {
                    print_time_stamp(tmp1, 0x30);
                    printf("%s ", tmp1);
                    printf("%s : receive %dth pattern from Aisc: %d, big_core %d, small_core %d, but the most pattern number is: %d\n",
                           "BTC_mid_auto_gen_check_nonce", uVar6, uVar2, uVar3, uVar5,
                           (Local_Config_Information->Test_Info).Test_Standard.Pattern_Number);
                }
            } else {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : receive nonce from Aisc: %d, big_core %d, but the most Asic big_core number is: %d\n",
                       "BTC_mid_auto_gen_check_nonce", uVar2, uVar3, 0x50);
                snprintf(tmp1, 0x100,
                         "receive nonce from Aisc: %d, big_core %d, but the most Asic big_core number is: %d ",
                         uVar2, uVar3, 0x50);
                log_to_file(tmp1, time_stamp);
            }
        } else {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : receive nonce from Aisc: %d, but the most Asic number is: %d\n",
                   "BTC_mid_auto_gen_check_nonce", uVar2,
                   (Local_Config_Information->Hash_Board).Asic_Num);
            snprintf(tmp1, 0x100, "receive nonce from Aisc: %d, but the most Asic number is: %d ",
                     uVar2, (Local_Config_Information->Hash_Board).Asic_Num);
            log_to_file(tmp1, time_stamp);
        }
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : receive nonce from chain: %d, but not gChain: %d\n",
               "BTC_mid_auto_gen_check_nonce", *(byte *)nonce_buf & 0xf, gChain);
        snprintf(tmp1, 0x100, "receive nonce from chain: %d, but not gChain: %d ",
                 (byte *)nonce_buf & 0xf, gChain);
        log_to_file(tmp1, time_stamp);
    }
    return;
}

/**
 * @brief Реверс массива байтов.
 */
void rev(uchar *s, size_t l) {
    uchar uVar1;
    size_t j;
    size_t i;
    
    for (i = 0, j = l - 1; i < j; i++, j--) {
        uVar1 = s[i];
        s[i] = s[j];
        s[j] = uVar1;
    }
    return;
}

/**
 * @brief Swap байтов в 32-битном целом (Endian swap).
 */
uint32_t swab32(uint32_t v) {
    return __bswap_32(v);
}

/**
 * @brief Flip и swap 32-битных слов в блоке данных.
 */
void flip32(void *dest_p, void *src_p) {
    uint32_t uVar1;
    uint32_t *src = (uint32_t *)src_p;
    uint32_t *dest = (uint32_t *)dest_p;
    int i;
    
    for (i = 0; i < 8; i++) {
        uVar1 = swab32(src[i]);
        dest[i] = uVar1;
    }
    return;
}

/**
 * @brief Утилита для flip/swab массива данных заданной длины.
 */
void utils_flip_swab(void *dest_p, void *src_p, uint32_t length) {
    uint32_t uVar1;
    uint32_t *src = (uint32_t *)src_p;
    uint32_t *dest = (uint32_t *)dest_p;
    int32_t i;
    
    for (i = 0; (uint)i < length >> 2; i++) {
        uVar1 = swab32(src[i]);
        dest[i] = uVar1;
    }
    return;
}

/**
 * @brief Проверка аппаратного хеша (HW check).
 * Вычисляет хеш и проверяет сложность.
 */
int check_hw(work_format *work, uint32_t which_midstate, uint32_t nonce) {
    int iVar1;
    sha2_context ctx; // Предполагаемая структура контекста SHA2
    uint32_t nonce_temp;
    uint8_t hash2[32];
    uint8_t hash1[32];
    
    memset(hash1, 0, 0x20);
    memset(hash2, 0, 0x20);
    
    // Byte swap nonce
    nonce_temp = (nonce << 24) | ((nonce & 0xFF00) << 8) | ((nonce & 0xFF0000) >> 8) | (nonce >> 24);
    
    if (work == nullptr) {
        iVar1 = 0;
    } else {
        // Копирование midstate
        memcpy(ctx.state, work->midstate + which_midstate, 0x20);
        rev((uchar *)ctx.state, 0x20);
        
        ctx.total[0] = 0x50;
        ctx.total[1] = 0;
        
        // Подготовка данных
        memcpy(hash1, work->data, 0xc);
        rev(hash1, 0xc);
        utils_flip_swab(ctx.buffer, hash1, 0xc);
        
        // Добавление nonce
        memcpy(hash1, &nonce_temp, 4);
        rev(hash1, 4);
        utils_flip_swab(ctx.buffer + 0xc, hash1, 4);
        
        // Вычисление хеша
        sha2_finish(&ctx, hash1);
        memset(&ctx, 0, 0xe8);
        sha2(hash1, 0x20, hash2);
        
        flip32(hash1, hash2);
        
        // Проверка старших битов хеша на нули (сложность)
        if (hash1[28] == 0 && hash1[29] == 0 && hash1[30] == 0 && hash1[31] == 0) { // Упрощенная проверка
             // В псевдокоде проверяется hash1.28_4 == 0, что означает 4 байта по смещению 28
             if (*(uint32_t*)(hash1 + 28) == 0) {
                 iVar1 = 0; // Success/HW found
             } else {
                 iVar1 = 1; // Fail
             }
        } else {
             iVar1 = 1;
        }
    }
    return iVar1;
}

/**
 * @brief Сохранение найденного nonce в лог и отправка через сокет.
 */
void save_nonce(work_format *work, uint32_t which_midstate, uint32_t nonce) {
    int fd;
    char *__ptr;
    char *__ptr_00;
    char *__ptr_01;
    char *__ptr_02;
    size_t sVar1;
    uint32_t version;
    char type[16];
    uchar data_tmp[32];
    uchar midstate_tmp[32];
    char szmsg[1024];
    char *szversion;
    char *sznonce;
    char *szdata;
    char *szmidstate;
    int j;
    
    memset(szmsg, 0, 0x400);
    memset(midstate_tmp, 0, 0x20);
    memset(data_tmp, 0, 0x20);
    memset(type, 0, 0x10);
    
    // Открытие файла лога
    if (fd_log == nullptr) {
        fd_log = fopen("/tmp/worklog.txt", "ab");
    }
    
    // Формирование версии
    version = ((which_midstate << 0x15) | 0x20000000) >> 0x18 | ((which_midstate & 7) << 0xd);
    
    // Byte swap nonce
    nonce = (nonce >> 24) | ((nonce & 0xFF0000) >> 8) | ((nonce & 0xFF00) << 8) | (nonce << 24);
    
    // Копирование midstate
    memcpy(midstate_tmp, work->midstate[which_midstate], 0x20);
    // Копирование данных
    memcpy(data_tmp, work->data, 0xc);
    
    // Конвертация в hex строки
    __ptr = bin2hex(midstate_tmp, 0x20);
    __ptr_00 = bin2hex(data_tmp, 0xc);
    __ptr_01 = bin2hex((uchar *)&nonce, 4);
    __ptr_02 = bin2hex((uchar *)&version, 4);
    
    sprintf(szmsg, "midstate %s data %s nonce %s version %s \r\n", __ptr, __ptr_00, __ptr_01, __ptr_02);
    
    sVar1 = strlen(szmsg);
    fwrite(szmsg, sVar1, 1, fd_log);
    fflush(fd_log);
    
    // Отправка через сокет
    if (g_sock == 0) {
        g_sock = create_socket();
    }
    
    if (g_sock > 0) {
        memset(szmsg, 0, 0x400);
        
        // Формирование типа ASIC (upper case)
        for (j = 0; j < 0x10; j++) {
            if (((byte)(Local_Config_Information->Hash_Board).Asic_Type[j] < 0x41) ||
                (0x5a < (byte)(Local_Config_Information->Hash_Board).Asic_Type[j])) {
                type[j] = (Local_Config_Information->Hash_Board).Asic_Type[j];
            } else {
                type[j] = (Local_Config_Information->Hash_Board).Asic_Type[j] + ' '; // Опечатка в оригинале? Обычно 'A'-'a' = 32. Здесь пробел?
            }
        }
        
        sprintf(szmsg, "type %s midstate %s data %s nonce %s version %s\n", type, __ptr, __ptr_00, __ptr_01, __ptr_02);
        
        fd = g_sock;
        sVar1 = strlen(szmsg);
        send_nonce(fd, szmsg, sVar1);
    }
    
    // Освобождение памяти
    if (__ptr != nullptr) free(__ptr);
    if (__ptr_00 != nullptr) free(__ptr_00);
    if (__ptr_01 != nullptr) free(__ptr_01);
    if (__ptr_02 != nullptr) free(__ptr_02);
    
    return;
}

/**
 * @brief Обработка nonce для режима CPU Gen Pattern.
 * Проверяет границы ядер и ASIC.
 */
void BTC_cpugen_pattern_check_nonce(return_nonce_128bit_t *nonce_buf) {
    byte bVar1;
    uint uVar3;
    uint uVar4;
    char time_stamp[48];
    char tmp1[256];
    uint32_t which_pattern;
    uint32_t which_small_core;
    uint32_t which_big_core;
    uint32_t which_asic;
    uint uVar2;
    
    if ((*(byte *)nonce_buf & 0xf) == gChain) {
        bVar1 = get_asic_index_by_nonce(nonce_buf->nonce, gAddress_interval);
        uVar2 = (uint)bVar1;
        
        bVar1 = get_coreid_by_nonce(nonce_buf->nonce);
        uVar3 = (uint)bVar1;
        
        // Извлечение small core из chip_address
        uVar4 = *(ushort *)&nonce_buf->reg.chip_address & 0x7f;
        
        if (uVar2 < (Local_Config_Information->Hash_Board).Asic_Num) {
            if (uVar3 < 0x50) {
                if (uVar3 == 0x4f) {
                    if (uVar4 < 4) {
                        print_time_stamp(time_stamp, 0x30);
                        printf("%s ", time_stamp);
                        printf("%s : receive nonce from Aisc: %d, big_core %d, small_core %d, but the most Asic small_core number is: %d\n",
                               "BTC_cpugen_pattern_check_nonce", uVar2, 0x4f, uVar4, 0x10);
                        snprintf(tmp1, 0x100,
                                 "receive nonce from Aisc: %d, big_core %d, small_core %d, but the most Asic small_core number is: %d ",
                                 uVar2, 0x4f, uVar4, 0x10);
                        log_to_file(tmp1, time_stamp);
                    }
                } else if (0xf < uVar4) {
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : receive nonce from Aisc: %d, big_core %d, small_core %d, but the most Asic small_core number is: %d\n",
                           "BTC_cpugen_pattern_check_nonce", uVar2, uVar3, uVar4, 0x10);
                    snprintf(tmp1, 0x100,
                             "receive nonce from Aisc: %d, big_core %d, small_core %d, but the most Asic small_core number is: %d ",
                             uVar2, uVar3, uVar4, 0x10);
                    log_to_file(tmp1, time_stamp);
                }
            } else {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : receive nonce from Aisc: %d, big_core %d, but the most Asic big_core number is: %d\n",
                       "BTC_cpugen_pattern_check_nonce", uVar2, uVar3, 0x50);
                snprintf(tmp1, 0x100,
                         "receive nonce from Aisc: %d, big_core %d, but the most Asic big_core number is: %d ",
                         uVar2, uVar3, 0x50);
                log_to_file(tmp1, time_stamp);
            }
        } else {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : receive nonce from Aisc: %d, but the most Asic number is: %d\n",
                   "BTC_cpugen_pattern_check_nonce", uVar2,
                   (Local_Config_Information->Hash_Board).Asic_Num);
            snprintf(tmp1, 0x100, "receive nonce from Aisc: %d, but the most Asic number is: %d ",
                     uVar2, (Local_Config_Information->Hash_Board).Asic_Num);
            log_to_file(tmp1, time_stamp);
        }
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : receive nonce from chain: %d, but not gChain: %d\n",
               "BTC_cpugen_pattern_check_nonce", *(byte *)nonce_buf & 0xf, gChain);
        snprintf(tmp1, 0x100, "receive nonce from chain: %d, but not gChain: %d ",
                 (byte *)nonce_buf & 0xf, gChain);
        log_to_file(tmp1, time_stamp);
    }
    return;
}

/**
 * @brief Создание сокета для отправки nonce.
 * Подключается к серверу паттернов.
 */
int create_socket(void) {
    bool _Var1;
    int __fd;
    int iVar2;
    char time_stamp[48];
    char tmp1[256];
    sockaddr_in serverAddr;
    int32_t timeout_counter;
    
    timeout_counter = 0x1e; // 30 секунд
    
    __fd = socket(AF_INET, SOCK_STREAM, 0); // 2, 1, 0
    if (__fd < 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Creat socket fail\n", "create_socket");
        strncpy(tmp1, "Creat socket fai", 0x10);
        tmp1[0x10] = 'l';
        tmp1[0x11] = '\0';
        log_to_file(tmp1, time_stamp);
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Creat socket success\n", "create_socket");
        strncpy(tmp1, "Creat socket success", 0x14);
        tmp1[0x14] = '\0'; // Null terminate properly
        log_to_file(tmp1, time_stamp);
        
        serverAddr.sin_family = AF_INET; // 2
        serverAddr.sin_port = htons(0x11d7); // Port 4567
        serverAddr.sin_addr.s_addr = inet_addr("10.76.16.200");
        
        while (0 < timeout_counter) {
            iVar2 = connect(__fd, (sockaddr *)&serverAddr, 0x10);
            if (iVar2 == 0) {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : Creat socket connection to pattern svr succeed\n", "create_socket");
                strncpy(tmp1, "Creat socket connection to pattern svr succe", 0x2c);
                tmp1[0x2c] = 'e';
                tmp1[0x2d] = 'd';
                tmp1[0x2e] = '\0';
                log_to_file(tmp1, time_stamp);
                
                extern bool set_socket_opt(int32_t socket_fd);
                _Var1 = set_socket_opt(__fd);
                if (_Var1) {
                    return __fd;
                }
                return -1;
            }
            sleep(1);
            timeout_counter--;
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : timeout_counter = %d\n", "create_socket", timeout_counter);
            snprintf(tmp1, 0x100, "timeout_counter = %d ", timeout_counter);
            log_to_file(tmp1, time_stamp);
        }
        
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Creat socket connection to  pattern svr fail\n", "create_socket");
        strncpy(tmp1, "Creat socket connection to  pattern svr fail", 0x2c);
        tmp1[0x2c] = '\0';
        log_to_file(tmp1, time_stamp);
    }
    return -1;
}

/**
 * @brief Отправка данных nonce через сокет.
 */
void send_nonce(int fd, char *buf, int len) {
    ssize_t sVar1;
    char time_stamp[48];
    char tmp1[256];
    
    if (fd < 1) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Socket error %d\n", "send_nonce", fd);
        snprintf(tmp1, 0x100, "Socket error %d ", fd);
        log_to_file(tmp1, time_stamp);
    } else {
        sVar1 = send(fd, buf, len, 0);
        if (sVar1 == 0) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Timeout. MES close socket\n", "send_nonce");
            strncpy(tmp1, "Timeout. MES close socke", 0x18);
            tmp1[0x18] = 't';
            tmp1[0x19] = '\0';
            log_to_file(tmp1, time_stamp);
        } else if (sVar1 < 0) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Socket return back: %d\n", "send_nonce", sVar1);
            snprintf(tmp1, 0x100, "Socket return back: %d ", sVar1);
            log_to_file(tmp1, time_stamp);
        }
        
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Socket send len: %d\n", "send_nonce", sVar1);
        snprintf(tmp1, 0x100, "Socket send len: %d ", sVar1);
        log_to_file(tmp1, time_stamp);
    }
    return;
}

/**
 * @brief Настройка опций сокета (keepalive, timeout и т.д.).
 */
bool set_socket_opt(int32_t socket_fd) {
    bool _Var1;
    char time_stamp[48];
    char tmp1[256];
    timeval timeout;
    int32_t tcp_keepintvl;
    int32_t tcp_keepidle;
    int32_t tcp_one;
    int32_t ret;
    
    tcp_one = 1;
    tcp_keepidle = 0x2d; // 45 sec
    tcp_keepintvl = 0x1e; // 30 sec
    ret = 0;
    
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : socket_fd = %d\n", "set_socket_opt", socket_fd);
    snprintf(tmp1, 0x100, "socket_fd = %d ", socket_fd);
    log_to_file(tmp1, time_stamp);
    
    // SO_SNDTIMEO
    ret = setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, 8);
    if (ret < 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Set socket send timeout fail, ret = %d\n", "set_socket_opt", ret);
        snprintf(tmp1, 0x100, "Set socket send timeout fail, ret = %d ", ret);
        log_to_file(tmp1, time_stamp);
        _Var1 = false;
    } else {
        // SO_RCVTIMEO
        ret = setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, 8);
        if (ret < 0) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Set socket recv timeout fail, ret = %d\n", "set_socket_opt", ret);
            snprintf(tmp1, 0x100, "Set socket recv timeout fail, ret = %d ", ret);
            log_to_file(tmp1, time_stamp);
            _Var1 = false;
        } else {
            // SO_KEEPALIVE
            ret = setsockopt(socket_fd, SOL_SOCKET, SO_KEEPALIVE, &tcp_one, 4);
            if (ret < 0) {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : Set socket keep alive fail, ret = %d\n", "set_socket_opt", ret);
                snprintf(tmp1, 0x100, "Set socket keep alive fail, ret = %d ", ret);
                log_to_file(tmp1, time_stamp);
                _Var1 = false;
            } else {
                // TCP_NODELAY
                ret = setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &tcp_one, 4);
                if (ret < 0) {
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : Set socket no delay fail, ret = %d\n", "set_socket_opt", ret);
                    snprintf(tmp1, 0x100, "Set socket no delay fail, ret = %d ", ret);
                    log_to_file(tmp1, time_stamp);
                    _Var1 = false;
                } else {
                    // TCP_KEEPCNT
                    ret = setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPCNT, &tcp_one, 4);
                    if (ret < 0) {
                        print_time_stamp(time_stamp, 0x30);
                        printf("%s ", time_stamp);
                        printf("%s : Set socket keep cnt fail, ret = %d\n", "set_socket_opt", ret);
                        snprintf(tmp1, 0x100, "Set socket keep cnt fail, ret = %d ", ret);
                        log_to_file(tmp1, time_stamp);
                        _Var1 = false;
                    } else {
                        // TCP_KEEPIDLE
                        ret = setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPIDLE, &tcp_keepidle, 4);
                        if (ret < 0) {
                            print_time_stamp(time_stamp, 0x30);
                            printf("%s ", time_stamp);
                            printf("%s : Set socket keep idle fail, ret = %d\n", "set_socket_opt", ret);
                            snprintf(tmp1, 0x100, "Set socket keep idle fail, ret = %d ", ret);
                            log_to_file(tmp1, time_stamp);
                            _Var1 = false;
                        } else {
                            // TCP_KEEPINTVL
                            ret = setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPINTVL, &tcp_keepintvl, 4);
                            if (ret < 0) {
                                print_time_stamp(time_stamp, 0x30);
                                printf("%s ", time_stamp);
                                printf("%s : Set socket keep intvl fail, ret = %d\n", "set_socket_opt", ret);
                                snprintf(tmp1, 0x100, "Set socket keep intvl fail, ret = %d ", ret);
                                log_to_file(tmp1, time_stamp);
                                _Var1 = false;
                            } else {
                                _Var1 = true;
                            }
                        }
                    }
                }
            }
        }
    }
    return _Var1;
}