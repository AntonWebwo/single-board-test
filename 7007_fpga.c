#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>

// --- Типы данных и Константы ---

#define AXI_FPGA_DEV "/dev/axi_fpga_dev"
#define FPGA_MEM_DEV "/dev/fpga_mem"
#define AXI_MAP_SIZE 0x1200
#define HAL_MAP_SIZE 0x1000000

// Карты регистров FPGA (извлечены из псевдокода)
static const uint32_t t9_fpga_map[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x10, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
    0x00, 0x30, 0x31, 0x3C, 0x3E, 0x3F, 0x40, 0x41,
    0x42, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4B,
    0x4C, 0x4D, 0x4E, 0x50, 0x60, 0x61, 0x62, 0x63,
    0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B,
    0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73,
    0x74, 0x75, 0x76, 0x77, 0x7C, 0x7D, 0x7E, 0x7F,
    0x80, 0x81, 0x82, 0x84, 0x85, 0x86, 0x87, 0x88,
    0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90,
    0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA4,
    0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0x00,
    // Остальные элементы массива равны 0
};

static const uint32_t v9_fpga_map[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x20, 0x21, 0x22, 0x23, 0x24, 0x28,
    0x25, 0x26, 0x30, 0x30, 0x31, 0x32, 0x33, 0x3C,
    0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44,
    0x45, 0x46, 0x47, 0x48, 0x49, 0x4B, 0x4C, 0x4D,
    0x4E, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56,
    0x57, 0x58, 0x59, 0x5A, 0x5B, 0x11C, 0x11D, 0x11E,
    0x11F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E,
    0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82,
    0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B,
    0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93,
    0x94, 0x95, 0x96, 0x97, 0x00, 0x00, 0x00, 0x00,
    // Остальные элементы массива равны 0
};

// --- Типы данных bitmain_fpga_e ---

