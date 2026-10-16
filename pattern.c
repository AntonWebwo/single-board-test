#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <cmath>

// --- Типы данных (на основе использования в псевдокоде) ---

typedef struct {
    uint32_t chain;
    uint32_t asics;
    uint32_t domains;
    uint32_t asics_in_domain;
    uint32_t asic_domain_num;
    uint32_t big_core;
    uint32_t asic_small_cores;
    uint32_t samll_core_num_in_big_core; // Опечатка в оригинале сохранена для совместимости имен
    uint32_t patter_num;
    uint32_t abnormal_core;
    uint32_t interval;
    struct {
        uint32_t ccdly_sel;
        uint32_t pwth_sel;
        uint32_t swpf_mode;
        uint32_t pulse_mode;
        uint32_t clk_sel;
        uint32_t ro_relay_en;
        uint32_t co_relay_en;
        uint32_t diode_vdd_mux_sel;
    } reg;
    struct {
        uint32_t baudrate;
        uint32_t timeout;
    } speed;
    struct {
        char asic_type[32];
        uint32_t wait_temp;
        struct {
            char sensor[32];
            uint8_t sensor_addr[2];
            uint8_t sensor_num;
            uint8_t sensor_i2c_addr;
            uint8_t sensor_reg_addr;
        } sensor;
    } board;
    struct {
        int32_t temp_gap;
        uint32_t wait_cool_time;
        uint32_t cool_board_time;
    } temp;
    struct {
        uint32_t fan_speed;
        uint32_t work_fan_speed;
    } fan;
    struct {
        uint32_t invalid_core_num;
        uint32_t least_nonce_per_core;
        uint32_t asic_nonce_rate;
        uint32_t nonce_rate;
    } standard;
    struct {
        uint32_t level;
        uint32_t pre_open_core_voltage;
        uint32_t freq;
        uint32_t voltage;
    } tests[11]; // Предполагаемый размер массива тестов
    uint32_t sweeped_res_index;
} config_ctx;

typedef struct {
    uint32_t nonce_num_domain[16]; // Примерный размер
    uint32_t nonce_num_domain_big[4];
    uint32_t nonce_stats[256][17]; // [asic][core_count]
    float nonce_stdev;
    uint32_t nonce_num_asic[256];
} pattern_test_stats;

typedef struct {
    bool ok_asic_list[256];
    bool bad_asic_list[256];
    uint32_t asic_core_nonce_num[256][100]; // [asic][core]
    uint32_t valid_nonce_num;
    uint32_t every_asic_got_nonce_num[256];
    uint32_t every_asic_reg_nonce_error_num[256];
    uint32_t every_asic_reg_got_nonce_num[256];
    uint32_t every_asic_reg_ntick_nonce_num[256];
    bool pic_sensor_OK[4];
    bool asic_sensor_OK[256];
    bool ctrlboard_sensor_OK[2];
    int8_t sensor_local_temperature_from_pic[4];
    int8_t sensor_local_temperature_from_ctrlboard[2];
    bool pic_sensor_all_OK;
    bool asic_sensor_all_OK;
    bool ctrlboard_sensor_all_OK;
    bool apw_power_ok;
    bool pic_ok;
    bool eeprom_ok;
    bool asic_ok;
    bool nonce_rate_ok;
    uint32_t level;
    double voltage;
    double pre_open_core_voltage;
    uint32_t frequence;
    uint8_t test_standard;
    double nonce_rate;
    uint32_t repeat_nonce_number;
} history_result_t;

typedef struct {
    config_ctx* config;
    pattern_test_stats stats;
    struct {
        bool ok_asic_list[256];
        bool bad_asic_list[256];
        uint32_t asic_core_nonce_num[256][100];
        uint32_t valid_nonce_num;
        uint32_t every_asic_got_nonce_num[256];
        uint32_t every_asic_reg_nonce_error_num[256];
        uint32_t every_asic_reg_got_nonce_num[256];
        uint32_t every_asic_reg_ntick_nonce_num[256];
        bool pic_sensor_OK[4];
        bool asic_sensor_OK[256];
        bool ctrlboard_sensor_OK[2];
        int8_t sensor_local_temperature_from_pic[4];
        int8_t sensor_local_temperature_from_ctrlboard[2];
        bool pic_sensor_all_OK;
        bool asic_sensor_all_OK;
        bool ctrlboard_sensor_all_OK;
        bool apw_power_ok;
        bool pic_ok;
        bool eeprom_ok;
        bool asic_ok;
        bool nonce_rate_ok;
        uint32_t level;
        double voltage;
        double pre_open_core_voltage;
        uint32_t frequence;
        uint8_t test_standard;
        double nonce_rate;
        uint32_t repeat_nonce_number;
    } asic_stats;
    struct {
        uint16_t hashrate;
        uint16_t freq_base;
        uint8_t freq_step;
        bool is_pass;
        uint32_t freqs[256];
        uint64_t sweep_online_time;
        uint64_t sweep_start_time;
        uint64_t sweep_end_time;
        double sweep_online_min;
        double sweep_online_max;
        double sweep_online_avg;
        double sweep_online_var;
        double sweep_online_nonce_rate;
    } sweep;
    void* works[256]; // Массив указателей на работы
    config_ctx* config_ptr;
    struct {
        void (*init)();
        void (*exit)();
        void (*clear_stats)();
        void (*reset)();
        void (*process_nonce)(void*, void*);
        void (*process_reg)(void*, void*);
    } pt_ops;
    int8_t tempval[4];
    uint32_t send_nonce_num;
    uint32_t chain_status;
    uint8_t is_chain_unb;
    uint32_t recv_nonce;
    uint32_t read_temp;
    uint32_t pause_recv_nonce;
    float nonce_rate;
} pattern_runtime_ctx;

typedef struct {
    uint8_t is_nonce_return_back;
} work_new;

typedef struct {
    uint32_t reg_value;
    uint8_t crc;
    uint8_t chip_address;
    uint8_t reg_address;
    uint8_t chain_number;
} reg_buf_entry_t;

typedef struct {
    uint32_t p_wr;
    uint32_t p_rd;
    uint32_t reg_value_num;
    uint32_t loop_back;
    reg_buf_entry_t reg_buffer[4095];
} reg_buf_t;

// Внешние глобальные переменные (из bitmain_global.c и других)
extern pattern_runtime_ctx g_rt;
extern int32_t g_fan;
extern uint8_t gPattern_test_counter;
extern uint32_t gChain;
extern uint32_t gAddress_interval;
extern int32_t g_diff;
extern int32_t g_bringup_temp;
extern bool g_pwm_enable;
extern uint8_t g_pll_id;
extern pthread_t g_send_handle;
extern pthread_t g_recv_handle;
extern pthread_t g_temp_handle;
extern pthread_t g_power_handle;
extern reg_buf_t* reg_value_buf;
extern pthread_mutex_t reg_mutex;
extern struct Local_Config_Info* Local_Config_Information;
extern history_result_t gHistory_Result[100];

