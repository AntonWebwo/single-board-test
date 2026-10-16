#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>

// --- Типы данных (на основе использования в псевдокоде) ---

// Заглушка для структуры eeprom_data_format_t
struct eeprom_data_format_t;
typedef struct eeprom_data_format_t* edf_ctx_p;

// Структура приватных данных V4 (предположительно)
struct edf_v4_priv {
    uint8_t data[0x72]; // Размер из malloc(0x72) в edf_v4_init
};

// Внешние функции
extern void print_time_stamp(char *buf, int len);
extern void log_to_file(char *msg, char *time);
extern void _applog(int level, const char* msg, bool newline);
extern bool data_dec(uint32_t *dst, uint32_t key, uint8_t k1, uint8_t k2);
extern bool data_enc(uint32_t *buf, uint32_t len, uint8_t algorithm_type, uint8_t key_version);
extern uint8_t CRC5_long(uint8_t *pdata, uint32_t length);

// --- Функции ---

/**
 * @brief Получение версии формата EEPROM.
 */
int edf_v4_version_get(edf_ctx_p ctx) {
    return (uint)*(byte *)ctx->priv;
}

/**
 * @brief Получение алгоритма шифрования.
 */
int edf_v4_enc_algorithm_get(edf_ctx_p ctx) {
    return (uint)(*(byte *)((int)ctx->priv + 1) >> 4);
}

/**
 * @brief Установка алгоритма шифрования.
 */
int edf_v4_enc_algorithm_set(edf_ctx_p ctx, uint8_t val) {
    *(byte *)((int)ctx->priv + 1) = *(byte *)((int)ctx->priv + 1) & 0xf;
    *(byte *)((int)ctx->priv + 1) = val << 4 | *(byte *)((int)ctx->priv + 1);
    return 0;
}

/**
 * @brief Получение версии ключа шифрования.
 */
int edf_v4_enc_key_version_get(edf_ctx_p ctx) {
    return *(byte *)((int)ctx->priv + 1) & 0xf;
}

/**
 * @brief Установка версии ключа шифрования.
 */
int edf_v4_enc_key_version_set(edf_ctx_p ctx, uint8_t val) {
    *(byte *)((int)ctx->priv + 1) = *(byte *)((int)ctx->priv + 1) & 0xf0;
    *(byte *)((int)ctx->priv + 1) = val & 0xf | *(byte *)((int)ctx->priv + 1);
    return 0;
}

/**
 * @brief Получение возможностей (capability).
 */
uint32_t edf_v4_capability(edf_ctx_p ctx) {
    return 6;
}

/**
 * @brief Установка серийного номера платы.
 */
int edf_v4_board_sn_set(edf_ctx_p ctx, char *array, size_t *len) {
    size_t __n = *len;
    if (0x11 < __n) {
        __n = 0x12;
    }
    memcpy((void *)((int)ctx->priv + 2), array, __n);
    *len = __n;
    return 0;
}

/**
 * @brief Получение серийного номера платы.
 */
int edf_v4_board_sn_get(edf_ctx_p ctx, char *array, size_t *len) {
    size_t __n = *len;
    if (0x11 < __n) {
        __n = 0x12;
    }
    memcpy(array, (void *)((int)ctx->priv + 2), __n);
    *len = __n;
    return 0;
}

/**
 * @brief Установка типа кристалла (chip die).
 */
int edf_v4_chip_die_set(edf_ctx_p ctx, char *array, size_t *len) {
    size_t __n = *len;
    if (2 < __n) {
        __n = 3;
    }
    memcpy((void *)((int)ctx->priv + 0x14), array, __n);
    *len = __n;
    return 0;
}

/**
 * @brief Получение типа кристалла (chip die).
 */
int edf_v4_chip_die_get(edf_ctx_p ctx, char *array, size_t *len) {
    size_t __n = *len;
    if (2 < __n) {
        __n = 3;
    }
    memcpy(array, (void *)((int)ctx->priv + 0x14), __n);
    *len = __n;
    return 0;
}

/**
 * @brief Установка маркировки чипа.
 */
int edf_v4_chip_marking_set(edf_ctx_p ctx, char *array, size_t *len) {
    size_t __n = *len;
    if (0xd < __n) {
        __n = 0xe;
    }
    memcpy((void *)((int)ctx->priv + 0x17), array, __n);
    *len = __n;
    return 0;
}

/**
 * @brief Получение маркировки чипа.
 */
