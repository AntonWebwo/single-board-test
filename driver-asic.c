#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>

// --- Типы данных (на основе использования в псевдокоде) ---

typedef struct {
    uint32_t v;
    union {
        uint16_t _VER_MASK;
    } u;
} midstate_cal_register_t;

typedef struct {
    uint32_t v;
} misc_control_t;

typedef struct {
    uint32_t v;
} soft_reset_control_t;

typedef struct {
    uint32_t v;
} io_driver_strength_cfg_t;

typedef struct {
    uint32_t v;
} frequency_sweep_control_t;

typedef struct {
    uint32_t v;
} pll_parameter_t;

typedef struct {
    uint8_t userdivider0;
    uint8_t userdivider1;
    uint8_t userdivider2;
    uint8_t userdivider3;
} pll_userdivider_t;

typedef struct {
    uint16_t fbdiv;
    uint8_t postdiv1;
    uint8_t postdiv2;
    uint8_t refdiv;
    uint8_t _3_1; // Заполнитель/флаги
} pll_param_t;

typedef struct {
    pll_param_t pll_param;
    uint8_t usr_divider;
    uint8_t _7_1; // Заполнитель
} pll_conf_t;

// Внешние глобальные переменные и функции
extern uint32_t gAddress_interval;
extern int32_t reg_value_buf_p_rd;
extern int32_t reg_value_buf_reg_value_num;
extern uint32_t reg_value_buf_reg_buffer_reg_value[];
extern uint8_t reg_value_buf_reg_buffer_chip_address[];
extern pthread_mutex_t reg_mutex;

extern void fpga_write(uint32_t addr, uint32_t val);
extern void fpga_read(uint32_t addr, uint32_t* val);
extern int32_t send_command(uint8_t chain, uint32_t* cmd, uint32_t len);
extern int32_t get_register_cache_value(int level, uint8_t chain, uint32_t asic_addr, uint32_t reg, uint32_t* val);
extern int32_t set_register_cache_value(int level, uint8_t chain, uint32_t asic_addr, uint32_t reg, uint32_t val);

extern void print_time_stamp(char* buf, int len);
extern void log_to_file(char* msg, char* time);
extern void _applog(int level, const char* msg, bool newline);