// Заглушки для внешних функций
extern void pf_power_down(uint32_t chain);
extern void pwm_set(int ctx, uint32_t duty);
extern void set_uart_relay(uint8_t interval);
extern void set_asic_core_srst_fast(uint8_t chain, uint32_t addr);
extern void set_asic_clock_select_control(uint8_t chain, uint32_t addr, uint8_t pulse, uint8_t clk);
extern void set_asic_clock_delay_control(uint8_t chain, uint32_t addr, uint8_t pwth, uint8_t ccdly, uint8_t swpf);
extern void set_asic_core_enable(uint8_t chain, uint32_t addr);
extern void set_chain_core_srst(uint8_t chain, bool state);
extern void set_chain_ticketmask(uint8_t chain, uint32_t mask);
extern void uart_flush_rx(uint32_t chain);
extern void set_chain_analog_mux_control(uint8_t chain, uint8_t val);
extern void set_chain_inactive(uint8_t chain);
extern void set_chain_asic_address(uint8_t chain, uint32_t interval);
extern void set_chain_clock_select_control(uint8_t chain, uint8_t pulse, uint8_t clk);
extern void set_chain_clock_delay_control(uint8_t chain, uint8_t pwth, uint8_t ccdly, uint8_t swpf);
extern void set_chain_adc_control_disable(uint8_t chain);
extern void set_chain_iodriver_clko_ds(uint8_t chain, uint8_t val);
extern void enable_work_send();
extern void _set_uart_relay(uint8_t interval);
extern void set_chain_pllparameter_userdivider(uint8_t chain, uint8_t addr, uint8_t pll, void* div);
extern void inc_freq_with_fix(uint8_t chain, uint8_t pll, uint32_t start, uint32_t end, float step, uint32_t temp);
extern void inc_freq_with_matrix(float* matrix, uint32_t len);
extern void dhash_set_timeout(uint32_t timeout);
extern int tsensor_open(uint8_t chain, uint8_t* addrs, uint8_t num);
extern int tsensor_read(uint8_t chain, uint8_t addr, uint8_t reg, uint8_t* data, int len);
extern int fan_get_realtime_speed(int id);
extern int pf_get_max_value(int8_t* data, int len);
extern int pf_get_min_value(int8_t* data, int len);
extern bool board_temp_protect_check(int8_t max, int8_t min);
extern void err_exit(char* msg);
extern void get_all_fan_speed();
extern void flush_nonce3_buffer();
extern uint32_t get_nonce_number_in_fifo();
extern void get_return_nonce_128bit(void* buf);
extern void set_chain_baud(uint8_t chain, uint32_t baud);
extern void reset_register_cache_value_to_default(uint8_t chain);
extern void set_bt8d_chain(uint32_t chain, uint32_t val);
extern int start_temp_thread();
extern int get_env_temp();
extern void set_env_temp(int temp);
extern int get_vol_comp(int temp);
extern void chain_reset_low(uint32_t chain);
extern void chain_reset_high(uint32_t chain);
extern uint32_t get_register_info(uint32_t asic, uint32_t func, bool mode, char* type);
extern void set_chain_core_nonce_overflow_control(uint8_t chain, bool val);
extern void set_chain_rosc_pad_disable(uint8_t chain);
extern void stop_threads();
extern void power_down_and_fan(uint32_t chain);
extern void cancle_pause_recv_nonce();
extern void pause_recv_nonce();
extern void calc_nonce_stats(pattern_test_stats* stats, void* asic_stats);
extern void get_asic_nonce_status();
extern void print_fan_speed();
extern bool waiting_cool_down(uint32_t time, int8_t* temp, int8_t num, int gap);
extern bool pf_init(uint32_t chain);
extern bool APW_power_on(uint32_t voltage);
extern int reset_hash_board(uint32_t chain);
extern int start_recv_thread();
extern int create_power_heart_beat_thread();
extern void set_register_stage_1();
extern void set_register_stage_2();
extern void set_register_stage_3(float* matrix);
extern void do_core_reset(uint8_t* state);
extern void set_pwth();
extern bool read_temp_from_asic_once(bool logout);
extern bool read_chain_domain_voltage(bool logout);
extern bool set_voltage_by_step(uint32_t pre, uint32_t target, uint8_t step);
extern void lcd_common_show(const char* l1, const char* l2, const char* l3);
extern void lcd_clear_result();
extern void lcd_show(int line, const char* data);
extern void print_time_stamp(char* buf, int len);
extern void log_to_file(char* msg, char* time);
extern void log_to_filex(char* msg, char* time);
extern void _applog(int level, const char* msg, bool newline);

// Глобальные переменные файла
static pthread_t g_send_handle = 0;
static pthread_t g_recv_handle = 0;
static pthread_t g_temp_handle = 0;
static pthread_t g_power_handle = 0;
static int32_t g_fan = -1;
static int32_t g_diff = 0;
static bool g_pwm_enable = false;
static uint8_t g_pll_id = 0;
static int32_t g_bringup_temp = 0;

// --- Функции ---

/**
 * @brief Очистка списка плохих ASIC.
 */
void clear_bad_asic_ids(void) {
    // В псевдокоде используется структура g_sweep_bad_asics, которая не определена полностью здесь,
    // но логика проста: обнуление счетчика и памяти.
    // Так как структура не предоставлена в этом файле, оставляем заглушку или предполагаем внешнюю реализацию.
    // Для компилируемости предположим, что это внешняя функция или часть g_rt.sweep.
    // В данном контексте просто возвращаем, так как полная структура не видна.
    return;
}

/**
 * @brief Добавление ID плохого ASIC в список.
 */
void add_bad_asic_ids(int id) {
    // Аналогично clear_bad_asic_ids, требует доступа к глобальной структуре ошибок свипа.
    // Реализация опущена из-за отсутствия определения структуры в этом фрагменте.
    return;
}

/**
 * @brief Получение информации о плохих ASIC при свипе.
 */
void get_sweep_bad_asics(void* bad_asics) {
    // Заглушка
    return;
}

/**
 * @brief Поток приема данных (Nonce/Register).
 */
