#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>

// --- Типы данных ---

typedef enum bitmain_uart_conf_type_t {
    BAUD=0,
    UART_VMIN=1,
    RESERVED=2
} bitmain_uart_conf_type_t;

// --- Глобальные переменные ---

// Массивы мьютексов для отправки и приема (по 16 штук, как в псевдокоде)
static pthread_mutex_t uart_send_mutex[16] = {};
static pthread_mutex_t uart_receive_mutex[16] = {};

// Внешние зависимости
extern uint32_t platform_inited;
extern int32_t fpga_read(uint32_t address, uint32_t* val);
extern int32_t fpga_write(uint32_t address, uint32_t val);
extern void _applog(int level, const char* msg, bool newline);

// --- Внутренние функции ---

/**
 * @brief Получение адресной информации для отправки по UART.
 * Заполняет указатели на адреса регистров FPGA в зависимости от chain_id.
 */
int32_t get_send_address_info(
    uint32_t which_uart,
    uint32_t* bit_shift_num,
    uint32_t* chain_send_fifo_status_addr,
    uint32_t* chain_send_ready_addr,
    uint32_t* chain_send_buffer_addr)
{
    uint32_t uVar1, uVar2, uVar3, uVar4;
    char tmp42[2048];

    switch (which_uart) {
        case 0:  uVar1 = 0x18; uVar4 = 0x41; uVar3 = 0x45; uVar2 = 0x46; break;
        case 1:  uVar1 = 0x10; uVar4 = 0x41; uVar3 = 0x47; uVar2 = 0x48; break;
        case 2:  uVar1 = 8;    uVar4 = 0x41; uVar3 = 0x49; uVar2 = 0x4A; break;
        case 3:  uVar3 = 0x41; uVar2 = 0x4B; uVar1 = 0x4C; goto LAB_ASSIGN;
        case 4:  uVar1 = 0x18; uVar4 = 0x42; uVar3 = 0x4D; uVar2 = 0x4E; break;
        case 5:  uVar1 = 0x10; uVar4 = 0x42; uVar3 = 0x4F; uVar2 = 0x50; break;
        case 6:  uVar1 = 8;    uVar4 = 0x42; uVar3 = 0x51; uVar2 = 0x52; break;
        case 7:  uVar3 = 0x42; uVar2 = 0x53; uVar1 = 0x54; goto LAB_ASSIGN;
        case 8:  uVar1 = 0x18; uVar4 = 0x43; uVar3 = 0x55; uVar2 = 0x56; break;
        case 9:  uVar1 = 0x10; uVar4 = 0x43; uVar3 = 0x57; uVar2 = 0x58; break;
        case 10: uVar1 = 8;    uVar4 = 0x43; uVar3 = 0x74; uVar2 = 0x75; break;
        case 11: uVar3 = 0x43; uVar2 = 0x76; uVar1 = 0x77; goto LAB_ASSIGN;
        case 12: uVar1 = 0x18; uVar4 = 0x44; uVar3 = 0x78; uVar2 = 0x79; break;
        case 13: uVar1 = 0x10; uVar4 = 0x44; uVar3 = 0x7A; uVar2 = 0x7B; break;
        default:
            snprintf(tmp42, sizeof(tmp42), "%s: The uart %d is not supported!!!\n", "get_send_address_info", which_uart);
            _applog(0, tmp42, false);
            return -1;
    }
    
    *bit_shift_num = uVar1;
    *chain_send_fifo_status_addr = uVar4;
    *chain_send_ready_addr = uVar3;
    *chain_send_buffer_addr = uVar2;
    return 0;

LAB_ASSIGN:
    *bit_shift_num = 0;
    *chain_send_fifo_status_addr = uVar3;
    *chain_send_ready_addr = uVar2;
    *chain_send_buffer_addr = uVar1;
    return 0;
}

/**
 * @brief Проверка количества данных в FIFO приема FPGA.
 */