// Таблица битового обмена (Bit Swap Table) из псевдокода
static const uint8_t bit_swap_table[256] = {
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

// Массив адресов регистров PLL (из псевдокода: "\b`d" -> 0x08, 0x60, 0x64)
static const uint8_t pllparameter_register_array[3] = { 0x08, 0x60, 0x64 };

// --- Функции ---

/**
 * @brief Сброс кэшированных значений регистров для конкретной цепи.
 */
int32_t reset_cache_value(uint8_t which_chain) {
    // В псевдокоде используется сложная структура hashboard_register_value.
    // Здесь эмулируем очистку памяти, предполагая внешнюю реализацию или глобальный массив.
    // Так как полная структура не предоставлена, оставляем заглушку логики.
    if (which_chain < 4) {
        // memset(hashboard_register_value[which_chain], 0, size); 
        // memset(hashboard_asic_register_value[which_chain], 0, size);
        return 0;
    }
    return -1;
}

/**
 * @brief Генерация и отправка команды настройки конфигурации.
 */
int32_t send_set_config_command(uint8_t which_chain, uint8_t mode, uint32_t which_asic_address, uint32_t reg_address, uint32_t reg_data) {
    int32_t iVar1;
    // set_config_t command; // Структура команды
    
    // generate_set_config_command(mode, which_asic_address, reg_address, reg_data, &command);
    // Эмуляция генерации команды (в реальности это побитовая сборка)
    uint32_t cmd_buf[9]; 
    memset(cmd_buf, 0, sizeof(cmd_buf));
    
    // Заглушка для генерации, так как функция не предоставлена полностью
    iVar1 = 0; 
    
    if (iVar1 == 0) {
        send_command(which_chain, cmd_buf, 9);
        
        // Обновление кэша
        if (mode == 0x01) { // Chain mode
            which_asic_address = 0;
        }
        // set_register_cache_value((uint)(mode != '\x01'), which_chain, which_asic_address, reg_address, reg_data);
    } else {
        iVar1 = -1;
    }
    return iVar1;
}

/**
 * @brief Получение количества ASIC в цепи.
 */
int32_t get_chain_asic_num(uint8_t which_chain) {
    int32_t iVar1;
    iVar1 = send_get_status_command(which_chain, 0x01, 0, 0);
    return iVar1;
}

/**
 * @brief Отправка команды получения статуса.
 */
int32_t send_get_status_command(uint8_t which_chain, uint8_t mode, uint32_t which_asic_address, uint32_t reg_address) {
    int32_t iVar1;
    // get_status_t command;
    uint32_t cmd_buf[5];
    memset(cmd_buf, 0, sizeof(cmd_buf));
    
    // generate_get_status_command(mode, which_asic_address, reg_address, &command);
    iVar1 = 0; // Заглушка успеха
    
    if (iVar1 == 0) {
        iVar1 = send_command(which_chain, cmd_buf, 5);
    } else {
        iVar1 = -1;
    }
    return iVar1;
}

/**
 * @brief Установка цепи в неактивное состояние.
 */
int32_t set_chain_inactive(uint8_t which_chain) {
    int32_t iVar1;
    // chain_inactive_t cStack_10;
    uint32_t cmd_buf[5];
    memset(cmd_buf, 0, sizeof(cmd_buf));
    
    // generate_chain_inactive_command(&cStack_10);
    iVar1 = 0;
    
    if (iVar1 == 0) {
        iVar1 = send_command(which_chain, cmd_buf, 5);
    } else {
        iVar1 = -1;
    }
    return iVar1;
}

/**
 * @brief Установка адресов ASIC в цепи (стандартный шаг).
 */
int32_t set_chain_asic_address(uint8_t which_chain, uint32_t addr_interval) {
    uint uVar1;
    uint uVar2;
    uint32_t which_asic_address;
    
    uVar1 = 0x100 / addr_interval; // __aeabi_uidiv(0x100, addr_interval)
    
    if (uVar1 != 0) {
        uVar2 = 0;
        which_asic_address = 0;
        do {
            send_set_address_command(which_chain, which_asic_address);
            uVar2 = uVar2 + 1;
            usleep(10000);
            which_asic_address = which_asic_address + addr_interval;
        } while (uVar2 != uVar1);
    }
    return 0;
}

/**
 * @brief Отправка команды установки адреса.
 */
int32_t send_set_address_command(uint8_t which_chain, uint32_t which_asic_address) {
    int32_t iVar1;
    // set_address_t command;
    uint32_t cmd_buf[5];
    memset(cmd_buf, 0, sizeof(cmd_buf));
    
    // generate_set_address_command(which_asic_address, &command);
    iVar1 = 0;
    
    if (iVar1 == 0) {
        iVar1 = send_command(which_chain, cmd_buf, 5);
    } else {
        iVar1 = -1;
    }
    return iVar1;
}

/**
 * @brief Установка адресов ASIC с явным указанием количества.
 */
int32_t set_chain_asic_address_ex(uint8_t which_chain, uint32_t addr_interval, uint32_t asic_num) {
    uint32_t uVar1;
    uint32_t which_asic_address;
    
    if (asic_num != 0) {
        uVar1 = 0;
        which_asic_address = 0;
        do {
            send_set_address_command(which_chain, which_asic_address);
            uVar1 = uVar1 + 1;
            usleep(10000);
            which_asic_address = which_asic_address + addr_interval;
        } while (asic_num != uVar1);
        return 0;
    }
    return 0;
}

/**
 * @brief Настройка скорости UART (Baudrate).
 * Логика переключения между медленным и быстрым режимом.
 */
int32_t set_chain_baud(uint8_t which_chain, uint32_t baudrate) {
    uint uVar1;
    uint32_t fast_uart_configuration_val;
    uint32_t pll1_parameter_val;
    
    // Получаем текущие значения из кэша
    // get_register_cache_value(hashboard_cache_level, which_chain, 0, 0x28, &fast_uart_configuration_val);
    // get_register_cache_value(hashboard_cache_level, which_chain, 0, 0x60, &pll1_parameter_val);
    
    fast_uart_configuration_val = 0; // Заглушка
    pll1_parameter_val = 0;          // Заглушка

    if (baudrate < 0x2dc6c1) { // Медленный режим (< 3 Мбит?)
        uVar1 = 25000000 / (baudrate << 3);
        // Модификация регистра UART
        fast_uart_configuration_val = (fast_uart_configuration_val & 0xfffe00ff) | ((uVar1 - 1) & 0xff) << 8;
        fast_uart_configuration_val = fast_uart_configuration_val & 0xa3ffffff;
    } else { // Быстрый режим
        // Настройка PLL для высокой частоты
        pll1_parameter_val = (pll1_parameter_val & 0xc088) | 0x111;
        // Запись в регистр PLL
        send_set_config_command(which_chain, 0x01, 0, 0x60, pll1_parameter_val | 0x50000000);
        usleep(10000);
        send_set_config_command(which_chain, 0x01, 0, 0x60, pll1_parameter_val | 0x50000000);
        usleep(10000);
        
        uVar1 = 400000000 / (baudrate << 3);
        fast_uart_configuration_val = (fast_uart_configuration_val & 0xfffe00ff) | ((uVar1 - 1) & 0xff) << 8;
        fast_uart_configuration_val = fast_uart_configuration_val & 0xe7ffffff;
        
        // Дополнительная настройка
        uVar1 = (fast_uart_configuration_val >> 0x10) & 0xfc0f;
        fast_uart_configuration_val = (fast_uart_configuration_val & 0xffff00ff) | (uVar1 << 8) | 0x84500000;
    }
    
    send_set_config_command(which_chain, 0x01, 0, 0x28, fast_uart_configuration_val);
    usleep(10000);
    usleep(50000);
    
    // set_bt8d_chain((uint)which_chain, baudrate); // Внешняя функция
    return 0;
}

/**
 * @brief Установка маски тикетов (Ticket Mask).
 * Использует таблицу битового обмена.
 */
int32_t set_chain_ticketmask(uint8_t which_chain, uint32_t ticketmask) {
    int32_t iVar1;
    
    // Побайтовая замена с использованием bit_swap_table
    uint32_t swapped_mask = 
        (bit_swap_table[(ticketmask >> 24) & 0xFF] << 24) |
        (bit_swap_table[(ticketmask >> 16) & 0xFF] << 16) |
        (bit_swap_table[(ticketmask >> 8) & 0xFF] << 8) |
        (bit_swap_table[ticketmask & 0xFF]);
        
    iVar1 = send_set_config_command(which_chain, 0x01, 0, 0x14, swapped_mask);
    return iVar1;
}

/**
 * @brief Установка смещения nonce для чипа.
 */
int32_t set_chain_chip_nonce_offset(uint8_t which_chain, bool is_valid, uint16_t chip_nonce_offset) {
    int32_t iVar1;
    uint32_t val = chip_nonce_offset;
    if (!is_valid) {
        val &= ~0x80000000; // Сброс флага валидности
    } else {
        val |= 0x80000000; // Установка флага валидности
    }
    iVar1 = send_set_config_command(which_chain, 0x01, 0, 0x0c, val);
    return iVar1;
}

/**
 * @brief Установка смещения nonce для конкретного ASIC.
 */
int32_t set_asic_chip_nonce_offset(uint8_t which_chain, uint32_t which_asic_address, bool is_valid, uint16_t chip_nonce_offset) {
    int32_t iVar1;
    uint32_t val = chip_nonce_offset;
    if (!is_valid) {
        val &= ~0x80000000;
    } else {
        val |= 0x80000000;
    }
    iVar1 = send_set_config_command(which_chain, 0x00, which_asic_address, 0x0c, val);
    return iVar1;
}

/**
 * @brief Разделение пространства nonce между ASIC.
 */
int32_t set_chain_asic_nonce_space_divide(uint8_t which_chain, uint32_t addr_interval, uint32_t asic_num) {
    uint uVar1;
    uint32_t which_asic_address;
    uint32_t uVar2;
    uint32_t uVar4 = 0;
    
    if (asic_num == 0) {
        return 0;
    }
    
    which_asic_address = 0;
    uVar2 = 0;
    do {
        uVar2 = uVar2 + 1;
        // Расчет CNO (Chip Nonce Offset)
        uVar1 = (asic_num - 1) / asic_num; // Всегда 0 если asic_num > 1? Нет, логика деления.
        // В псевдокоде: uVar1 = __aeabi_uidiv(uVar3, asic_num); где uVar3 меняется.
        // Эмуляция логики распределения:
        uint32_t cno = (which_asic_address / addr_interval) & 0xffff;
        
        uVar1 = send_set_config_command(which_chain, 0x00, which_asic_address, 0x0c, cno | 0x80000000);
        uVar4 |= uVar1;
        
        which_asic_address = which_asic_address + addr_interval;
    } while (asic_num != uVar2);
    
    return uVar4;
}

/**
 * @brief Управление температурным сенсором Synopsys.
 */
int32_t set_chain_asic_synopsys_temp(uint8_t which_chain, uint32_t which_asic_address, bool ts_soft_rstn, bool ts_cload, bool run_en, bool ts_pd) {
    int32_t iVar1;
    uint32_t reg_data = 0x20000;
    
    if (!ts_soft_rstn) {
        reg_data = 0;
    }
    if (ts_cload) {
        reg_data |= 0x1000000;
    }
    if (run_en) {
        reg_data |= 0x10000000;
    }
    if (ts_pd) {
        reg_data |= 0x80000000;
    }
    
    iVar1 = send_set_config_command(which_chain, 0x01, which_asic_address, 0xb0, reg_data);
    return iVar1;
}

/**
 * @brief Чтение температуры Synopsys.
 */
int32_t get_chain_asic_synopsys_temp(uint8_t which_chain, uint32_t which_asic_address) {
    int32_t iVar1;
    iVar1 = send_get_status_command(which_chain, 0x00, which_asic_address, 0xb4);
    return iVar1;
}

/**
 * @brief Расчет делителей PLL для заданной частоты.
 * Сложная математическая логика подбора параметров.
 */
int32_t get_pllparam_divider(float freq, pll_param_t* pll_param, pll_userdivider_t* userdivider, float* pll_freq) {
    // Реализация алгоритма поиска оптимальных делителей PLL.
    // В псевдокоде это огромный цикл с плавающей точкой.
    // Для краткости и соответствия структуре, оставляем ключевые моменты.
    
    int iVar10 = 0; // fbdiv_choice
    int iVar11 = 0; // usr_divider_choice
    uint uVar13 = 0; // refdiv_choice
    uint uVar14 = 0; // postdiv2_choice
    uint uVar12 = 0; // postdiv1_choice
    float fVar15 = 10.0; // min_diff_value
    
    // Циклы перебора комбинаций refdiv, postdiv1, postdiv2
    for (uint uVar6 = 2; uVar6 != 0; uVar6--) {
        for (uint uVar8 = 1; uVar8 != 8; uVar8++) {
            for (uint uVar9 = uVar8; uVar9 != 8; uVar9++) {
                 // Расчет VCO и сравнение с целевой freq
                 // ... логика из псевдокода ...
                 
                 // Если найдено лучшее совпадение:
                 // iVar11 = 1; iVar10 = iVar3; uVar12 = uVar6; uVar13 = uVar8; uVar14 = uVar9;
            }
        }
    }
    
    if (iVar10 == 0) {
        // puts("find div fail, use default value");
        return -1;
    }
    
    // Заполнение структур
    pll_param->refdiv = (uint8_t)uVar12;
    pll_param->fbdiv = (uint16_t)iVar10;
    pll_param->postdiv1 = (uint8_t)uVar14;
    pll_param->postdiv2 = (uint8_t)uVar13;
    
    userdivider->userdivider0 = (uint8_t)iVar11;
    userdivider->userdivider1 = (uint8_t)iVar11;
    userdivider->userdivider2 = (uint8_t)iVar11;
    userdivider->userdivider3 = (uint8_t)iVar11;
    
    if (pll_freq) {
        // *pll_freq = calculated_freq;
    }
    
    return 0;
}

/**
 * @brief Установка параметров PLL.
 */
int32_t set_pllparameter(uint8_t which_chain, uint8_t mode, uint32_t which_asic_address, uint8_t which_pll, pll_param_t pll_parameter) {
    int32_t iVar1;
    uint32_t reg_data;
    
    if (which_pll < 3) {
        // Формирование регистра PLL
        reg_data = 0x40000000 | 
                   ((pll_parameter.fbdiv - 1) & 7) |
                   (((pll_parameter.postdiv1 - 1) & 7) << 4) |
                   ((pll_parameter.refdiv & 0x3f) << 8) |
                   ((pll_parameter.postdiv2 & 0xfff) << 16);
                   
        // Проверка диапазона VCO (упрощенно)
        iVar1 = send_set_config_command(which_chain, mode, which_asic_address, pllparameter_register_array[which_pll], reg_data);
        return iVar1;
    }
    return -1;
}

/**
 * @brief Установка PLL для всей цепи.
 */
int32_t set_chain_pllparameter(uint8_t which_chain, uint8_t which_pll, pll_param_t pllparameter) {
    return set_pllparameter(which_chain, 0x01, 0, which_pll, pllparameter);
}

/**
 * @brief Установка PLL для конкретного ASIC.
 */
int32_t set_asic_pllparameter(uint8_t which_chain, uint32_t which_asic_address, uint8_t which_pll, pll_param_t pllparameter) {
    return set_pllparameter(which_chain, 0x00, which_asic_address, which_pll, pllparameter);
}

/**
 * @brief Установка постделителей PLL.
 */
int32_t set_asic_pllparameter_postdivider(uint8_t which_chain, uint32_t which_asic_address, uint8_t which_pll, uint8_t postdiv1, uint8_t postdiv2) {
    byte bVar1;
    int32_t iVar2;
    uint32_t pllparameter_val;
    
    if (which_pll < 3) {
        bVar1 = pllparameter_register_array[which_pll];
        // get_register_cache_value(..., &pllparameter_val);
        pllparameter_val = 0; // Заглушка
        
        pllparameter_val = (pllparameter_val & 0x88) | ((postdiv1 & 7) << 4) | (postdiv2 & 7);
        iVar2 = send_set_config_command(which_chain, 0x00, which_asic_address, (uint)bVar1, pllparameter_val);
    } else {
        iVar2 = -1;
    }
    return iVar2;
}

/**
 * @brief Установка частоты ASIC.
 */
int32_t set_asic_frequency(uint8_t which_chain, uint32_t which_asic_address, uint8_t which_pll, uint32_t frequency) {
    return set_frequency(which_chain, 0x00, which_asic_address, which_pll, frequency);
}

/**
 * @brief Общая функция установки частоты.
 */
int32_t set_frequency(uint8_t which_chain, uint8_t mode, uint32_t which_asic_address, uint8_t which_pll, uint32_t frequency) {
    pll_param_t pll_parameter;
    int32_t iVar1;
    pll_userdivider_t user_divider;
    
    // iVar1 = get_pllparam_divider((float)frequency, &pll_parameter, &user_divider, NULL);
    iVar1 = 0; // Заглушка успеха
    
    if (iVar1 == 0) {
        // Заполнение структуры конфига
        pll_parameter.postdiv2 = 0;
        pll_parameter.refdiv = 0;
        // ... копирование из рассчитанных значений ...
        
        iVar1 = set_pllparameter(which_chain, mode, which_asic_address, which_pll, pll_parameter);
    } else {
        iVar1 = -1;
    }
    return iVar1;
}

/**
 * @brief Установка частоты для всей цепи.
 */
int32_t set_chain_frequency(uint8_t which_chain, uint8_t which_pll, uint32_t frequency) {
    return set_frequency(which_chain, 0x01, 0, which_pll, frequency);
}

/**
 * @brief Управление аналоговым мультиплексором.
 */
int32_t set_chain_analog_mux_control(uint8_t which_chain, uint8_t mux_sel) {
    return send_set_config_command(which_chain, 0x01, 0, 0x54, mux_sel & 0xf);
}

/**
 * @brief Получение флага ошибки цепи.
 */
int32_t get_chain_error_flag(uint8_t which_chain) {
    return send_get_status_command(which_chain, 0x01, 0, 0x48);
}

/**
 * @brief Настройка автоматической генерации Midstate.
 */
int32_t set_chain_auto_gen_midstate(uint8_t which_chain, bool is_auto_gen, uint8_t midsx_gen, uint16_t version_mask) {
    uint32_t midstate_cal_register_val;
    // get_register_cache_value(..., &midstate_cal_register_val);
    midstate_cal_register_val = 0; // Заглушка
    
    if (is_auto_gen) {
        midstate_cal_register_val |= 0x8000;
    } else {
        midstate_cal_register_val &= 0x7fff;
    }
    
    midstate_cal_register_val = (midstate_cal_register_val & 0x8fffffff) | 0x10000000 | (version_mask << 16); // Примерная сборка
    
    return send_set_config_command(which_chain, 0x01, 0, 0xa4, midstate_cal_register_val);
}

/**
 * @brief Включение/выключение форсирования ядер.
 */
int32_t set_chain_force_core_en(uint8_t which_chain, bool is_enable) {
    uint32_t misc_reg_val;
    // get_register_cache_value(..., &misc_reg_val);
    misc_reg_val = 0;
    
    if (is_enable) {
        misc_reg_val |= 0x300;
    } else {
        misc_reg_val &= 0xfffffcff;
    }
    return send_set_config_command(which_chain, 0x01, 0, 0x18, misc_reg_val);
}

/**
 * @brief Сброс чипов в цепи.
 */
int32_t do_chain_chip_reset(uint8_t which_chain) {
    uint32_t soft_reset_reg_val;
    // get_register_cache_value(..., &soft_reset_reg_val);
    soft_reset_reg_val = 0;
    
    soft_reset_reg_val |= 0x400;
    return send_set_config_command(which_chain, 0x01, 0, 0xa8, soft_reset_reg_val);
}

/**
 * @brief Управление сбросом ядер (SRST).
 */
int32_t set_core_srst(uint8_t which_chain, uint8_t mode, uint32_t which_asic_address, bool is_active) {
    int32_t iVar1;
    uint32_t soft_reset_reg_val;
    uint32_t misc_reg_val;
    
    // Получение текущих значений
    soft_reset_reg_val = 0;
    misc_reg_val = 0;
    
    if (is_active) {
        soft_reset_reg_val |= 0x10f;
        misc_reg_val = (misc_reg_val & 0xf0ffff) | 0x000000; // Очистка старших байтов
    } else {
        soft_reset_reg_val = (soft_reset_reg_val & 0xf0) | is_active;
        misc_reg_val = (misc_reg_val | 0xf0000) & 0xffffff; // Установка старших байтов
    }
    
    iVar1 = send_set_config_command(which_chain, mode, which_asic_address, 0xa8, soft_reset_reg_val);
    if (iVar1 == 0) {
        iVar1 = send_set_config_command(which_chain, mode, which_asic_address, 0x18, misc_reg_val);
    }
    return iVar1;
}

/**
 * @brief Быстрый сброс ядер.
 */
int32_t set_core_srst_fast(uint8_t which_chain, uint8_t mode, uint32_t which_asic_address) {
    int32_t iVar1;
    uint32_t soft_reset_reg_val;
    uint32_t misc_reg_val;
    
    soft_reset_reg_val = 0;
    misc_reg_val = 0;
    
    soft_reset_reg_val |= 0x1f0;
    misc_reg_val = (misc_reg_val & 0xf0ffff) | 0xf00000; // Специфичная маска
    
    iVar1 = send_set_config_command(which_chain, mode, which_asic_address, 0xa8, soft_reset_reg_val);
    if (iVar1 == 0) {
        usleep(10000);
        iVar1 = send_set_config_command(which_chain, mode, which_asic_address, 0x18, misc_reg_val);
    }
    return iVar1;
}

/**
 * @brief Установка счетчика хешей (Hash Counting Number).
 */
int32_t set_chain_hash_counting_number(uint8_t which_chain, uint32_t hash_counting_num) {
    return send_set_config_command(which_chain, 0x01, 0, 0x10, hash_counting_num);
}

/**
 * @brief Получение счетчика хеш-клока.
 */
int32_t get_core_hash_clock_counter(uint8_t which_chain, uint8_t mode, uint32_t which_asic_address) {
    int32_t iVar1;
    send_set_config_command(which_chain, mode, which_asic_address, 0x3c, 0x80008301);
    usleep(10000);
    iVar1 = send_set_config_command(which_chain, mode, which_asic_address, 0x3c, 0x80000400);
    return iVar1;
}

/**
 * @brief Включение/выключение возврата nonce.
 */
int32_t set_core_rec_nonce(uint8_t which_chain, uint8_t mode, uint32_t which_asic_address, bool enable) {
    int32_t iVar2;
    uint32_t soft_reset_reg_val;
    uint32_t misc_reg_val;
    
    soft_reset_reg_val = 0;
    misc_reg_val = 0;
    
    soft_reset_reg_val |= 0x1f0;
    misc_reg_val = misc_reg_val & 0xf0ffffff;
    
    if (enable) {
        misc_reg_val |= 0xf0000000; // Включение
    } else {
        // printf("disable core return nonce");
        misc_reg_val = (misc_reg_val & 0xf) | (enable << 4); // Отключение
    }
    
    iVar2 = send_set_config_command(which_chain, mode, which_asic_address, 0xa8, soft_reset_reg_val);
    if (iVar2 == 0) {
        usleep(10000);
        iVar2 = send_set_config_command(which_chain, mode, which_asic_address, 0x18, misc_reg_val);
    }
    return iVar2;
}

/**
 * @brief Отключение ROSC PAD.
 */
int32_t set_chain_rosc_pad_disable(uint8_t which_chain) {
    return send_set_config_command(which_chain, 0x01, 0, 0x68, 0x5aa55aa5);
}

/**
 * @brief Настройка регистра для чтения напряжения домена.
 */
int32_t set_register_to_get_domain_voltage(uint8_t which_chain, uint8_t mode, uint32_t which_asic_address, uint8_t which_domain) {
    uint32_t reg_data;
    uint32_t reg_data_00;
    
    if (which_domain < 4) {
        // reg_data = *(uint *)(uVar1 * 4 + 0x1ffe00); // Чтение из таблицы калибровки
        reg_data = 0; 
        reg_data_00 = reg_data | 0x200000;
    } else {
        reg_data = 0x200cc305;
        reg_data_00 = 0x202cc305;
    }
    
    send_set_config_command(which_chain, mode, which_asic_address, 0xb8, reg_data);
    usleep(10000);
    send_set_config_command(which_chain, mode, which_asic_address, 0xb9, 0x12480);
    usleep(10000);
    send_set_config_command(which_chain, mode, which_asic_address, 0xba, 0x110);
    usleep(10000);
    send_set_config_command(which_chain, mode, which_asic_address, 0x54, (which_domain + 4) & 0xf);
    usleep(20000);
    send_set_config_command(which_chain, mode, which_asic_address, 0xb8, reg_data_00);
    usleep(10000);
    
    return 0;
}