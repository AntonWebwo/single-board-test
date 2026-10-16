#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <unistd.h>

// --- Типы данных (на основе использования в псевдокоде) ---

// Заглушки для внешних функций взаимодействия с PIC/I2C и FPGA
extern int32_t pic_read_iic(uint8_t chain, uint8_t addr, uint8_t reg, uint8_t *data, int32_t len);
extern int32_t pic_write_iic(uint8_t chain, uint8_t addr, uint8_t reg, uint8_t *data, int32_t len);
extern void lcd_clear_result();
extern void lcd_show(int line, const char* data);
extern void print_time_stamp(char *buf, int len);
extern void log_to_file(char *msg, char *time);
extern void _applog(int level, const char* msg, bool newline);
extern int32_t bitmain_set_voltage(double vol);
extern int32_t bitmain_convert_V_to_N(double vol);
extern int32_t bitmain_set_voltage_by_n(uint32_t n);
extern struct Local_Config_Info* Local_Config_Information; // Внешняя структура конфигурации

// Глобальная переменная версии APW (из bitmain_global.c или аналогичного)
extern uint8_t g_apw_power_version; 

// --- Функции ---

/**
 * @brief Чтение данных из DAC.
 * Поддерживается только DAC с адресом 0x48 ('H').
 */
bool read_dac(uint8_t which_chain, uint8_t which_dac, uint8_t which_reg, uint8_t *data, int32_t data_length) {
    int32_t iVar1;
    char time_stamp[48];
    char tmp1[256];
    uint8_t dac_addr = 0;

    if (which_dac == 0x01) { // Предполагаем, что 0x01 соответствует 'H' (0x48) в логике адреса
        dac_addr = 0x48; 
        iVar1 = pic_read_iic(which_chain, dac_addr, which_reg, data, data_length);
        
        if (iVar1 == data_length) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : dac: %d, register 0x%02x data is:\n", "read_dac", 1, (uint)which_reg);
            snprintf(tmp1, 0x100, "dac: %d, register 0x%02x data is: ", 1, (uint)which_reg);
            log_to_file(tmp1, time_stamp);
            
            for (int i = 0; i < data_length; i++) {
                printf("0x%02x ", (uint)data[i]);
            }
            putchar(10);
            return true;
        } else {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : chain: %d, dac_addr: 0x%02x, data_length = %d, fail.\n", "read_dac", (uint)which_chain, dac_addr, data_length);
            snprintf(tmp1, 0x100, "chain: %d, dac_addr: 0x%02x, data_length = %d, fail. ", (uint)which_chain, dac_addr, data_length);
            log_to_file(tmp1, time_stamp);
            
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : read dac: %d, register: 0x%02x, fail.\n", "read_dac", 1, (uint)which_reg);
            snprintf(tmp1, 0x100, "read dac: %d, register: 0x%02x, fail. ", 1, (uint)which_reg);
            log_to_file(tmp1, time_stamp);
            return false;
        }
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Don't support DAC: %d\n", "read_dac", (uint)which_dac);
        snprintf(tmp1, 0x100, "Don't support DAC: %d ", (uint)which_dac);
        log_to_file(tmp1, time_stamp);
        return false;
    }
}

/**
 * @brief Запись данных в DAC.
 * Поддерживается только DAC с адресом 0x48 ('H').
 */
bool write_dac(uint8_t which_chain, uint8_t which_dac, uint8_t which_reg, uint8_t *data, int32_t data_length) {
    int32_t iVar1;
    char time_stamp[48];
    char tmp1[256];
    uint8_t dac_addr = 0;

    if (which_dac == 0x01) { // Предполагаем, что 0x01 соответствует 'H' (0x48)
        dac_addr = 0x48;
        iVar1 = pic_write_iic(which_chain, dac_addr, which_reg, data, data_length);
        
        if (iVar1 == data_length) {
            return true;
        } else {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : write dac: %d, register:%d, fail.\n", "write_dac", 1, (uint)which_reg);
            snprintf(tmp1, 0x100, "write dac: %d, register:%d, fail. ", 1, (uint)which_reg);
            log_to_file(tmp1, time_stamp);
            return false;
        }
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Don't support DAC: %d\n", "write_dac", (uint)which_dac);
        snprintf(tmp1, 0x100, "Don't support DAC: %d ", (uint)which_dac);
        log_to_file(tmp1, time_stamp);
        return false;
    }
}

