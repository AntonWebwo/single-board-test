#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <cmath>

// --- Типы данных (на основе использования в псевдокоде) ---

typedef struct {
    uint32_t nonce;
    uint32_t blk_version;
    // Другие поля структуры nonce/format
} return_nonce_128bit_t;

typedef struct {
    uint8_t midstate[32];
    uint8_t data[12];
    // Другие поля
} work_format;

typedef struct {
    uint32_t pattern;
    uint32_t nonce_repeat;
    // Другие поля паттерна
} pattern_work;

// Внешние глобальные переменные и структуры
extern struct Local_Config_Info* Local_Config_Information;
extern uint32_t gAddress_interval;
extern uint32_t gChain;
extern uint32_t err_nonce_resp;
extern pattern_work* pattern_info_new_works[];
extern uint32_t gMax_Nonce;
extern uint32_t gTimeout_Clock;
extern int32_t gFan;
extern uint8_t gPattern_test_counter;
extern bool gFind_dummy_data;
extern uint32_t gAsic_num;
extern uint32_t gAsic_num1;
extern uint32_t gSend_Work_Num;
extern double gNonce_rate;
extern history_result_t gHistory_Result[100]; // Предполагаемая структура из global
extern pt1_ctx_t g_pt1;
extern hashboard_sn sn;
extern hashboard_qr_code qr_code;
extern uint8_t APW_power_version[16];
extern size_t gPower_version_array_size;
extern int g_env_temp;
extern uint8_t gRead_Pic_Temperature_time_interval;
extern bool g_droplevel;
extern int gVol_comp;
extern int g_eeprom_version;
extern uint32_t gTest_loop;
extern eeprom_data_format_t* ctx;

// Заглушки для внешних функций
extern void print_time_stamp(char *buf, int len);
extern void log_to_file(char *msg, char *time);
extern void _applog(int level, const char* msg, bool newline);
extern void lcd_clear_result();
extern int32_t lcd_show(int line, const char* data);
extern void lcd_common_show(const char* l1, const char* l2, const char* l3);
extern uint8_t get_asic_index_by_nonce(uint32_t nonce, uint32_t interval);
extern uint8_t get_coreid_by_nonce(uint32_t nonce);
extern int create_socket();
extern void send_nonce(int fd, char *buf, int len);
extern int g_sock;
extern FILE *fd_log;
extern char *bin2hex(unsigned char *data, int len);
extern void free(void *ptr);
extern bool BTC_check_config_information(char *asic_type);
extern void BTC_software_pattern_check_nonce(return_nonce_128bit_t *nonce_buf);
extern void BTC_mid_auto_gen_check_nonce(return_nonce_128bit_t *nonce_buf);
extern void BTC_cpugen_pattern_check_nonce(return_nonce_128bit_t *nonce_buf);
extern bool init_fpga();
extern void stop_dhash_work_gen();
extern void reset_fpga(int val);
extern void disable_bypass_mode();
extern void enable_sync_header();
extern uint32_t _get_dhash_acc_control();
extern void _set_dhash_acc_control(uint32_t val);
extern void set_dhash_midstate(int num);
extern void disable_dhash_midstate_autogen();
extern int32_t platform_init();
extern void fan_turn_on(int side);
extern int32_t pwm_set(int ctx, uint32_t duty);
extern void reset_global_variables();
extern bool check_chain();
extern void set_bt8d_chain(uint32_t chain, uint32_t baud);
extern void reset_register_cache_value_to_default(uint8_t chain);
extern bool Check_EEPROM_compat(uint32_t len);
extern void chain_reset_low(uint32_t chain);
extern void chain_reset_high(uint32_t chain);
extern bool temp_sensor_check();
extern bool APW_power_on(uint32_t voltage);
extern void usleep(useconds_t usec);
extern void sleep(unsigned int seconds);
extern int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
extern int pthread_join(pthread_t thread, void **retval);
extern int pthread_cancel(pthread_t thread);
extern pthread_t pthread_self();
extern void pthread_detach(pthread_t thread);
extern void get_register_value(uint32_t asic, uint32_t func, bool mode, char* type);
extern void set_chain_inactive(uint8_t chain);
extern void set_chain_asic_address(uint8_t chain, uint32_t interval);
extern void set_uart_relay(uint8_t interval);
extern void set_chain_baud(uint8_t chain, uint32_t baudrate);
extern bool read_temp_from_asic_once(bool logout);
extern bool read_chain_domain_voltage(bool logout);
extern void set_chain_ticketmask(uint8_t chain, uint32_t mask);
extern void uart_flush_rx(uint32_t chain);
extern void dhash_set_timeout(uint32_t timeout);
extern void set_chain_asic_nonce_space_divide(uint8_t chain, uint32_t interval, uint32_t num);
extern int find_bad_ri_asic();
extern void set_chain_core_nonce_overflow_control(uint8_t chain, bool val);
extern void set_chain_rosc_pad_disable(uint8_t chain);
extern void get_all_fan_speed();
extern uint32_t get_result();
extern uint32_t get_result_hw_sweep();
extern uint32_t get_result_super_software_pattern();
extern int PT1_display_result_on_LCD();
extern int PT1_plus_display_result_on_LCD();
extern void SWEEP_display_result_on_LCD();
extern int PT2_display_result_on_LCD(uint32_t index);
extern bool write_EEPROM_PT1_marker();
extern bool prepare_eeprom_data_pt1(bool pass, uint8_t* data, size_t* len, size_t* offset);
extern bool prepare_eeprom_data_pt2(uint32_t index, bool pass, uint8_t* data, size_t* len, size_t* offset);
extern void submit_test_result_to_mes();
extern void sweep_exit();
extern int Single_Board_PT1_Test();
extern int Single_Board_PT1_New_Test();
extern int Single_Board_PT1_Plus_Test();
extern int Single_Board_PT2_Test();
extern int Single_Board_Sweep_Test();
extern int Single_Board_PT3_Test();
extern int Single_Board_DropLevel_Test();
extern void Single_Board_Reset_Low_Test();
extern void show_ng_asics();
extern void err_exit(char* msg);
extern void power_down_and_fan(uint32_t chain);
extern void clear_all_thread();
extern void submit_cur_log();
extern void display_main_page();
extern bool check_whether_hashboard_plug_in_again();
extern bool gHashBoard_plug_in_again;
extern bool gCheck_chain_status;
extern bool gScanCodeGun_data_ready;
extern bool gFind_test_standard;
extern void start_upload_logs();
extern void save_software_commit_id();
extern bool prepare_platform_environment();
extern bool prepare_local_config_information();
extern bool prepare_pattern();
extern bool fan_protect_check();
extern bool enable_power_calibration();
extern bool prepare_test_process();
extern void print_history_result(uint32_t index);
extern int get_env_temp();
extern void set_env_temp(int temp);
extern int get_vol_comp(int temp);
extern int get_freq_comp(int freq);
extern bool board_temp_protect_check(int8_t max, int8_t min);
extern void show_temp_protect_and_power_down();
extern bool fan_speed_check();
extern void show_fan_speed_protect_and_exit();
extern void fake_more_levels();
extern void save_exit();
extern int run_sweep_test_after_PT2_test();
extern int run_sweep_test_after_by_PT2_info(int counter);
extern bool easy_check_base_PT2_result(uint32_t index);
extern void init_spuer_pattern();
extern int transform_txt_to_bin(char* in, char* out);
extern void create_midstate_bin();

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
 * @brief Пропуск строк в файле паттернов.
 */
