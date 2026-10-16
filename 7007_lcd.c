#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

// --- Глобальные переменные ---

static int32_t lcd_fd = 0;
static int32_t lcd_inited = 0;
static pthread_mutex_t lcd_mutex = PTHREAD_MUTEX_INITIALIZER;

// Буфер вывода: 4 строки по 16 символов (всего 64 байта)
static int8_t lcd_output[4][16] = {};

// --- Заглушки для внешних зависимостей ---
extern void _applog(int level, const char* msg, bool newline);

// --- Функции ---

/**
 * @brief Инициализация LCD дисплея.
 * Открывает устройство по указанному пути.
 */
int32_t lcd_init(int8_t* path2dev) {
    if (lcd_inited != 0) {
        return 0;
    }

    if (!path2dev) {
        char tmp42[2048] = {0};
        snprintf(tmp42, sizeof(tmp42), "bad param\n");
        _applog(0, tmp42, false);
        return -3;
    }

    // O_RDWR | O_NONBLOCK (0x802)
    lcd_fd = open(reinterpret_cast<const char*>(path2dev), O_RDWR | O_NONBLOCK);
    
    if (lcd_fd < 0) {
        char tmp42[2048] = {0};
        snprintf(tmp42, sizeof(tmp42), "open lcd failed!!!\n");
        _applog(0, tmp42, false);
        return -1;
    }

    lcd_inited = 1;
    return 0;
}

/**
 * @brief Запись данных на LCD.
 * 
 * Логика:
 * 1. Проверяет параметры (ctx должен совпадать с lcd_fd, line < 4, size <= 64).
 * 2. Копирует данные во внутренний буфер lcd_output, распределяя их по строкам.
 * 3. Записывает весь буфер (64 байта) в устройство.
 */
int32_t lcd_write(int32_t ctx, int8_t line, int8_t* data, uint32_t size) {
    if (lcd_inited == 0) {
        return -2;
    }

    // Проверка контекста и параметров
    if (lcd_fd != ctx || line > 3 || size > 64) {
        char tmp42[2048] = {0};
        snprintf(tmp42, sizeof(tmp42), "bad param\n");
        _applog(1, tmp42, false);
        return -3;
    }

    int32_t lock_res = pthread_mutex_lock(&lcd_mutex);
    if (lock_res != 0) {
        char tmp42[2048] = {0};
        snprintf(tmp42, sizeof(tmp42), "failed to lcd lock\n");
        _applog(0, tmp42, false);
        return -4;
    }

    int32_t offset = 0;
    int8_t current_line = line;
    uint32_t remaining_size = size;

    // Цикл распределения данных по строкам дисплея
    while (remaining_size > 0 && current_line < 4) {
        // Определяем размер куска для текущей строки (максимум 16 байт)
        uint32_t chunk_size = remaining_size;
        if (chunk_size > 16) {
            chunk_size = 16;
        }

        // Копируем данные в соответствующую строку буфера
        memcpy(lcd_output[current_line], data + offset, chunk_size);

        offset += chunk_size;
        remaining_size -= chunk_size;
        current_line++;
    }

    // Запись всего буфера (64 байта) в устройство
    write(lcd_fd, lcd_output, 64);

    pthread_mutex_unlock(&lcd_mutex);
    return 0;
}

/**
 * @brief Очистка экрана LCD.
 * Заполняет буфер пробелами и записывает его.
 */
int32_t lcd_clear(int32_t ctx) {
    if (lcd_inited == 0) {
        return -2;
    }

    if (lcd_fd != ctx) {
        char tmp42[2048] = {0};
        snprintf(tmp42, sizeof(tmp42), "bad param\n");
        _applog(1, tmp42, false);
        return -3;
    }

    int32_t lock_res = pthread_mutex_lock(&lcd_mutex);
    if (lock_res != 0) {
        char tmp42[2048] = {0};
        snprintf(tmp42, sizeof(tmp42), "failed to lcd lock\n");
        _applog(0, tmp42, false);
        return -4;
    }

    // Заполняем буфер пробелами (0x20)
    memset(lcd_output, 0x20, 64);

    // Возвращаем курсор в начало файла
    lseek(lcd_fd, 0, SEEK_SET);
    
    // Записываем очищенный буфер
    write(lcd_fd, lcd_output, 64);

    pthread_mutex_unlock(&lcd_mutex);
    return 0;
}

/**
 * @brief Заглушка для flush.
 */
int32_t lcd_flush(int32_t ctx) {
    (void)ctx; // Подавление предупреждения
    return 0;
}

/**
 * @brief Деинициализация LCD.
 * Закрывает файловый дескриптор.
 */
int32_t lcd_uninit(int32_t ctx) {
    if (lcd_inited == 0) {
        return 0;
    }

    int32_t close_res = close(ctx);
    int32_t result = 0;

    if (close_res != 0) {
        char tmp42[2048] = {0};
        snprintf(tmp42, sizeof(tmp42), "lcd close failed!!!\n");
        _applog(0, tmp42, false);
        result = -1;
    }

    lcd_inited = 0;
    return result;
}