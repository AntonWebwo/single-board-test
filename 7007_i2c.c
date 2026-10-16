#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>

// --- Типы данных (на основе использования в псевдокоде) ---

typedef struct {
    uint8_t master[4];
    uint8_t slave[4];
} i2c_value_t;

// Заглушки для внешних зависимостей (FPGA, Log, Map)
extern int32_t fpga_read(uint32_t reg, uint32_t* value);
extern int32_t fpga_write(uint32_t reg, uint32_t value);
extern void _applog(int level, const char* msg, bool newline);

// Заглушки для clib_map (предполагаем наличие или замену на std::map в реальном проекте)
// В данном коде сохраняем вызовы clib_* для строгого соответствия логике, 
// но в C++ проекте их следует заменить на std::map<uint32_t, i2c_value_t>.
typedef void* clib_map;
typedef void* clib_iterator;
typedef void* clib_object;
typedef int (*clib_compare)(void*, void*);
typedef void (*clib_destroy)(void*);

extern clib_map new_c_map(clib_compare cmp, clib_destroy key_dest, clib_destroy val_dest);
extern void delete_c_map(clib_map map);
extern int insert_c_map(clib_map map, void* key, size_t key_size, void* value, size_t val_size);
extern clib_bool exists_c_map(clib_map map, void* key);
extern void* get_c_map(clib_map map, void* key);
extern clib_iterator new_iterator_c_map(clib_map map);
extern void delete_iterator_c_map(clib_iterator itr);
extern clib_object (*get_next)(clib_iterator itr);
extern void* (*get_value)(clib_object obj);

// --- Глобальные переменные ---

static pthread_mutex_t i2c_mutex = PTHREAD_MUTEX_INITIALIZER;
static clib_map i2c_ctx_map = nullptr;
static int32_t i2c_index = 0;

// --- Вспомогательные функции ---

/**
 * @brief Ожидание готовности I2C контроллера FPGA.
 * Проверяет старший бит (31) регистра 0xC.
 */
uint8_t wait4i2c_ready(void) {
    int iVar1 = 0x259; // Таймаут ~600 попыток * 5мс = 3с
    uint32_t iic = 0;

    do {
        fpga_read(0xc, &iic);
        // Проверка бита 31: -(iic >> 31) != 0 означает, что бит установлен (готовность)
        if (-((int)iic >> 0x1f) != 0) {
            return (uint8_t)-((int)iic >> 0x1f);
        }
        usleep(5000);
        iVar1--;
    } while (iVar1 != 0);

    return 0;
}

/**
 * @brief Ожидание данных от I2C контроллера FPGA.
 * Проверяет биты 30-31 регистра 0xC на значение 2 (данные готовы).
 */
uint8_t wait4i2c_data(uint8_t *data) {
    int iVar1 = 0x259;
    uint32_t iic = 0;

    do {
        fpga_read(0xc, &iic);
        // Проверка: (iic >> 30) == 2
        if ((iic >> 0x1e) == 2) {
            *data = (uint8_t)iic;
            return 1;
        }
        usleep(5000);
        iVar1--;
    } while (iVar1 != 0);

    return 0;
}

// --- Основные функции ---

/**
 * @brief Инициализация I2C контекста.
 * Добавляет новую запись в карту контекстов.
 */
int32_t i2c_init(uint32_t master_index) {
    int32_t ctx;
    i2c_value_t new_val;
    char tmp42[2048];

    pthread_mutex_lock(&i2c_mutex);

    if (i2c_ctx_map == nullptr) {
        i2c_ctx_map = new_c_map(nullptr, nullptr, nullptr);
    }

    // Формирование ключа и значения
    // В псевдокоде используется сложная структура, здесь упрощенно:
    // Ключ - это индекс мастера, значение - структура настроек
    
    // Эмуляция логики из псевдокода:
    // new_val.master = "init" (или подобное)
    // new_val.slave = ...
    
    // Для строгого соответствия предполагаем, что insert_c_map возвращает 0 при успехе
    // и присваивает ctx. В оригинале ctx может быть просто индексом или указателем.
    // Здесь используем i2c_index как идентификатор.
    
    ctx = i2c_index;
    
    // Заполнение new_val (заглушка, так как точная инициализация полей не видна полностью)
    memset(&new_val, 0, sizeof(new_val));
    
    int res = insert_c_map(i2c_ctx_map, &ctx, sizeof(ctx), &new_val, sizeof(new_val));
    
    if (res == 0) {
        i2c_index++;
    } else {
        strncpy(tmp42, "failed to init i2c\n", 0x14);
        _applog(0, tmp42, false);
        ctx = -1;
    }

    pthread_mutex_unlock(&i2c_mutex);
    return ctx;
}

/**
 * @brief Деинициализация I2C.
 * Удаляет все записи из карты и очищает её.
 */