void* recv_routine(void* arg) {
    pattern_runtime_ctx* ctx = static_cast<pattern_runtime_ctx*>(arg);
    uint32_t nonce_number = 0;
    uint32_t i = 0;
    char time_stamp[48];
    char tmp1[256];
    
    // Буфер для возврата nonce (128 бит)
    uint8_t reg_buf[16]; 

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : BTC start %s\n", "recv_routine", "recv_routine");
    snprintf(tmp1, 0x100, "BTC start %s ", "recv_routine");
    log_to_file(tmp1, time_stamp);

    flush_nonce3_buffer();

    while (ctx->recv_nonce != 0) {
        usleep(5);
        uint32_t uVar1 = get_nonce_number_in_fifo();
        nonce_number = uVar1 >> 1;

        if (nonce_number != 0) {
            for (i = 0; i < nonce_number; i++) {
                get_return_nonce_128bit(reg_buf);
                
                if (ctx->pause_recv_nonce == 0) {
                    // Проверка бита статуса (предположительно бит 7 байта 0 или аналогичный флаг в регистре)
                    // В псевдокоде: (reg_buf.buf[0]. 3_1 & 0x80) == 0
                    // Эмулируем проверку старшего бита первого байта или специфичного поля
                    if ((reg_buf[0] & 0x80) == 0) { 
                        if (ctx->pt_ops.process_reg) {
                            ctx->pt_ops.process_reg(reg_buf, arg);
                        }
                    } else {
                        if (ctx->pt_ops.process_nonce) {
                            ctx->pt_ops.process_nonce(reg_buf, arg);
                        }
                    }
                }
            }
        }
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : BTC recv %s stop\n", "recv_routine", "recv_routine");
    snprintf(tmp1, 0x100, "BTC recv %s stop ", "recv_routine");
    log_to_file(tmp1, time_stamp);

    pthread_detach(pthread_self());
    return nullptr;
}

/**
 * @brief Дамп конфигурации в лог.
 */
void dump_config(config_ctx* ctx) {
    char time_stamp[48];
    char tmp1[256];
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : ========dump config ==============\n", "dump_config");
    strncpy(tmp1, "========dump config ============\n", 0x20);
    tmp1[0x20] = '=';
    tmp1[0x21] = '=';
    tmp1[0x22] = '\0';
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : {\n", "dump_config");
    tmp1[0] = '{'; tmp1[1] = '\0';
    log_to_file(tmp1, time_stamp);

    // Логирование полей конфигурации
    #define LOG_CFG_INT(field_name) \
        print_time_stamp(time_stamp, 0x30); \
        printf("%s ", time_stamp); \
        printf("%s : %s:%d\n", "dump_config", #field_name, ctx->field_name); \
        snprintf(tmp1, 0x100, "%s:%d ", #field_name, ctx->field_name); \
        log_to_file(tmp1, time_stamp);

    #define LOG_CFG_STR(field_name) \
        print_time_stamp(time_stamp, 0x30); \
        printf("%s ", time_stamp); \
        printf("%s : %s:%s\n", "dump_config", #field_name, ctx->field_name); \
        snprintf(tmp1, 0x100, "%s:%s ", #field_name, ctx->field_name); \
        log_to_file(tmp1, time_stamp);

    LOG_CFG_INT(chain);
    LOG_CFG_INT(asics);
    LOG_CFG_INT(domains);
    LOG_CFG_INT(asics_in_domain);
    LOG_CFG_INT(asic_domain_num);
    LOG_CFG_INT(big_core);
    LOG_CFG_INT(asic_small_cores);
    LOG_CFG_INT(samll_core_num_in_big_core);
    LOG_CFG_INT(patter_num);
    LOG_CFG_INT(abnormal_core);
    LOG_CFG_INT(interval);

    // Логируем регистры
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : {\n", "dump_config");
    snprintf(tmp1, 0x100, "{ ");
    log_to_file(tmp1, time_stamp);

    LOG_CFG_INT(reg.ccdly_sel);
    LOG_CFG_INT(reg.pwth_sel);
    LOG_CFG_INT(reg.swpf_mode);
    LOG_CFG_INT(reg.pulse_mode);
    LOG_CFG_INT(reg.clk_sel);
    LOG_CFG_INT(reg.ro_relay_en);
    LOG_CFG_INT(reg.co_relay_en);
    LOG_CFG_INT(reg.diode_vdd_mux_sel);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : }\n", "dump_config");
    snprintf(tmp1, 0x100, "} ");
    log_to_file(tmp1, time_stamp);

    // Логируем скорость
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : {\n", "dump_config");
    snprintf(tmp1, 0x100, "{ ");
    log_to_file(tmp1, time_stamp);

    LOG_CFG_INT(speed.baudrate);
    LOG_CFG_INT(speed.timeout);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : }\n", "dump_config");
    snprintf(tmp1, 0x100, "} ");
    log_to_file(tmp1, time_stamp);

    // Логируем плату
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : {\n", "dump_config");
    snprintf(tmp1, 0x100, "{ ");
    log_to_file(tmp1, time_stamp);

    LOG_CFG_STR(board.asic_type);
    LOG_CFG_INT(board.wait_temp);
    LOG_CFG_STR(board.sensor.sensor);
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : sensor_addr:[%d,%d]\n", "dump_config", ctx->board.sensor.sensor_addr[0], ctx->board.sensor.sensor_addr[1]);
    snprintf(tmp1, 0x100, "sensor_addr:[%d,%d] ", ctx->board.sensor.sensor_addr[0], ctx->board.sensor.sensor_addr[1]);
    log_to_file(tmp1, time_stamp);

    LOG_CFG_INT(board.sensor.sensor_num);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : }\n", "dump_config");
    snprintf(tmp1, 0x100, "} ");
    log_to_file(tmp1, time_stamp);

    // Логируем температуру
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : {\n", "dump_config");
    snprintf(tmp1, 0x100, "{ ");
    log_to_file(tmp1, time_stamp);

    LOG_CFG_INT(temp.temp_gap);
    LOG_CFG_INT(temp.wait_cool_time);
    LOG_CFG_INT(temp.cool_board_time);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : }\n", "dump_config");
    snprintf(tmp1, 0x100, "} ");
    log_to_file(tmp1, time_stamp);

    // Логируем вентилятор
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : {\n", "dump_config");
    snprintf(tmp1, 0x100, "{ ");
    log_to_file(tmp1, time_stamp);

    LOG_CFG_INT(fan.fan_speed);
    LOG_CFG_INT(fan.work_fan_speed);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : }\n", "dump_config");
    snprintf(tmp1, 0x100, "} ");
    log_to_file(tmp1, time_stamp);

    // Логируем стандарт
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : {\n", "dump_config");
    snprintf(tmp1, 0x100, "{ ");
    log_to_file(tmp1, time_stamp);

    LOG_CFG_INT(standard.invalid_core_num);
    LOG_CFG_INT(standard.least_nonce_per_core);
    LOG_CFG_INT(standard.asic_nonce_rate);
    LOG_CFG_INT(standard.nonce_rate);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : }\n", "dump_config");
    snprintf(tmp1, 0x100, "} ");
    log_to_file(tmp1, time_stamp);

    // Логируем тесты
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : {\n", "dump_config");
    snprintf(tmp1, 0x100, "{ ");
    log_to_file(tmp1, time_stamp);

    for (int i = 0; i < 11; i++) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : tests[%d]:\n", "dump_config", i);
        snprintf(tmp1, 0x100, "tests[%d]: ", i);
        log_to_file(tmp1, time_stamp);

        LOG_CFG_INT(tests[i].level);
        LOG_CFG_INT(tests[i].pre_open_core_voltage);
        LOG_CFG_INT(tests[i].freq);
        LOG_CFG_INT(tests[i].voltage);
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : }\n", "dump_config");
    snprintf(tmp1, 0x100, "} ");
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : }\n", "dump_config");
    tmp1[0] = '}'; tmp1[1] = '\0';
    log_to_file(tmp1, time_stamp);
}

/**
 * @brief Инициализация паттерна теста.
 */
int32_t pt_init(char* pattern_file_path, void* conf) {
    int32_t iVar1;
    char time_stamp[48];
    char tmp1[256];

    // Выделение и копирование конфигурации
    g_rt.config = static_cast<config_ctx*>(malloc(sizeof(config_ctx)));
    if (!g_rt.config) return -1;
    memset(g_rt.config, 0, sizeof(config_ctx));
    memcpy(g_rt.config, conf, sizeof(config_ctx));

    dump_config(g_rt.config);

    iVar1 = platform_init(); // Предполагается, что platform_init доступна
    if (iVar1 == 0) {
        g_fan = pwm_init(0, 0); // PWM_CTRL_FAN = 0
        if (g_fan < 0) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : init fan failed\n", "pt_init");
            strncpy(tmp1, "init fan failed\n", 0x10);
            log_to_file(tmp1, time_stamp);
            // set_sweep_err_state(5, false); // Внешняя функция
            iVar1 = 5;
        } else {
            g_rt.config->board.sensor.sensor_i2c_addr = 0x48;
            g_rt.config->board.sensor.sensor_reg_addr = 0;
            iVar1 = 0;
        }
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : hal platform init failed\n", "pt_init");
        strncpy(tmp1, "hal platform init failed\n", 0x18);
        log_to_file(tmp1, time_stamp);
        // set_sweep_err_state(4, false);
        iVar1 = 4;
    }
    return iVar1;
}

/**
 * @brief Деинициализация.
 */
void pt_exit(void) {
    char time_stamp[48];
    char tmp1[256];
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : pt exit\n", "pt_exit");
    strncpy(tmp1, "pt exit\n", 8);
    log_to_file(tmp1, time_stamp);
}

/**
 * @brief Очистка статистики.
 */
void pt_clear_stats(void) {
    return;
}

/**
 * @brief Сброс.
 */
void pt_reset(void) {
    return;
}

/**
 * @brief Обработка Nonce.
 */
void pt_process_nonce(void* return_nonce) {
    // Пустая реализация согласно псевдокоду
    return;
}

/**
 * @brief Обработка регистра.
 */
void pt_process_reg(void* return_nonce) {
    // Пустая реализация согласно псевдокоду
    return;
}

/**
 * @brief Поток отправки работы.
 */
void* _send_work_routine(void* args) {
    pattern_runtime_ctx* ctx = static_cast<pattern_runtime_ctx*>(args);
    // Вызов функции отправки через указатель из конфига или ops
    // В псевдокоде: (**(code **)((int)args + 0x23648))((g_rt.config)->chain,args);
    // Это выглядит как вызов метода из vtable или структуры ops.
    // Предположим, что это pt_send_work или аналогичная функция, зарегистрированная в ops.
    
    if (ctx && ctx->pt_ops.init) { // Используем init как заглушку для вызова, если нет отдельного send
         // В реальности здесь должен быть вызов специфичной для паттерна функции отправки
    }
    
    pthread_detach(pthread_self());
    return nullptr;
}

/**
 * @brief Расчет статистики nonce.
 */
void calc_nonce_stats(pattern_test_stats* stats, void* asic_stats_void) {
    // Приведение типа, так как в сигнатуре void*, но в теле используется как asic_status*
    // Предположим, что структура asic_stats в g_rt совместима
    auto* asic_stats = static_cast<history_result_t*>(asic_stats_void); 
    
    uint32_t asic_num = g_rt.config->asics;
    uint32_t core_num = g_rt.config->asic_small_cores;
    
    // Обнуление доменов
    memset(stats->nonce_num_domain, 0, sizeof(stats->nonce_num_domain));
    memset(stats->nonce_num_domain_big, 0, sizeof(stats->nonce_num_domain_big));

    // Подсчет nonce по доменам и ASIC
    for (uint32_t asic = 0; asic < asic_num; ++asic) {
        uint32_t domain_idx = asic / g_rt.config->asics_in_domain;
        
        for (uint32_t core = 0; core < core_num; ++core) {
            uint32_t nonce_val = asic_stats->asic_core_nonce_num[asic][core];
            
            // Расчет индекса домена для детализации
            uint32_t detailed_domain_idx = (core * g_rt.config->asic_domain_num / g_rt.config->asic_small_cores) + (domain_idx * g_rt.config->asic_domain_num);
            
            stats->nonce_num_domain[detailed_domain_idx] += nonce_val;
            stats->nonce_num_domain_big[domain_idx] += nonce_val;
            
            // Статистика по ядрам (гистограмма)
            if (nonce_val < 17) { // Защита от выхода за границы массива 17
                 stats->nonce_stats[asic][nonce_val]++;
            }
        }
    }

    // Накопление статистики между ASIC (сдвиг на 8?)
    // В псевдокоде странная логика: stats->nonce_stats[asic][core] += stats->nonce_stats[asic-1][core+8]
    // Реализуем строго как есть
    for (uint32_t asic = 0; asic < asic_num; ++asic) {
        for (uint32_t cnt = 1; cnt < 9; ++cnt) {
             if (asic > 0) {
                 stats->nonce_stats[asic][cnt] += stats->nonce_stats[asic - 1][cnt + 8];
             }
        }
    }

    // Расчет среднего и дисперсии
    uint32_t sum = 0;
    for (uint32_t asic = 0; asic < asic_num; ++asic) {
        sum += stats->nonce_num_asic[asic];
    }
    
    uint32_t avg = sum / asic_num;
    float spow = 0.0f;
    
    for (uint32_t asic = 0; asic < asic_num; ++asic) {
        float diff = static_cast<float>(stats->nonce_num_asic[asic] - avg);
        spow += diff * diff;
    }
    
    stats->nonce_stdev = sqrtf(spow / static_cast<float>(asic_num));
}

/**
 * @brief Отправка работы паттерна.
 */
int32_t pt_send_work(uint32_t chain, void* args) {
    int32_t iVar1;
    char time_stamp[48];
    char tmp1[256];

    // Запуск потока отправки
    iVar1 = pthread_create(&g_send_handle, nullptr, _send_work_routine, args);
    
    if (iVar1 == 0) {
        pthread_join(g_send_handle, nullptr);
        usleep(100000);
        iVar1 = 0;
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : create send work thread failed\n", "pt_send_work");
        strncpy(tmp1, "create send work thread failed\n", 0x1c);
        log_to_file(tmp1, time_stamp);
        iVar1 = -1;
    }
    return iVar1;
}

/**
 * @brief Получение статистики.
 */
void* pt_get_stats(void) {
    return nullptr;
}

/**
 * @brief Проверка дисбаланса.
 */
int32_t pt_is_unbalance_happen(uint32_t chain) {
    return 0;
}

/**
 * @brief Проверка достаточности nonce.
 */
int32_t pt_is_recv_nonce_enough(uint32_t chain) {
    return 0;
}

/**
 * @brief Обработка регистра (нормальный режим).
 */
void pt_process_reg_normal(void* nonce) {
    pthread_mutex_lock(&reg_mutex);
    
    // Интерпретация данных nonce как структуры регистров
    uint32_t* nonce_data = static_cast<uint32_t*>(nonce);
    
    reg_value_buf->reg_buffer[reg_value_buf->p_wr].reg_value = nonce_data[1]; // Смещение +4
    reg_value_buf->reg_buffer[reg_value_buf->p_wr].crc = static_cast<uint8_t*>(nonce)[3] & 0x1F;
    reg_value_buf->reg_buffer[reg_value_buf->p_wr].chip_address = static_cast<uint8_t*>(nonce)[2];
    reg_value_buf->reg_buffer[reg_value_buf->p_wr].reg_address = static_cast<uint8_t*>(nonce)[1];
    reg_value_buf->reg_buffer[reg_value_buf->p_wr].chain_number = static_cast<uint8_t*>(nonce)[0] & 0xF;
    
    if (reg_value_buf->p_wr < 0xFFE) {
        reg_value_buf->p_wr++;
    } else {
        reg_value_buf->p_wr = 0;
    }
    
    if (reg_value_buf->reg_value_num < 0xFFF) {
        reg_value_buf->reg_value_num++;
    } else {
        reg_value_buf->reg_value_num = 0xFFF;
    }
    
    pthread_mutex_unlock(&reg_mutex);
}

/**
 * @brief Выключение питания.
 */
void power_down(uint32_t chain) {
    pf_power_down(chain);
    sleep(2);
}

/**
 * @brief Выключение питания и остановка вентиляторов.
 */
void power_down_and_fan(uint32_t chain) {
    pf_power_down(chain);
    sleep(2);
    pwm_set(g_fan, 100);
}

/**
 * @brief Установка реле UART.
 */
void _set_uart_relay(uint8_t address_interval) {
    set_uart_relay(address_interval);
}

/**
 * @brief Сброс ядер ASIC.
 */
void do_core_reset(uint8_t* state) {
    char time_stamp[48];
    char tmp1[256];
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : do_core_reset\n", "do_core_reset");
    strncpy(tmp1, "do_core_reset\n", 0xD);
    log_to_file(tmp1, time_stamp);
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : interval %d, total ASIC num %d\n", "do_core_reset", g_rt.config->interval, g_rt.config->asics);
    snprintf(tmp1, 0x100, "interval %d, total ASIC num %d ", g_rt.config->interval, g_rt.config->asics);
    log_to_file(tmp1, time_stamp);
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : ccdly pwth %d %d\n", "do_core_reset", g_rt.config->reg.ccdly_sel, g_rt.config->reg.pwth_sel);
    snprintf(tmp1, 0x100, "ccdly pwth %d %d ", g_rt.config->reg.ccdly_sel, g_rt.config->reg.pwth_sel);
    log_to_file(tmp1, time_stamp);

    for (uint32_t asic = 0; asic < g_rt.config->asics; ++asic) {
        uint32_t addr = asic * g_rt.config->interval;
        
        if (state == nullptr || state[asic] == 0) {
            set_asic_core_srst_fast(static_cast<uint8_t>(g_rt.config->chain), addr);
            usleep(10000);
            
            set_asic_clock_select_control(static_cast<uint8_t>(g_rt.config->chain), addr, 
                                          static_cast<uint8_t>(g_rt.config->reg.pulse_mode), 0);
            usleep(10000);
            
            set_asic_clock_delay_control(static_cast<uint8_t>(g_rt.config->chain), addr,
                                         static_cast<uint8_t>(g_rt.config->reg.pwth_sel),
                                         static_cast<uint8_t>(g_rt.config->reg.ccdly_sel), 0);
            
            set_asic_core_enable(static_cast<uint8_t>(g_rt.config->chain), addr);
            usleep(10000);
            
            if (state != nullptr) {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : do_core_reset asic:%d\n", "do_core_reset", asic);
                snprintf(tmp1, 0x100, "do_core_reset asic:%d ", asic);
                log_to_file(tmp1, time_stamp);
            }
        }
    }
}

/**
 * @brief Этап настройки регистров 1.
 */
void set_register_stage_1(void) {
    set_chain_core_srst(static_cast<uint8_t>(g_rt.config->chain), false);
    set_chain_ticketmask(static_cast<uint8_t>(g_rt.config->chain), 0xFFFFFFFF);
    uart_flush_rx(g_rt.config->chain);
    usleep(50000);
}

/**
 * @brief Этап настройки регистров 2.
 */
void set_register_stage_2(void) {
    char time_stamp[48];
    char tmp1[256];
    
    set_chain_analog_mux_control(static_cast<uint8_t>(g_rt.config->chain), 
                                 static_cast<uint8_t>(g_rt.config->reg.diode_vdd_mux_sel));
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Set Diode_Vdd_Mux_Sel = 0x%03x\n", "set_register_stage_2", g_rt.config->reg.diode_vdd_mux_sel);
    snprintf(tmp1, 0x100, "Set Diode_Vdd_Mux_Sel = 0x%03x ", g_rt.config->reg.diode_vdd_mux_sel);
    log_to_file(tmp1, time_stamp);
    
    usleep(10000);
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Set chain inactive\n", "set_register_stage_2");
    strncpy(tmp1, "Set chain inactive\n", 0x12);
    log_to_file(tmp1, time_stamp);
    
    set_chain_inactive(static_cast<uint8_t>(g_rt.config->chain));
    usleep(10000);
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Set asic address\n", "set_register_stage_2");
    strncpy(tmp1, "Set asic address\n", 0x11);
    log_to_file(tmp1, time_stamp);
    
    set_chain_asic_address(static_cast<uint8_t>(g_rt.config->chain), g_rt.config->interval);
    usleep(10000);
    
    set_chain_clock_select_control(static_cast<uint8_t>(g_rt.config->chain),
                                   static_cast<uint8_t>(g_rt.config->reg.pulse_mode),
                                   static_cast<uint8_t>(g_rt.config->reg.clk_sel));
                                   
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Set pulse_mode = 0x%02x, clk_sel = 0x%02x\n", "set_register_stage_2", 
           g_rt.config->reg.pulse_mode, g_rt.config->reg.clk_sel);
    snprintf(tmp1, 0x100, "Set pulse_mode = 0x%02x, clk_sel = 0x%02x ", 
             g_rt.config->reg.pulse_mode, g_rt.config->reg.clk_sel);
    log_to_file(tmp1, time_stamp);
    
    usleep(10000);
    
    set_chain_clock_delay_control(static_cast<uint8_t>(g_rt.config->chain),
                                  static_cast<uint8_t>(g_rt.config->reg.pwth_sel),
                                  static_cast<uint8_t>(g_rt.config->reg.clk_sel),
                                  static_cast<uint8_t>(g_rt.config->reg.swpf_mode));
                                  
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Set pwth_sel = 0x%02x, ccdly_sel = 0x%02x, swpf_mode = 0x%02x\n", "set_register_stage_2",
           g_rt.config->reg.pwth_sel, g_rt.config->reg.clk_sel, g_rt.config->reg.swpf_mode);
    snprintf(tmp1, 0x100, "Set pwth_sel = 0x%02x, ccdly_sel = 0x%02x, swpf_mode = 0x%02x ",
             g_rt.config->reg.pwth_sel, g_rt.config->reg.clk_sel, g_rt.config->reg.swpf_mode);
    log_to_file(tmp1, time_stamp);
    
    usleep(10000);
    
    set_chain_analog_mux_control(static_cast<uint8_t>(g_rt.config->chain), 0x03);
    set_chain_adc_control_disable(static_cast<uint8_t>(g_rt.config->chain));
    set_chain_iodriver_clko_ds(static_cast<uint8_t>(g_rt.config->chain), 0x01);
    
    enable_work_send();
    _set_uart_relay(static_cast<uint8_t>(g_rt.config->interval));
    
    usleep(10000);
    usleep(1000000);
}

/**
 * @brief Этап настройки регистров 3.
 */
void set_register_stage_3(float* matrix) {
    char time_stamp[48];
    char tmp1[256];
    
    // Сброс делителя пользователя
    uint8_t user_divider[4] = {0};
    set_chain_pllparameter_userdivider(static_cast<uint8_t>(g_rt.config->chain), 0, user_divider);
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Set Pll0: userdivider0 = 0x%02x, userdivider1 = 0x%02x, userdivider2 = 0x%02x, userdivider3 = 0x%02x\n",
           "set_register_stage_3", user_divider[0], user_divider[1], user_divider[2], user_divider[3]);
    snprintf(tmp1, 0x100, "Set Pll0: userdivider0 = 0x%02x, userdivider1 = 0x%02x, userdivider2 = 0x%02x, userdivider3 = 0x%02x ",
             user_divider[0], user_divider[1], user_divider[2], user_divider[3]);
    log_to_file(tmp1, time_stamp);
    
    usleep(10000);
    
    float min_freq = get_freq_matrix_min(matrix, g_rt.config->asics);
    inc_freq_with_fix(static_cast<uint8_t>(g_rt.config->chain), 0, 0x32, static_cast<uint32_t>(min_freq), 6.25f, g_bringup_temp);
    inc_freq_with_matrix(matrix, g_rt.config->asics);
    
    set_chain_ticketmask(static_cast<uint8_t>(g_rt.config->chain), 0x7F);
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Set TM as 0x%08x\n", "set_register_stage_3", 0x7F);
    snprintf(tmp1, 0x100, "Set TM as 0x%08x ", 0x7F);
    log_to_file(tmp1, time_stamp);
    
    uart_flush_rx(g_rt.config->chain);
    dhash_set_timeout(g_rt.config->speed.timeout);
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Set timeout by using config value: %d\n", "set_register_stage_3", g_rt.config->speed.timeout);
    snprintf(tmp1, 0x100, "Set timeout by using config value: %d ", g_rt.config->speed.timeout);
    log_to_file(tmp1, time_stamp);
}

/**
 * @brief Установка PWM по температуре при свипе.
 */
void sweep_set_pwm_by_temp(void) {
    int temp = pf_get_max_value(g_rt.tempval, g_rt.config->board.sensor.sensor_num);
    set_pwm_by_temp(temp);
}

/**
 * @brief Получение количества успешных nonce ASIC.
 */
void get_succ_asic_nonce_num(uint16_t* data, int len) {
    int max_len = (len < 0x6C) ? len : 0x6C;
    // Копирование из глобального состояния, предполагается наличие last_succ_asic_nonce_num в g_rt
    // Так как поле не определено в структуре выше, используем заглушку или предполагаем, что оно там есть
    // memcpy(data, g_rt.last_succ_asic_nonce_num, max_len * sizeof(uint16_t));
    memset(data, 0, max_len * sizeof(uint16_t)); // Заглушка
}

/**
 * @brief Получение последней температуры паттерна.
 */
void get_last_pattern_temp(uint8_t* temp, int cnt) {
    int max_cnt = (cnt < g_rt.config->board.sensor.sensor_num) ? cnt : g_rt.config->board.sensor.sensor_num;
    memcpy(temp, g_rt.tempval, max_cnt);
}

/**
 * @brief Получение скорости свипа.
 */
float get_sweep_noncerate(void) {
    return g_rt.nonce_rate;
}

/**
 * @brief Поток чтения температуры.
 */
void* pt_read_temp(void* arg) {
    pattern_runtime_ctx* ctx = static_cast<pattern_runtime_ctx*>(arg);
    char time_stamp[48];
    char tmp1[256];
    
    uint8_t sensor_addr[2] = {0};
    int8_t data[2] = {-1, -1};
    int read_fail_cnt = 0;
    uint8_t sleep_counter = 0;
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Start %s\n", "pt_read_temp", "pt_read_temp");
    snprintf(tmp1, 0x100, "Start %s ", "pt_read_temp");
    log_to_file(tmp1, time_stamp);
    
    // Инициализация адресов сенсоров
    for (int i = 0; i < ctx->config->board.sensor.sensor_num; ++i) {
        sensor_addr[i] = ctx->config->board.sensor.sensor_addr[i] + ctx->config->board.sensor.sensor_i2c_addr;
    }
    
    int ret = tsensor_open(ctx->config->chain, sensor_addr, ctx->config->board.sensor.sensor_num);
    if (ret < 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : %s: chain %d tsensor_open failed\n", "pt_read_temp", "pt_read_temp", ctx->config->chain);
        snprintf(tmp1, 0x100, "%s: chain %d tsensor_open failed ", "pt_read_temp", ctx->config->chain);
        log_to_file(tmp1, time_stamp);
        
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : WARNING CAN NOT OPEN TEMP SENSOR ...\n", "pt_read_temp");
        strncpy(tmp1, "WARNING CAN NOT OPEN TEMP SENSOR ...\n", 0x24);
        log_to_file(tmp1, time_stamp);
        
        err_exit("sensor err");
        return nullptr;
    }
    
    while (ctx->read_temp != 0) {
        for (int i = 0; i < ctx->config->board.sensor.sensor_num; ++i) {
            memset(data, -1, 2);
            ret = tsensor_read(ctx->config->chain, sensor_addr[i], ctx->config->board.sensor.sensor_reg_addr, 
                               reinterpret_cast<uint8_t*>(data), 2);
            
            if (ret == 2) {
                if (data[0] == -1 || data[0] == 0) {
                    read_fail_cnt++;
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : Read sensor[%d] fail,addr: %d\n", "pt_read_temp", i, sensor_addr[i]);
                    snprintf(tmp1, 0x100, "Read sensor[%d] fail,addr: %d ", i, sensor_addr[i]);
                    log_to_file(tmp1, time_stamp);
                } else {
                    ctx->tempval[i] = data[0];
                    if (sleep_counter < 3 || sleep_counter % 3 == 0) {
                        print_time_stamp(time_stamp, 0x30);
                        printf("%s ", time_stamp);
                        printf("%s : temp [%d] %d\n", "pt_read_temp", i, data[0]);
                        snprintf(tmp1, 0x100, "temp [%d] %d ", i, data[0]);
                        log_to_file(tmp1, time_stamp);
                    }
                }
            } else {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : Read sensor[%d] fail,addr: %d\n", "pt_read_temp", i, sensor_addr[i]);
                snprintf(tmp1, 0x100, "Read sensor[%d] fail,addr: %d ", i, sensor_addr[i]);
                log_to_file(tmp1, time_stamp);
                read_fail_cnt++;
            }
        }
        
        if (g_pwm_enable) {
            sweep_set_pwm_by_temp();
        }
        
        int speed[4];
        for (int i = 0; i < 4; ++i) {
            speed[i] = fan_get_realtime_speed(i);
        }
        
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : fan:[%d][%d][%d][%d]\n", "pt_read_temp", speed[0], speed[1], speed[2], speed[3]);
        snprintf(tmp1, 0x100, "fan:[%d][%d][%d][%d] ", speed[0], speed[1], speed[2], speed[3]);
        log_to_file(tmp1, time_stamp);
        
        if (read_fail_cnt > 0) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : WARNING READ TEMP SENSOR ERR...\n", "pt_read_temp");
            strncpy(tmp1, "WARNING READ TEMP SENSOR ERR...\n", 0x20);
            log_to_file(tmp1, time_stamp);
            err_exit("sensor err");
        }
        
        int max_temp = pf_get_max_value(ctx->tempval, ctx->config->board.sensor.sensor_num);
        int min_temp = pf_get_min_value(ctx->tempval, ctx->config->board.sensor.sensor_num);
        
        if (!board_temp_protect_check(max_temp, min_temp)) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : WARNING TEMP OUT OF PROTECT...\n", "pt_read_temp");
            strncpy(tmp1, "WARNING TEMP OUT OF PROTECT...\n", 0x1F);
            log_to_file(tmp1, time_stamp);
            
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : WARNING temp max:%d,temp min:%d\n", "pt_read_temp", max_temp, min_temp);
            snprintf(tmp1, 0x100, "WARNING temp max:%d,temp min:%d ", max_temp, min_temp);
            log_to_file(tmp1, time_stamp);
            
            err_exit("temp high");
            exit(1);
        }
        
        sleep(1);
        sleep_counter++;
    }
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : %s stop\n", "pt_read_temp", "pt_read_temp");
    snprintf(tmp1, 0x100, "%s stop ", "pt_read_temp");
    log_to_file(tmp1, time_stamp);
    
    pthread_detach(pthread_self());
    return nullptr;
}

