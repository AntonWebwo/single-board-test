#include <cstdint>
#include <cstddef>
#include <cstring>

// --- Внешние зависимости (заглушки для функций SHA-2) ---
// В реальном проекте они должны быть подключены через заголовочный файл sha2.h или аналогичный

struct sha2_context {
    uint32_t total[2];
    uint32_t state[8];
    unsigned char buffer[64];
};

extern void sha2_starts(sha2_context *ctx);
extern void sha2_update(sha2_context *ctx, const unsigned char *input, size_t length);
extern void sha2_finish(sha2_context *ctx, unsigned char output[32]);

// --- Функции ---

/**
 * @brief Вычисление HMAC-SHA256.
 * 
 * Логика:
 * 1. Проверяет входные указатели на NULL.
 * 2. Если ключ длиннее 64 байт, хеширует его через SHA-256.
 * 3. Создает внутренний ключ (kx), XOR'я его с 0x36 (ipad).
 * 4. Заполняет остаток kx символами '6' (0x36).
 * 5. Вычисляет H((K ^ ipad) || data).
 * 6. Создает внешний ключ, XOR'я оригинальный ключ с 0x5C (opad).
 * 7. Заполняет остаток kx символами '\' (0x5C).
 * 8. Вычисляет H((K ^ opad) || InnerHash).
 */
void hmac_sha256(uint8_t *out, uint8_t *data, size_t data_len, uint8_t *key, size_t key_len) {
    uint8_t *key_local = key;
    size_t data_len_local = data_len;
    uint8_t *data_local = data;
    uint8_t *out_local = out;
    
    // Временные буферы
    uint8_t kx[64];      // Key XORed with pad
    uint8_t kh[32];      // Hashed key (if key > 64 bytes)
    
    sha2_context ss;     // SHA-2 context
    size_t i;
    size_t i_1;
    size_t i_2;
    size_t i_3;

    // Проверка на NULL
    if ((out != nullptr) && (data != nullptr) && (key != nullptr)) {
        
        // Если ключ больше размера блока SHA-256 (64 байта), хешируем его
        if (0x40 < key_len) {
            sha2_starts(&ss);
            sha2_update(&ss, key, key_len);
            sha2_finish(&ss, kh);
            
            key_len = 0x20; // Длина хеша SHA-256
            key_local = kh; // Используем хеш как новый ключ
        }

        // Шаг 1: Создание внутреннего ключа (ipad = 0x36)
        for (i = 0; i < key_len; i = i + 1) {
            kx[i] = key_local[i] ^ 0x36;
        }
        // Заполнение остатка блока ipad'ом
        for (i_1 = key_len; i_1 < 0x40; i_1 = i_1 + 1) {
            kx[i_1] = '6'; // ASCII '6' is 0x36
        }

        // Шаг 2: H((K ^ ipad) || data)
        sha2_starts(&ss);
        sha2_update(&ss, kx, 0x40);
        sha2_update(&ss, data, data_len);
        sha2_finish(&ss, out); // Результат сохраняется во out (временно)

        // Шаг 3: Создание внешнего ключа (opad = 0x5C)
        for (i_2 = 0; i_2 < key_len; i_2 = i_2 + 1) {
            kx[i_2] = key_local[i_2] ^ 0x5C;
        }
        // Заполнение остатка блока opad'ом
        for (i_3 = key_len; i_3 < 0x40; i_3 = i_3 + 1) {
            kx[i_3] = '\\'; // ASCII '\' is 0x5C
        }

        // Шаг 4: H((K ^ opad) || InnerHash)
        sha2_starts(&ss);
        sha2_update(&ss, kx, 0x40);
        sha2_update(&ss, out, 0x20); // Используем предыдущий результат как данные
        sha2_finish(&ss, out);       // Финальный результат в out
    }
    
    return;
}