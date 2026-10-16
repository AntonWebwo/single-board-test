#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include <map>
#include <mutex>

// --- Типы данных ---

typedef enum bitmain_gpio_direction_e {
    DIRECTION_IN=0,
    DIRECTION_OUT=1
} bitmain_gpio_direction_e;

typedef void (*bitmain_gpio_callback)(uint32_t port, uint8_t val);

struct GpioCallbackData {
    bitmain_gpio_callback function;
    uint8_t last_val;
};

// --- Глобальные переменные ---

static int32_t gpio_inited = 0;
static int32_t gpio_thread_running = 0;
static pthread_t gpio_thread_id;

// Мьютексы для защиты доступа к API и карте колбэков
static std::mutex api_mutex;
static std::mutex ctrl_mutex;

// Карта колбэков: Key = Port (uint32_t), Value = GpioCallbackData
static std::map<uint32_t, GpioCallbackData> gpio_callback_map;

// Строковые константы для sysfs
static const char* dir_str[] = {"in", "out"};
static const char values_str[] = "01";

// --- Заглушки для внешних зависимостей ---
extern void _applog(int level, const char* msg, bool newline);

// --- Внутренние функции ---

/**
 * @brief Функция потока мониторинга GPIO.
 * Опрос значений портов и вызов зарегистрированных колбэков при изменении состояния.
 */
void* gpio_thread_function(void* args) {
    // Установка имени потока (аналог prctl)
    // prctl(PR_SET_NAME, "gpio_monitor", 0, 0, 0); 

    while (gpio_thread_running) {
        std::lock_guard<std::mutex> lock(ctrl_mutex);
        
        // Итерация по карте колбэков
        for (auto it = gpio_callback_map.begin(); it != gpio_callback_map.end(); ++it) {
            uint32_t port = it->first;
            GpioCallbackData& cb_data = it->second;
            
            uint8_t current_val = 0;
            if (gpio_read(port, &current_val) != 0) {
                continue; // Ошибка чтения, пропускаем
            }

            // Если значение изменилось
            if (cb_data.last_val != current_val) {
                // Вызов колбэка
                if (cb_data.function) {
                    cb_data.function(port, current_val);
                }
                // Обновление последнего известного значения
                cb_data.last_val = current_val;
            }
        }
        
        // Разблокировка происходит автоматически при выходе из scope lock_guard
        
        usleep(200000); // Задержка 200мс
    }
    
    return nullptr;
}

// --- Публичные функции ---

/**
 * @brief Инициализация подсистемы GPIO.
 */
int32_t gpio_init(void) {
    return 0;
}

/**
 * @brief Деинициализация подсистемы GPIO.
 */
void gpio_uninit(void) {
    if (gpio_inited) {
        gpio_thread_running = 0;
        pthread_join(gpio_thread_id, nullptr);
        gpio_inited = 0;
    }
}

/**
 * @brief Регистрация колбэка на изменение состояния GPIO.
 */
int32_t gpio_reg_callback(uint32_t port, bitmain_gpio_callback function) {
    if (!function) {
        _applog(0, "bad param\n", false);
        return -1;
    }

    if (!gpio_inited) {
        // Инициализация карты и потока при первом колбэке
        gpio_thread_running = 1;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 0x200000); // 2MB stack
        
        if (pthread_create(&gpio_thread_id, &attr, gpio_thread_function, nullptr) != 0) {
            _applog(0, "Failed to create gpio thread\n", false);
            return -1;
        }
        pthread_attr_destroy(&attr);
        gpio_inited = 1;
    }

    std::lock_guard<std::mutex> lock(ctrl_mutex);
    
    // Проверка наличия порта в карте
    if (gpio_callback_map.find(port) == gpio_callback_map.end()) {
        GpioCallbackData new_cb;
        new_cb.function = function;
        new_cb.last_val = 0; // Начальное значение
        gpio_callback_map[port] = new_cb;
    } else {
        // Обновление существующего колбэка
        gpio_callback_map[port].function = function;
    }

    return 0;
}

/**
 * @brief Отмена регистрации колбэка.
 */