typedef enum bitmain_fpga_e {
    HARDWARE_VERSION=0,
    FAN_SPEED=1,
    HASH_ON_PLUG=2,
    WORK_FIFO_STATE=3,
    RETURN_NONCE=4,
    RETURN_NONCE_HI=5,
    NONCE_NUMBER_IN_FIFO=6,
    NONCE_FIFO_INTERRUPT=7,
    TEMPERATURE_0_3=8,
    TEMPERATURE_4_7=9,
    TEMPERATURE_8_11=10,
    TEMPERATURE_12_15=11,
    IIC_COMMAND=12,
    RESET_HASH_BOARD=13,
    BMC_COUNTER=14,
    FPGA_BT8D=15,
    TW_WRITE_COMMAND=16,
    QN_WRITE_DATA_COMMAND=17,
    RESET_CHAIN=18,
    FAN_CONTROL=19,
    TIME_OUT_CONTROL=20,
    TICKET_MASK_FPGA=21,
    HASH_COUNTING_NUMBER_FPGA=22,
    FAN_CONTROL_1=23,
    SNO=24,
    SOCKET_ID=25,
    BC_WRITE_COMMAND=26,
    FRIST_WORD=27,
    BC_COMMAND_BUFFER=28,
    BC_COMMAND_BUFFER_1=29,
    BC_COMMAND_BUFFER_2=30,
    FPGA_CHIP_ID_ADDR=31,
    FPGA_CHIP_ID_HI=32,
    CRC_ERROR_CNT=33,
    I2C_SW_ID=34,
    DHASH_ACC_CONTROL=35,
    COINBASE_AND_NONCE2_LENGTH=36,
    WORK_NONCE_2=37,
    WORK_NONCE_2_HI=38,
    NONCE2_AND_JOBID_STORE_ADDRESS=39,
    MERKLE_BIN_NUMBER=40,
    JOB_START_ADDRESS=41,
    JOB_LENGTH=42,
    JOB_DATA_READY=43,
    JOB_ID=44,
    WORK_ENABLE=45,
    BLOCK_HEADER_VERSION=46,
    TIME_STAMP=47,
    TARGET_BITS=48,
    PRE_HEADER_HASH=49,
    PRE_HEADER_HASH1=50,
    PRE_HEADER_HASH2=51,
    PRE_HEADER_HASH3=52,
    PRE_HEADER_HASH4=53,
    PRE_HEADER_HASH5=54,
    PRE_HEADER_HASH6=55,
    PRE_HEADER_HASH7=56,
    BLOCK_VERSION_0=57,
    BLOCK_VERSION_1=58,
    BLOCK_VERSION_2=59,
    BLOCK_VERSION_3=60,
    BLOCK_VERSION_4=61,
    BLOCK_VERSION_5=62,
    BLOCK_VERSION_6=63,
    BLOCK_VERSION_7=64,
    CHAIN0_3_SEND_FIFO_STATUS=65,
    CHAIN4_7_SEND_FIFO_STATUS=66,
    CHAIN8_11_SEND_FIFO_STATUS=67,
    CHAIN12_13_SEND_FIFO_STATUS=68,
    CHAIN0_SEND_READY=69,
    CHAIN0_SEND_BUFFER=70,
    CHAIN1_SEND_READY=71,
    CHAIN1_SEND_BUFFER=72,
    CHAIN2_SEND_READY=73,
    CHAIN2_SEND_BUFFER=74,
    CHAIN3_SEND_READY=75,
    CHAIN3_SEND_BUFFER=76,
    CHAIN4_SEND_READY=77,
    CHAIN4_SEND_BUFFER=78,
    CHAIN5_SEND_READY=79,
    CHAIN5_SEND_BUFFER=80,
    CHAIN6_SEND_READY=81,
    CHAIN6_SEND_BUFFER=82,
    CHAIN7_SEND_READY=83,
    CHAIN7_SEND_BUFFER=84,
    CHAIN8_SEND_READY=85,
    CHAIN8_SEND_BUFFER=86,
    CHAIN9_SEND_READY=87,
    CHAIN9_SEND_BUFFER=88,
    RECEIVE_FIFO_0_1_STATUS=89,
    RECEIVE_FIFO_2_3_STATUS=90,
    RECEIVE_FIFO_4_5_STATUS=91,
    RECEIVE_FIFO_6_7_STATUS=92,
    RECEIVE_FIFO_8_9_STATUS=93,
    RECEIVE_FIFO_10_11_STATUS=94,
    RECEIVE_FIFO_12_13_STATUS=95,
    CHAIN0_READ_ENABLE=96,
    CHAIN0_READ_DATA=97,
    CHAIN1_READ_ENABLE=98,
    CHAIN1_READ_DATA=99,
    CHAIN2_READ_ENABLE=100,
    CHAIN2_READ_DATA=101,
    CHAIN3_READ_ENABLE=102,
    CHAIN3_READ_DATA=103,
    CHAIN4_READ_ENABLE=104,
    CHAIN4_READ_DATA=105,
    CHAIN5_READ_ENABLE=106,
    CHAIN5_READ_DATA=107,
    CHAIN6_READ_ENABLE=108,
    CHAIN6_READ_DATA=109,
    CHAIN7_READ_ENABLE=110,
    CHAIN7_READ_DATA=111,
    CHAIN8_READ_ENABLE=112,
    CHAIN8_READ_DATA=113,
    CHAIN9_READ_ENABLE=114,
    CHAIN9_READ_DATA=115,
    CHAIN10_SEND_READY=116,
    CHAIN10_SEND_BUFFER=117,
    CHAIN11_SEND_READY=118,
    CHAIN11_SEND_BUFFER=119,
    CHAIN12_SEND_READY=120,
    CHAIN12_SEND_BUFFER=121,
    CHAIN13_SEND_READY=122,
    CHAIN13_SEND_BUFFER=123,
    CHAIN10_READ_ENABLE=124,
    CHAIN10_READ_DATA=125,
    CHAIN11_READ_ENABLE=126,
    CHAIN11_READ_DATA=127,
    CHAIN12_READ_ENABLE=128,
    CHAIN12_READ_DATA=129,
    CHAIN13_READ_ENABLE=130,
    CHAIN13_READ_DATA=131,
    SEED_ADDRESS_0=132,
    SEED_ADDRESS_1=133,
    SEED_ADDRESS_2=134,
    SEED_ADDRESS_3=135,
    SEED_ADDRESS_4=136,
    SEED_ADDRESS_5=137,
    SEED_ADDRESS_6=138,
    SEED_ADDRESS_7=139,
    FULL_NODES_ADDR=140,
    NUM_MODE_ADDR=141,
    CACHE_SAVE_ADDR=142,
    CACHE_START_ADDR=143,
    DS_START_ADDR=144,
    DS_DONE_ADDR=145,
    DATA_SAVE_ADDR=146,
    UART0_STATUS_ADDR=147,
    UART1_STATUS_ADDR=148,
    UART2_STATUS_ADDR=149,
    UART3_STATUS_ADDR=150,
    UART4_STATUS_ADDR=151,
    UART5_STATUS_ADDR=152,
    UART6_STATUS_ADDR=153,
    UART7_STATUS_ADDR=154,
    UART8_STATUS_ADDR=155,
    UART9_STATUS_ADDR=156,
    UART10_STATUS_ADDR=157,
    UART11_STATUS_ADDR=158,
    UART12_STATUS_ADDR=159,
    UART13_STATUS_ADDR=160,
    UART14_STATUS_ADDR=161,
    UART15_STATUS_ADDR=162,
    UART16_STATUS_ADDR=163,
    UART17_STATUS_ADDR=164,
    UART18_STATUS_ADDR=165,
    UART19_STATUS_ADDR=166,
    UART20_STATUS_ADDR=167,
    UART21_STATUS_ADDR=168,
    UART22_STATUS_ADDR=169,
    UART23_STATUS_ADDR=170,
    UART24_STATUS_ADDR=171,
    UART25_STATUS_ADDR=172,
    UART26_STATUS_ADDR=173,
    UART27_STATUS_ADDR=174,
    UART28_STATUS_ADDR=175,
    UART29_STATUS_ADDR=176,
    UART30_STATUS_ADDR=177,
    UART31_STATUS_ADDR=178,
    UART_WORK_FLAG_ADDR=179,
    DATA_SIZE_ADDR=180,
    DATA_STORE_ADDR=181,
    UART_CTRL_STATE_ADDR=182,
    UART_DBG_WR_ADDR=183,
    UART_DBG_RD_ADDR=184,
    UART_DBG_STATUS_ADDR=185,
    BIT_DIV_ADDR=186,
    MAX_FPGA_INDEX=187
} bitmain_fpga_e;

