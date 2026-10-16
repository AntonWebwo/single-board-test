#ifndef JANSSON_H
#define JANSSON_H

#include <cstdint>
#include <cstddef>
#include <atomic>

#ifdef __cplusplus
extern "C" {
#endif

// --- Типы данных ---

/**
 * @brief Структура JSON объекта.
 * В реальной библиотеке jansson она сложнее, но для данной реализации
 * критично только поле refcount.
 */
typedef struct json_t {
    std::atomic<size_t> refcount; // Используем atomic для соответствия логике ExclusiveAccess/Barrier
    // ... остальные внутренние поля библиотеки jansson ...
} json_t;

// --- Объявления внешних функций библиотеки jansson ---
// Эти функции должны быть предоставлены самой библиотекой jansson при линковке

/**
 * @brief Освобождение памяти JSON объекта.
 */
void json_delete(json_t *json);

/**
 * @brief Установка значения в объект без увеличения счетчика ссылок.
 */
int json_object_set_new(json_t *object, const char *key, json_t *value);

/**
 * @brief Добавление значения в массив без увеличения счетчика ссылок.
 */
int json_array_append_new(json_t *array, json_t *value);

// --- Реализации функций из псевдокода ---

/**
 * @brief Уменьшение счетчика ссылок на JSON объект.
 * 
 * Логика:
 * 1. Проверяет валидность указателя и переполнение счетчика.
 * 2. Использует атомарные операции для безопасного уменьшения счетчика.
 * 3. Если счетчик достигает 0, вызывает json_delete.
 */
static inline void json_decref(json_t *json) {
    if (json == nullptr || json->refcount.load(std::memory_order_relaxed) == 0xFFFFFFFF) {
        return;
    }

    // Эмуляция DataMemoryBarrier и ExclusiveAccess через атомарную операцию fetch_sub
    // memory_order_acq_rel обеспечивает необходимый барьер памяти
    size_t old_count = json->refcount.fetch_sub(1, std::memory_order_acq_rel);
    
    if (old_count == 1) {
        json_delete(json);
    }
}

/**
 * @brief Увеличение счетчика ссылок на JSON объект.
 * 
 * Логика:
 * 1. Проверяет валидность указателя.
 * 2. Атомарно увеличивает счетчик.
 * 3. Возвращает тот же указатель (для цепочек вызовов).
 */
static inline json_t * json_incref(json_t *json) {
    if (json == nullptr || json->refcount.load(std::memory_order_relaxed) == 0xFFFFFFFF) {
        return json;
    }

    // Эмуляция ExclusiveAccess и DataMemoryBarrier через атомарную операцию fetch_add
    json->refcount.fetch_add(1, std::memory_order_acq_rel);
    
    return json;
}

/**
 * @brief Установка значения в JSON объект с автоматическим управлением ссылками.
 * 
 * Логика:
 * 1. Увеличивает счетчик ссылок передаваемого значения.
 * 2. Вызывает внутреннюю функцию set_new.
 */
static inline int json_object_set(json_t *object, const char *key, json_t *value) {
    json_t *value_incref = json_incref(value);
    return json_object_set_new(object, key, value_incref);
}

/**
 * @brief Добавление значения в JSON массив с автоматическим управлением ссылками.
 * 
 * Логика:
 * 1. Увеличивает счетчик ссылок передаваемого значения.
 * 2. Вызывает внутреннюю функцию append_new.
 */
static inline int json_array_append(json_t *array, json_t *value) {
    json_t *value_incref = json_incref(value);
    return json_array_append_new(array, value_incref);
}

#ifdef __cplusplus
}
#endif

#endif // JANSSON_H