int edf_v4_chip_marking_get(edf_ctx_p ctx, char *array, size_t *len) {
    size_t __n = *len;
    if (0xd < __n) {
        __n = 0xe;
    }
    memcpy(array, (void *)((int)ctx->priv + 0x17), __n);
    *len = __n;
    return 0;
}

/**
 * @brief Установка версии прошивки FT.
 */
int edf_v4_ft_version_set(edf_ctx_p ctx, char *array, size_t *len) {
    size_t __n = *len;
    if (9 < __n) {
        __n = 10;
    }
    memcpy((void *)((int)ctx->priv + 0x26), array, __n);
    *len = __n;
    return 0;
}

/**
 * @brief Получение версии прошивки FT.
 */
int edf_v4_ft_version_get(edf_ctx_p ctx, char *array, size_t *len) {
    size_t __n = *len;
    if (9 < __n) {
        __n = 10;
    }
    memcpy(array, (void *)((int)ctx->priv + 0x26), __n);
    *len = __n;
    return 0;
}

/**
 * @brief Установка технологии чипа.
 */
int edf_v4_chip_tech_set(edf_ctx_p ctx, char *array, size_t *len) {
    size_t __n = *len;
    if (2 < __n) {
        __n = 3;
    }
    memcpy((void *)((int)ctx->priv + 0x3b), array, __n);
    *len = __n;
    return 0;
}

/**
 * @brief Получение технологии чипа.
 */
int edf_v4_chip_tech_get(edf_ctx_p ctx, char *array, size_t *len) {
    size_t __n = *len;
    if (2 < __n) {
        __n = 3;
    }
    memcpy(array, (void *)((int)ctx->priv + 0x3b), __n);
    *len = __n;
    return 0;
}

/**
 * @brief Получение бина чипа.
 */
int edf_v4_chip_bin_get(edf_ctx_p ctx) {
    return (uint)*(byte *)((int)ctx->priv + 0x25);
}

/**
 * @brief Установка бина чипа.
 */
int edf_v4_chip_bin_set(edf_ctx_p ctx, uint8_t val) {
    *(uint8_t *)((int)ctx->priv + 0x25) = val;
    return 0;
}

/**
 * @brief Получение типа сенсора ASIC.
 */
int edf_v4_asic_sensor_type_get(edf_ctx_p ctx) {
    return (uint)*(byte *)((int)ctx->priv + 0x34);
}

/**
 * @brief Установка типа сенсора ASIC.
 */
int edf_v4_asic_sensor_type_set(edf_ctx_p ctx, uint8_t val) {
    *(uint8_t *)((int)ctx->priv + 0x34) = val;
    return 0;
}

/**
 * @brief Получение результата теста PT1.
 */
int edf_v4_pt1_result_get(edf_ctx_p ctx) {
    return (uint)*(byte *)((int)ctx->priv + 0x5f);
}

/**
 * @brief Установка результата теста PT1.
 */
int edf_v4_pt1_result_set(edf_ctx_p ctx, uint8_t val) {
    *(uint8_t *)((int)ctx->priv + 0x5f) = val;
    return 0;
}

/**
 * @brief Получение счетчика тестов PT1.
 */
int edf_v4_pt1_count_get(edf_ctx_p ctx) {
    return (uint)*(byte *)((int)ctx->priv + 0x60);
}

/**
 * @brief Установка счетчика тестов PT1.
 */
int edf_v4_pt1_count_set(edf_ctx_p ctx, uint8_t val) {
    *(uint8_t *)((int)ctx->priv + 0x60) = val;
    return 0;
}

/**
 * @brief Получение результата теста PT2.
 */
int edf_v4_pt2_result_get(edf_ctx_p ctx) {
    return (uint)*(byte *)((int)ctx->priv + 0x6c);
}

/**
 * @brief Установка результата теста PT2.
 */
int edf_v4_pt2_result_set(edf_ctx_p ctx, uint8_t val) {
    *(uint8_t *)((int)ctx->priv + 0x6c) = val;
    return 0;
}

/**
 * @brief Получение счетчика тестов PT2.
 */
int edf_v4_pt2_count_get(edf_ctx_p ctx) {
    return (uint)*(byte *)((int)ctx->priv + 0x6d);
}

/**
 * @brief Установка счетчика тестов PT2.
 */
int edf_v4_pt2_count_set(edf_ctx_p ctx, uint8_t val) {
    *(uint8_t *)((int)ctx->priv + 0x6d) = val;
    return 0;
}