/**
 * @brief Инициализация DAC53401 для платы NBT2006-36.
 * Настраивает регистр конфигурации.
 */
bool init_dac53401_NBT2006_36(uint8_t which_chain) {
    bool _Var1;
    char time_stamp[48];
    char tmp1[256];
    uint8_t data[2] = {0, 0};

    // Чтение регистра конфигурации (0xD1)
    _Var1 = read_dac(which_chain, 0x01, 0xD1, data, 2);
    
    if (_Var1) {
        // Модификация данных: очистка младших 5 бит и установка значения 5
        // data[1] = data[1] & 0xE0 | 5;
        data[1] = (data[1] & 0xE0) | 5;
        
        usleep(200000);
        
        // Запись обратно в регистр конфигурации
        _Var1 = write_dac(which_chain, 0x01, 0xD1, data, 2);
        
        if (_Var1) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : success\n", "init_dac53401_NBT2006_36");
            strncpy(tmp1, "success\0", 8);
            log_to_file(tmp1, time_stamp);
            return true;
        } else {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : fail\n", "init_dac53401_NBT2006_36");
            strncpy(tmp1, "fail\0", 4);
            log_to_file(tmp1, time_stamp);
            return false;
        }
    }
    return false;
}

/**
 * @brief Преобразование напряжения (V) в цифровой код (N) для DAC.
 * Формула: N = ((15.32 - V) / 4.69) * 1024
 * Поддерживается только плата NBT2006-36.
 */
bool convert_V_to_N(char *board_name, double voltage, uint32_t *N) {
    int iVar1;
    char time_stamp[48];
    char tmp1[256];

    iVar1 = strcmp(board_name, "NBT2006-36");
    if (iVar1 == 0) {
        if (voltage <= 15.32) {
            // Расчет N
            double dVar3 = ((15.32 - voltage) / 4.690000057220459) * 1024.0;
            *N = (uint32_t)(dVar3); // Приведение к целому
            
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : %s dac voltage N: %d\n", "convert_V_to_N", board_name, *N);
            snprintf(tmp1, 0x100, "%s dac voltage N: %d ", board_name, *N);
            log_to_file(tmp1, time_stamp);
            return true;
        } else {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : voltage can't bigger than 15.32v\n", "convert_V_to_N");
            strncpy(tmp1, "voltage can't bigger than 15.32v\0", 0x20);
            log_to_file(tmp1, time_stamp);
            return false;
        }
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Do not support %s\n", "convert_V_to_N", board_name);
        snprintf(tmp1, 0x100, "Do not support %s ", board_name);
        log_to_file(tmp1, time_stamp);
        return false;
    }
}

/**
 * @brief Преобразование цифрового кода (N) в напряжение (V) для DAC.
 * Формула: V = 15.32 - (N / 1024) * 4.69
 * Поддерживается только плата NBT2006-36.
 */
bool convert_N_to_V(char *board_name, int32_t n, double *voltage) {
    int iVar1;
    char time_stamp[48];
    char tmp1[256];

    iVar1 = strcmp(board_name, "NBT2006-36");
    if (iVar1 == 0) {
        if (n < 0x400) { // 1024
            // Расчет напряжения
            // В псевдокоде используется VectorSignedToFloat, здесь эмулируем приведением
            double dVar3 = (double)n; 
            *voltage = 15.32 - (dVar3 / 1024.0) * 4.69;
            
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : %s dac voltage: %f\n", "convert_N_to_V", board_name, *voltage);
            snprintf(tmp1, 0x100, "%s dac voltage: %f ", board_name, *voltage);
            log_to_file(tmp1, time_stamp);
            return true;
        } else {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : N can't bigger than 1023\n", "convert_N_to_V");
            strncpy(tmp1, "N can't bigger than 1023\0", 0x18);
            log_to_file(tmp1, time_stamp);
            return false;
        }
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Do not support %s\n", "convert_N_to_V", board_name);
        snprintf(tmp1, 0x100, "Do not support %s ", board_name);
        log_to_file(tmp1, time_stamp);
        return false;
    }
}

/**
 * @brief Установка напряжения через DAC53401 по цифровому коду N.
 * Формирует 2 байта данных для регистрации данных DAC.
 */