// --- Глобальные переменные ---

static int32_t fpga_inited = 0;
static int32_t fpga_fd = 0;
static uint32_t* axi_fpga_addr = nullptr;
static int32_t fd_fpga_mem_hal = 0;
static uint32_t* fpga_mem_addr_hal = nullptr;

// Внешняя переменная из common модуля
extern uint32_t platform_is_t9;

// --- Заглушки для внешних зависимостей ---
extern void _applog(int level, const char* msg, bool newline);

// --- Функции ---

/**
 * @brief Инициализация FPGA.
 * Открывает устройства /dev/axi_fpga_dev и /dev/fpga_mem, выполняет mmap.
 */
int32_t fpga_init(void) {
    int32_t iVar1 = 0;
    char tmp42[2048] = {0};

    // Открытие основного устройства FPGA
    fpga_fd = open(AXI_FPGA_DEV, O_RDWR);
    if (fpga_fd < 0) {
        snprintf(tmp42, sizeof(tmp42), "/dev/axi_fpga_dev open failed. fd = %d\n", fpga_fd);
        _applog(0, tmp42, false);
        return -1;
    }

    // Маппинг памяти AXI
    axi_fpga_addr = static_cast<uint32_t*>(mmap(nullptr, AXI_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fpga_fd, 0));
    if (axi_fpga_addr == MAP_FAILED || axi_fpga_addr == nullptr) {
        snprintf(tmp42, sizeof(tmp42), "mmap axi_fpga_addr failed. axi_fpga_addr = %p\n", nullptr);
        _applog(0, tmp42, false);
        close(fpga_fd);
        return -2;
    }

    // Открытие устройства памяти HAL
    fd_fpga_mem_hal = open(FPGA_MEM_DEV, O_RDWR);
    if (fd_fpga_mem_hal < 0) {
        snprintf(tmp42, sizeof(tmp42), "/dev/fpga_mem open failed. fd_fpga_mem_hal = %d\n", fd_fpga_mem_hal);
        _applog(0, tmp42, false);
        perror("open");
        munmap(axi_fpga_addr, AXI_MAP_SIZE);
        close(fpga_fd);
        return -1;
    }

    // Маппинг памяти HAL
    fpga_mem_addr_hal = static_cast<uint32_t*>(mmap(nullptr, HAL_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fpga_mem_hal, 0));
    if (fpga_mem_addr_hal == MAP_FAILED || fpga_mem_addr_hal == nullptr) {
        snprintf(tmp42, sizeof(tmp42), "mmap fpga_mem_addr_hal failed. fpga_mem_addr_hal = 0x%x\n", 0);
        _applog(0, tmp42, false);
        munmap(axi_fpga_addr, AXI_MAP_SIZE);
        close(fpga_fd);
        close(fd_fpga_mem_hal);
        return -1;
    }

    // Успешная инициализация
    snprintf(tmp42, sizeof(tmp42), "mmap fpga_mem_addr_hal = 0x%x\n", reinterpret_cast<uintptr_t>(fpga_mem_addr_hal));
    _applog(3, tmp42, false);
    
    fpga_inited = 1;
    return 0;
}