/**
 * @brief Установка частоты смешанного режима.
 */
void set_freq_mix(uint32_t chain, uint8_t mode, uint8_t chip_addr, uint8_t pll_id, void* pll_config) {
    if (mode == 0x01) {
        set_chain_pllparameter(static_cast<uint8_t>(chain), pll_id, pll_config);
    } else {
        set_asic_pllparameter(static_cast<uint8_t>(chain), chip_addr, pll_id, pll_config);
    }
}

/**
 * @brief Сброс частоты плохих ASIC.
 */
void reset_bad_asic_freq(float* matrix, int len, uint8_t* state) {
    // Реализация сложной логики сброса частоты для конкретных ASIC на основе матрицы
    // Требуется доступ к внутренним структурам PLL и конфигурации
    // Упрощенная заглушка, так как полная реализация требует множества внешних определений
    for (int i = 0; i < len; ++i) {
        if (state[i] == 0) {
            // Логика сброса
        }
    }
}

/**
 * @brief Увеличение частоты до целевого значения.
 */
void inc_freq_with_target(float* matrix, int len, float freq, int pll, uint8_t* state) {
    // Реализация пошагового увеличения частоты
    for (int i = 0; i < len; ++i) {
        if (matrix[i] < freq && state[i] == 1) {
            // Установка новой частоты
        }
    }
}