bool set_dac53401_voltage(uint8_t which_chain, uint32_t voltage_N) {
    bool _Var1;
    char time_stamp[48];
    char tmp1[256];
    uint8_t data[2];

    // Формирование данных: сдвиг N на 2 бита влево (так как DAC 10-битный, но данные 12-битные или выравнивание)
    // data[0] = (voltage_N << 2) >> 8;
    // data[1] = (voltage_N << 2) & 0xFF;
    uint32_t shifted_N = voltage_N << 2;
    data[0] = (uint8_t)(shifted_N >> 8);
    data[1] = (uint8_t)(shifted_N & 0xFF);

    // Запись в регистр данных DAC (0x21, '!' в ASCII)
    _Var1 = write_dac(which_chain, 0x01, 0x21, data, 2);
    
    if (!_Var1) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : fail\n", "set_dac53401_voltage");
        strncpy(tmp1, "fail\0", 4);
        log_to_file(tmp1, time_stamp);
    }
    return _Var1;
}

/**
 * @brief Установка напряжения по коду N с проверкой типа платы.
 */
bool set_voltage_by_N(char *board_name, uint8_t which_chain, uint32_t voltage_N) {
    bool _Var1;
    int iVar2;
    char time_stamp[48];
    char tmp1[256];

    iVar2 = strcmp(board_name, "NBT2006-36");
    if (iVar2 == 0) {
        _Var1 = set_dac53401_voltage(which_chain, voltage_N);
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : %s not use dac\n", "set_voltage_by_N", board_name);
        snprintf(tmp1, 0x100, "%s not use dac", board_name);
        log_to_file(tmp1, time_stamp);
        _Var1 = false;
    }
    return _Var1;
}

/**
 * @brief Инициализация DAC с проверкой типа платы.
 */
bool init_dac(char *board_name, uint8_t which_chain) {
    bool _Var1;
    int iVar2;
    char time_stamp[48];
    char tmp1[256];

    iVar2 = strcmp(board_name, "NBT2006-36");
    if (iVar2 == 0) {
        _Var1 = init_dac53401_NBT2006_36(which_chain);
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : %s not use dac\n", "init_dac", board_name);
        snprintf(tmp1, 0x100, "%s not use dac", board_name);
        log_to_file(tmp1, time_stamp);
        _Var1 = false;
    }
    return _Var1;
}

/**
 * @brief Установка напряжения через DAC по значению в вольтах.
 */
bool set_dac_voltage(char *board_name, uint8_t which_chain, double voltage) {
    bool _Var1;
    int iVar2;
    char time_stamp[48];
    char tmp1[256];
    uint32_t voltage_N = 0;

    iVar2 = strcmp(board_name, "NBT2006-36");
    if (iVar2 == 0) {
        _Var1 = convert_V_to_N(board_name, voltage, &voltage_N);
        if (_Var1) {
            _Var1 = set_voltage_by_N(board_name, which_chain, voltage_N);
            if (!_Var1) {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : fail\n", "set_dac_voltage");
                strncpy(tmp1, "fail\0", 4);
                log_to_file(tmp1, time_stamp);
            }
        } else {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : fail\n", "set_dac_voltage");
            strncpy(tmp1, "fail\0", 4);
            log_to_file(tmp1, time_stamp);
            _Var1 = false;
        }
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : %s not use dac\n", "set_dac_voltage", board_name);
        snprintf(tmp1, 0x100, "%s not use dac", board_name);
        log_to_file(tmp1, time_stamp);
        _Var1 = false;
    }
    return _Var1;
}

/**
 * @brief Плавное изменение напряжения через DAC шаг за шагом.
 */
