#include <cstdint>
#include <cstdio>
#include <cstring>
#include <pthread.h>

// --- Типы данных (на основе использования в псевдокоде) ---

typedef struct {
    uint32_t register_addr;
    uint32_t register_value;
} asic_register_value_t;

// Внешние глобальные переменные (объявлены в других файлах, но используются здесь)
extern asic_register_value_t hashboard_register_value[4][128]; // [chain][reg_index]
extern asic_register_value_t hashboard_asic_register_value[4][256][128]; // [chain][asic][reg_index]
extern pthread_mutex_t hashboard_register_value_mutex;

// Заглушки для внешних функций
extern void print_time_stamp(char *buf, int len);
extern void log_to_file(char *msg, char *time);
extern void _applog(int level, const char* msg, bool newline);

// Глобальная таблица значений по умолчанию (из псевдокода)
extern asic_register_value_t default_register_value[128];

// --- Функции ---

/**
 * @brief Сброс кэшированных значений регистров для конкретной цепи.
 * Копирует значения по умолчанию в структуры кэша.
 */
int32_t reset_register_cache_value_to_default(uint8_t which_chain) {
    asic_register_value_t (*paaVar1)[128];
    asic_register_value_t (*__dest)[128];
    asic_register_value_t (*paaVar2)[128];
    uint uVar3;

    if (which_chain < 4) {
        __dest = &hashboard_register_value[0];
        uVar3 = 0;
        paaVar1 = &hashboard_asic_register_value[0];
        
        do {
            // Итерация по цепям
            if (which_chain == uVar3) {
                // Сброс кэша для каждого ASIC в цепи
                asic_register_value_t* current_asic_cache = *paaVar1;
                for (int i = 0; i < 256; ++i) {
                    memcpy(current_asic_cache, default_register_value, sizeof(asic_register_value_t) * 128);
                    current_asic_cache += 128; // Переход к следующему ASIC
                }
                
                // Сброс общего кэша платы
                memcpy(*__dest, default_register_value, sizeof(asic_register_value_t) * 128);
            }
            
            uVar3 = uVar3 + 1;
            __dest = __dest + 1;
            paaVar1 = paaVar1 + 1;
        } while (uVar3 != 4);
        
        return 0;
    }
    return -1;
}

/**
 * @brief Получение значения из кэша регистров.
 * Проверяет уровень кэша (плата или чип) и возвращает значение, если адрес совпадает.
 */
int32_t get_register_cache_value(
    int level,          // hashboard_cache_level или asic_cache_level
    uint8_t which_chain, 
    uint32_t which_asic_address, 
    uint32_t register_address, 
    uint32_t *cache_value
) {
    uint uVar1;
    uint uVar2;
    int32_t iVar3;

    uVar1 = (uint)which_chain;
    
    // Проверка границ цепи
    if (3 < uVar1) {
        return -1;
    }
    
    // Проверка границ адреса ASIC
    if (0xff < which_asic_address) {
        return -2;
    }
    
    // Вычисление индекса регистра (адрес / 4, так как регистры 32-битные)
    uVar2 = register_address >> 2;
    
    // Проверка границ массива кэша (128 элементов)
    if (uVar2 < 0x80) {
        pthread_mutex_lock(&hashboard_register_value_mutex);
        
        if (level == 0) { // hashboard_cache_level
            // Проверка совпадения адреса
            if (register_address == hashboard_register_value[uVar1][uVar2].register_addr) {
                *cache_value = hashboard_register_value[uVar1][uVar2].register_value;
                pthread_mutex_unlock(&hashboard_register_value_mutex);
                return 0;
            }
            iVar3 = -4; // Адрес не найден в кэше платы
        } 
        else if (level == 1) { // asic_cache_level
            // Проверка совпадения адреса для конкретного ASIC
            if (register_address == hashboard_asic_register_value[uVar1][which_asic_address][uVar2].register_addr) {
                *cache_value = hashboard_asic_register_value[uVar1][which_asic_address][uVar2].register_value;
                pthread_mutex_unlock(&hashboard_register_value_mutex);
                return 0;
            }
            iVar3 = -5; // Адрес не найден в кэше ASIC
        } 
        else {
            iVar3 = -6; // Неверный уровень кэша
        }
        
        pthread_mutex_unlock(&hashboard_register_value_mutex);
        
        // Логирование ошибки (опционально, согласно псевдокоду)
        /*
        printf("[%s] level::%d, which_chain::%u, which_asic::%u, register_index::%u, hash_reg_val::%08x, register_address::%02x failed.\r\n",
               "get_register_cache_value", level, uVar1, which_asic_address, uVar2,
               hashboard_register_value[uVar1][uVar2].register_addr, register_address);
        */
    } 
    else {
        iVar3 = -3; // Индекс регистра вне диапазона
    }
    
    return iVar3;
}