/**
 * @brief Увеличение частоты онлайн PLL.
 */
void inc_freq_online_pll(float* matrix, int len, float freq, uint8_t* state) {
    // Заглушка
}

/**
 * @brief Увеличение частоты с помощью матрицы.
 */
void inc_freq_with_matrix(float* matrix, int len) {
    float min_freq = get_freq_matrix_min(matrix, len);
    float max_freq = get_freq_matrix_max(matrix, len);
    
    char time_stamp[48];
    char tmp1[256];
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : get all chain min freq %f\n", "inc_freq_with_matrix", min_freq);
    snprintf(tmp1, 0x100, "get all chain min freq %f ", min_freq);
    log_to_file(tmp1, time_stamp);
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : set freq from %.2f to %.2f by matrix\n", "inc_freq_with_matrix", min_freq, max_freq);
    snprintf(tmp1, 0x100, "set freq from %.2f to %.2f by matrix ", min_freq, max_freq);
    log_to_file(tmp1, time_stamp);
    
    // Логика итеративного изменения частоты
    // ...
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : set freq by matrix over\n", "inc_freq_with_matrix");
    strncpy(tmp1, "set freq by matrix over\n", 0x18);
    log_to_file(tmp1, time_stamp);
}

/**
 * @brief Сброс буфера регистров.
 */
