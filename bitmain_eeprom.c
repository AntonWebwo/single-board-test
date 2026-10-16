#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

// --- Типы данных (на основе использования в псевдокоде) ---

typedef struct {
    uint32_t eeprom_fd;      // Дескриптор контекста I2C
    uint32_t chain_has_opened; // Флаг состояния открытия
} bitmain_eeprom_state_t;

// Внешние функции I2C/IIC
extern int32_t iic_init(void* param); // Параметр может быть структурой или NULL
extern void iic_uninit(int32_t ctx);
extern int32_t iic_write_reg(int32_t ctx, uint8_t* reg, uint32_t bytes, void* data, uint32_t size, bool reg_addr_valid);
extern int32_t iic_read_reg(int32_t ctx, uint8_t* reg, uint32_t bytes, void* data, uint32_t size, bool reg_addr_valid);

// Заглушка для логирования
extern void _applog(int level, const char* msg, bool newline);
extern void print_time_stamp(char* buf, int len);
extern void log_to_file(char* msg, char* time);

// --- Глобальные переменные ---

// Массив состояний для 16 возможных цепей (chains)
static bitmain_eeprom_state_t g_eeprom_state[16] = {};

// --- Функции ---

/**
 * @brief Открытие соединения с EEPROM для указанной цепи.
 * Инициализирует I2C контекст, если он еще не открыт.
 */
int32_t eeprom_open(uint32_t which_chain) {
    char tmp42[2048];
    
    // Проверка границ массива
    if (which_chain >= 16) {
        snprintf(tmp42, sizeof(tmp42), "%s: Bad eeprom param, input chain is %d\n", "eeprom_open", which_chain);
        _applog(0, tmp42, false);
        return -0x7ffffeff; // Специфичный код ошибки из псевдокода
    }

    // Если уже открыто, возвращаем успех (или существующий дескриптор)
    if (g_eeprom_state[which_chain].chain_has_opened != 0) {
        return 0; 
    }

    // Инициализация I2C. В псевдокоде передается структура param, но здесь она не определена полностью.
    // Предполагаем, что iic_init возвращает контекст (fd).
    // Аргумент NULL или специфичная структура инициализации для EEPROM.
    int32_t fd = iic_init(nullptr); 
    
    if (fd < 0) {
        snprintf(tmp42, sizeof(tmp42), "%s: iic_init failed for chain %d\n", "eeprom_open", which_chain);
        _applog(0, tmp42, false);
        return -1;
    }

    g_eeprom_state[which_chain].eeprom_fd = fd;
    g_eeprom_state[which_chain].chain_has_opened = 1;

    return 0;
}

/**
 * @brief Закрытие соединения с EEPROM для указанной цепи.
 */
void eeprom_close(uint32_t which_chain) {
    char tmp42[2048];

    if (which_chain >= 16) {
        snprintf(tmp42, sizeof(tmp42), "%s: Bad eeprom param, input chain is %d\n", "eeprom_close", which_chain);
        _applog(0, tmp42, false);
        return;
    }

    if (g_eeprom_state[which_chain].chain_has_opened != 0) {
        iic_uninit(g_eeprom_state[which_chain].eeprom_fd);
        g_eeprom_state[which_chain].chain_has_opened = 0;
        g_eeprom_state[which_chain].eeprom_fd = 0;
    }
}

/**
 * @brief Запись данных в EEPROM.
 * Автоматически открывает соединение, если оно закрыто.
 */
int32_t eeprom_write(uint32_t which_chain, uint8_t addr, uint8_t* buf, uint32_t len) {
    int32_t iVar1;
    char tmp42[2048];
    uint8_t tmp_addr;

    if (which_chain >= 16) {
        snprintf(tmp42, sizeof(tmp42), "%s: Bad eeprom param, input chain is %d\n", "eeprom_write", which_chain);
        _applog(0, tmp42, false);
        return -0x7ffffeff;
    }

    // Авто-открытие, если не открыто
    if (g_eeprom_state[which_chain].chain_has_opened == 0) {
        iVar1 = eeprom_open(which_chain);
        if (iVar1 < 0) {
            snprintf(tmp42, sizeof(tmp42), "%s: auto exec eeprom_open, but chain %d open eeprom failed\n", "eeprom_write", which_chain);
            _applog(0, tmp42, false);
            return iVar1;
        }
    }

    if (len == 0) {
        return 0;
    }

    uint8_t* data_ptr = buf;
    uint8_t current_addr = addr;

    // Побайтовая запись (цикл do-while из псевдокода)
    do {
        tmp_addr = current_addr;
        // Запись 1 байта. reg_addr_valid = true (так как передаем адрес регистра)
        iVar1 = iic_write_reg(
            g_eeprom_state[which_chain].eeprom_fd, 
            &tmp_addr, 
            1, 
            data_ptr, 
            1, 
            true
        );

        if (iVar1 != 1) {
            snprintf(tmp42, sizeof(tmp42), "fail to write eeprom by iic, chain:%d, addr: %d", which_chain, (uint)tmp_addr);
            _applog(0, tmp42, false);
            return -0x7fffff00; // Код ошибки записи
        }

        current_addr++;
        data_ptr++;
        
    } while (data_ptr != buf + len);

    usleep(1000000); // Задержка 1 секунда после записи, как в псевдокоде (sleep(1))
    
    return 0;
}

/**
 * @brief Чтение данных из EEPROM.
 * Автоматически открывает соединение, если оно закрыто.
 */
int32_t eeprom_read(uint32_t which_chain, uint8_t addr, uint8_t* buf, uint32_t len) {
    int32_t iVar1;
    char tmp42[2048];
    uint8_t tmp_addr;

    if (which_chain >= 16) {
        snprintf(tmp42, sizeof(tmp42), "%s: Bad eeprom param, input chain is %d\n", "eeprom_read", which_chain);
        _applog(0, tmp42, false);
        return -0x7ffffeff;
    }

    // Авто-открытие, если не открыто
    if (g_eeprom_state[which_chain].chain_has_opened == 0) {
        iVar1 = eeprom_open(which_chain);
        if (iVar1 < 0) {
            snprintf(tmp42, sizeof(tmp42), "%s: auto exec eeprom_open, but chain %d open eeprom failed\n", "eeprom_read", which_chain);
            _applog(0, tmp42, false);
            return iVar1;
        }
    }

    if (len == 0) {
        return 0;
    }

    uint8_t* data_ptr = buf;
    uint8_t current_addr = addr;

    // Побайтовое чтение (цикл do-while из псевдокода)
    do {
        tmp_addr = current_addr;
        // Чтение 1 байта. reg_addr_valid = true
        iVar1 = iic_read_reg(
            g_eeprom_state[which_chain].eeprom_fd, 
            &tmp_addr, 
            1, 
            data_ptr, 
            1, 
            true
        );

        if (iVar1 != 1) {
            snprintf(tmp42, sizeof(tmp42), "fail to read eeprom by iic, chain: %d, addr: %d\n", which_chain, (uint)tmp_addr);
            _applog(0, tmp42, false);
            return -0x7fffff00; // Код ошибки чтения
        }

        current_addr++;
        data_ptr++;

    } while (data_ptr != buf + len);

    usleep(500000); // Задержка 0.5 секунды после чтения
    
    return 0;
}