bool set_dac_voltage_step_by_step(char *board_name, uint8_t which_chain, uint32_t pre_voltage, uint32_t target_voltage, uint8_t step_num) {
    bool _Var1;
    int iVar3;
    char time_stamp[48];
    char tmp1[256];
    uint32_t target_N = 0;
    uint32_t pre_N = 0;
    uint32_t setting_N = 0;
    uint32_t N_gap = 0;
    uint8_t step_length = 0;
    uint8_t i = 0;

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : pre_voltage = %d, target_voltage = %d, step_num = %d\n", "set_dac_voltage_step_by_step", pre_voltage, target_voltage, (uint)step_num);
    snprintf(tmp1, 0x100, "pre_voltage = %d, target_voltage = %d, step_num = %d ", pre_voltage, target_voltage, (uint)step_num);
    log_to_file(tmp1, time_stamp);

    // Конвертация начального напряжения в N
    double dVar4 = (double)pre_voltage; // Эмуляция VectorUnsignedToFloat
    _Var1 = convert_V_to_N(board_name, dVar4 / 100.0, &pre_N);
    
    if (_Var1) {
        // Конвертация целевого напряжения в N
        dVar4 = (double)target_voltage; // Эмуляция VectorUnsignedToFloat
        _Var1 = convert_V_to_N(board_name, dVar4 / 100.0, &target_N);
        
        if (_Var1) {
            if (target_N < pre_N) {
                N_gap = pre_N - target_N;
            } else {
                if (target_N <= pre_N) {
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : target_voltage = pre_voltage. Don't need do anything.\n", "set_dac_voltage_step_by_step");
                    strncpy(tmp1, "target_voltage = pre_voltage. Don't need do anything.\0", 0x34);
                    log_to_file(tmp1, time_stamp);
                    return true;
                }
                N_gap = target_N - pre_N;
            }

            // Расчет длины шага
            uint uVar2 = N_gap / step_num; // __aeabi_uidiv
            step_length = (uint8_t)uVar2;

            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : pre_N = %d, target_N = %d, N_gap = %d, step_length = %d\n", "set_dac_voltage_step_by_step", pre_N, target_N, N_gap, (uint)step_length);
            snprintf(tmp1, 0x100, "pre_N = %d, target_N = %d, N_gap = %d, step_length = %d ", pre_N, target_N, N_gap, (uint)step_length);
            log_to_file(tmp1, time_stamp);

            if (step_length == 0) {
                // Если шаг слишком мал, меняем по 1 единице пока не достигнем цели
                for (i = 0; i < N_gap; i++) {
                    if (target_N < pre_N) {
                        setting_N = pre_N - 1;
                    } else {
                        setting_N = pre_N + 1;
                    }
                    // В оригинале цикл просто обновляет setting_N, но не вызывает установку каждый раз? 
                    // Нет, в оригинале цикл просто вычисляет конечное значение, если шаг 0.
                    // Но потом вызывается set_voltage_by_N один раз.
                }
                
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : setting_N = %d\n", "set_dac_voltage_step_by_step", setting_N);
                snprintf(tmp1, 0x100, "setting_N = %d ", setting_N);
                log_to_file(tmp1, time_stamp);
                
                _Var1 = set_voltage_by_N(board_name, which_chain, setting_N);
                if (!_Var1) {
                    lcd_clear_result();
                    lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
                    lcd_show(1, "Set ADC voltage ");
                    lcd_show(2, "fail ");
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : Set adc voltage by N fail\n", "set_dac_voltage_step_by_step");
                    strncpy(tmp1, "Set adc voltage by N fail\0", 0x1A);
                    log_to_file(tmp1, time_stamp);
                    return false;
                }
            } else {
                // Пошаговое изменение
                for (i = 0; i < step_num; i++) {
                    if (i == 0) {
                        if (target_N < pre_N) {
                            // Расчет первого шага с учетом остатка от деления
                            iVar3 = pre_N - step_length;
                            // __aeabi_uidivmod(N_gap, step_num) возвращает частное в r0 и остаток в r1
                            // В псевдокоде используется extraout_r1
                            int remainder = N_gap % step_num; 
                            setting_N = iVar3 - remainder;
                        } else {
                            iVar3 = step_length + pre_N;
                            int remainder = N_gap % step_num;
                            setting_N = remainder + iVar3;
                        }
                    } else if (target_N < pre_N) {
                        setting_N = setting_N - step_length;
                    } else {
                        setting_N = step_length + setting_N;
                    }

                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : setting_N = %d, step=%d\n", "set_dac_voltage_step_by_step", setting_N, (uint)i);
                    snprintf(tmp1, 0x100, "setting_N = %d, step=%d ", setting_N, (uint)i);
                    log_to_file(tmp1, time_stamp);

                    _Var1 = set_voltage_by_N(board_name, which_chain, setting_N);
                    if (!_Var1) {
                        lcd_clear_result();
                        lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
                        lcd_show(1, "Set ADC voltage ");
                        lcd_show(2, "fail ");
                        print_time_stamp(time_stamp, 0x30);
                        printf("%s ", time_stamp);
                        printf("%s : Set adc voltage by N = %d fail\n", "set_dac_voltage_step_by_step", setting_N);
                        snprintf(tmp1, 0x100, "Set adc voltage by N = %d fail ", setting_N);
                        log_to_file(tmp1, time_stamp);
                        return false;
                    }
                    usleep(300000);
                }
            }
            _Var1 = true;
        } else {
            _Var1 = false;
        }
    } else {
        _Var1 = false;
    }
    return _Var1;
}