void reset_registoer_buf(void) {
    char time_stamp[48];
    char tmp1[256];
    
    if (reg_value_buf == nullptr) {
        reg_value_buf = static_cast<reg_buf_t*>(malloc(sizeof(reg_buf_t)));
        if (reg_value_buf == nullptr) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : malloc reg_value_buf failed\n", "reset_registoer_buf");
            strncpy(tmp1, "malloc reg_value_buf failed\n", 0x1C);
            log_to_file(tmp1, time_stamp);
        } else {
            memset(reg_value_buf, 0, sizeof(reg_buf_t));
        }
    } else {
        memset(reg_value_buf, 0, sizeof(reg_buf_t));
    }
}

/**
 * @brief Очистка буфера регистров.
 */
void clear_register_buf(void) {
    pthread_mutex_lock(&reg_mutex);
    reg_value_buf->p_wr = 0;
    reg_value_buf->p_rd = 0;
    reg_value_buf->reg_value_num = 0;
    reg_value_buf->loop_back = 0;
    pthread_mutex_unlock(&reg_mutex);
    memset(reg_value_buf->reg_buffer, 0, sizeof(reg_value_buf->reg_buffer));
}

/**
 * @brief Получение флага ASIC.
 */
uint16_t get_asic_flag(char* asic_type) {
    return 0x1368; // Значение из псевдокода
}