/**
 * @brief Установка адресов сенсоров ASIC.
 */
int edf_v4_asic_sensor_addr_set(edf_ctx_p ctx, uint8_t *array, size_t *len) {
    size_t __n = *len;
    if (3 < __n) {
        __n = 4;
    }
    memcpy((void *)((int)ctx->priv + 0x35), array, __n);
    *len = __n;
    return 0;
}

/**
 * @brief Получение адресов сенсоров ASIC.
 */
int edf_v4_asic_sensor_addr_get(edf_ctx_p ctx, uint8_t *array, size_t *len) {
    size_t __n = *len;
    if (3 < __n) {
        __n = 4;
    }
    memcpy(array, (void *)((int)ctx->priv + 0x35), __n);
    *len = __n;
    return 0;
}

/**
 * @brief Получение типа сенсора PIC.
 */
int edf_v4_pic_sensor_type_get(edf_ctx_p ctx) {
    return (uint)*(byte *)((int)ctx->priv + 0x39);
}

/**
 * @brief Установка типа сенсора PIC.
 */
int edf_v4_pic_sensor_type_set(edf_ctx_p ctx, uint8_t val) {
    *(uint8_t *)((int)ctx->priv + 0x39) = val;
    return 0;
}

/**
 * @brief Получение адреса сенсора PIC.
 */
int edf_v4_pic_sensor_addr_get(edf_ctx_p ctx) {
    return (uint)*(byte *)((int)ctx->priv + 0x3a);
}

/**
 * @brief Установка адреса сенсора PIC.
 */
int edf_v4_pic_sensor_addr_set(edf_ctx_p ctx, uint8_t val) {
    *(uint8_t *)((int)ctx->priv + 0x3a) = val;
    return 0;
}

/**
 * @brief Получение версии PCB.
 */
int edf_v4_pcb_version_get(edf_ctx_p ctx) {
    return (uint)*(ushort *)((int)ctx->priv + 0x30);
}

/**
 * @brief Установка версии PCB.
 */
int edf_v4_pcb_version_set(edf_ctx_p ctx, uint16_t val) {
    *(uint16_t *)((int)ctx->priv + 0x30) = val;
    return 0;
}

/**
 * @brief Получение версии BOM.
 */
int edf_v4_bom_version_get(edf_ctx_p ctx) {
    return (uint)*(ushort *)((int)ctx->priv + 0x32);
}

/**
 * @brief Установка версии BOM.
 */
int edf_v4_bom_version_set(edf_ctx_p ctx, uint16_t val) {
    *(uint16_t *)((int)ctx->priv + 0x32) = val;
    return 0;
}

/**
 * @brief Установка имени платы.
 */
int edf_v4_board_name_set(edf_ctx_p ctx, char *array, size_t *len) {
    size_t __n = *len;
    if (8 < __n) {
        __n = 9;
    }
    memcpy((void *)((int)ctx->priv + 0x3e), array, __n);
    *len = __n;
    return 0;
}

/**
 * @brief Получение имени платы.
 */
int edf_v4_board_name_get(edf_ctx_p ctx, char *array, size_t *len) {
    size_t __n = *len;
    if (8 < __n) {
        __n = 9;
    }
    memcpy(array, (void *)((int)ctx->priv + 0x3e), __n);
    *len = __n;
    return 0;
}

/**
 * @brief Установка заводского задания (factory job).
 */
int edf_v4_factory_job_set(edf_ctx_p ctx, char *array, size_t *len) {
    size_t __n = *len;
    if (0x17 < __n) {
        __n = 0x18;
    }
    memcpy((void *)((int)ctx->priv + 0x47), array, __n);
    *len = __n;
    return 0;
}

/**
 * @brief Получение заводского задания (factory job).
 */
int edf_v4_factory_job_get(edf_ctx_p ctx, char *array, size_t *len) {
    size_t __n = *len;
    if (0x17 < __n) {
        __n = 0x18;
    }
    memcpy(array, (void *)((int)ctx->priv + 0x47), __n);
    *len = __n;
    return 0;
}

/**
 * @brief Получение напряжения.
 */
int edf_v4_voltage_get(edf_ctx_p ctx) {
    return (uint)*(ushort *)((int)ctx->priv + 0x62);
}

/**
 * @brief Установка напряжения.
 */
int edf_v4_voltage_set(edf_ctx_p ctx, uint16_t val) {
    *(uint16_t *)((int)ctx->priv + 0x62) = val;
    return 0;
}

