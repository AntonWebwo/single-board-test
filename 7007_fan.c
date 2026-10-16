#include <cstdint>
#include <unistd.h>

// --- Типы данных (на основе логики использования в функциях) ---

typedef enum bitmain_fan_side_e {
    FRONT_SIDE=0,
    REAR_SIDE=1,
    BOTH_SIZE=2
} bitmain_fan_side_e;

typedef enum bitmain_fan_id_e {
    FAN1=0,
    FAN2=1,
    FAN3=2,
    FAN4=3,
    FAN5=4,
    FAN6=5,
    FAN_MAX=6
} bitmain_fan_id_e;

// --- Заглушки для внешних зависимостей ---
extern int32_t fpga_read(uint32_t reg, uint32_t* value);
extern int32_t gpio_write(uint32_t pin, char val);

// --- Функции ---

/**
 * @brief Инициализация подсистемы вентиляторов.
 * В данном псевдокоде является заглушкой.
 */
int32_t fan_init(void) {
    return 0;
}

/**
 * @brief Деинициализация подсистемы вентиляторов.
 * В данном псевдокоде является заглушкой.
 */
void fan_uninit(void) {
    return;
}

/**
 * @brief Получение текущей скорости вращения вентилятора в реальном времени.
 * 
 * Логика:
 * 1. Считывает версию FPGA из регистра 0.
 * 2. В цикле (максимум 6 попыток с задержкой 10мс) считывает регистр 1.
 * 3. Проверяет биты 8-10 прочитанного значения на совпадение с id вентилятора.
 * 4. Если совпадение найдено, вычисляет скорость:
 *    - Для версии FPGA 0xb025: (младший байт) * 0xf0
 *    - Для остальных версий: (младший байт) * 0x78
 * 5. Возвращает -1, если вентилятор не найден за 6 попыток.
 */
int32_t fan_get_realtime_speed(bitmain_fan_id_e id) {
    int32_t iVar1 = 6; // Счетчик попыток
    uint32_t speed = 0;
    uint32_t ver = 0;

    // Чтение версии FPGA
    fpga_read(0, &ver);
    ver = ver & 0xffff;

    do {
        // Чтение регистра скорости
        fpga_read(1, &speed);

        // Проверка ID вентилятора в битах 8-10: (speed & 0x7ff) >> 8
        if (((speed & 0x7ff) >> 8) == static_cast<uint32_t>(id)) {
            if (ver == 0xb025) {
                // Версия B025
                iVar1 = (speed & 0xff) * 0xf0;
            } else {
                // Другие версии
                iVar1 = (speed & 0xff) * 0x78;
            }
            return iVar1;
        }

        // Задержка 10 мс перед следующей попыткой
        usleep(10000);
        
        // Уменьшение счетчика попыток
        iVar1 = iVar1 - 1;
    } while (iVar1 != 0);

    return -1;
}

/**
 * @brief Получение максимального количества вентиляторов.
 */
int32_t fan_get_max_num(void) {
    return 6;
}

/**
 * @brief Получение максимальной скорости вентилятора.
 */
int32_t fan_get_max_speed(bitmain_fan_id_e id) {
    (void)id; // Подавление предупреждения о неиспользуемом параметре
    return -1;
}

/**
 * @brief Включение вентиляторов.
 * 
 * Логика:
 * - FRONT_SIDE: включает GPIO 0x3ba
 * - REAR_SIDE: включает GPIO 0x3bb
 * - Иначе (или оба): включает оба GPIO
 */
int32_t fan_turn_on(bitmain_fan_side_e side) {
    int32_t iVar1;
    int32_t iVar2;

    if (side == FRONT_SIDE) {
        iVar1 = gpio_write(0x3ba, '\x01');
        return iVar1;
    }
    
    if (side == REAR_SIDE) {
        iVar1 = gpio_write(0x3bb, '\x01');
        return iVar1;
    }

    // Включение обоих сторон
    iVar1 = gpio_write(0x3ba, '\x01');
    iVar2 = gpio_write(0x3bb, '\x01');
    return iVar2 + iVar1;
}

/**
 * @brief Выключение вентиляторов.
 * 
 * Логика аналогична fan_turn_on, но записывает 0.
 */
int32_t fan_turn_off(bitmain_fan_side_e side) {
    int32_t iVar1;
    int32_t iVar2;

    if (side == FRONT_SIDE) {
        iVar1 = gpio_write(0x3ba, '\0');
        return iVar1;
    }
    
    if (side == REAR_SIDE) {
        iVar1 = gpio_write(0x3bb, '\0');
        return iVar1;
    }

    // Выключение обоих сторон
    iVar1 = gpio_write(0x3ba, '\0');
    iVar2 = gpio_write(0x3bb, '\0');
    return iVar2 + iVar1;
}