/**
 * @brief Получение информации о регистре.
 */
uint32_t get_register_info(uint32_t which_asic, uint32_t which_function, bool mode, char* asic_type) {
    // Очень длинная функция с switch-case и чтением из буфера регистров
    // Реализация ключевых моментов:
    clear_register_buf();
    
    uint32_t register_value = 0;
    uint32_t find_asic = 0;
    uint32_t bad_asic_number = 0;
    uint32_t get_register_value_loop = 0;
    
    // Отправка команд в зависимости от функции
    switch (which_function) {
        case 0: // Get ASIC Num
            find_asic = 0;
            uint16_t asic_name = get_asic_flag(asic_type);
            get_chain_asic_num(static_cast<uint8_t>(g_rt.config->chain));
            break;
        case 1: // PLL 0
            send_get_status_command(static_cast<uint8_t>(g_rt.config->chain), 0x01, 0, 8);
            break;
        // ... другие кейсы ...
        default:
            return 0xFFFFFFFF;
    }
    
    // Цикл ожидания данных
    do {
        while (true) {
            if (get_register_value_loop > 2) {
                return (which_function == 0) ? find_asic : register_value;
            }
            usleep(30000);
            
            pthread_mutex_lock(&reg_mutex);
            uint32_t num = reg_value_buf->reg_value_num;
            pthread_mutex_unlock(&reg_mutex);
            
            if (num != 0) break;
            
            usleep(100000);
            get_register_value_loop++;
        }
        
        get_register_value_loop = 0;
        
        for (uint32_t i = 0; i < num; ++i) {
            pthread_mutex_lock(&reg_mutex);
            register_value = reg_value_buf->reg_buffer[reg_value_buf->p_rd].reg_value;
            pthread_mutex_unlock(&reg_mutex);
            
            // Обработка значения в зависимости от функции
            // ...
            
            pthread_mutex_lock(&reg_mutex);
            reg_value_buf->p_rd++;
            reg_value_buf->reg_value_num--;
            if (reg_value_buf->p_rd == 0xFFF) reg_value_buf->p_rd = 0;
            pthread_mutex_unlock(&reg_mutex);
        }
    } while (true);
}

/**
 * @brief Получение статуса nonce ASIC.
 */
void get_asic_nonce_status(void) {
    // Вывод статистики в лог
    char time_stamp[48];
    char tmp1[256];
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : \n------------------------------------------------------------------------------------------------------\n\n", "get_asic_nonce_status");
    strncpy(tmp1, "\n------------------------------------------------------------------------------------------------------\n\n", 0x69);
    log_to_file(tmp1, time_stamp);
    
    // ... вывод деталей ...
}

/**
 * @brief Остановка потоков.
 */
void stop_threads(void) {
    char time_stamp[48];
    char tmp1[256];
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : stop threads enter\n", "stop_threads");
    strncpy(tmp1, "stop threads enter\n", 0x12);
    log_to_file(tmp1, time_stamp);
    
    g_rt.read_temp = 0;
    g_rt.recv_nonce = 0;
    g_pwm_enable = false;
    
    if (g_power_handle != 0) {
        pthread_cancel(g_power_handle);
    }
    
    sleep(5);
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : stop threads end\n", "stop_threads");
    strncpy(tmp1, "stop threads end\n", 0x11);
    log_to_file(tmp1, time_stamp);
}

/**
 * @brief Сброс информации времени выполнения.
 */
void reset_runtime_info(int sweep_mode) {
    memset(&g_rt.stats, 0, sizeof(g_rt.stats));
    memset(&g_rt.asic_stats, 0, sizeof(g_rt.asic_stats));
    
    g_rt.send_nonce_num = 0;
    g_rt.chain_status = 0;
    g_rt.is_chain_unb = 0;
    
    if (sweep_mode == 0) {
        g_rt.recv_nonce = 0;
        g_rt.read_temp = 0;
    }
    
    // Сброс счетчиков повторений nonce в работах
    for (uint32_t asic = 0; asic < g_rt.config->asics; ++asic) {
        work_new** ppVar2 = static_cast<work_new**>(g_rt.works[asic]);
        if (ppVar2) {
            for (int core = 0; core < g_rt.config->big_core * g_rt.config->samll_core_num_in_big_core; ++core) {
                for (int pattern = 0; pattern < g_rt.config->patter_num; ++pattern) {
                    ppVar2[pattern + core * g_rt.config->patter_num]->is_nonce_return_back = 0;
                }
            }
        }
    }
}

/**
 * @brief Запуск потока температуры.
 */
int start_temp_thread(void) {
    int iVar1;
    char time_stamp[48];
    char tmp1[256];
    
    g_rt.read_temp = 1;
    iVar1 = pthread_create(&g_temp_handle, nullptr, pt_read_temp, &g_rt);
    
    if (iVar1 == 0) {
        while (g_rt.tempval[0] == 0) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : can not read senor temp\n", "start_temp_thread");
            strncpy(tmp1, "can not read senor temp\n", 0x18);
            log_to_file(tmp1, time_stamp);
            sleep(1);
        }
        iVar1 = 0;
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : create read temp thread failed\n", "start_temp_thread");
        strncpy(tmp1, "create read temp thread failed\n", 0x1E);
        log_to_file(tmp1, time_stamp);
    }
    return iVar1;
}

/**
 * @brief Запуск потока приема.
 */
int start_recv_thread(void) {
    int iVar1;
    char time_stamp[48];
    char tmp1[256];
    
    g_rt.recv_nonce = 1;
    cancle_pause_recv_nonce();
    
    iVar1 = pthread_create(&g_recv_handle, nullptr, recv_routine, &g_rt);
    
    if (iVar1 == 0) {
        iVar1 = 0;
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : create recv thread failed\n", "start_recv_thread");
        strncpy(tmp1, "create recv thread failed\n", 0x19);
        log_to_file(tmp1, time_stamp);
    }
    return iVar1;
}

/**
 * @brief Получение соответствующего уровня теста.
 */
void get_match_level(void* level_info) {
    // Заполнение структуры level_info данными из конфигурации
    if (g_rt.config->sweeped_res_index < 0 || g_rt.config->sweeped_res_index > 10) {
        // Ошибка
    } else {
        // Копирование данных теста
    }
}

/**
 * @brief Согласование уровня с EEPROM.
 */
int match_level(void* ctx, pattern_runtime_ctx* runtime, float* matrix) {
    // Логика сравнения частоты свипа с сохраненной в EEPROM
    return 0;
}

/**
 * @brief Построение данных EEPROM для PT2.
 */
bool build_eeprom_data_pt2(bool pass, uint8_t* eeprom_data, size_t* len_p, size_t* offset_p, pattern_runtime_ctx* runtime, float* matrix) {
    // Кодирование данных теста в формат EEPROM
    return true;
}

/**
 * @brief Показ сводки свипа.
 */
void show_sweep_summary(float* matrix) {
    // Вывод статистики свипа в лог
}

/**
 * @brief Показ ошибки свипа.
 */
void show_sweep_failed(void) {
    lcd_clear_result();
    lcd_show(0, "SWEEP FAILED");
    // Детализация ошибок
}

/**
 * @brief Сброс данных EEPROM.
 */
