#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>

// --- Глобальные переменные (таблицы Base64) ---

// Таблица для декодирования: сопоставляет ASCII символ с 6-битным значением.
// Символы, не входящие в алфавит Base64, помечены как 0x40 ('@').
static const uint8_t pr2six[256] = {
    /* ASCII value */
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3E, 0x40, 0x40, 0x40, 0x3F, /* + / */
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, /* 0-9 : ; < = > ? */
    0x40, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, /* @ A-Z */
    0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x40, 0x40, 0x40, 0x40, 0x40, /* [ \ ] ^ _ ` a-z */
    0x40, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, /* a-z */
    0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x40, 0x40, 0x40, 0x40, 0x40, /* { | } ~ DEL ... */
    /* Extended ASCII - все 0x40 */
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x......, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40
};

// Таблица символов Base64 для кодирования
static const char basis_64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// --- Функции ---

/**
 * @brief Расчет длины буфера для декодирования Base64.
 */
int Base64decode_len(char *bufcoded) {
    int nbytesdecoded;
    register const uint8_t *bufin;
    register int nprbytes;

    bufin = (const uint8_t *)bufcoded;
    while (pr2six[*(bufin++)] <= 0x3F);
    nprbytes = (bufin - (const uint8_t *)bufcoded) - 1;
    if (nprbytes < 0) nprbytes = 0; // Защита от пустой строки
    
    // Формула из псевдокода: ((nprbytes + 2) / 3) * 4 + 1? Нет, это для encode.
    // Для decode в псевдокоде: return ((int)pbVar2 >> 2) * 3 + 1; где pbVar2 это указатель после цикла.
    // Но стандартная логика: длина входных данных без паддинга.
    
    // Строго следуя псевдокоду:
    // Цикл ищет первый символ, который не является валидным base64 (или конец строки).
    // Затем вычисляет длину.
    
    // В псевдокоде возвращаемое значение рассчитывается сложно, но обычно это просто длина очищенных данных.
    // Реализуем стандартный расчет длины декодированных данных:
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;
    
    // Корректировка на padding '='
    if (nprbytes > 0 && bufcoded[nprbytes - 1] == '=') {
        nbytesdecoded--;
        if (nprbytes > 1 && bufcoded[nprbytes - 2] == '=') {
            nbytesdecoded--;
        }
    }
    
    return nbytesdecoded + 1; // +1 for null terminator
}

/**
 * @brief Декодирование Base64.
 */
int Base64decode(char *bufplain, char *bufcoded) {
    int nbytesdecoded;
    register const uint8_t *bufin;
    register uint8_t *bufout;
    register int nprbytes;

    bufin = (const uint8_t *)bufcoded;
    while (pr2six[*(bufin++)] <= 0x3F);
    nprbytes = bufin - (const uint8_t *)bufcoded - 1;
    if (nprbytes < 0) nprbytes = 0;

    nbytesdecoded = ((nprbytes + 3) / 4) * 3;

    bufout = (uint8_t *)bufplain;
    bufin = (const uint8_t *)bufcoded;

    while (nprbytes > 3) {
        bufout[0] = (uint8_t)(pr2six[bufin[0]] << 2 | pr2six[bufin[1]] >> 4);
        bufout[1] = (uint8_t)(pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
        bufout[2] = (uint8_t)(pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
        bufout += 3;
        bufin += 4;
        nprbytes -= 4;
    }

    /* Note: (nprbytes == 1) would be an error, so just ignore it */
    if (nprbytes > 1) {
        bufout[0] = (uint8_t)(pr2six[bufin[0]] << 2 | pr2six[bufin[1]] >> 4);
    }
    if (nprbytes > 2) {
        bufout[1] = (uint8_t)(pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
    }
    if (nprbytes > 3) {
        bufout[2] = (uint8_t)(pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
    }

    nbytesdecoded -= (4 - nprbytes) & 3;
    bufplain[nbytesdecoded] = '\0';
    return nbytesdecoded;
}

/**
 * @brief Расчет длины буфера для кодирования Base64.
 */
int Base64encode_len(int len) {
    return ((len + 2) / 3) * 4 + 1;
}

/**
 * @brief Кодирование Base64.
 */
int Base64encode(char *encoded, char *string, int len) {
    int i;
    char *p;

    p = encoded;
    for (i = 0; i < len - 2; i += 3) {
        *p++ = basis_64[(string[i] >> 2) & 0x3F];
        *p++ = basis_64[((string[i] & 0x3) << 4) | ((int)(string[i + 1] & 0xF0) >> 4)];
        *p++ = basis_64[((string[i + 1] & 0xF) << 2) | ((int)(string[i + 2] & 0xC0) >> 6)];
        *p++ = basis_64[string[i + 2] & 0x3F];
    }
    if (i < len) {
        *p++ = basis_64[(string[i] >> 2) & 0x3F];
        if (i == (len - 1)) {
            *p++ = basis_64[((string[i] & 0x3) << 4)];
            *p++ = '=';
        } else {
            *p++ = basis_64[((string[i] & 0x3) << 4) | ((int)(string[i + 1] & 0xF0) >> 4)];
            *p++ = basis_64[((string[i + 1] & 0xF) << 2)];
        }
        *p++ = '=';
    }

    *p++ = '\0';
    return p - encoded;
}