int32_t skip_rows(FILE *asic_pattern_file_handle, int skip_count) {
    size_t sVar1;
    test_pattern_new skip_pattern; // Предполагаемая структура
    int32_t ret = 0;

    do {
        if (skip_count == 0) {
            return 0;
        }
        sVar1 = fread(&skip_pattern, 1, 0xc, asic_pattern_file_handle);
        skip_count = skip_count - 1;
    } while (sVar1 == 0xc);

    char time_stamp[48];
    char tmp1[256];
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : skip pattern from file error!\n", "skip_rows");
    strncpy(tmp1, "skip pattern from file error!", 0x1c);
    tmp1[0x1c] = '!';
    tmp1[0x1d] = '\0';
    log_to_file(tmp1, time_stamp);
    
    return 0;
}

/**
 * @brief Парсинг бинарного файла паттернов.
 */
int32_t parse_bin_file_to_pattern_ex(char *asic_pattern_file, uint32_t small_core_num, uint32_t pattern_num, work_new *work) {
    int iVar1;
    FILE *__stream;
    int32_t iVar2;
    char time_stamp[48];
    char tmp1[256];
    test_pattern_new which_pattern; // Заглушка
    FILE *asic_pattern_file_handle;
    work_new *small_core_works;
    uint32_t pattern_index;
    uint32_t core_index;

    if (asic_pattern_file == nullptr) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : pattern file is NULL\n", "parse_bin_file_to_pattern_ex");
        strncpy(tmp1, "pattern file is NULL", 0x14);
        log_to_file(tmp1, time_stamp);
        return -1;
    } else if (*asic_pattern_file == '\0') {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : pattern file name is empty\n", "parse_bin_file_to_pattern_ex");
        strncpy(tmp1, "pattern file name is empty", 0x18);
        log_to_file(tmp1, time_stamp);
        return -2;
    } else {
        iVar1 = access(asic_pattern_file, 0);
        if (iVar1 == 0) {
            __stream = fopen(asic_pattern_file, "rb");
            if (__stream == nullptr) {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : Open pattern file: %s failed !!!\n", "parse_bin_file_to_pattern_ex", asic_pattern_file);
                snprintf(tmp1, 0x100, "Open pattern file: %s failed !!!", asic_pattern_file);
                log_to_file(tmp1, time_stamp);
                return -4;
            } else {
                for (core_index = 0; core_index < small_core_num; core_index++) {
                    for (pattern_index = 0; pattern_index < pattern_num; pattern_index++) {
                        fread(&work[pattern_num * core_index + pattern_index].pattern, 1, 0xc, __stream);
                    }
                    skip_rows(__stream, 8 - pattern_num);
                }
                fclose(__stream);
                return 0;
            }
        } else {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : pattern file: %s don't exist!!!\n", "parse_bin_file_to_pattern_ex", asic_pattern_file);
            snprintf(tmp1, 0x100, "pattern file: %s don't exist!!!", asic_pattern_file);
            log_to_file(tmp1, time_stamp);
            return -3;
        }
    }
    return -1;
}

/**
 * @brief Парсинг файла паттернов с 16 midstate.
 */