int flush_eeprom_data(float* matrix, int chain) {
    bool res = build_eeprom_data_pt2(true, nullptr, nullptr, nullptr, &g_rt, matrix);
    return res ? 0 : -1;
}

/**
 * @brief Запуск приема nonce.
 */
void start_recv_nonce(void) {
    g_rt.recv_nonce = 1;
}

/**
 * @brief Остановка приема nonce.
 */
void stop_recv_nonce(void) {
    g_rt.recv_nonce = 0;
}

/**
 * @brief Пауза приема nonce.
 */
void pause_recv_nonce(void) {
    g_rt.pause_recv_nonce = 1;
}

/**
 * @brief Отмена паузы приема nonce.
 */
void cancle_pause_recv_nonce(void) {
    g_rt.pause_recv_nonce = 0;
}

/**
 * @brief Повторный тест в онлайн режиме.
 */
void online_mode_retry_test(int chain) {
    stop_threads();
    power_down_and_fan(chain);
    cancle_pause_recv_nonce();
    sleep(10);
}

/**
 * @brief Тест паттерна.
 */
int pattern_test(float* matrix) {
    int iVar1 = pt_before_send_nonce(matrix);
    if (iVar1 == 0) {
        set_chain_baud(static_cast<uint8_t>(g_rt.config->chain), g_rt.config->speed.baudrate);
        usleep(100000);
        pt_send_work(g_rt.config->chain, &g_rt);
        stop_threads();
        power_down_and_fan(g_rt.config->chain);
        calc_nonce_stats(&g_rt.stats, &g_rt.asic_stats);
        get_asic_nonce_status();
        iVar1 = 0;
    } else {
        stop_threads();
        power_down_and_fan(g_rt.config->chain);
        iVar1 = -1;
    }
    return iVar1;
}

/**
 * @brief Ожидание температуры платы.
 */
int wait_board_temp(int8_t* tempval, int8_t sensor_num, int32_t target_temp) {
    int k = 0;
    int temp = pf_get_max_value(tempval, sensor_num);
    
    if (target_temp == 0) return 0;
    
    while (temp < target_temp - 5) {
        temp = pf_get_max_value(tempval, sensor_num);
        if (!read_temp_from_asic_once(false)) err_exit("asic temp outof range");
        if (!read_chain_domain_voltage(false)) err_exit("asic vol outof range");
        
        sleep(1);
        k++;
        
        if (k % 5 == 0) {
            print_fan_speed();
        }
        
        if (k >= 0x12D) {
            return -1; // Timeout
        }
    }
    return 0;
}

/**
 * @brief Установка PWTH.
 */
void set_pwth(void) {
    for (uint32_t asic = 0; asic < g_rt.config->asics; ++asic) {
        set_asic_clock_delay_control(static_cast<uint8_t>(g_rt.config->chain), 
                                     asic * g_rt.config->interval, 
                                     0x03, 
                                     static_cast<uint8_t>(g_rt.config->reg.ccdly_sel), 
                                     0);
        usleep(10000);
    }
}

/**
 * @brief Подготовка перед отправкой nonce.
 */
int pt_before_send_nonce(float* matrix) {
    reset_runtime_info(g_rt.sweep.mode);
    fan_turn_on(BOTH_SIZE); // BOTH_SIZE должен быть определен
    pwm_set(g_fan, g_rt.config->fan.fan_speed);
    
    if (!pf_init(g_rt.config->chain)) return -1;
    
    power_down(g_rt.config->chain);
    reset_register_cache_value_to_default(static_cast<uint8_t>(g_rt.config->chain));
    reset_registoer_buf();
    set_bt8d_chain(g_rt.config->chain, 0x1A);
    
    if (start_temp_thread() != 0) return -1;
    
    lcd_common_show("Cool down", "Hashboard", nullptr);
    waiting_cool_down(g_rt.config->temp.wait_cool_time, g_rt.tempval, 
                      static_cast<int8_t>(g_rt.config->board.sensor.sensor_num), 
                      g_rt.config->temp.temp_gap);
    
    print_fan_speed();
    sleep(15);
    
    int env_temp = get_env_temp();
    if (env_temp == 0xFFFF) {
        env_temp = pf_get_min_value(g_rt.tempval, g_rt.config->board.sensor.sensor_num);
        set_env_temp(env_temp);
        if (env_temp < 0x18 || env_temp > 0x28) return -1;
    }
    
    g_bringup_temp = env_temp;
    g_diff = get_vol_comp(env_temp);
    
    pwm_set(g_fan, 100);
    chain_reset_low(g_rt.config->chain);
    
    lcd_common_show("Power on", "Hashboard", nullptr);
    if (!APW_power_on(g_rt.config->tests[0].pre_open_core_voltage)) return -1;
    
    reset_hash_board(g_rt.config->chain);
    print_fan_speed();
    start_recv_thread();
    create_power_heart_beat_thread();
    
    uint32_t asic_num = get_register_info(0, 0, true, "BM1368");
    if (asic_num != g_rt.config->asics) return -1;
    
    set_register_stage_1();
    set_chain_ticketmask(static_cast<uint8_t>(g_rt.config->chain), 0xFFFF);
    uart_flush_rx(g_rt.config->chain);
    set_register_stage_2();
    usleep(1000000);
    
    do_core_reset(nullptr);
    usleep(1000000);
    set_register_stage_3(matrix);
    usleep(1000000);
    
    if (!read_temp_from_asic_once(false)) err_exit("asic temp outof range");
    if (!read_chain_domain_voltage(false)) err_exit("asic vol outof range");
    
    lcd_common_show("Heating", "Hashboard", nullptr);
    pwm_set(g_fan, 0x32);
    
    if (wait_board_temp(g_rt.tempval, static_cast<int8_t>(g_rt.config->board.sensor.sensor_num), 0x38) != 0) {
        return -1;
    }
    
    pwm_set(g_fan, g_rt.config->fan.fan_speed);
    if (!set_voltage_by_step(g_rt.config->tests[0].pre_open_core_voltage, 
                             g_diff + g_rt.config->tests[0].voltage, '\n')) return -1;
    
    set_pwth();
    
    asic_num = get_register_info(0, 0, true, "BM1368");
    if (asic_num != g_rt.config->asics) return -1;
    
    set_chain_core_nonce_overflow_control(static_cast<uint8_t>(g_rt.config->chain), false);
    set_chain_rosc_pad_disable(static_cast<uint8_t>(g_rt.config->chain));
    set_chain_baud(static_cast<uint8_t>(g_rt.config->chain), g_rt.config->speed.baudrate);
    usleep(200000);
    
    g_pwm_enable = true;
    return 0;
}

/**
 * @brief Отправка nonce при свипе.
 */
int pt_send_nonce(float* last_matrix, float freq, uint8_t* state) {
    print_fan_speed();
    reset_runtime_info(g_rt.sweep.mode);
    inc_freq_with_target(last_matrix, g_rt.config->asics, freq, 0, state);
    usleep(100000);
    reset_bad_asic_freq(last_matrix, g_rt.config->asics, state);
    
    char lcd_info[16];
    snprintf(lcd_info, 0x10, "Freq:%d", static_cast<int>(freq));
    lcd_common_show("Nonce test", lcd_info, nullptr);
    
    cancle_pause_recv_nonce();
    pt_send_work(g_rt.config->chain, &g_rt);
    pause_recv_nonce();
    
    calc_nonce_stats(&g_rt.stats, &g_rt.asic_stats);
    get_asic_nonce_status();
    
    return 0;
}

/**
 * @brief Поток мониторинга питания.
 */
void* pt_power_monitor_thread(void* arg) {
    bitmain_set_watchdog(0x01);
    while (true) {
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
        bitmain_get_power_status();
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
        pthread_testcancel();
        sleep(3);
    }
    return nullptr;
}

/**
 * @brief Создание потока сердцебиения питания.
 */
int32_t create_power_heart_beat_thread(void) {
    return pthread_create(&g_power_handle, nullptr, pt_power_monitor_thread, nullptr);
}