uint32_t check_how_many_uart_data_in_fpga(uint8_t which_uart) {
    uint32_t val = 0;
    uint32_t result = 0;
    char tmp42[2048];

    switch (which_uart) {
        case 0:  fpga_read(0x59, &val); result = (val >> 16) & 0x3FF; break;
        case 1:  fpga_read(0x59, &val); result = val & 0x3FF; break;
        case 2:  fpga_read(0x5A, &val); result = (val >> 16) & 0x3FF; break;
        case 3:  fpga_read(0x5A, &val); result = val & 0x3FF; break;
        case 4:  fpga_read(0x5B, &val); result = (val >> 16) & 0x3FF; break;
        case 5:  fpga_read(0x5B, &val); result = val & 0x3FF; break;
        case 6:  fpga_read(0x5C, &val); result = (val >> 16) & 0x3FF; break;
        case 7:  fpga_read(0x5C, &val); result = val & 0x3FF; break;
        case 8:  fpga_read(0x5D, &val); result = (val >> 16) & 0x3FF; break;
        case 9:  fpga_read(0x5D, &val); result = val & 0x3FF; break;
        case 10: fpga_read(0x5E, &val); result = (val >> 16) & 0x3FF; break;
        case 11: fpga_read(0x5E, &val); result = val & 0x3FF; break;
        case 12: fpga_read(0x5F, &val); result = (val >> 16) & 0x3FF; break;
        case 13: fpga_read(0x5F, &val); result = val & 0x3FF; break;
        default:
            snprintf(tmp42, sizeof(tmp42), "%s: The uart%d is not supported!!!\n", "check_how_many_uart_data_in_fpga", which_uart);
            _applog(0, tmp42, false);
            result = 0;
    }
    return result;
}

/**
 * @brief Чтение данных из FIFO приема FPGA.
 */
uint32_t read_uart_data_in_fpga(uint32_t which_uart, uint8_t* buf, uint32_t length) {
    uint32_t address, enable_addr;
    uint32_t ret_data;
    uint32_t full_words = length >> 2;
    uint32_t remaining_bytes = length & 3;
    uint32_t total_read = 0;
    uint8_t* ptr = buf;
    char tmp42[2048];

    // Определение адресов чтения
    switch (which_uart) {
        case 0:  address = 0x61; enable_addr = 0x60; break;
        case 1:  address = 99;   enable_addr = 0x62; break; // 99 = 0x63
        case 2:  address = 0x65; enable_addr = 100;  break; // 100 = 0x64
        case 3:  address = 0x67; enable_addr = 0x66; break;
        case 4:  address = 0x69; enable_addr = 0x68; break;
        case 5:  address = 0x6B; enable_addr = 0x6A; break;
        case 6:  address = 0x6D; enable_addr = 0x6C; break;
        case 7:  address = 0x6F; enable_addr = 0x6E; break;
        case 8:  address = 0x71; enable_addr = 0x70; break;
        case 9:  address = 0x73; enable_addr = 0x72; break;
        case 10: address = 0x7D; enable_addr = 0x7C; break;
        case 11: address = 0x7F; enable_addr = 0x7E; break;
        case 12: address = 0x81; enable_addr = 0x80; break;
        case 13: address = 0x83; enable_addr = 0x82; break;
        default:
            snprintf(tmp42, sizeof(tmp42), "%s: The uart%d is not supported!!!\n", "get_read_address_info", which_uart);
            _applog(0, tmp42, false);
            return 0;
    }

    // Активация чтения
    fpga_write(enable_addr, (length & 0x3FF) | 0x80000000);

    // Чтение полных слов (4 байта)
    if (full_words > 0) {
        for (uint32_t i = 0; i < full_words; ++i) {
            fpga_read(address, &ret_data);
            // Little-endian unpacking based on pseudocode logic:
            // ptr[0] = MSB, ptr[1] = next, ptr[2] = next, ptr[3] = LSB
            ptr[0] = static_cast<uint8_t>(ret_data >> 24);
            ptr[1] = static_cast<uint8_t>(ret_data >> 16);
            ptr[2] = static_cast<uint8_t>(ret_data >> 8);
            ptr[3] = static_cast<uint8_t>(ret_data);
            ptr += 4;
        }
        total_read = full_words * 4;
    }

    // Чтение оставшихся байтов
    if (remaining_bytes != 0) {
        fpga_read(address, &ret_data);
        if (remaining_bytes == 1) {
            ptr[0] = static_cast<uint8_t>(ret_data >> 24);
            total_read += 1;
        } else if (remaining_bytes == 2) {
            ptr[0] = static_cast<uint8_t>(ret_data >> 24);
            ptr[1] = static_cast<uint8_t>(ret_data >> 16);
            total_read += 2;
        } else if (remaining_bytes == 3) {
            ptr[0] = static_cast<uint8_t>(ret_data >> 24);
            ptr[1] = static_cast<uint8_t>(ret_data >> 16);
            ptr[2] = static_cast<uint8_t>(ret_data >> 8);
            total_read += 3;
        }
    }

    return total_read;
}

/**
 * @brief Очистка RX FIFO.
 */