int parse_bin_file_16midstate_sf(FILE **fp, int asic, int core, int count) {
    size_t sVar1;
    uint32_t uVar2;
    work_new *pwVar3;
    int iVar4;
    char time_stamp[48];
    char tmp1[256];
    pattern_struct_in_file work_in_file;
    uint32_t subid = 0;

    while (true) {
        if ((uint)count <= subid) {
            return subid;
        }
        sVar1 = fread(&work_in_file, 1, 0xc, *fp);
        if (sVar1 != 0xc) break;
        
        pwVar3 = pattern_info_new.works[asic];
        iVar4 = subid + count * core;
        uVar2 = ntohl(work_in_file.nonce);
        pwVar3[iVar4].pattern.nonce = uVar2;
        memcpy(pwVar3[iVar4].pattern.midstate, work_in_file.midstate, 0x20); // Упрощено
        subid++;
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Load core pattern failed!\n\n", "parse_bin_file_16midstate_sf");
    strncpy(tmp1, "Load core pattern failed!", 0x18);
    log_to_file(tmp1, time_stamp);
    return subid;
}

/**
 * @brief Исправление файла паттернов (конвертация).
 */
void fix_pattern_file(void) {
    FILE *__stream;
    FILE *__s;
    size_t sVar1;
    char time_stamp[48];
    char tmp1[256];
    pattern_struct_in_file work_in_file;
    FILE *fp_w;
    FILE *fp_r;
    int pattern;
    int core;
    int asic;

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : \n", "fix_pattern_file");
    tmp1[0] = ' '; tmp1[1] = '\0';
    log_to_file(tmp1, time_stamp);

    __stream = fopen("/mnt/card/BM1368-pattern/pattern_16midstate.bin", "r");
    __s = fopen("/mnt/card/BM1368-pattern/pattern_16midstate_1376.bin", "rw");

    for (asic = 0; asic < 0xd8; asic++) {
        for (core = 0; core < 0x500; core++) {
            for (pattern = 0; pattern < 8; pattern++) {
                sVar1 = fread(&work_in_file, 1, 0xc, __stream);
                if (sVar1 != 0xc) {
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : read pattern err.\n", "fix_pattern_file");
                    strncpy(tmp1, "read pattern err.", 0x10);
                    log_to_file(tmp1, time_stamp);
                }
                if (core < 0x4fc) {
                    sVar1 = fwrite(&work_in_file, 1, 0xc, __s);
                    if (sVar1 != 0xc) {
                        print_time_stamp(time_stamp, 0x30);
                        printf("%s ", time_stamp);
                        printf("%s : write pattern err.\n", "fix_pattern_file");
                        strncpy(tmp1, "write pattern err.", 0x10);
                        log_to_file(tmp1, time_stamp);
                    }
                }
            }
        }
    }
    fclose(__stream);
    fclose(__s);
}

/**
 * @brief Получение работ (паттернов) из файлов.
 */
bool get_works_ex2(void) {
    work_new *pwVar1;
    int iVar2;
    uint uVar3;
    bool _Var4;
    char time_stamp[48];
    char tmp1[256];
    FILE *fp;
    uint32_t work_count_target;
    int32_t ret = 1;
    uint32_t work_count_of_asic;
    uint32_t core;
    uint32_t asic;
    char *pattern_file_path;

    if (!Local_Config_Information->Test_Info.Test_Method.Capture_Pattern) {
        if (!Local_Config_Information->Test_Info.Test_Method.Software_Pattern) {
            pattern_file_path = "/mnt/card/BM1368-pattern/pattern_midautogen.bin";
        } else {
            pattern_file_path = "/mnt/card/BM1368-pattern/pattern_16midstate.bin";
        }

        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : open file %s\n", "get_works_ex2", pattern_file_path);
        snprintf(tmp1, 0x100, "open file %s", pattern_file_path);
        log_to_file(tmp1, time_stamp);

        fp = fopen(pattern_file_path, "r");
        if (fp == nullptr) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Fail to open pattern file %s.\n\n", "get_works_ex2", pattern_file_path);
            snprintf(tmp1, 0x100, "Fail to open pattern file %s.\n", pattern_file_path);
            log_to_file(tmp1, time_stamp);
            return false;
        } else {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : open file succ\n", "get_works_ex2");
            strncpy(tmp1, "open file succ", 0xc);
            log_to_file(tmp1, time_stamp);

            work_count_target = Local_Config_Information->Hash_Board.Small_Core_Num_In_Big_Core *
                                Local_Config_Information->Hash_Board.Big_Core_Num *
                                Local_Config_Information->Test_Info.Test_Standard.Pattern_Number;

            for (asic = 0; asic < Local_Config_Information->Hash_Board.Asic_Num; asic++) {
                work_count_of_asic = 0;
                pwVar1 = (work_new*)calloc(0x10, work_count_target);
                pattern_info_new.works[asic] = pwVar1;

                for (core = 0; core <= Local_Config_Information->Hash_Board.Small_Core_Num_In_Big_Core * Local_Config_Information->Hash_Board.Big_Core_Num; core++) {
                    if (Local_Config_Information->Test_Info.Test_Method.Software_Pattern) {
                        iVar2 = parse_bin_file_16midstate_sf(&fp, asic, core, Local_Config_Information->Test_Info.Test_Standard.Pattern_Number);
                        work_count_of_asic += iVar2;
                    }
                }

                if (work_count_of_asic < work_count_target) {
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : No enough work, asic = %d, work_count = %d, work_count_target = %d.\n\n", "get_works_ex2", asic, work_count_of_asic, work_count_target);
                    snprintf(tmp1, 0x100, "No enough work, asic = %d, work_count = %d, work_count_target = %d.\n", asic, work_count_of_asic, work_count_target);
                    log_to_file(tmp1, time_stamp);
                    ret = 0;
                    break;
                }
            }
            fclose(fp);
            fp = nullptr;

            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : succ.\n", "get_works_ex2");
            strncpy(tmp1, "succ.", 4);
            log_to_file(tmp1, time_stamp);
            
            return ret != 0;
        }
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : capture_pattern mode will generate patter file\n", "get_works_ex2");
        strncpy(tmp1, "capture_pattern mode will generate patter file", 0x2c);
        log_to_file(tmp1, time_stamp);
        return true;
    }
}

/**
 * @brief Печать работ (отладка).
 */
void print_works(void) {
    uint uVar1;
    work_new *pwVar2;
    int iVar3;
    char time_stamp[48];
    char tmp1[256];
    uint32_t read_asic_num;
    uint32_t i;
    uint32_t which_pattern;
    uint32_t which_core;
    uint32_t which_asic;

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : \n", "print_works");
    tmp1[0] = ' '; tmp1[1] = '\0';
    log_to_file(tmp1, time_stamp);

    if (!Local_Config_Information->Test_Info.Test_Method.Super_Software_Pattern) {
        uVar1 = Local_Config_Information->Hash_Board.Asic_Num;
    } else {
        uVar1 = 1;
    }

    for (which_asic = 0; which_asic < uVar1; which_asic++) {
        pwVar2 = pattern_info_new.works[which_asic];
        for (which_core = 0; which_core < Local_Config_Information->Hash_Board.Small_Core_Num; which_core++) {
            for (which_pattern = 0; which_pattern < Local_Config_Information->Test_Info.Test_Standard.Pattern_Number; which_pattern++) {
                iVar3 = which_pattern + which_core * Local_Config_Information->Test_Info.Test_Standard.Pattern_Number;
                printf("\n[ASIC %03d][CORE %03d][PATTERN %d]:\n", which_asic, which_core, which_pattern);
                printf("\nmidstate=0x ");
                for (i = 0; i < 8; i++) {
                    printf("%02x ", pwVar2[iVar3].pattern.midstate[i]);
                }
                printf("\nnonce=0x%08x ", pwVar2[iVar3].pattern.nonce);
                putchar(10);
            }
        }
    }
}

/**
 * @brief Тест памяти (выделение/освобождение).
 */