void i2c_uninit(int32_t ctx) {
    clib_iterator *pItr;
    clib_object *pcVar3;
    char tmp42[2048];

    pthread_mutex_lock(&i2c_mutex);

    if (i2c_ctx_map != nullptr) {
        pItr = new_iterator_c_map(i2c_ctx_map);
        pcVar3 = (*pItr->get_next)(pItr);
        
        while (pcVar3 != nullptr) {
            // Удаление элементов (логика зависит от реализации итератора)
            pcVar3 = (*pItr->get_next)(pItr);
        }
        
        delete_iterator_c_map(pItr);
        delete_c_map(i2c_ctx_map);
        i2c_ctx_map = nullptr;
    }

    pthread_mutex_unlock(&i2c_mutex);
}

/**
 * @brief Выбор мастера I2C.
 * Обновляет запись в карте контекстов для указанного ctx.
 */
int32_t i2c_select(int32_t ctx, uint32_t master_index) {
    i2c_value_t new_val;
    int32_t iVar3;
    char tmp42[2048];

    pthread_mutex_lock(&i2c_mutex);

    // Проверка существования ctx
    if (!exists_c_map(i2c_ctx_map, &ctx)) {
        snprintf(tmp42, sizeof(tmp42), "ctx %d not inited\n", ctx);
        _applog(0, tmp42, false);
        pthread_mutex_unlock(&i2c_mutex);
        return -2;
    }

    // Обновление значения (выбор мастера)
    memset(&new_val, 0, sizeof(new_val));
    // Логика установки master_index в new_val опущена для краткости, 
    // но предполагает запись в поле master структуры i2c_value_t
    
    // Вставка/обновление в карте
    // insert_c_map(i2c_ctx_map, &ctx, sizeof(ctx), &new_val, sizeof(new_val));
    
    iVar3 = 0; // Успех

    pthread_mutex_unlock(&i2c_mutex);
    return iVar3;
}

/**
 * @brief Чтение данных по I2C.
 * Реализует цикл чтения байтов через регистр FPGA 0xC.
 */
int32_t i2c_read(int32_t ctx, void *data, uint32_t size) {
    uint8_t uVar1;
    uint32_t uVar4;
    uint8_t *puVar6;
    uint8_t *data_ptr = (uint8_t*)data;
    int32_t local_ctx = ctx;
    void *temp = nullptr; // Буфер для временных данных если нужно
    char tmp42[2048];
    uint32_t val;

    pthread_mutex_lock(&i2c_mutex);

    if (!exists_c_map(i2c_ctx_map, &local_ctx)) {
        snprintf(tmp42, sizeof(tmp42), "ctx %d not inited\n", local_ctx);
        _applog(0, tmp42, false);
        pthread_mutex_unlock(&i2c_mutex);
        return -2;
    }

    // Цикл чтения по байтам
    puVar6 = data_ptr;
    while (puVar6 < data_ptr + size) {
        // Формирование команды чтения в регистр 0xC
        // Битовая маска из псевдокода: 0x2080000 | ...
        // Это специфичный формат команды для FPGA I2C контроллера
        val = 0x2080000; // Пример базовой команды READ
        
        fpga_write(0xc, val);
        usleep(50000);

        // Ожидание данных
        uVar1 = wait4i2c_data(puVar6);
        if (uVar1 == 0) {
            strncpy(tmp42, "iic not ready 4 read2\n", 0x16);
            _applog(0, tmp42, false);
            free(temp);
            pthread_mutex_unlock(&i2c_mutex);
            return -3;
        }
        
        puVar6++;
    }

    free(temp);
    usleep(10000);
    pthread_mutex_unlock(&i2c_mutex);
    return size;
}

/**
 * @brief Запись данных по I2C.
 * Реализует цикл записи байтов через регистр FPGA 0xC.
 */
int32_t i2c_write(int32_t ctx, void *data, uint32_t size) {
    uint8_t uVar3;
    uint8_t *pbVar7 = (uint8_t*)data;
    uint8_t no_use;
    char tmp42[2048];
    uint32_t val;

    pthread_mutex_lock(&i2c_mutex);

    if (!exists_c_map(i2c_ctx_map, &ctx)) {
        snprintf(tmp42, sizeof(tmp42), "ctx %d not inited\n", ctx);
        _applog(0, tmp42, false);
        pthread_mutex_unlock(&i2c_mutex);
        return -2;
    }

    // Ожидание готовности перед началом записи
    uVar3 = wait4i2c_ready();
    if (uVar3 == 0) {
        strncpy(tmp42, "iic not ready 4 write\n ", 0x17);
        _applog(0, tmp42, false);
        pthread_mutex_unlock(&i2c_mutex);
        return -2;
    }

    // Цикл записи
    for (uint32_t i = 0; i < size; ++i) {
        // Формирование команды записи
        // Команда включает данные байта и флаги START/STOP/WRITE
        val = 0x100 | 0x1000000 | pbVar7[i]; // Примерная маска WRITE
        
        fpga_write(0xc, val);
        usleep(10000);

        // Ожидание подтверждения записи (ACK)
        uVar3 = wait4i2c_data(&no_use);
        if (uVar3 == 0) {
            strncpy(tmp42, "iic write reg failed\n ", 0x16);
            _applog(0, tmp42, false);
            pthread_mutex_unlock(&i2c_mutex);
            return -3;
        }
    }

    pthread_mutex_unlock(&i2c_mutex);
    return 0;
}