/**
 * @brief Получение частоты.
 */
int edf_v4_frequency_get(edf_ctx_p ctx) {
    return (uint)*(ushort *)((int)ctx->priv + 100);
}

/**
 * @brief Установка частоты.
 */
int edf_v4_frequency_set(edf_ctx_p ctx, uint16_t val) {
    *(uint16_t *)((int)ctx->priv + 100) = val;
    return 0;
}

/**
 * @brief Получение скорости хеширования (nonce rate).
 */
int edf_v4_nonce_rate_get(edf_ctx_p ctx) {
    return (uint)*(ushort *)((int)ctx->priv + 0x66);
}

/**
 * @brief Установка скорости хеширования (nonce rate).
 */
int edf_v4_nonce_rate_set(edf_ctx_p ctx, uint16_t val) {
    *(uint16_t *)((int)ctx->priv + 0x66) = val;
    return 0;
}

/**
 * @brief Получение температуры PCB (вход).
 */
int edf_v4_pcb_temp_in_get(edf_ctx_p ctx) {
    return (int)*(char *)((int)ctx->priv + 0x68);
}

/**
 * @brief Установка температуры PCB (вход).
 */
int edf_v4_pcb_temp_in_set(edf_ctx_p ctx, int8_t val) {
    *(int8_t *)((int)ctx->priv + 0x68) = val;
    return 0;
}

/**
 * @brief Получение температуры PCB (выход).
 */
int edf_v4_pcb_temp_out_get(edf_ctx_p ctx) {
    return (int)*(char *)((int)ctx->priv + 0x69);
}

/**
 * @brief Установка температуры PCB (выход).
 */
int edf_v4_pcb_temp_out_set(edf_ctx_p ctx, int8_t val) {
    *(int8_t *)((int)ctx->priv + 0x69) = val;
    return 0;
}

/**
 * @brief Получение версии теста.
 */
int edf_v4_test_version_get(edf_ctx_p ctx) {
    return (uint)*(byte *)((int)ctx->priv + 0x6a);
}

/**
 * @brief Установка версии теста.
 */
int edf_v4_test_version_set(edf_ctx_p ctx, uint8_t val) {
    *(uint8_t *)((int)ctx->priv + 0x6a) = val;
    return 0;
}

/**
 * @brief Получение стандарта теста.
 */
int edf_v4_test_standard_get(edf_ctx_p ctx) {
    return (uint)*(byte *)((int)ctx->priv + 0x6b);
}

/**
 * @brief Установка стандарта теста.
 */
int edf_v4_test_standard_set(edf_ctx_p ctx, uint8_t val) {
    *(uint8_t *)((int)ctx->priv + 0x6b) = val;
    return 0;
}

/**
 * @brief Дамп данных в лог.
 */