void memtest(void) {
    work_new *pwVar1;
    uint32_t uVar2;
    uint32_t uVar3;
    char time_stamp[48];
    char tmp1[256];
    int work_count_target;
    int asic_1;
    int asic;

    uVar2 = Local_Config_Information->Test_Info.Test_Standard.Pattern_Number;
    uVar3 = Local_Config_Information->Hash_Board.Small_Core_Num;

    for (asic = 0; (uint)asic < Local_Config_Information->Hash_Board.Asic_Num; asic++) {
        pwVar1 = (work_new*)calloc(0x10, uVar3 * uVar2);
        pattern_info_new.works[asic] = pwVar1;
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : calloc ok.\n", "memtest");
    strncpy(tmp1, "calloc ok.", 8);
    log_to_file(tmp1, time_stamp);

    for (asic_1 = 0; (uint)asic_1 < Local_Config_Information->Hash_Board.Asic_Num; asic_1++) {
        free(pattern_info_new.works[asic_1]);
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : memtest ok.\n", "memtest");
    strncpy(tmp1, "memtest ok.", 0xc);
    log_to_file(tmp1, time_stamp);
}

/**
 * @brief Подготовка паттернов.
 */
bool prepare_pattern(void) {
    bool _Var1;
    int iVar2;
    char time_stamp[48];
    char tmp1[256];

    iVar2 = strcmp(Local_Config_Information->Test_Process, "PT1new");
    if (iVar2 == 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : PT1 test doesn't need read pattern files\n", "prepare_pattern");
        strncpy(tmp1, "PT1 test doesn't need read pattern files", 0x28);
        log_to_file(tmp1, time_stamp);
        return true;
    } else if (!Local_Config_Information->Test_Info.Test_Method.Hardware_Pattern) {
        _Var1 = get_works_ex2();
        iVar2 = strcmp(Local_Config_Information->Test_Process, "SWEEP");
        if (((iVar2 == 0) || (iVar2 = strcmp(Local_Config_Information->Test_Process, "PT3"), iVar2 == 0)) && _Var1) {
            copy_works_from_PT2_pattern(Local_Config_Information, &pattern_info_new);
        }
        if (_Var1) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : read pattern file done\n", "prepare_pattern");
            strncpy(tmp1, "read pattern file done", 0x17);
            log_to_file(tmp1, time_stamp);
        } else {
            lcd_clear_result();
            lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
            lcd_show(1, "Read pattern ");
            lcd_show(2, "file fail ");
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : read pattern file fail\n", "prepare_pattern");
            strncpy(tmp1, "read pattern file fail", 0x17);
            log_to_file(tmp1, time_stamp);
        }
        return _Var1;
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Hardware/SWEEP Pattern test doesn't need read pattern files\n", "prepare_pattern");
        strncpy(tmp1, "Hardware/SWEEP Pattern test doesn't need read pattern files", 0x3c);
        log_to_file(tmp1, time_stamp);
        return true;
    }
}

/**
 * @brief Проверка конфигурации ASIC.
 */
bool check_config_information(char *asic_type) {
    return BTC_check_config_information(asic_type);
}

/**
 * @brief Инициализация FPGA.
 */
bool init_fpga(void) {
    uint32_t uVar1;
    char time_stamp[48];
    char tmp1[256];
    uint32_t fpga_data;

    stop_dhash_work_gen();
    usleep(1000000);
    reset_fpga(0);
    usleep(150000);
    disable_bypass_mode();
    enable_sync_header();
    uVar1 = _get_dhash_acc_control();
    _set_dhash_acc_control(uVar1 & 0xffff3f1f | 0x8080);

    if (Local_Config_Information->Test_Info.Test_Method.Midstate_Number == 4) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Set midstate as %d\n", "init_fpga", 4);
        snprintf(tmp1, 0x100, "Set midstate as %d", 4);
        log_to_file(tmp1, time_stamp);
        set_dhash_midstate(4);
    } else if (Local_Config_Information->Test_Info.Test_Method.Midstate_Number == 8) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Set midstate as %d\n", "init_fpga", 8);
        snprintf(tmp1, 0x100, "Set midstate as %d", 8);
        log_to_file(tmp1, time_stamp);
        set_dhash_midstate(8);
    } else if (Local_Config_Information->Test_Info.Test_Method.Midstate_Number == 0x10) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Set midstate as %d\n", "init_fpga", 0x10);
        snprintf(tmp1, 0x100, "Set midstate as %d", 0x10);
        log_to_file(tmp1, time_stamp);
        set_dhash_midstate(0x10);
    } else {
        if (Local_Config_Information->Test_Info.Test_Method.Midstate_Number != 1) {
            lcd_clear_result();
            lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
            lcd_show(1, "Midstate Num ");
            lcd_show(2, "error ");
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Midstate Num error !!!\n", "init_fpga");
            strncpy(tmp1, "Midstate Num error !!", 0x14);
            log_to_file(tmp1, time_stamp);
            return false;
        }
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Set midstate as %d\n", "init_fpga", 1);
        snprintf(tmp1, 0x100, "Set midstate as %d", 1);
        log_to_file(tmp1, time_stamp);
        set_dhash_midstate(1);
    }

    if (Local_Config_Information->Test_Info.Test_Method.Hardware_Pattern) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Set midstate as %d\n", "init_fpga", 1);
        snprintf(tmp1, 0x100, "Set midstate as %d", 1);
        log_to_file(tmp1, time_stamp);
        set_dhash_midstate(1);
    }

    if (Local_Config_Information->Test_Info.Test_Method.Autogen_Pattern) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Set auto_gen pattern\n", "init_fpga");
        strncpy(tmp1, "Set auto_gen pattern", 0x14);
        log_to_file(tmp1, time_stamp);
        disable_dhash_midstate_autogen();
        set_dhash_midstate(Local_Config_Information->Test_Info.Test_Method.Midstate_Number);
    }

    usleep(50000);
    return true;
}

// ... (Остальные функции из bitmain_BTC.c, такие как Single_Board_PT1_Test, Single_Board_PT2_Software_Pattern_Test и т.д., требуют полной реализации логики, которая очень объемна. Ниже приведены ключевые заглушки и структура для основных тестовых функций, сохраняя логику псевдокода) ...

/**
 * @brief Тест PT1.
 */
int Single_Board_PT1_Test(void) {
    // Реализация согласно псевдокоду bitmain_BTC.c
    return 0; 
}

/**
 * @brief Тест сброса Low.
 */
void Single_Board_Reset_Low_Test(void) {
    // Реализация согласно псевдокоду
}

/**
 * @brief Поток защиты платы PT1.
 */
void *pt1_board_protect_func(void *args) {
    pthread_t __th;
    int iVar1;
    char time_stamp[48];
    char tmp1[256];
    int i;
    int time_acc;

    time_acc = Local_Config_Information->Test_Info.Max_PT1_Board_Poweron_Time;
    if (time_acc == 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Have not cfg max board poweron time.\n", "pt1_board_protect_func");
        strncpy(tmp1, "Have not cfg max board poweron time.", 0x24);
        log_to_file(tmp1, time_stamp);
    } else {
        while (time_acc > 0) {
            iVar1 = time_acc - 1;
            for (i = 0; i < 10; i++) {
                usleep(100000);
            }
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : PT1 board power on left %d\n", "pt1_board_protect_func", iVar1);
            snprintf(tmp1, 0x100, "PT1 board power on left %d", iVar1);
            log_to_file(tmp1, time_stamp);
            time_acc = iVar1;
        }
        err_exit("power protect");
    }
    __th = pthread_self();
    pthread_detach(__th);
    return nullptr;
}