int32_t gpio_unreg_callback(uint32_t port, bitmain_gpio_callback function) {
    if (!function) {
        _applog(0, "bad param\n", false);
        return -1;
    }

    std::lock_guard<std::mutex> lock(ctrl_mutex);
    
    auto it = gpio_callback_map.find(port);
    if (it != gpio_callback_map.end()) {
        gpio_callback_map.erase(it);
        return 0;
    } else {
        _applog(1, "callback function not registered before\n", false);
        return -1;
    }
}

/**
 * @brief Проверка существования GPIO порта в sysfs.
 */
int32_t is_gpio_exist(uint32_t port) {
    std::lock_guard<std::mutex> lock(api_mutex);
    
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d", port);
    
    if (access(path, F_OK) == 0) {
        return 1;
    }
    return 0;
}

/**
 * @brief Экспорт GPIO порта (создание файла в sysfs).
 */
int32_t gpio_export(uint32_t port) {
    std::lock_guard<std::mutex> lock(api_mutex);
    
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d", port);
    
    // Если уже существует, ничего не делаем
    if (access(path, F_OK) == 0) {
        return 0;
    }

    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        _applog(0, "Failed to open export for writing!\n", false);
        return -2;
    }

    char buf[16];
    int len = snprintf(buf, sizeof(buf), "%d", port);
    if (write(fd, buf, len) < 0) {
        _applog(0, "Failed to export gpio!\n", false);
        close(fd);
        return -3;
    }
    
    close(fd);
    return 0;
}

/**
 * @brief Удаление экспорта GPIO порта.
 */
int32_t gpio_unexport(uint32_t port) {
    std::lock_guard<std::mutex> lock(api_mutex);
    
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d", port);
    
    if (access(path, F_OK) != 0) {
        _applog(1, "port already unexported\n", false);
        return 0;
    }

    int fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd < 0) {
        _applog(0, "Failed to open unexport for writing!\n", false);
        return -2;
    }

    char buf[16];
    int len = snprintf(buf, sizeof(buf), "%d", port);
    if (write(fd, buf, len) < 0) {
        _applog(0, "Failed to unexport gpio!\n", false);
        close(fd);
        return -3;
    }
    
    close(fd);
    return 0;
}

/**
 * @brief Установка направления GPIO (Input/Output).
 */
int32_t gpio_direction(uint32_t port, bitmain_gpio_direction_e direction) {
    std::lock_guard<std::mutex> lock(api_mutex);
    
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", port);
    
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        _applog(0, "Failed to open gpio direction for writing!\n", false);
        return -2;
    }

    const char* dir = (direction == DIRECTION_IN) ? "in" : "out";
    if (write(fd, dir, strlen(dir)) < 0) {
        _applog(0, "Failed to set gpio direction!\n", false);
        close(fd);
        return -3;
    }
    
    close(fd);
    return 0;
}

/**
 * @brief Чтение значения GPIO.
 */
int32_t gpio_read(uint32_t port, uint8_t* val) {
    if (!val) return -1;

    std::lock_guard<std::mutex> lock(api_mutex);
    
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", port);
    
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        _applog(0, "Failed to open gpio value for reading!\n", false);
        return -2;
    }

    char buf[4] = {0};
    if (read(fd, buf, sizeof(buf)) < 0) {
        _applog(0, "Failed to read value!\n", false);
        close(fd);
        return -3;
    }
    
    close(fd);
    
    // Преобразование строки "0\n" или "1\n" в число
    *val = static_cast<uint8_t>(strtol(buf, nullptr, 10));
    return 0;
}

/**
 * @brief Запись значения в GPIO.
 */
int32_t gpio_write(uint32_t port, uint8_t val) {
    std::lock_guard<std::mutex> lock(api_mutex);
    
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", port);
    
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        _applog(0, "Failed to open gpio value for writing!\n", false);
        return -2;
    }

    // Используем строку "01", где индекс 0 -> '0', индекс 1 -> '1'
    char write_val = values_str[val & 1];
    
    if (write(fd, &write_val, 1) < 0) {
        _applog(0, "Failed to write value!\n", false);
        close(fd);
        return -3;
    }
    
    close(fd);
    return 0;
}