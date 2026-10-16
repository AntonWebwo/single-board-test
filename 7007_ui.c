#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <map>
#include <sys/prctl.h>

// --- Типы данных ---

typedef enum ui_type_e {
    RED_LED=1,
    GREEN_LED=2,
    BEEP_1=16,
    RESET_KEY=256,
    IP_REPORT_KEY=257,
    PS2PL_RST_KEY=258,
    UI_RESERVED=259
} ui_type_e;

typedef enum ui_status_e {
    UI_ON=0,
    UI_OFF=1,
    UI_FLICKER=2
} ui_status_e;

typedef enum bitmain_key_status_e {
    HAL_KEY_PRESSED=0,
    HAL_KEY_UP=1
} bitmain_key_status_e;

typedef void (*bitmain_key_callback)(ui_type_e key, bitmain_key_status_e status);

// Структура для хранения состояния мерцания (аналог filker_interval_t)
struct FilkerInterval {
    uint32_t interval;
    uint32_t timeout;
    uint32_t next_val;
};

// --- Глобальные переменные ---

static pthread_mutex_t ui_api_mutex = PTHREAD_MUTEX_INITIALIZER;
static std::map<uint32_t, FilkerInterval> ui_flicker_map;
static int32_t ui_inited = 0;
static int32_t ui_pull_thread = 0;
static pthread_t ui_thread;
static bitmain_key_callback ui_callback[5] = {nullptr};
static int32_t lcd_ctx = -1;
static const char* lcd_path = "/dev/bitmain-lcd";

// Флаг регистрации колбэков клавиш (статическая переменная внутри reg_key_callback в оригинале)
static int32_t g_key_reged = 0;

// --- Заглушки для внешних зависимостей ---

extern int32_t gpio_export(uint32_t port);
extern int32_t gpio_direction(uint32_t port, int direction);
extern int32_t gpio_write(uint32_t port, uint8_t val);
extern int32_t gpio_read(uint32_t port, uint8_t* val);
extern int32_t gpio_reg_callback(uint32_t port, void* function);
extern void _applog(int level, const char* msg, bool newline);

// Внешние функции LCD
extern int32_t lcd_init(const char* path);
extern int32_t lcd_write(int32_t ctx, int8_t line, int8_t* data, uint32_t size);
extern int32_t lcd_clear(int32_t ctx);

// HAL функции для получения адресов портов
extern int32_t hal_led_red_addr();
extern int32_t hal_key_reset_addr();
extern int32_t hal_key_ipreport_addr();

#define DIRECTION_OUT 1
#define DIRECTION_IN 0

// --- Внутренние функции ---

/**
 * @brief Преобразование типа UI элемента в номер GPIO порта.
 */
uint32_t convert_ui_type_to_port(ui_type_e type) {
    // В оригинале используется hal_led_red_addr() и т.д.
    // Для упрощения и соответствия псевдокоду оставляем вызовы HAL.
    // Если HAL возвращает -1, функция вернет 0.
    
    if (type == RED_LED) {
        int32_t addr = hal_led_red_addr();
        return (addr >= 0) ? static_cast<uint32_t>(addr) : 0;
    }
    // Остальные типы в псевдокоде convert_ui_type_to_port обрабатываются аналогично
    // или через switch, но в предоставленном фрагменте видна только проверка hal_led_red_addr.
    // Дополняем логикой по аналогии для других типов, если они требуются, 
    // но строго следуя предоставленному коду:
    
    int32_t addr = hal_led_red_addr(); // Псевдокод вызывает именно эту функцию
    if (addr >= 0) {
        return static_cast<uint32_t>(addr);
    }
    return 0;
}

/**
 * @brief Преобразование номера порта в тип UI элемента.
 */
