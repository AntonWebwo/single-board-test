#include <cstdint>

// --- Заглушки для внешних зависимостей ---
extern int32_t is_gpio_exist(uint32_t port);
extern int32_t gpio_export(uint32_t port);
extern int32_t gpio_direction(uint32_t port, int direction); // Предполагаем, что DIRECTION_OUT определен где-то
extern int32_t gpio_write(uint32_t port, uint8_t val);

// Константа для направления вывода (если не определена в другом месте)
#ifndef DIRECTION_OUT
#define DIRECTION_OUT 1
#endif

// --- Функции ---

/**
 * @brief Включение питания.
 * 
 * Логика:
 * 1. Проверяет наличие GPIO 0x38b.
 * 2. Если не существует, экспортирует его и устанавливает направление на выход.
 * 3. Записывает 0 (низкий уровень) для включения питания.
 */
int32_t bitmain_power_on(void) {
    int32_t iVar1;
    
    iVar1 = is_gpio_exist(0x38b);
    if (iVar1 == 0) {
        gpio_export(0x38b);
        gpio_direction(0x38b, DIRECTION_OUT);
    }
    
    iVar1 = gpio_write(0x38b, '\0');
    return iVar1;
}

/**
 * @brief Выключение питания.
 * 
 * Логика:
 * 1. Проверяет наличие GPIO 0x38b.
 * 2. Если не существует, экспортирует его и устанавливает направление на выход.
 * 3. Записывает 1 (высокий уровень) для выключения питания.
 */
int32_t bitmain_power_off(void) {
    int32_t iVar1;
    
    iVar1 = is_gpio_exist(0x38b);
    if (iVar1 == 0) {
        gpio_export(0x38b);
        gpio_direction(0x38b, DIRECTION_OUT);
    }
    
    iVar1 = gpio_write(0x38b, '\x01');
    return iVar1;
}