void clear_uart_rx_fifo(uint32_t chain_id) {
    pthread_mutex_lock(&uart_receive_mutex[chain_id]);
    
    uint32_t size = check_how_many_uart_data_in_fpga(static_cast<uint8_t>(chain_id));
    if (size != 0) {
        uint8_t* buf = static_cast<uint8_t*>(malloc(size));
        if (buf) {
            read_uart_data_in_fpga(chain_id, buf, size);
            free(buf);
            usleep(10000);
        } else {
            char tmp42[2048];
            snprintf(tmp42, sizeof(tmp42), "%s: uart%d malloc buffer error\n", "clear_uart_rx_fifo", chain_id);
            _applog(0, tmp42, false);
            usleep(500000);
        }
    }
    
    pthread_mutex_unlock(&uart_receive_mutex[chain_id]);
}

/**
 * @brief Очистка TX FIFO.
 */
void clear_uart_tx_fifo(uint32_t chain_id) {
    uint32_t fifo_status_addr, ready_addr, buffer_addr, bit_shift;
    uint32_t space;
    int attempts = 0x15; // 21 попытка
    char tmp42[2048];

    pthread_mutex_lock(&uart_send_mutex[chain_id]);

    if (get_send_address_info(chain_id, &bit_shift, &fifo_status_addr, &ready_addr, &buffer_addr) == 0) {
        do {
            fpga_read(fifo_status_addr, &space);
            // Извлечение значения свободного места
            space = (space >> (bit_shift & 0xFF)) & 0xFF;
            
            if (space == 0xFF) {
                break; // FIFO пусто или готово
            }

            printf("%s: waiting fpga uart%d clear send fifo space ...\n", "clear_uart_tx_fifo", chain_id);
            // Сброс счетчика или триггер очистки
            fpga_write(ready_addr, space | 0x80000000);
            usleep(3000);
            attempts--;
        } while (attempts != 0);

        if (attempts == 0) {
            snprintf(tmp42, sizeof(tmp42), "%s: uart%d always dose not has enough send fifo space, break\n", "clear_uart_tx_fifo", chain_id);
            _applog(0, tmp42, false);
        }
    } else {
        snprintf(tmp42, sizeof(tmp42), "get_send_address_info error, chain_id = %d\n", chain_id);
        _applog(0, tmp42, false);
    }

    pthread_mutex_unlock(&uart_send_mutex[chain_id]);
}

// --- Публичные функции ---

/**
 * @brief Инициализация UART подсистемы.
 */
int32_t uart_init(void) {
    if (platform_inited == 0) {
        char tmp42[2048];
        strncpy(tmp42, "please init platform first!!", 28);
        tmp42[28] = '\n';
        tmp42[29] = '\0';
        _applog(0, tmp42, false);
        return -1;
    }
    return 0;
}

/**
 * @brief Деинициализация UART.
 */
void uart_uninit(void) {
    return;
}

/**
 * @brief Отправка данных через UART.
 */
int32_t uart_send(uint32_t chain_id, void* data, uint32_t size) {
    uint32_t fifo_status_addr, ready_addr, buffer_addr, bit_shift;
    uint32_t max_space;
    uint8_t send_buf[256];
    char tmp42[2048];
    
    memset(send_buf, 0, sizeof(send_buf));
    
    pthread_mutex_lock(&uart_send_mutex[chain_id]);

    if (get_send_address_info(chain_id, &bit_shift, &fifo_status_addr, &ready_addr, &buffer_addr) != 0) {
        snprintf(tmp42, sizeof(tmp42), "get_send_address_info error, chain_id = %d\n", chain_id);
        _applog(0, tmp42, false);
        pthread_mutex_unlock(&uart_send_mutex[chain_id]);
        return 0;
    }

    int attempts = 0x15; // 21 попытка ожидания места в FIFO
    do {
        fpga_read(fifo_status_addr, &max_space);
        max_space = (max_space >> (bit_shift & 0xFF)) & 0xFF;

        if (size <= max_space) {
            // Ожидание готовности UART к приему данных
            int ready_attempts = 0x16; // 22 попытки
            do {
                fpga_read(ready_addr, &max_space);
                if (static_cast<int32_t>(max_space) >= 0) {
                    memcpy(send_buf, data, size);
                    
                    // Запись полными словами (4 байта)
                    uint32_t words = size >> 2;
                    uint8_t* ptr = send_buf;
                    for (uint32_t i = 0; i < words; ++i) {
                        // Pack little-endian bytes into uint32 for FPGA write
                        uint32_t val = (static_cast<uint32_t>(ptr[0]) << 24) |
                                       (static_cast<uint32_t>(ptr[1]) << 16) |
                                       (static_cast<uint32_t>(ptr[2]) << 8) |
                                       static_cast<uint32_t>(ptr[3]);
                        fpga_write(buffer_addr, val);
                        ptr += 4;
                    }

                    // Запись остатка
                    uint32_t rem = size & 3;
                    if (rem != 0) {
                        uint32_t val = 0;
                        if (rem == 1) {
                            val = static_cast<uint32_t>(ptr[0]) << 24;
                        } else if (rem == 2) {
                            val = (static_cast<uint32_t>(ptr[0]) << 24) | (static_cast<uint32_t>(ptr[1]) << 16);
                        } else if (rem == 3) {
                            val = (static_cast<uint32_t>(ptr[0]) << 24) | (static_cast<uint32_t>(ptr[1]) << 16) | (static_cast<uint32_t>(ptr[2]) << 8);
                        }
                        fpga_write(buffer_addr, val);
                    }

                    // Триггер отправки
                    fpga_write(ready_addr, size | 0x80000000);
                    pthread_mutex_unlock(&uart_send_mutex[chain_id]);
                    return size;
                }
                usleep(3000);
                ready_attempts--;
            } while (ready_attempts != 0);

            snprintf(tmp42, sizeof(tmp42), "%s: uart%d always busy, break\n", "uart_send", chain_id);
            _applog(0, tmp42, false);
            pthread_mutex_unlock(&uart_send_mutex[chain_id]);
            return 0;
        }
        
        usleep(3000);
        attempts--;
    } while (attempts != 0);

    snprintf(tmp42, sizeof(tmp42), "%s: uart%d always dose not has enough send fifo space, break\n", "uart_send", chain_id);
    _applog(0, tmp42, false);
    pthread_mutex_unlock(&uart_send_mutex[chain_id]);
    return 0;
}