void edf_v4_dump_data(edf_ctx_p ctx) {
    byte *pbVar1 = (byte *)ctx->priv;
    char time_stamp[48];
    char tmp1[256];
    char tmp[100];

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : board_info_version = %d\n", "edf_v4_dump_data", (uint)*pbVar1);
    snprintf(tmp1, 0x100, "board_info_version = %d", (uint)*pbVar1);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : algorithm_and_key_version = 0x%x\n", "edf_v4_dump_data", (uint)pbVar1[1]);
    snprintf(tmp1, 0x100, "algorithm_and_key_version = 0x%x", (uint)pbVar1[1]);
    log_to_file(tmp1, time_stamp);

    memset(tmp, 0, 100);
    memcpy(tmp, pbVar1 + 2, 0x12);
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : board_sn = %s\n", "edf_v4_dump_data", tmp);
    snprintf(tmp1, 0x100, "board_sn = %s", tmp);
    log_to_file(tmp1, time_stamp);

    memset(tmp, 0, 100);
    memcpy(tmp, pbVar1 + 0x14, 3);
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : chip_die = %s\n", "edf_v4_dump_data", tmp);
    snprintf(tmp1, 0x100, "chip_die = %s", tmp);
    log_to_file(tmp1, time_stamp);

    memset(tmp, 0, 100);
    memcpy(tmp, pbVar1 + 0x17, 0xe);
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : chip_marking = %s\n", "edf_v4_dump_data", tmp);
    snprintf(tmp1, 0x100, "chip_marking = %s", tmp);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : chip_bin = %d\n", "edf_v4_dump_data", (uint)pbVar1[0x25]);
    snprintf(tmp1, 0x100, "chip_bin = %d", (uint)pbVar1[0x25]);
    log_to_file(tmp1, time_stamp);

    memset(tmp, 0, 100);
    memcpy(tmp, pbVar1 + 0x26, 10);
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : ft_version = %s\n", "edf_v4_dump_data", tmp);
    snprintf(tmp1, 0x100, "ft_version = %s", tmp);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : pcb_version = 0x%x\n", "edf_v4_dump_data", (uint)*(ushort *)(pbVar1 + 0x30));
    snprintf(tmp1, 0x100, "pcb_version = 0x%x", (uint)*(ushort *)(pbVar1 + 0x30));
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : bom_version = 0x%x\n", "edf_v4_dump_data", (uint)*(ushort *)(pbVar1 + 0x32));
    snprintf(tmp1, 0x100, "bom_version = 0x%x", (uint)*(ushort *)(pbVar1 + 0x32));
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : asic_sensor_type = 0x%x\n", "edf_v4_dump_data", (uint)pbVar1[0x34]);
    snprintf(tmp1, 0x100, "asic_sensor_type = 0x%x", (uint)pbVar1[0x34]);
    log_to_file(tmp1, time_stamp);

    for (int i = 0; i < 4; ++i) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : asic_sensor_addr[%d] = 0x%x\n", "edf_v4_dump_data", i, (uint)pbVar1[0x35 + i]);
        snprintf(tmp1, 0x100, "asic_sensor_addr[%d] = 0x%x", i, (uint)pbVar1[0x35 + i]);
        log_to_file(tmp1, time_stamp);
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : pic_sensor_type = 0x%x\n", "edf_v4_dump_data", (uint)pbVar1[0x39]);
    snprintf(tmp1, 0x100, "pic_sensor_type = 0x%x", (uint)pbVar1[0x39]);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : pic_sensor_addr = 0x%x\n", "edf_v4_dump_data", (uint)pbVar1[0x3a]);
    snprintf(tmp1, 0x100, "pic_sensor_addr = 0x%x", (uint)pbVar1[0x3a]);
    log_to_file(tmp1, time_stamp);

    memset(tmp, 0, 100);
    memcpy(tmp, pbVar1 + 0x3b, 3);
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : chip_tech = %s\n", "edf_v4_dump_data", tmp);
    snprintf(tmp1, 0x100, "chip_tech = %s", tmp);
    log_to_file(tmp1, time_stamp);

    memset(tmp, 0, 100);
    memcpy(tmp, pbVar1 + 0x3e, 9);
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : board_name = %s\n", "edf_v4_dump_data", tmp);
    snprintf(tmp1, 0x100, "board_name = %s", tmp);
    log_to_file(tmp1, time_stamp);

    memset(tmp, 0, 100);
    memcpy(tmp, pbVar1 + 0x47, 0x18);
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : factory_job = %s\n", "edf_v4_dump_data", tmp);
    snprintf(tmp1, 0x100, "factory_job = %s", tmp);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : board_info_crc = 0x%x\n", "edf_v4_dump_data", (uint)pbVar1[0x61]);
    snprintf(tmp1, 0x100, "board_info_crc = 0x%x", (uint)pbVar1[0x61]);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : voltage = %d\n", "edf_v4_dump_data", (uint)*(ushort *)(pbVar1 + 0x62));
    snprintf(tmp1, 0x100, "voltage = %d", (uint)*(ushort *)(pbVar1 + 0x62));
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : frequency = %d\n", "edf_v4_dump_data", (uint)*(ushort *)(pbVar1 + 100));
    snprintf(tmp1, 0x100, "frequency = %d", (uint)*(ushort *)(pbVar1 + 100));
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : nonce_rate = %d\n", "edf_v4_dump_data", (uint)*(ushort *)(pbVar1 + 0x66));
    snprintf(tmp1, 0x100, "nonce_rate = %d", (uint)*(ushort *)(pbVar1 + 0x66));
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : pcb_temp_in = %d\n", "edf_v4_dump_data", (int)(char)pbVar1[0x68]);
    snprintf(tmp1, 0x100, "pcb_temp_in = %d", (int)(char)pbVar1[0x68]);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : pcb_temp_out = %d\n", "edf_v4_dump_data", (int)(char)pbVar1[0x69]);
    snprintf(tmp1, 0x100, "pcb_temp_out = %d", (int)(char)pbVar1[0x69]);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : test_version = %d\n", "edf_v4_dump_data", (uint)pbVar1[0x6a]);
    snprintf(tmp1, 0x100, "test_version = %d", (uint)pbVar1[0x6a]);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : test_standard = %d\n", "edf_v4_dump_data", (uint)pbVar1[0x6b]);
    snprintf(tmp1, 0x100, "test_standard = %d", (uint)pbVar1[0x6b]);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : param_info_crc = 0x%x\n", "edf_v4_dump_data", (uint)pbVar1[0x71]);
    snprintf(tmp1, 0x100, "param_info_crc = 0x%x", (uint)pbVar1[0x71]);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : pt1_result = %d\n", "edf_v4_dump_data", (uint)pbVar1[0x5f]);
    snprintf(tmp1, 0x100, "pt1_result = %d", (uint)pbVar1[0x5f]);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : pt1_count = %d\n", "edf_v4_dump_data", (uint)pbVar1[0x60]);
    snprintf(tmp1, 0x100, "pt1_count = %d", (uint)pbVar1[0x60]);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : pt2_result = %d\n", "edf_v4_dump_data", (uint)pbVar1[0x6c]);
    snprintf(tmp1, 0x100, "pt2_result = %d", (uint)pbVar1[0x6c]);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : pt2_count = %d\n", "edf_v4_dump_data", (uint)pbVar1[0x6d]);
    snprintf(tmp1, 0x100, "pt2_count = %d", (uint)pbVar1[0x6d]);
    log_to_file(tmp1, time_stamp);
}