/**
 * @brief Установка значения в кэш регистров.
 * Обновляет кэш на указанном уровне и синхронизирует данные при необходимости.
 */
int32_t set_register_cache_value(
    int level,          // hashboard_cache_level или asic_cache_level
    uint8_t which_chain, 
    uint32_t which_asic_address, 
    uint32_t register_address, 
    uint32_t cache_value
) {
    uint uVar1;
    uint uVar2;
    asic_register_value_t *paVar3;
    int32_t iVar4;
    uint uVar5;
    uint uVar6;

    uVar2 = (uint)which_chain;
    
    // Разделение адреса на индекс и смещение
    uVar5 = register_address & 3;
    uVar1 = register_address >> 2;
    uVar6 = uVar5 + uVar1;
    
    // Проверка границ цепи
    if (uVar2 < 4) {
        // Проверка границ адреса ASIC
        if (which_asic_address < 0x100) {
            // Проверка границ индекса кэша
            if (uVar6 < 0x81) {
                pthread_mutex_lock(&hashboard_register_value_mutex);
                
                if (level == 0) { // hashboard_cache_level
                    // Обновление кэша платы
                    if (register_address == hashboard_register_value[uVar2][uVar6].register_addr) {
                        hashboard_register_value[uVar2][uVar6].register_value = cache_value;
                        
                        // Синхронизация: обновление этого же регистра для всех ASIC в цепи
                        paVar3 = &hashboard_asic_register_value[uVar2][0][uVar6];
                        do {
                            paVar3->register_value = cache_value;
                            paVar3 = paVar3 + 0x80; // Переход к следующему ASIC (128 regs * 4 bytes approx stride logic from pseudo)
                        } while (paVar3 != &hashboard_asic_register_value[uVar2 + 1][0][uVar6]);
                        
                        pthread_mutex_unlock(&hashboard_register_value_mutex);
                        return 0;
                    }
                    iVar4 = -4;
                } 
                else if (level == 1) { // asic_cache_level
                    // Обновление кэша конкретного ASIC
                    if (register_address == hashboard_asic_register_value[uVar2][which_asic_address][uVar1 + uVar5].register_addr) {
                        hashboard_asic_register_value[uVar2][which_asic_address][uVar1 + uVar5].register_value = cache_value;
                        pthread_mutex_unlock(&hashboard_register_value_mutex);
                        return 0;
                    }
                    iVar4 = -5;
                } 
                else {
                    iVar4 = -6;
                }
                
                pthread_mutex_unlock(&hashboard_register_value_mutex);
                
                // Логирование ошибки
                /*
                printf("[%s] level::%d, which_chain::%u, which_asic::%u, register_index::%u, hash_register_value::%08x, register_address::%02x failed, ret:%d.\r\n",
                       "set_register_cache_value", level, uVar2, which_asic_address, uVar6,
                       hashboard_register_value[uVar2][uVar6].register_addr, register_address, iVar4);
                */
            } 
            else {
                iVar4 = -3;
            }
        } 
        else {
            iVar4 = -2;
        }
    } 
    else {
        iVar4 = -1;
    }
    
    return iVar4;
}