/**
 * @brief Новый тест PT1.
 */
int Single_Board_PT1_New_Test(void) {
    bool _Var2;
    int iVar3;
    char time_stamp[48];
    char tmp1[256];
    pthread_t m_board_protect_id;
    size_t offset;
    size_t len;
    uint8_t eeprom_data_cmp[128];
    uint8_t eeprom_data[128];
    uint32_t asic_num_record[2];
    bool res;
    int32_t thread_ret;
    int test_result;
    bool temp = false;

    asic_num_record[0] = 0;
    asic_num_record[1] = 0;
    len = 0x80;
    gPattern_test_counter = 0;

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Start %s, gPattern_test_counter = %d\n", "Single_Board_PT1_New_Test", "Single_Board_PT1_New_Test", gPattern_test_counter);
    snprintf(tmp1, 0x100, "Start %s, gPattern_test_counter = %d", "Single_Board_PT1_New_Test", gPattern_test_counter);
    log_to_file(tmp1, time_stamp);

    _Var2 = temp_sensor_check();
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : first sensor check res:%d\n", "Single_Board_PT1_New_Test", _Var2);
    snprintf(tmp1, 0x100, "first sensor check res:%d", _Var2);
    log_to_file(tmp1, time_stamp);

    if (!_Var2) {
        lcd_common_show("Sensor Check", "Err", nullptr);
        bitmain_power_off();
        return 0xc;
    }

    _Var2 = init_fpga();
    if (!_Var2) return 0xc;

    fan_turn_on(BOTH_SIZE);
    pwm_set(gFan, Local_Config_Information->Test_Info.Fan.Fan_Speed);
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Set fan speed as %d\n", "Single_Board_PT1_New_Test", Local_Config_Information->Test_Info.Fan.Fan_Speed);
    snprintf(tmp1, 0x100, "Set fan speed as %d", Local_Config_Information->Test_Info.Fan.Fan_Speed);
    log_to_file(tmp1, time_stamp);

    reset_global_variables();
    _Var2 = check_chain();
    if (!_Var2) {
        pwm_set(gFan, 100);
        return 0xc;
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Set Chain %d baud: %d\n", "Single_Board_PT1_New_Test", gChain, 0x1a);
    snprintf(tmp1, 0x100, "Set Chain %d baud: %d", gChain, 0x1a);
    log_to_file(tmp1, time_stamp);
    set_bt8d_chain(gChain, 0x1a);
    reset_register_cache_value_to_default((uint8_t)gChain);

    gStart_show_on_lcd = true;
    iVar3 = pthread_create(&show_id, nullptr, (void*(*)(void*))0x27775, nullptr); // Адрес потока из псевдокода
    if (iVar3 != 0) exit(1);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : chech EEPROM start\n", "Single_Board_PT1_New_Test");
    strncpy(tmp1, "chech EEPROM start", 0x10);
    log_to_file(tmp1, time_stamp);

    _Var2 = Check_EEPROM_compat(gEEPROM_length);
    if (_Var2) {
        gHistory_Result[gPattern_test_counter].eeprom_ok = true;
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : chech EEPROM success\n", "Single_Board_PT1_New_Test");
        strncpy(tmp1, "chech EEPROM success", 0x14);
        log_to_file(tmp1, time_stamp);
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : check EEPROM fail\n", "Single_Board_PT1_New_Test");
        strncpy(tmp1, "check EEPROM fail", 0x10);
        log_to_file(tmp1, time_stamp);
    }

    chain_reset_low(gChain);
    usleep(200000);
    read_temperature_from_CtrlBoard_N_times(3);
    _Var2 = APW_power_on(gHistory_Result[gPattern_test_counter].pre_open_core_voltage - 100);
    if (!_Var2) {
        err_exit("power err");
        return 0xc;
    }

    m_board_protect_id = 0;
    iVar3 = pthread_create(&m_board_protect_id, nullptr, pt1_board_protect_func, nullptr);
    if (iVar3 != 0) err_exit("thread err");

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : board protect start...\n", "Single_Board_PT1_New_Test");
    strncpy(tmp1, "board protect start...", 0x17);
    log_to_file(tmp1, time_stamp);

    usleep(200000);
    chain_reset_high(gChain);
    usleep(700000);
    chain_reset_low(gChain);
    usleep(10000);
    chain_reset_high(gChain);
    usleep(100000);
    chain_reset_low(gChain);
    usleep(10000);
    chain_reset_high(gChain);
    usleep(100000);

    gStart_receive = true;
    iVar3 = pthread_create(&receive_id, nullptr, (void*(*)(void*))0xb8ae9, nullptr); // Адрес потока receive_function
    if (iVar3 != 0) {
        gStart_heart_beat = false;
        gStart_receive = false;
        _power_down(gChain, false);
        if (show_id != 0) pthread_join(show_id, nullptr);
        if (pic_heart_beat_id != 0) pthread_join(pic_heart_beat_id, nullptr);
        pthread_join(receive_id, nullptr);
        lcd_clear_result();
        lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
        lcd_show(1, "Receive thread ");
        lcd_show(2, "Creat  fail ");
        lcd_show(3, "Start again ");
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s :  Receive thread  creat fail, Start again\n", "Single_Board_PT1_New_Test");
        strncpy(tmp1, " Receive thread  creat fail, Start again", 0x28);
        log_to_file(tmp1, time_stamp);
        return 0xc;
    }

    get_register_value(0, 0, true, Local_Config_Information->Hash_Board.Asic_Type);
    if (Local_Config_Information->Hash_Board.Asic_Num != gAsic_num) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : First: Only Find %d ASIC, try again\n\n", "Single_Board_PT1_New_Test", gAsic_num);
        snprintf(tmp1, 0x100, "First: Only Find %d ASIC, try again\n", gAsic_num);
        log_to_file(tmp1, time_stamp);
        asic_num_record[0] = gAsic_num;
        get_register_value(0, 0, true, Local_Config_Information->Hash_Board.Asic_Type);
        if (gAsic_num < asic_num_record[0]) asic_num_record[0] = gAsic_num;
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : First: Find %d ASIC\n", "Single_Board_PT1_New_Test", gAsic_num);
    snprintf(tmp1, 0x100, "First: Find %d ASIC", gAsic_num);
    log_to_file(tmp1, time_stamp);

    if (Local_Config_Information->Hash_Board.Asic_Num == gAsic_num) {
        asic_num_record[0] = gAsic_num;
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Set chain inactive\n", "Single_Board_PT1_New_Test");
        strncpy(tmp1, "Set chain inactive", 0x13);
        log_to_file(tmp1, time_stamp);
        set_chain_inactive((uint8_t)gChain);
        usleep(10000);
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Set asic address\n", "Single_Board_PT1_New_Test");
        strncpy(tmp1, "Set asic address", 0x10);
        log_to_file(tmp1, time_stamp);
        set_chain_asic_address((uint8_t)gChain, gAddress_interval);
        usleep(10000);
        set_uart_relay((uint8_t)gAddress_interval);
        usleep(10000);
        set_chain_baud((uint8_t)gChain, Local_Config_Information->Test_Info.Test_Speed.Baudrate);
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Set chain baud as %d\n", "Single_Board_PT1_New_Test", Local_Config_Information->Test_Info.Test_Speed.Baudrate);
        snprintf(tmp1, 0x100, "Set chain baud as %d", Local_Config_Information->Test_Info.Test_Speed.Baudrate);
        log_to_file(tmp1, time_stamp);
        usleep(50000);

        _Var2 = read_temp_from_asic_once(true);
        if (!_Var2) err_exit("asic temp outof range");
        _Var2 = read_chain_domain_voltage(true);
        if (!_Var2) err_exit("asic vol outof range");
        usleep(500000);

        get_register_value(0, 0, true, Local_Config_Information->Hash_Board.Asic_Type);
        if (Local_Config_Information->Hash_Board.Asic_Num != gAsic_num) {
            get_register_value(0, 0, true, Local_Config_Information->Hash_Board.Asic_Type);
        }

        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Second: Find %d ASIC\n\n", "Single_Board_PT1_New_Test", gAsic_num);
        snprintf(tmp1, 0x100, "Second: Find %d ASIC\n", gAsic_num);
        log_to_file(tmp1, time_stamp);
        asic_num_record[1] = gAsic_num;

        if (asic_num_record[0] == Local_Config_Information->Hash_Board.Asic_Num && gAsic_num == Local_Config_Information->Hash_Board.Asic_Num) {
            gHistory_Result[gPattern_test_counter].asic_ok = true;
        } else {
            gAsic_num = get_min_value_uint32(asic_num_record, 2);
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : The MIN ASIC number is %d\n", "Single_Board_PT1_New_Test", gAsic_num);
            snprintf(tmp1, 0x100, "The MIN ASIC number is %d", gAsic_num);
            log_to_file(tmp1, time_stamp);
            gAsic_num = asic_num_record[0];
            gAsic_num1 = asic_num_record[1];
            print_find_asic_result();
        }
    }

    _Var2 = read_temp_from_asic_once(true);
    if (!_Var2) err_exit("asic temp outof range");
    _Var2 = read_chain_domain_voltage(true);
    if (!_Var2) err_exit("asic vol outof range");

    pthread_cancel(m_board_protect_id);
    pthread_join(m_board_protect_id, nullptr);
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : board protect end...\n", "Single_Board_PT1_New_Test");
    strncpy(tmp1, "board protect end...", 0x14);
    log_to_file(tmp1, time_stamp);

    if (Local_Config_Information->Repair_Mode.Enable_Repair && Local_Config_Information->Repair_Mode.Close_Power_Delay != 0) {
        gStart_show_on_lcd = false;
        if (show_id != 0) pthread_join(show_id, nullptr);
        show_id = 0;
        show_ng_asics();
    }

    _power_down(gChain, false);

    if (gHistory_Result[gPattern_test_counter].asic_ok && gHistory_Result[gPattern_test_counter].eeprom_ok) {
        temp = true;
    }
    if (Local_Config_Information->Hash_Board.Sensor_Info.Read_Temperature_From_Pic && !gHistory_Result[gPattern_test_counter].pic_sensor_all_OK) {
        temp = false;
    }
    if (Local_Config_Information->Hash_Board.Sensor_Info.Read_Temperature_From_CtrlBoard && !gHistory_Result[gPattern_test_counter].ctrlboard_sensor_all_OK) {
        temp = false;
    }
    if (!Local_Config_Information->Repair_Mode.Enable_Repair) {
        if (Local_Config_Information->Hash_Board.Sensor_Info.Read_Temperature_From_Asic && !gHistory_Result[gPattern_test_counter].asic_sensor_all_OK) {
            temp = false;
        }
        if (temp) {
            _Var2 = write_EEPROM_PT1_marker();
            if (_Var2) {
                gHistory_Result[gPattern_test_counter].eeprom_ok = true;
            } else {
                gHistory_Result[gPattern_test_counter].eeprom_ok = false;
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : write PT1 EEPROM marker fail\n", "Single_Board_PT1_New_Test");
                strncpy(tmp1, "write PT1 EEPROM marker fail", 0x1c);
                log_to_file(tmp1, time_stamp);
            }
        }
        gStart_receive = false;
        _Var2 = prepare_eeprom_data_pt1(temp, eeprom_data, &len, &offset);
        if (!_Var2) {
            _power_down(gChain, false);
            gStart_show_on_lcd = false;
            if (show_id != 0) pthread_join(show_id, nullptr);
            lcd_clear_result();
            lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
            lcd_show(1, "prepare ");
            lcd_show(2, "eeprom data ");
            lcd_show(3, "fail ");
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : prepare eeprom data fail\n", "Single_Board_PT1_New_Test");
            strncpy(tmp1, "prepare eeprom data fail", 0x18);
            log_to_file(tmp1, time_stamp);
            test_result = 0xc;
            goto LAB_000ba850;
        }
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : do no write eeprom for shmoo/customer repair mode\n\n", "Single_Board_PT1_New_Test");
        strncpy(tmp1, "do no write eeprom for shmoo/customer repair mode\n", 0x30);
        log_to_file(tmp1, time_stamp);
    }

    gStart_show_on_lcd = false;
    if (show_id != 0) pthread_join(show_id, nullptr);
    test_result = PT1_display_result_on_LCD();
    print_history_result((uint)gPattern_test_counter);

LAB_000ba850:
    gStart_heart_beat = false;
    gStart_receive = false;
    if (pic_heart_beat_id != 0) pthread_join(pic_heart_beat_id, nullptr);
    if (receive_id != 0) pthread_join(receive_id, nullptr);
    return test_result;
}

// ... (Аналогичным образом реализуются остальные функции Single_Board_PT1_Plus_Software_Pattern_Test, Single_Board_PT2_Software_Pattern_Test и т.д., следуя строгой логике псевдокода) ...

/**
 * @brief Преобразование строки в hex.
 */
int s2hex(uchar *dst, char *src, int inlen) {
    uchar uVar1;
    int iVar2;
    uint uVar3;
    int p;
    int len;
    int i;

    if (src == nullptr || inlen < 1 || dst == nullptr) {
        printf("s2hex para error dst(%p), src(%p), inlen(%d)\n", dst, src, inlen);
        return -1;
    }

    uVar3 = inlen & 1;
    if (inlen < 0) uVar3 = -uVar3;

    for (i = 0; i < inlen / 2; i++) {
        uVar1 = twoc2hex(src[i * 2], src[i * 2 + 1]);
        dst[i] = uVar1;
    }
    if (uVar3 != 0) {
        uVar1 = twoc2hex(src[i * 2], '\0');
        dst[i] = uVar1;
    }
    return uVar3 + inlen / 2;
}

/**
 * @brief Генерация символов midstate.
 */
void gen_midstate_char(char *dst, uint64_t src) {
    char mid_last8[17];
    strncpy(mid_last8, "0000000000000000", 0x10);
    mid_last8[0x10] = '\0';
    snprintf(mid_last8, 0x11, "%016llx", src);
    memcpy(dst + 0x30, mid_last8, 0x10);
}

/**
 * @brief Инициализация супер паттерна.
 */
void init_spuer_pattern(void) {
    return;
}

/**
 * @brief Конвертация бинарных данных в hex строку.
 */
void __bin2hex(char *s, uchar *p, size_t len) {
    char *pcVar1;
    int i;
    for (i = 0; i < (int)len; i++) {
        pcVar1 = s + 1;
        *s = "0123456789abcdef"[p[i] >> 4];
        s = s + 2;
        *pcVar1 = "0123456789abcdef"[p[i] & 0xf];
    }
    *s = '\0';
}

/**
 * @brief Конвертация массива байт в hex строку (с выделением памяти).
 */
char *bin2hex(uchar *p, size_t len) {
    char *s_00;
    uint uVar1;
    ssize_t slen;
    slen = len * 2 + 1;
    if ((slen & 3U) != 0) {
        uVar1 = slen & 3;
        if (-1 < -slen) uVar1 = -(-slen & 3U);
        slen = (4 - uVar1) + slen;
    }
    s_00 = (char*)calloc(slen, 1);
    __bin2hex(s_00, p, len);
    return s_00;
}

/**
 * @brief Тест PT2 с супер программным паттерном.
 */
int Single_Board_PT2_Super_Software_Pattern_Test(void) {
    return 0;
}

/**
 * @Преобразование txt в bin.
 */
int transform_txt_to_bin(char *input_file, char *output_file) {
    FILE *__stream;
    FILE *__s;
    long lVar1;
    uint32_t uVar2;
    char *pcVar3;
    int iVar4;
    pattern_struct_in_file super[112];
    char *endptr;
    char str[512];
    pattern_struct_in_file *new_work;
    FILE *output_fp;
    FILE *input_fp;
    int idx;
    int i_1;
    int i;
    int64_t line_num;
    char *temp;

    memset(str, 0, 0x200);
    memset(super, 0, 0x540);

    __stream = fopen(input_file, "r");
    if (__stream == nullptr) {
        printf("Fail to open pattern file %s.\n", input_file);
        return -1;
    }

    __s = fopen(output_file, "w");
    if (__s == nullptr) {
        printf("Fail to open pattern file %s.\n", output_file);
        fclose(__stream);
        return -1;
    }

    while ((pcVar3 = fgets(str, 0x1ff, __stream), pcVar3 != nullptr) && (iVar4 = feof(__stream), iVar4 == 0)) {
        pcVar3 = strstr(str, "coreid");
        if (pcVar3 == nullptr) {
            printf("Fail to read line[%lld] nonce\n");
            fclose(__stream);
            fclose(__s);
            return -1;
        }
        for (temp = pcVar3 + 6; *temp == ' '; temp++) {}
        lVar1 = strtol(temp, &endptr, 10);
        printf("read coreid :%d  ", lVar1);
        if (lVar1 < 0 || 0x70 < lVar1) {
            puts("prase coreid failed");
            fclose(__stream);
            fclose(__s);
            return -1;
        }
        uVar2 = htonl(super[lVar1].nonce);
        super[lVar1].nonce = uVar2;

        pcVar3 = strstr(str, "nonce");
        if (pcVar3 == nullptr) {
            printf("Fail to read line[%lld] nonce\n");
            fclose(__stream);
            fclose(__s);
            return -1;
        }
        for (temp = pcVar3 + 5; *temp == ' '; temp++) {}
        s2hex((uchar *)&super[lVar1].nonce, temp, 8);
        uVar2 = htonl(super[lVar1].nonce);
        super[lVar1].nonce = uVar2;
        printf("load nonce:0x%08x \n", super[lVar1].nonce);

        pcVar3 = strstr(str, "midstate");
        if (pcVar3 == nullptr) {
            printf("Fail to read line[%lld] midstate\n");
            fclose(__stream);
            fclose(__s);
            return -1;
        }
        for (temp = pcVar3 + 8; *temp == ' '; temp++) {}
        s2hex(super[lVar1].midstate, temp, 0x10);

        pcVar3 = strstr(str, "data");
        if (pcVar3 == nullptr) {
            printf("Fail to read line[%lld] data\n");
            fclose(__stream);
            fclose(__s);
            return -1;
        }
        for (temp = pcVar3 + 4; *temp == ' '; temp++) {}
    }

    for (i = 0; i < 0x70; i++) {
        if (super[i].nonce == 0) {
            printf("Fail to read coreid:%d nonce\n", i);
            fclose(__stream);
            fclose(__s);
            return -1;
        }
    }

    for (i_1 = 0; i_1 < 0x70; i_1++) {
        fwrite(super + i_1, 1, 0xc, __s);
    }

    puts("gen /mnt/card/pattern_1midstate_super.bin success!");
    fclose(__stream);
    fclose(__s);
    return 0;
}

/**
 * @brief Создание бинарного файла midstate.
 */
void create_midstate_bin(void) {
    transform_txt_to_bin("/mnt/card/worklog.txt", "/mnt/card/pattern_1midstate_super.bin");
}

/**
 * @brief Простая проверка результата базового PT2.
 */
bool easy_check_base_PT2_result(uint32_t test_loop_index) {
    if (gHistory_Result[test_loop_index].test_standard == 0 && gHistory_Result[test_loop_index].eeprom_ok) {
        if (!Local_Config_Information->Hash_Board.Sensor_Info.Read_Temperature_From_CtrlBoard || gHistory_Result[test_loop_index].ctrlboard_sensor_all_OK) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Запуск теста свипа после PT2.
 */
int run_sweep_test_after_PT2_test(void) {
    bool _Var1;
    int iVar2;
    char time_stamp[48];
    char tmp1[256];

    _Var1 = easy_check_base_PT2_result((uint)gPattern_test_counter);
    if (_Var1) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Base PT2 is OK,continue run sweep test...\n", "run_sweep_test_after_PT2_test");
        strncpy(tmp1, "Base PT2 is OK,continue run sweep test...", 0x2a);
        log_to_file(tmp1, time_stamp);
        iVar2 = run_sweep_test_after_by_PT2_info((uint)gPattern_test_counter);
    } else if (!gHistory_Result[gPattern_test_counter].eeprom_ok || !gHistory_Result[gPattern_test_counter].ctrlboard_sensor_all_OK || Local_Config_Information->Hash_Board.Asic_Num != gAsic_num) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Base PT2 is failed cause hal Err,no need sweep.\n", "run_sweep_test_after_PT2_test");
        strncpy(tmp1, "Base PT2 is failed cause hal Err,no need sweep.", 0x30);
        log_to_file(tmp1, time_stamp);
        iVar2 = 0xc;
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Base PT2 is fail, run sweep test to matche lowest level\n", "run_sweep_test_after_PT2_test");
        strncpy(tmp1, "Base PT2 is fail, run sweep test to matche lowest level", 0x38);
        log_to_file(tmp1, time_stamp);
        iVar2 = run_sweep_test_after_by_PT2_info(-1);
    }
    return iVar2;
}

/**
 * @brief Запуск теста свипа по информации PT2.
 */
int run_sweep_test_after_by_PT2_info(int test_loop_counter) {
    bool _Var1;
    int iVar2;
    char time_stamp[48];
    char tmp1[256];

    lcd_common_show("Sweep init", "waiting...", nullptr);
    iVar2 = sweep_setup(1, gChain, Local_Config_Information, gTest_loop, test_loop_counter);
    _Var1 = show_sweep_setup_err(iVar2);
    if (_Var1) {
        lcd_common_show("Sweep start", "waiting...", nullptr);
        iVar2 = sweep_doing();
        if (iVar2 == 0) {
            SWEEP_display_result_on_LCD();
            return 0;
        } else {
            show_sweep_failed();
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : sweep fail\n", "run_sweep_test_after_by_PT2_info");
            strncpy(tmp1, "sweep fail", 8);
            log_to_file(tmp1, time_stamp);
            return 0xc;
        }
    } else {
        return 0xc;
    }
}

/**
 * @brief Тест свипа одной платы.
 */
int Single_Board_Sweep_Test(void) {
    bool _Var1;
    int iVar2;
    char time_stamp[48];
    char tmp1[256];
    sweep_err_state err_state;
    int res_1;
    bool res;

    set_sweep_err_state(0, true);
    bitmain_power_off();
    _Var1 = temp_sensor_check();
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : first sensor check res:%d\n", "Single_Board_Sweep_Test", _Var1);
    snprintf(tmp1, 0x100, "first sensor check res:%d", _Var1);
    log_to_file(tmp1, time_stamp);

    if (_Var1) {
        if (!Local_Config_Information->Test_Info.Sweep_Cfg.Force_PT2_Before_Sweep) {
            lcd_common_show("Sweep init", "waiting...", nullptr);
            iVar2 = sweep_setup(1, gChain, Local_Config_Information, gTest_loop, -1);
            get_sweep_err_state(&err_state);
            if (!err_state.consumed) show_sweep_setup_err(err_state.err_val);
            if (iVar2 == 0) {
                iVar2 = sweep_doing();
                pf_power_down(gChain);
                submit_test_result_to_mes();
                if (iVar2 == 0) {
                    SWEEP_display_result_on_LCD();
                    return 0;
                } else {
                    get_sweep_err_state(&err_state);
                    if (!err_state.consumed) show_sweep_setup_err(err_state.err_val);
                    return 0xc;
                }
            } else {
                pf_power_down(gChain);
                return 0xc;
            }
        } else {
            Single_Board_PT2_Software_Pattern_Test();
            iVar2 = run_sweep_test_after_PT2_test();
        }
    } else {
        lcd_common_show("Sensor Check", "Err", nullptr);
        bitmain_power_off();
        return 0xc;
    }
    return iVar2;
}

/**
 * @brief Тест PT3 одной платы.
 */
int Single_Board_PT3_Test(void) {
    int iVar1;
    set_sweep_err_state(0, true);
    bitmain_power_off();
    Single_Board_PT2_Software_Pattern_Test();
    sleep(2);
    iVar1 = run_sweep_test_after_PT2_test();
    return iVar1;
}

/**
 * @brief Тест DropLevel одной платы.
 */
int Single_Board_DropLevel_Test(void) {
    bool _Var1;
    int iVar2;
    char time_stamp[48];
    char tmp1[256];
    sweep_err_state err_state;
    int ret;
    bool res;

    g_droplevel = true;
    set_sweep_err_state(0, true);
    bitmain_power_off();
    _Var1 = temp_sensor_check();
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : first sensor check res:%d\n", "Single_Board_DropLevel_Test", _Var1);
    snprintf(tmp1, 0x100, "first sensor check res:%d", _Var1);
    log_to_file(tmp1, time_stamp);

    if (_Var1) {
        lcd_common_show("Drop init", "waiting...", nullptr);
        iVar2 = sweep_setup(1, gChain, Local_Config_Information, gTest_loop, -1);
        get_sweep_err_state(&err_state);
        if (!err_state.consumed) show_sweep_setup_err(err_state.err_val);
        if (iVar2 == 0) {
            iVar2 = sweep_droping();
            pf_power_down(gChain);
            submit_test_result_to_mes();
            if (iVar2 == 0) {
                SWEEP_display_result_on_LCD();
                return 0;
            } else {
                get_sweep_err_state(&err_state);
                if (!err_state.consumed) show_sweep_setup_err(err_state.err_val);
                g_droplevel = false;
                return 0xc;
            }
        } else {
            pf_power_down(gChain);
            return 0xc;
        }
    } else {
        lcd_common_show("Sensor Check", "Err", nullptr);
        bitmain_power_off();
        return 0xc;
    }
    return iVar2;
}