uint32_t convert_port_to_ui_type(uint32_t port) {
    uint32_t reset_port = static_cast<uint32_t>(hal_key_reset_addr());
    if (port == reset_port) {
        return 0x100; // RESET_KEY event ID
    }
    
    uint32_t ipreport_port = static_cast<uint32_t>(hal_key_ipreport_addr());
    if (port == ipreport_port) {
        return 0x101; // IP_REPORT_KEY event ID
    }
    
    char tmp42[2048];
    snprintf(tmp42, sizeof(tmp42), "unmaped port = %d\n", port);
    _applog(0, tmp42, false);
    
    return 0x103;
}

/**
 * @brief Удаление порта из карты мерцания.
 */
void remove_port_from_gpio_map(uint32_t port) {
    auto it = ui_flicker_map.find(port);
    if (it != ui_flicker_map.end()) {
        ui_flicker_map.erase(it);
    }
}

/**
 * @brief Общий обработчик прерываний GPIO для клавиш.
 * Вызывается из gpio_thread при изменении состояния.
 */
void gpio_key_callback(uint32_t port, uint8_t val) {
    uint32_t key_type = convert_port_to_ui_type(port);
    bitmain_key_status_e status = (val != 0) ? KEY_PRESSED : KEY_RELEASED;
    
    // Вызов всех зарегистрированных колбэков
    pthread_mutex_lock(&ui_api_mutex);
    for (int i = 0; i < 5; ++i) {
        if (ui_callback[i] != nullptr) {
            ui_callback[i](static_cast<ui_type_e>(key_type), status);
        }
    }
    pthread_mutex_unlock(&ui_api_mutex);
}

/**
 * @brief Поток обработки мерцания UI элементов.
 */
void* ui_pull_function(void* args) {
    prctl(PR_SET_NAME, "ui_pull", 0, 0, 0);
    
    while (true) {
        if (ui_pull_thread == 0) {
            return nullptr;
        }
        
        pthread_mutex_lock(&ui_api_mutex);
        
        // Итерация по карте мерцания
        for (auto it = ui_flicker_map.begin(); it != ui_flicker_map.end(); ) {
            uint32_t port = it->first;
            FilkerInterval& val = it->second;
            
            if (val.timeout == 0) {
                // Переключение состояния
                gpio_write(port, static_cast<uint8_t>(val.next_val));
                
                // Инверсия следующего значения
                val.next_val = (val.next_val != 1) ? 1 : 0;
                
                // Сброс таймаута
                val.timeout = val.interval;
            } else {
                // Уменьшение таймаута (шаг 200мс, так как usleep(200000))
                if (val.timeout > 200) {
                    val.timeout -= 200;
                } else {
                    val.timeout = 0;
                }
            }
            ++it;
        }
        
        pthread_mutex_unlock(&ui_api_mutex);
        
        usleep(200000); // 200 мс
        
        if (ui_pull_thread == 0) {
            return nullptr;
        }
    }
}

/**
 * @brief Инициализация потока мерцания и структур.
 */
int32_t routine_init(void) {
    pthread_attr_t tattr_ctrl;
    
    pthread_mutex_init(&ui_api_mutex, nullptr);
    
    // Очистка колбэков
    for (int i = 0; i < 5; ++i) {
        ui_callback[i] = nullptr;
    }
    
    // Карта уже инициализирована конструктором std::map
    
    pthread_attr_init(&tattr_ctrl);
    pthread_attr_setstacksize(&tattr_ctrl, 0x200000); // 2MB
    
    int32_t ret = pthread_create(&ui_thread, &tattr_ctrl, ui_pull_function, nullptr);
    ui_pull_thread = 1;
    
    pthread_attr_destroy(&tattr_ctrl);
    return ret;
}

// --- Публичные функции ---

/**
 * @brief Инициализация UI подсистемы.
 * Экспорт и настройка направлений GPIO для LED и клавиш.
 */