/**
 * @brief Освобождение ресурсов контекста V4.
 */
void edf_v4_destroy(eeprom_data_format_t *ctx) {
    char time_stamp[48];
    char tmp1[256];

    if ((ctx == nullptr) || (ctx->priv == nullptr)) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : edf_v4_destroy: input arg wrong\n", "edf_v4_destroy");
        strncpy(tmp1, "edf_v4_destroy: input arg wrong", 0x20);
        log_to_file(tmp1, time_stamp);
    } else {
        free(ctx->priv);
        free(ctx);
    }
}

/**
 * @brief Инициализация контекста V4.
 */
eeprom_data_format_t * edf_v4_init(void) {
    eeprom_data_format_t *__dest = (eeprom_data_format_t *)malloc(0x128);
    if (__dest == nullptr) {
        return nullptr;
    }

    uint8_t *__s = (uint8_t *)malloc(0x72);
    if (__s == nullptr) {
        free(__dest);
        return nullptr;
    }

    memset(__s, 0, 0x72);
    *__s = 4;       // Version
    __s[1] = 0x11;  // Algorithm/Key version default

    // Копирование таблицы операций (предполагается, что edf_v4_ops определена глобально или здесь)
    // В псевдокоде это memcpy(__dest, &edf_v4_ops, 0x128);
    // Так как структура ops не предоставлена полностью, мы предполагаем, что она заполняется externally 
    // или через статическую инициализацию в заголовке. Для данного файла оставим заглушку или предположим, 
    // что указатели на функции уже присвоены конструктором или внешним кодом.
    
    __dest->priv = __s;
    return __dest;
}

/**
 * @brief Загрузка данных из EEPROM (формат V4).
 */
