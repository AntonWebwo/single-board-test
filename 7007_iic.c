#include <cstdint>
#include <cstdio>
#include <cstring>
#include <pthread.h>

// --- Типы данных (на основе использования в псевдокоде) ---

typedef struct {
    uint8_t master_addr;
    uint8_t slave_low;
    uint8_t slave_high;
} iic_init_param_t;

// Внешние функции (заглушки для драйвера I2C)
extern int32_t i2c_init(int8_t *path); // Предполагаем путь или NULL для дефолта
extern int32_t i2c_uninit(int32_t ctx);
extern int32_t i2c_select(int32_t ctx, uint32_t addr);
extern int32_t i2c_ioctl(int32_t ctx, uint32_t cmd, uint32_t arg);
extern int32_t i2c_read(int32_t ctx, void *data, uint32_t size);
extern int32_t i2c_write(int32_t ctx, void *data, uint32_t size);
extern int32_t i2c_read_reg(int32_t ctx, uint8_t *reg, uint32_t bytes, void *data, uint32_t size, bool reg_addr_valid);
extern int32_t i2c_write_reg(int32_t ctx, uint8_t *reg, uint32_t bytes, void *data, uint32_t size, bool reg_addr_valid);

// Заглушка для логирования
extern void _applog(int level, const char* msg, bool newline);

// --- Глобальные переменные ---

static pthread_mutex_t iic_mutex = PTHREAD_MUTEX_INITIALIZER;

// --- Функции ---

/**
 * @brief Инициализация IIC интерфейса.
 * Настраивает мастер и слейв адреса через ioctl.
 */
int32_t iic_init(iic_init_param_t *param) {
    int iVar1;
    int32_t ctx;
    int32_t iVar2;
    char tmp42[2048];

    iVar1 = pthread_mutex_lock(&iic_mutex);
    if (iVar1 != 0) {
        strncpy(tmp42, "failed to iic lock\n", 0x14);
        _applog(0, tmp42, false);
        return -4;
    }

    if (param == nullptr) {
        ctx = -3;
        strncpy(tmp42, "bad param\n", 10); // "bad para" + 'm' + '\n'
        _applog(0, tmp42, false);
    } else {
        // Инициализация I2C контекста. В псевдокоде передается 0x0, возможно дефолтный путь
        ctx = i2c_init((int8_t *)0x0);
        
        if (ctx < 0) {
            ctx = -1;
            strncpy(tmp42, "failed to i2c_init\n", 0x14);
            _applog(0, tmp42, false);
        } else {
            // Выбор мастера
            iVar2 = i2c_select(ctx, (uint)param->master_addr);
            
            if (iVar2 == 0) {
                // Настройка слейв адресов через ioctl. 
                // Аргумент: (slave_low << 1) | (slave_high << 4)
                iVar2 = i2c_ioctl(ctx, 0x703, ((uint)param->slave_low << 1) | ((uint)param->slave_high << 4));
                
                if (iVar2 == 0) {
                    // Успех, ctx возвращается
                } else {
                    strncpy(tmp42, "failed to i2c_ioctl\n", 0x14);
                    _applog(0, tmp42, false);
                    
                    if (ctx == 0) {
                        ctx = -1;
                    } else {
                        i2c_uninit(ctx);
                        ctx = -1;
                    }
                }
            } else {
                strncpy(tmp42, "failed to i2c_select\n", 0x15);
                _applog(0, tmp42, false);
                
                if (ctx == 0) {
                    ctx = -1;
                } else {
                    i2c_uninit(ctx);
                    ctx = -1;
                }
            }
        }
    }

    pthread_mutex_unlock(&iic_mutex);
    return ctx;
}

/**
 * @brief Деинициализация IIC интерфейса.
 */
void iic_uninit(int32_t ctx) {
    int iVar1;
    char tmp42[2048];

    iVar1 = pthread_mutex_lock(&iic_mutex);
    if (iVar1 == 0) {
        i2c_uninit(ctx);
        pthread_mutex_unlock(&iic_mutex);
    } else {
        strncpy(tmp42, "failed to i2c lock\n", 0x14);
        _applog(0, tmp42, false);
    }
    return;
}

/**
 * @brief Чтение данных из IIC.
 */
int32_t iic_read(int32_t ctx, void *data, uint32_t size) {
    int iVar1;
    int32_t iVar2;
    char tmp42[2048];

    iVar1 = pthread_mutex_lock(&iic_mutex);
    if (iVar1 == 0) {
        iVar2 = i2c_read(ctx, data, size);
        pthread_mutex_unlock(&iic_mutex);
    } else {
        iVar2 = -4;
        strncpy(tmp42, "failed to i2c lock\n", 0x14);
        _applog(0, tmp42, false);
    }
    return iVar2;
}

/**
 * @brief Запись данных в IIC.
 */
int32_t iic_write(int32_t ctx, void *data, uint32_t size) {
    int iVar1;
    int32_t iVar2;
    char tmp42[2048];

    iVar1 = pthread_mutex_lock(&iic_mutex);
    if (iVar1 == 0) {
        iVar2 = i2c_write(ctx, data, size);
        pthread_mutex_unlock(&iic_mutex);
    } else {
        iVar2 = -4;
        strncpy(tmp42, "failed to i2c lock\n", 0x14);
        _applog(0, tmp42, false);
    }
    return iVar2;
}

/**
 * @brief Чтение регистра IIC.
 */
int32_t iic_read_reg(int32_t ctx, uint8_t *reg, uint32_t bytes, void *data, uint32_t size, bool reg_addr_valid) {
    int iVar1;
    int32_t iVar2;
    char tmp42[2048];

    iVar1 = pthread_mutex_lock(&iic_mutex);
    if (iVar1 == 0) {
        iVar2 = i2c_read_reg(ctx, reg, bytes, data, size, reg_addr_valid);
        pthread_mutex_unlock(&iic_mutex);
    } else {
        iVar2 = -4;
        strncpy(tmp42, "failed to i2c lock\n", 0x14);
        _applog(0, tmp42, false);
    }
    return iVar2;
}

/**
 * @brief Запись в регистр IIC.
 */
int32_t iic_write_reg(int32_t ctx, uint8_t *reg, uint32_t bytes, void *data, uint32_t size, bool reg_addr_valid) {
    int iVar1;
    int32_t iVar2;
    char tmp42[2048];

    iVar1 = pthread_mutex_lock(&iic_mutex);
    if (iVar1 == 0) {
        iVar2 = i2c_write_reg(ctx, reg, bytes, data, size, reg_addr_valid);
        pthread_mutex_unlock(&iic_mutex);
    } else {
        iVar2 = -4;
        strncpy(tmp42, "failed to i2c lock\n", 0x14);
        _applog(0, tmp42, false);
    }
    return iVar2;
}