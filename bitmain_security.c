#include <cstdint>
#include <cstdio>
#include <cstring>

// --- Типы данных и Константы ---

// Ключи для XXTEA (извлечены из псевдокода)
static const uint32_t xxtea_key[4][4] = {
    {0x6A696C69, 0x6E69616E, 0x78757961, 0x6F78696E}, // Key 0
    {0x7451ED7C, 0x7B5CD872, 0x174FE079, 0x0A15E4XX}, // Key 1 (примерная структура, точные значения зависят от памяти)
    {0x756F687A, 0x6F61687A, 0x75686964, 0x6B676EXX}, // Key 2
    {0x75696C65, 0x796E696D, 0x67676E61, 0x676E61XX}  // Key 3
};

// Внешние функции XXTEA
extern void xxtea_encode(uint32_t *v, uint32_t n, const uint32_t *k);
extern void xxtea_decode(uint32_t *v, uint32_t n, const uint32_t *k);

// Заглушки для логирования
extern void print_time_stamp(char *buf, int len);
extern void log_to_file(char *msg, char *time);
extern void _applog(int level, const char* msg, bool newline);

// --- Функции ---

/**
 * @brief Шифрование данных.
 * 
 * Логика:
 * 1. Проверяет версию ключа (должна быть < 4).
 * 2. Проверяет тип алгоритма (поддерживается только 0x01 - XXTEA).
 * 3. Проверяет длину данных (должна быть кратна 8 байтам для XXTEA).
 * 4. Вызывает xxtea_encode.
 */
bool data_enc(uint32_t *buf, uint32_t len, uint8_t algorithm_type, uint8_t key_version) {
    bool ret;
    char time_stamp[48];
    char tmp1[256];

    if (key_version < 4) {
        if (algorithm_type == 0x01) {
            // Проверка выравнивания по 8 байт (len & 7 == 0)
            if ((len & 7) == 0) {
                // Вызов xxtea_encode
                // n = len / 4 (так как xxtea работает с uint32_t)
                xxtea_encode(buf, len >> 2, xxtea_key[key_version]);
                ret = true;
            } else {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : xxtea input data length %% 8 must be 0, but now is %d\n", "data_enc", len & 7);
                snprintf(tmp1, 0x100, "xxtea input data length %% 8 must be 0, but now is %d", len & 7);
                log_to_file(tmp1, time_stamp);
                ret = false;
            }
        } else {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : algorithm_type is %d, but not support it\n", "data_enc", (uint)algorithm_type);
            snprintf(tmp1, 0x100, "algorithm_type is %d, but not support it", (uint)algorithm_type);
            log_to_file(tmp1, time_stamp);
            ret = false;
        }
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : key_version %u not support\n", "data_enc", (uint)key_version);
        snprintf(tmp1, 0x100, "key_version %u not support", (uint)key_version);
        log_to_file(tmp1, time_stamp);
        ret = false;
    }
    return ret;
}

/**
 * @brief Дешифрование данных.
 * 
 * Логика аналогична data_enc, но вызывает xxtea_decode.
 */
bool data_dec(uint32_t *buf, uint32_t len, uint8_t algorithm_type, uint8_t key_version) {
    bool ret;
    char time_stamp[48];
    char tmp1[256];

    if (key_version < 4) {
        if (algorithm_type == 0x01) {
            // Проверка выравнивания по 8 байт
            if ((len & 7) == 0) {
                // Вызов xxtea_decode
                xxtea_decode(buf, len >> 2, xxtea_key[key_version]);
                ret = true;
            } else {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : xxtea input data length %% 8 must be 0, but now is %d\n", "data_dec", len & 7);
                snprintf(tmp1, 0x100, "xxtea input data length %% 8 must be 0, but now is %d", len & 7);
                log_to_file(tmp1, time_stamp);
                ret = false;
            }
        } else {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : algorithm_type is %d, but not support it\n", "data_dec", (uint)algorithm_type);
            snprintf(tmp1, 0x100, "algorithm_type is %d, but not support it", (uint)algorithm_type);
            log_to_file(tmp1, time_stamp);
            ret = false;
        }
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : key_version %u not support\n", "data_dec", (uint)key_version);
        snprintf(tmp1, 0x100, "key_version %u not support", (uint)key_version);
        log_to_file(tmp1, time_stamp);
        ret = false;
    }
    return ret;
}