int edf_v4_load(edf_ctx_p ctx, uint8_t *src, size_t len, int flag) {
    byte bVar1;
    bool _Var2;
    uint8_t uVar3;
    uint uVar4;
    int iVar5;
    uint32_t uVar6;
    void *pvVar7;
    uint32_t *puVar8;
    int flag_local = flag;
    size_t len_local = len;
    uint8_t *src_local = src;
    edf_ctx_p ctx_local = ctx;
    char time_stamp[48];
    char tmp1[256];
    uint8_t buf[256];
    uint8_t param_info_crc;
    uint8_t board_info_crc;
    uint8_t *src_p;
    uint8_t *dst_p;
    edf_v4_priv *priv_ctx;
    char *errmsg;
    int ret = 0;

    pvVar7 = ctx->priv;
    bVar1 = *src;
    uVar4 = edf_v4_version_get(ctx);
    
    if (bVar1 != uVar4) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        bVar1 = *src;
        iVar5 = edf_v4_version_get(ctx);
        printf("%s : version invalid %d!=%d\n", "edf_v4_load", (uint)bVar1, iVar5);
        bVar1 = *src;
        iVar5 = edf_v4_version_get(ctx);
        snprintf(tmp1, 0x100, "version invalid %d!=%d", (uint)bVar1, iVar5);
        log_to_file(tmp1, time_stamp);
        return -1;
    }

    uVar6 = edf_v4_capability(ctx);
    if ((flag & uVar6) == 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : flag invalid\n", "edf_v4_load");
        strncpy(tmp1, "flag invalid", 0xc);
        tmp1[0xc] = '\0';
        log_to_file(tmp1, time_stamp);
        return -1;
    }

    if (((flag & 2U) != 0) && (len < 0x62)) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : len invalid\n", "edf_v4_load");
        strncpy(tmp1, "len invalid", 0xc);
        log_to_file(tmp1, time_stamp);
        return -1;
    }

    if (((flag & 4U) != 0) && (len < 0x72)) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : len invalid\n", "edf_v4_load");
        strncpy(tmp1, "len invalid", 0xc);
        log_to_file(tmp1, time_stamp);
        return -1;
    }

    if (0x100 < len) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : len too large, should less than 256\n", "edf_v4_load");
        strncpy(tmp1, "len too large, should less than 256", 0x24);
        log_to_file(tmp1, time_stamp);
    }

    *(uint8_t *)((int)pvVar7 + 1) = src[1];
    memcpy(buf, src, len);

    if ((flag & 2U) != 0) {
        puVar8 = (uint32_t *)(buf + 2);
        memcpy(puVar8, src + 2, 0x60);
        _Var2 = data_dec(puVar8, 0x60, (byte)((int)pvVar7 + 1) >> 4, (byte *)((int)pvVar7 + 1) & 0xf);
        if (!_Var2) {
            errmsg = "DEC 1ST REGION";
            ret = -1;
            goto LAB_ERROR;
        }
        uVar3 = CRC5_long(buf, 0x308); // 0x61 * 8 bits? No, 0x308 is likely bit count or specific len
        if (uVar3 != buf[0x61]) {
            errmsg = "CRC 1ST REGION";
            ret = -1;
            goto LAB_ERROR;
        }
        memcpy((void *)((int)pvVar7 + 2), puVar8, 0x60);
        if (*(char *)((int)pvVar7 + 0x5f) != '\x01') {
            errmsg = "PT1 test NOT pass, redo it";
            ret = -1;
            goto LAB_ERROR;
        }
        ret = 2;
    }

    if ((flag & 4U) == 0) {
        return ret;
    }

    puVar8 = (uint32_t *)(buf + 0x62);
    memcpy(puVar8, src + 0x62, 0x10);
    _Var2 = data_dec(puVar8, 0x10, (byte)((int)pvVar7 + 1) >> 4, (byte *)((int)pvVar7 + 1) & 0xf);
    if (_Var2) {
        uVar3 = CRC5_long(buf + 0x62, 0x78);
        if (uVar3 == buf[0x71]) {
            memcpy((void *)((int)pvVar7 + 0x62), puVar8, 0x10);
            if (*(char *)((int)pvVar7 + 0x6c) == '\x01') {
                return ret | 4;
            }
            errmsg = "PT2 test NOT pass, redo it";
        } else {
            errmsg = "CRC 2ND REGION";
        }
    } else {
        errmsg = "DEC 2ND REGION";
    }

LAB_ERROR:
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : EEPROM error: %s\n", "edf_v4_load", errmsg);
    snprintf(tmp1, 0x100, "EEPROM error: %s", errmsg);
    log_to_file(tmp1, time_stamp);
    return ret;
}

/**
 * @brief Кодирование первого региона EEPROM.
 */