int32_t ui_init(void) {
    if (ui_inited == 0) {
        // RED LED
        uint32_t port = convert_ui_type_to_port(RED_LED);
        gpio_export(port);
        gpio_direction(port, DIRECTION_OUT);
        
        // GREEN LED
        port = convert_ui_type_to_port(GREEN_LED);
        gpio_export(port);
        gpio_direction(port, DIRECTION_OUT);
        
        // RESET KEY
        port = convert_ui_type_to_port(RESET_KEY);
        gpio_export(port);
        gpio_direction(port, DIRECTION_IN);
        
        // IP REPORT KEY
        port = convert_ui_type_to_port(IP_REPORT_KEY);
        gpio_export(port);
        gpio_direction(port, DIRECTION_IN);
        
        ui_inited = 1;
    }
    return 0;
}

/**
 * @brief Деинициализация UI.
 */
void ui_uninit(void) {
    return;
}

/**
 * @brief Управление состоянием UI элемента.
 */
int32_t gpio_ctrl_ui(ui_type_e type, ui_status_e status) {
    uint32_t port = convert_ui_type_to_port(type);
    
    if (port == 0) {
        _applog(0, "unsupported ported gpio\n", false);
        return -1;
    }
    
    int32_t lock_res = pthread_mutex_lock(&ui_api_mutex);
    if (lock_res != 0) {
        _applog(0, "failed to api lock\n", false);
        return -1;
    }
    
    int32_t ret = -1;
    
    if (status == UI_OFF) {
        remove_port_from_gpio_map(port);
        ret = gpio_write(port, 0);
    } else if (status == UI_ON) {
        remove_port_from_gpio_map(port);
        ret = gpio_write(port, 1);
    } else if (status == UI_FLICKER) {
        // Проверка: если уже есть в карте, не добавляем повторно
        if (ui_flicker_map.find(port) == ui_flicker_map.end()) {
            FilkerInterval val;
            val.interval = 200; // Интервал мерцания
            val.next_val = 1;   // Следующее состояние (вкл)
            val.timeout = 200;  // Начальный таймаут
            
            ui_flicker_map[port] = val;
            ret = 0;
        } else {
            ret = -1; // Уже мерцает
        }
    } else {
        _applog(0, "unsupported ported l\n", false); // Оригинальная строка "unsupported ported l"
        ret = -2;
    }
    
    pthread_mutex_unlock(&ui_api_mutex);
    return ret;
}

// --- Обертки для LED и Beeper ---

int32_t red_led_on(void) { return gpio_ctrl_ui(RED_LED, UI_ON); }
int32_t red_led_off(void) { return gpio_ctrl_ui(RED_LED, UI_OFF); }
int32_t red_led_flicker(void) {
    if (ui_pull_thread == 0) routine_init();
    return gpio_ctrl_ui(RED_LED, UI_FLICKER);
}

int32_t green_led_on(void) { return gpio_ctrl_ui(GREEN_LED, UI_ON); }
int32_t green_led_off(void) { return gpio_ctrl_ui(GREEN_LED, UI_OFF); }
int32_t green_led_flicker(void) {
    if (ui_pull_thread == 0) routine_init();
    return gpio_ctrl_ui(GREEN_LED, UI_FLICKER);
}

int32_t beeper_on(void) { return gpio_ctrl_ui(BEEP_1, UI_ON); }
int32_t beeper_off(void) { return gpio_ctrl_ui(BEEP_1, UI_OFF); }
int32_t beeper_flicker(void) {
    if (ui_pull_thread == 0) routine_init();
    return gpio_ctrl_ui(BEEP_1, UI_FLICKER);
}

// --- Работа с колбэками клавиш ---

/**
 * @brief Регистрация колбэка на нажатие клавиш.
 */