/**
 * @brief Деинициализация FPGA.
 * Выполняет unmap и закрывает файловые дескрипторы.
 */
void fpga_uninit(void) {
    char tmp42[2048] = {0};

    if (fpga_inited == 0) {
        return;
    }

    int iVar1 = munmap(axi_fpga_addr, AXI_MAP_SIZE);
    if (iVar1 < 0) {
        strncpy(tmp42, "munmap failed!\n", sizeof(tmp42) - 1);
        _applog(0, tmp42, false);
    }

    iVar1 = munmap(fpga_mem_addr_hal, HAL_MAP_SIZE);
    if (iVar1 < 0) {
        strncpy(tmp42, "munmap fpga_mem_addr_hal failed!", sizeof(tmp42) - 1);
        strncat(tmp42, "\n", sizeof(tmp42) - strlen(tmp42) - 1);
        _applog(0, tmp42, false);
    }

    fpga_inited = 0;
    close(fpga_fd);
    close(fd_fpga_mem_hal);
}

/**
 * @brief Чтение регистра FPGA.
 * Использует карты t9_fpga_map или v9_fpga_map в зависимости от платформы.
 */
int32_t fpga_read(uint32_t address, uint32_t* val) {
    int32_t iVar1;
    uint32_t uVar2;

    // Авто-инициализация если не инициализировано
    if ((fpga_inited == 0) && ((iVar1 = fpga_init()), iVar1 != 0)) {
        return -1;
    }

    // Выбор карты регистров
    if (platform_is_t9 == 0) {
        uVar2 = v9_fpga_map[address];
    } else {
        uVar2 = t9_fpga_map[address];
    }

    *val = axi_fpga_addr[uVar2];
    return 0;
}

/**
 * @brief Запись в регистр FPGA.
 * Использует карты t9_fpga_map или v9_fpga_map в зависимости от платформы.
 */
int32_t fpga_write(uint32_t address, uint32_t val) {
    int32_t iVar1;
    uint32_t uVar2;

    // Авто-инициализация если не инициализировано
    if ((fpga_inited == 0) && ((iVar1 = fpga_init()), iVar1 != 0)) {
        return -1;
    }

    // Выбор карты регистров
    if (platform_is_t9 == 0) {
        uVar2 = v9_fpga_map[address];
    } else {
        uVar2 = t9_fpga_map[address];
    }

    axi_fpga_addr[uVar2] = val;
    return 0;
}

/**
 * @brief Получение адреса отображенной памяти HAL.
 */
uint32_t* get_fpga_map_mem_addr(void) {
    int32_t iVar1;

    // Авто-инициализация если не инициализировано
    if ((fpga_inited == 0) && ((iVar1 = fpga_init()), iVar1 != 0)) {
        return nullptr;
    }

    return fpga_mem_addr_hal;
}