int edf_v4_region_1_encode(edf_ctx_p ctx, char *dst, size_t *len, size_t *offset) {
    uint8_t uVar1;
    bool _Var2;
    uint8_t *pdata;
    int iVar3;
    size_t *offset_local = offset;
    size_t *len_local = len;
    char *dst_local = dst;
    edf_ctx_p ctx_local = ctx;
    char time_stamp[48];
    char tmp1[256];
    char src[256];
    edf_v4_priv *priv_ctx;

    pdata = (uint8_t *)ctx->priv;
    if (*len < 0x62) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : INPUT too short %u <%u\n", "edf_v4_region_1_encode", *len, 0x62);
        snprintf(tmp1, 0x100, "INPUT too short %u <%u", *len, 0x62);
        log_to_file(tmp1, time_stamp);
        iVar3 = -1;
    } else {
        // Логирование параметров
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : BOARD_CRC_DATA_LEN=%d\n", "edf_v4_region_1_encode", 0x61);
        snprintf(tmp1, 0x100, "BOARD_CRC_DATA_LEN=%d", 0x61);
        log_to_file(tmp1, time_stamp);
        
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : BOARD_ENC_DATA_START=%d\n", "edf_v4_region_1_encode", 2);
        snprintf(tmp1, 0x100, "BOARD_ENC_DATA_START=%d", 2);
        log_to_file(tmp1, time_stamp);

        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : BOARD_ENC_DATA_LEN=%d\n", "edf_v4_region_1_encode", 0x60);
        snprintf(tmp1, 0x100, "BOARD_ENC_DATA_LEN=%d", 0x60);
        log_to_file(tmp1, time_stamp);

        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : PARAM_CRC_DATA_START=%d\n", "edf_v4_region_1_encode", 0x62);
        snprintf(tmp1, 0x100, "PARAM_CRC_DATA_START=%d", 0x62);
        log_to_file(tmp1, time_stamp);

        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : PARAM_CRC_DATA_LEN=%d\n", "edf_v4_region_1_encode", 0xf);
        snprintf(tmp1, 0x100, "PARAM_CRC_DATA_LEN=%d", 0xf);
        log_to_file(tmp1, time_stamp);

        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : PARAM_ENC_DATA_START=%d\n", "edf_v4_region_1_encode", 0x62);
        snprintf(tmp1, 0x100, "PARAM_ENC_DATA_START=%d", 0x62);
        log_to_file(tmp1, time_stamp);

        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : PARAM_ENC_DATA_LEN=%d\n", "edf_v4_region_1_encode", 0x10);
        snprintf(tmp1, 0x100, "PARAM_ENC_DATA_LEN=%d", 0x10);
        log_to_file(tmp1, time_stamp);

        uVar1 = CRC5_long(pdata, 0x308);
        pdata[0x61] = uVar1;
        memcpy(src, pdata + 2, 0x60);
        _Var2 = data_enc((uint32_t *)src, 0x60, pdata[1] >> 4, pdata[1] & 0xf);
        if (_Var2) {
            memcpy(dst, pdata, 2);
            memcpy(dst + 2, src, 0x60);
            *len = 0x62;
            *offset = 0;
            iVar3 = 0;
        } else {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : %s: data_enc error\n", "edf_v4_region_1_encode", "edf_v4_region_1_encode");
            snprintf(tmp1, 0x100, "%s: data_enc error", "edf_v4_region_1_encode");
            log_to_file(tmp1, time_stamp);
            iVar3 = -1;
        }
    }
    return iVar3;
}

/**
 * @brief Кодирование второго региона EEPROM.
 */
int edf_v4_region_2_encode(edf_ctx_p ctx, char *dst, size_t *len, size_t *offset) {
    uint8_t uVar1;
    bool _Var2;
    void *pvVar3;
    int iVar4;
    size_t *offset_local = offset;
    size_t *len_local = len;
    char *dst_local = dst;
    edf_ctx_p ctx_local = ctx;
    char time_stamp[48];
    char tmp1[256];
    edf_v4_priv *priv_ctx;

    pvVar3 = ctx->priv;
    if (*len < 0x10) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : INPUT too short %u <%u\n", "edf_v4_region_2_encode", *len, 0x10);
        snprintf(tmp1, 0x100, "INPUT too short %u <%u", *len, 0x10);
        log_to_file(tmp1, time_stamp);
        iVar4 = -1;
    } else {
        uVar1 = CRC5_long((uint8_t *)((int)pvVar3 + 0x62), 0x78);
        *(uint8_t *)((int)pvVar3 + 0x71) = uVar1;
        memcpy(dst, (void *)((int)pvVar3 + 0x62), 0x10);
        _Var2 = data_enc((uint32_t *)dst, 0x10, (byte *)((int)pvVar3 + 1) >> 4, *(byte *)((int)pvVar3 + 1) & 0xf);
        if (_Var2) {
            *len = 0x10;
            *offset = 0x62;
            iVar4 = 0;
        } else {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : %s: data_enc error\n", "edf_v4_region_2_encode", "edf_v4_region_2_encode");
            snprintf(tmp1, 0x100, "%s: data_enc error", "edf_v4_region_2_encode");
            log_to_file(tmp1, time_stamp);
            iVar4 = -1;
        }
    }
    return iVar4;
}