int32_t reg_key_callback(bitmain_key_callback function) {
    int32_t ret = pthread_mutex_lock(&ui_api_mutex);
    if (ret != 0) {
        _applog(0, "failed to api lock\n", false);
        return -1;
    }
    
    // Регистрация GPIO колбэков при первом вызове
    if (g_key_reged == 0) {
        uint32_t reset_port = convert_ui_type_to_port(RESET_KEY);
        gpio_reg_callback(reset_port, reinterpret_cast<void*>(gpio_key_callback));
        
        uint32_t ipreport_port = convert_ui_type_to_port(IP_REPORT_KEY);
        gpio_reg_callback(ipreport_port, reinterpret_cast<void*>(gpio_key_callback));
        
        g_key_reged = 1;
    }
    
    // Поиск свободного слота в массиве колбэков
    ret = -2;
    for (int i = 0; i < 5; ++i) {
        if (ui_callback[i] == nullptr) {
            ui_callback[i] = function;
            ret = 0;
            break;
        }
    }
    
    if (ret == -2) {
        _applog(0, "no more listener available\n", false);
    }
    
    pthread_mutex_unlock(&ui_api_mutex);
    return ret;
}

/**
 * @brief Отмена регистрации колбэка.
 */
int32_t unreg_key_callback(bitmain_key_callback function) {
    int32_t ret = pthread_mutex_lock(&ui_api_mutex);
    if (ret != 0) {
        _applog(0, "failed to api lock\n", false);
        return -1;
    }
    
    ret = 0;
    bool found = false;
    for (int i = 0; i < 5; ++i) {
        if (ui_callback[i] == function) {
            ui_callback[i] = nullptr;
            found = true;
            break;
        }
    }
    
    if (!found) {
        _applog(1, "listener not registered before\n", false);
    }
    
    pthread_mutex_unlock(&ui_api_mutex);
    return ret;
}

// --- Чтение состояния клавиш ---

int32_t read_reset_key_val(void) {
    int32_t ret = pthread_mutex_lock(&ui_api_mutex);
    if (ret == 0) {
        uint32_t port = static_cast<uint32_t>(hal_key_reset_addr());
        uint8_t val = 0;
        gpio_read(port, &val);
        pthread_mutex_unlock(&ui_api_mutex);
        return (val != 0) ? 1 : 0;
    } else {
        _applog(0, "failed to api lock\n", false);
        return -1;
    }
}

int32_t read_ipreport_key_val(void) {
    int32_t ret = pthread_mutex_lock(&ui_api_mutex);
    if (ret == 0) {
        uint32_t port = static_cast<uint32_t>(hal_key_ipreport_addr());
        uint8_t val = 0;
        gpio_read(port, &val);
        pthread_mutex_unlock(&ui_api_mutex);
        return (val != 0) ? 1 : 0;
    } else {
        _applog(0, "failed to api lock\n", false);
        return -1;
    }
}

// --- LCD функции ---

/**
 * @brief Вывод результата на LCD.
 */
int32_t lcd_show_result(int32_t line, int8_t* data, uint32_t size) {
    int32_t ret = pthread_mutex_lock(&ui_api_mutex);
    if (ret != 0) {
        _applog(0, "failed to api lock\n", false);
        return -4;
    }
    
    int32_t res = -1;
    
    // Ленивая инициализация LCD
    if (lcd_ctx < 0) {
        lcd_ctx = lcd_init(lcd_path);
        if (lcd_ctx >= 0) {
            lcd_clear(lcd_ctx);
        }
    }
    
    if (lcd_ctx < 0) {
        char tmp42[2048];
        snprintf(tmp42, sizeof(tmp42), "failed to init %s\n", lcd_path);
        _applog(0, tmp42, false);
        res = -1;
    } else {
        res = lcd_write(lcd_ctx, static_cast<int8_t>(line), data, size);
    }
    
    pthread_mutex_unlock(&ui_api_mutex);
    return res;
}

/**
 * @brief Очистка LCD экрана.
 */
void lcd_clear_result(void) {
    int32_t ret = pthread_mutex_lock(&ui_api_mutex);
    if (ret == 0) {
        if (lcd_ctx >= 0) {
            lcd_clear(lcd_ctx);
        }
        pthread_mutex_unlock(&ui_api_mutex);
    } else {
        _applog(0, "failed to api lock\n", false);
    }
}