/**
 * @brief Прием данных через UART.
 */
int32_t uart_receive(uint32_t chain_id, void* data, uint32_t size, int32_t timeout) {
    (void)timeout; // Timeout не используется в данной реализации псевдокода
    
    pthread_mutex_lock(&uart_receive_mutex[chain_id]);
    
    uint32_t available = check_how_many_uart_data_in_fpga(static_cast<uint8_t>(chain_id));
    if (available < size) {
        size = available;
    }
    
    uint32_t read_len = 0;
    if (size > 0) {
        read_len = read_uart_data_in_fpga(chain_id, static_cast<uint8_t*>(data), size);
    }
    
    pthread_mutex_unlock(&uart_receive_mutex[chain_id]);
    return read_len;
}

/**
 * @brief Настройка конфигурации UART (например, baudrate).
 */
int32_t uart_set_config(uint32_t chain_id, bitmain_uart_conf_type_t type, void* val, int32_t size) {
    (void)size;
    if (val != nullptr) {
        pthread_mutex_lock(&uart_send_mutex[chain_id]);
        if (type == BAUD) {
            fpga_write(0xF, *static_cast<uint32_t*>(val));
        } else {
            char tmp42[2048];
            snprintf(tmp42, sizeof(tmp42), "unknown set config type = %d\n", type);
            _applog(1, tmp42, false);
        }
        pthread_mutex_unlock(&uart_send_mutex[chain_id]);
        return -5;
    }
    return -3;
}

/**
 * @brief Получение конфигурации UART.
 */
int32_t uart_get_config(uint32_t chain_id, bitmain_uart_conf_type_t type, void* val, int32_t* size) {
    if (size == nullptr || val == nullptr) {
        return -3;
    }

    pthread_mutex_lock(&uart_send_mutex[chain_id]);
    int32_t res = -5;
    
    if (type == BAUD) {
        uint32_t baud_val = 0;
        fpga_read(0xF, &baud_val);
        *static_cast<uint32_t*>(val) = baud_val;
        *size = 4;
        res = 0;
        
        char tmp42[2048];
        snprintf(tmp42, sizeof(tmp42), "uart_get_config return %d\n", baud_val);
        _applog(3, tmp42, false);
    } else {
        char tmp42[2048];
        snprintf(tmp42, sizeof(tmp42), "unknown set config type = %d\n", type);
        _applog(1, tmp42, false);
    }
    
    pthread_mutex_unlock(&uart_send_mutex[chain_id]);
    return res;
}

/**
 * @brief Сброс буферов UART.
 */
int32_t uart_flush(uint32_t chain_id) {
    clear_uart_rx_fifo(chain_id);
    usleep(10000);
    clear_uart_tx_fifo(chain_id);
    return 0;
}

/**
 * @brief Получение количества доступных для чтения байт.
 */
int32_t uart_get_readable_byte_num(uint32_t chain_id) {
    pthread_mutex_lock(&uart_receive_mutex[chain_id]);
    uint32_t num = check_how_many_uart_data_in_fpga(static_cast<uint8_t>(chain_id));
    pthread_mutex_unlock(&uart_receive_mutex[chain_id]);
    return num;
}