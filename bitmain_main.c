#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>

// --- Типы данных (строго по структуре из псевдокода) ---

typedef struct {
    char board_sn[32];
    char chip_die[16];
    char chip_marking[16];
    char chip_ftversion[16];
    char temp_sensor_type[16];
    char hashboard_ctn[16];
    char hashboard_in[16];
    char chip_tech[16];
    char board_name[16];
    char factory_job[32];
    uint32_t chip_bin;
} pt1_ctx_t;

// Заглушка для внешней структуры конфигурации
struct LocalConfigInfo {
    char Test_Process[16];
    struct {
        bool Bypass_Scan_Code_Gun;
        bool Factory_Mode;
    } Test_Info;
    struct {
        char Board_Name[32];
        char Miner_Type[32];
    } Hash_Board;
};

// --- Глобальные переменные (строго по адресам и именам из псевдокода) ---

static int32_t g_user_chain = 0xFF; 
static bool gCheck_chain_status = false;
static bool gHashBoard_plug_in_again = false;
static bool gHashBoard_plug_in = false;
static bool gFind_test_standard = false;
static bool gScanCodeGun_data_ready = false;
static bool g_droplevel = false;
static int32_t g_apw_power_version = 0;
static int32_t g_eeprom_version = 0;

static pt1_ctx_t g_pt1 = {};
static LocalConfigInfo* Local_Config_Information = nullptr; 

// Структуры сигналов
static struct sigaction termhandler;
static struct sigaction inthandler;
static struct sigaction abrthandler;
static struct sigaction crashhandler;

// --- Внешние функции (заглушки для линковки) ---
extern int32_t platform_init();
extern void platform_uninit();
extern int32_t fpga_read(uint32_t reg, uint32_t* value);
extern int32_t bitmain_power_on();
extern int32_t bitmain_power_off();
extern int32_t bitmain_power_version();
extern bool prepare_platform_environment();
extern bool prepare_local_config_information();
extern bool prepare_pattern();
extern bool fan_protect_check();
extern bool enable_power_calibration();
extern bool prepare_test_process();
extern void save_software_commit_id();
extern bool read_board_info_from_eeprom();
extern void print_local_config_after_parse_MES_system_information();
extern int32_t read_ipreport_key_val();
extern bool check_chain();
extern int32_t get_pt1_test_permission(char* sn, char* msg);
extern uint32_t Single_Board_PT1_Test();
extern int32_t send_pt1_test_result(char* sn, bool pass, uint8_t err_no, uint8_t err_info, char* time, char* msg);
extern int32_t get_pt2_test_permission(char* sn, char* msg);
extern uint32_t Single_Board_PT2_Test();
extern int32_t send_pt2_test_result(char* sn, bool pass, uint8_t err_no, uint8_t err_info, char* time, char* msg);
extern uint32_t Single_Board_Sweep_Test();
extern uint32_t Single_Board_PT3_Test();
extern uint32_t Single_Board_DropLevel_Test();
extern void Single_Board_Reset_Low_Test();
extern void sweep_exit();
extern void start_upload_logs();
extern void submit_cur_log();
extern void start_new_log(const char* type, char* sn, char* miner, char* board);
extern void print_time_stamp(char* buf, int len);
extern void log_to_file(char* msg, char* time);
extern void _applog(int level, const char* msg, bool newline);
extern void lcd_common_show(const char* l1, const char* l2, const char* l3);
extern void lcd_clear_result();
extern int32_t lcd_show(int line, const char* data);
extern void display_main_page();
extern void check_whether_hashboard_plug_in_again();
extern void create_midstate_bin();
extern void set_env_temp(uint16_t val);

// Вспомогательная функция для MES
bool save_mes_submit_result(bool pt1, char* msg) {
    return true;
}

// --- Функции обработки сигналов ---

void handle_pipe(int sig) {
    (void)sig;
    return;
}

void sighandler(int sig) {
    char time_stamp[48];
    char tmp1[256];

    // Восстановление обработчиков
    sigaction(SIGTERM, &termhandler, nullptr);
    sigaction(SIGINT, &inthandler, nullptr);
    sigaction(SIGABRT, &abrthandler, nullptr);
    sigaction(SIGBUS, &abrthandler, nullptr);
    sigaction(SIGFPE, &abrthandler, nullptr);
    sigaction(SIGSEGV, &abrthandler, nullptr);

    printf("sig:%d ", sig);
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : sig:%d\n", "sighandler", sig);
    
    snprintf(tmp1, 0x100, "sig:%d ", sig);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : sighandler power off hashboard\n", "sighandler");
    
    strncpy(tmp1, "sighandler power off hashboard\0", 0x1c);
    log_to_file(tmp1, time_stamp);

    bitmain_power_off();
    exit(0);
}

// --- Вспомогательная функция check_hashboard_plug_in ---
int check_hashboard_plug_in(void) {
    // Здесь должна быть логика с get_system_capability
    gHashBoard_plug_in = true;
    return 0;
}

// --- Основная функция main ---

int32_t main(int argc, char** argv) {
    int32_t iVar3;
    size_t sVar4;
    uint32_t uVar5;
    int32_t iVar6;
    char* pcVar7;
    char time_stamp[48];
    char tmp1[256];
    char response_msg[100];
    
    // Локальные буферы для LCD и SN
    char lcd_data[16];
    char lcd_data_1[16];
    char lcd_data_2[16];
    char lcd_data_3[16];
    char lcd_data_4[16];
    char lcd_data_5[16];
    char lcd_data_6[16];
    char lcd_data_7[16];
    char lcd_data_8[16];
    char lcd_data_9[16];
    char lcd_data_10[16];
    char lcd_data_11[16];
    char lcd_data_12[16];
    char lcd_data_13[16];
    char lcd_data_14[16];
    char lcd_data_15[16];
    char lcd_data_16[16];
    char lcd_data_17[16];
    
    char board_sn[20];
    char board_sn_1[20];
    char board_sn_2[20];
    char board_sn_3[20];
    char board_sn_4[20];
    char board_sn_5[20];
    char board_sn_6[20];
    char board_sn_7[20];
    char board_sn_8[20];

    // Инициализация обработчиков сигналов
    struct sigaction sa;
    struct sigaction handler;
    
    memset(&handler, 0, sizeof(handler));
    handler.sa_flags = 0;
    handler.sa_handler = (void (*)(int))0x16b39; 
    sigemptyset(&handler.sa_mask);
    
    sigaction(SIGTERM, &handler, &termhandler);
    sigaction(SIGINT, &handler, &inthandler);
    sigaction(SIGABRT, &handler, &abrthandler);
    sigaction(SIGBUS, &handler, &crashhandler);
    sigaction(SIGFPE, &handler, &crashhandler);
    sigaction(SIGSEGV, &handler, &crashhandler);
    
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = 0;
    sa.sa_handler = handle_pipe;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPIPE, &sa, nullptr);

    // Проверка /mnt/card
    iVar3 = access("/mnt/card", 0);
    if (iVar3 != 0) {
        system("rm -r /mnt");
        system("ln -s /tmp /mnt");
    }

    // Логирование версии
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : build version information:: %s.\n\n", "main", 
           "version: ba3b5818273ddb65ee40f46cfa891ff80c6da00f commit: 2024-01-22 12:51:34 build: 2024-01-23 05:39:16");
    
    snprintf(tmp1, 0x100, "build version information:: %s.\n", 
             "version: ba3b5818273ddb65ee40f46cfa891ff80c6da00f commit: 2024-01-22 12:51:34 build: 2024-01-23 05:39:16");
    log_to_file(tmp1, time_stamp);

    // Логирование автора
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : author information:: %s.\n\n", "main", 
           "author: harlay      email: harlay.liu@bitmain.com");
    
    snprintf(tmp1, 0x100, "author information:: %s.\n", 
             "author: harlay      email: harlay.liu@bitmain.com");
    log_to_file(tmp1, time_stamp);

    // Парсинг аргументов
    if (argc == 2) {
        g_user_chain = atoi(argv[1]);
        printf("force to use chain %d\n", g_user_chain);
        if (3 < g_user_chain) {
            if (g_user_chain == 0x70) {
                create_midstate_bin();
            }
            printf("chain ID error, should < 3 ");
            return -1;
        }
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : user chain ID %d\n", "main", g_user_chain);
        snprintf(tmp1, 0x100, "user chain ID %d ", g_user_chain);
        log_to_file(tmp1, time_stamp);
    }

    if (2 < argc) {
        g_user_chain = atoi(argv[1]);
        memset(&g_pt1, 0, 0xb0); 
        
        if (3 < argc) {
            // Board SN
            pcVar7 = argv[3];
            sVar4 = strlen(argv[3]);
            memcpy(g_pt1.board_sn, pcVar7, sVar4);
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : input board_sn %s\n", "main", g_pt1.board_sn);
            snprintf(tmp1, 0x100, "input board_sn %s ", g_pt1.board_sn);
            log_to_file(tmp1, time_stamp);

            // Chip Die
            pcVar7 = argv[4];
            sVar4 = strlen(argv[4]);
            memcpy(g_pt1.chip_die, pcVar7, sVar4);
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : input chip_die %s\n", "main", g_pt1.chip_die);
            snprintf(tmp1, 0x100, "input chip_die %s ", g_pt1.chip_die);
            log_to_file(tmp1, time_stamp);

            // Chip Marking
            pcVar7 = argv[5];
            sVar4 = strlen(argv[5]);
            memcpy(g_pt1.chip_marking, pcVar7, sVar4);
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : input chip_marking %s\n", "main", g_pt1.chip_marking);
            snprintf(tmp1, 0x100, "input chip_marking %s ", g_pt1.chip_marking);
            log_to_file(tmp1, time_stamp);

            // Chip FT Version
            pcVar7 = argv[6];
            sVar4 = strlen(argv[6]);
            memcpy(g_pt1.chip_ftversion, pcVar7, sVar4);
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : input chip_ftversion %s\n", "main", g_pt1.chip_ftversion);
            snprintf(tmp1, 0x100, "input chip_ftversion %s ", g_pt1.chip_ftversion);
            log_to_file(tmp1, time_stamp);

            // Chip Tech
            pcVar7 = argv[7];
            sVar4 = strlen(argv[7]);
            memcpy(g_pt1.chip_tech, pcVar7, sVar4);
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : input chip_tech %s\n", "main", g_pt1.chip_tech);
            snprintf(tmp1, 0x100, "input chip_tech %s ", g_pt1.chip_tech);
            log_to_file(tmp1, time_stamp);

            // Board Name
            pcVar7 = argv[8];
            sVar4 = strlen(argv[8]);
            memcpy(g_pt1.board_name, pcVar7, sVar4);
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : input board_name %s\n", "main", g_pt1.board_name);
            snprintf(tmp1, 0x100, "input board_name %s ", g_pt1.board_name);
            log_to_file(tmp1, time_stamp);

            // Factory Job
            pcVar7 = argv[9];
            sVar4 = strlen(argv[9]);
            memcpy(g_pt1.factory_job, pcVar7, sVar4);
            
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : input factory_job %s\n", "main", g_pt1.factory_job);
            snprintf(tmp1, 0x100, "input factory_job %s ", g_pt1.factory_job);
            log_to_file(tmp1, time_stamp);

            // Chip Bin
            g_pt1.chip_bin = atoi(argv[10]);
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : input chip_bin %d\n", "main", g_pt1.chip_bin);
            snprintf(tmp1, 0x100, "input chip_bin %d ", g_pt1.chip_bin);
            log_to_file(tmp1, time_stamp);
        }
    }

    start_upload_logs();
    
    uint32_t fpgaval = 0;
    fpga_read(0, &fpgaval);
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : FPGA hardware ver:%x\n", "main", fpgaval);
    snprintf(tmp1, 0x100, "FPGA hardware ver:%x ", fpgaval);
    log_to_file(tmp1, time_stamp);

    bool _Var2 = prepare_platform_environment();
    if (_Var2) {
        _Var2 = prepare_local_config_information();
        if (_Var2) {
            _Var2 = prepare_pattern();
            if (_Var2) {
                iVar3 = strncmp(Local_Config_Information->Test_Process, "PT2", 3);
                if (iVar3 == 0 || 
                    (iVar3 = strncmp(Local_Config_Information->Test_Process, "PT3", 3), iVar3 == 0) ||
                    (iVar3 = strncmp(Local_Config_Information->Test_Process, "SWEEP", 5), iVar3 == 0)) {
                    
                    _Var2 = fan_protect_check();
                    if (!_Var2) {
                        print_time_stamp(time_stamp, 0x30);
                        printf("%s ", time_stamp);
                        printf("%s : pt2 test,fan speed err,exit.\n", "main");
                        strncpy(tmp1, "pt2 test,fan speed err,exit.\0", 0x1c);
                        log_to_file(tmp1, time_stamp);
                        return 0;
                    }
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : fan speed ok.\n", "main");
                    strncpy(tmp1, "fan speed ok.\0", 0xc);
                    log_to_file(tmp1, time_stamp);
                }

                bitmain_power_off();
                lcd_common_show("Read Power Ver", nullptr, nullptr);
                g_apw_power_version = bitmain_power_version();
                
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : g_apw_power_version:%d\n", "main", g_apw_power_version);
                snprintf(tmp1, 0x100, "g_apw_power_version:%d ", g_apw_power_version);
                log_to_file(tmp1, time_stamp);

                if (g_apw_power_version == 0) {
                    lcd_common_show("Power version", "err", nullptr);
                    bitmain_power_off();
                    iVar6 = 0xc;
                } else {
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : enable power calibration.\n", "main");
                    strncpy(tmp1, "enable power calibration.\0", 0x1a);
                    log_to_file(tmp1, time_stamp);
                    
                    _Var2 = enable_power_calibration();
                    if (!_Var2) {
                        lcd_common_show("Power is not", "calibrated", nullptr);
                        iVar3 = strcmp(Local_Config_Information->Test_Process, "PT2");
                        if (iVar3 == 0 || 
                            (iVar3 = strcmp(Local_Config_Information->Test_Process, "PT3"), iVar3 == 0) ||
                            (iVar3 = strcmp(Local_Config_Information->Test_Process, "SWEEP"), iVar3 == 0)) {
                            bitmain_power_off();
                            return 0xc;
                        }
                    }

                    bitmain_power_off();
                    _Var2 = prepare_test_process();
                    if (_Var2) {
                        print_time_stamp(time_stamp, 0x30);
                        printf("%s ", time_stamp);
                        printf("%s : prepare_test_process\n", "main");
                        strncpy(tmp1, "prepare_test_process\0", 0x14);
                        log_to_file(tmp1, time_stamp);
                        
                        save_software_commit_id();
                        bitmain_power_off();

                        do {
                            if (!Local_Config_Information->Test_Info.Bypass_Scan_Code_Gun && !gFind_test_standard) {
                                gScanCodeGun_data_ready = 0x03;
                                iVar3 = strcmp(Local_Config_Information->Test_Process, "PT2");
                                if ((iVar3 == 0 || 
                                     (iVar3 = strcmp(Local_Config_Information->Test_Process, "PT3"), iVar3 == 0) ||
                                     (iVar3 = strcmp(Local_Config_Information->Test_Process, "SWEEP"), iVar3 == 0) ||
                                     (iVar3 = strcmp(Local_Config_Information->Test_Process, "DropLevel"), iVar3 == 0)) &&
                                    Local_Config_Information->Test_Info.Factory_Mode) {
                                    
                                    print_time_stamp(time_stamp, 0x30);
                                    printf("%s ", time_stamp);
                                    printf("%s : Customer repair mode, use level stored in eeprom\n", "main");
                                    strncpy(tmp1, "Customer repair mode, use level stored in eeprom\0", 0x30);
                                    log_to_file(tmp1, time_stamp);
                                }
                                
                                print_local_config_after_parse_MES_system_information();
                                gFind_test_standard = true;
                                lcd_common_show("Find test std", "Insert Board", "Press Start Key");
                                
                                print_time_stamp(time_stamp, 0x30);
                                printf("%s ", time_stamp);
                                printf("%s : found test standard, Press Start Key to Begin Test\n", "main");
                                strncpy(tmp1, "found test standard, Press Start Key to Begin Test\0", 0x30);
                                log_to_file(tmp1, time_stamp);
                            }

                            iVar6 = read_ipreport_key_val();
                            if (iVar6 == 0) {
                                print_time_stamp(time_stamp, 0x30);
                                printf("%s ", time_stamp);
                                printf("%s : got user input \n", "main");
                                strncpy(tmp1, "got user input \0", 0x10);
                                log_to_file(tmp1, time_stamp);
                                
                                lcd_clear_result();
                                lcd_show(0, "Start Test");
                                g_droplevel = false;

                                while (!check_chain()) {
                                    lcd_clear_result();
                                    lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
                                    lcd_show(1, "No Hashboard");
                                    lcd_show(2, "Insert Board 1st");
                                    lcd_show(3, "Press Start Key");
                                    
                                    print_time_stamp(time_stamp, 0x30);
                                    printf("%s ", time_stamp);
                                    printf("%s : Didn't detect hashboard\n", "main");
                                    strncpy(tmp1, "Didn't detect hashboard\0", 0x18);
                                    log_to_file(tmp1, time_stamp);
                                    
                                    do {
                                        iVar6 = read_ipreport_key_val();
                                    } while (iVar6 != 0);
                                }
                                
                                gScanCodeGun_data_ready = 0;
                                gFind_test_standard = false;
                                _Var2 = read_board_info_from_eeprom();
                                
                                if (_Var2) {
                                    print_local_config_after_parse_MES_system_information();
                                    gFind_test_standard = true;
                                    lcd_common_show("Find test", "standard", "Press Start Key");
                                    
                                    print_time_stamp(time_stamp, 0x30);
                                    printf("%s ", time_stamp);
                                    printf("%s : found test standard, Press Start Key to Begin Test\n", "main");
                                    strncpy(tmp1, "found test standard, Press Start Key to Begin Test\0", 0x30);
                                    log_to_file(tmp1, time_stamp);
                                }
                            }
                            
                            print_time_stamp(time_stamp, 0x30);
                            printf("%s ", time_stamp);
                            printf("%s : build version information:: %s.\n\n", "main", 
                                   "version: ba3b5818273ddb65ee40f46cfa891ff80c6da00f commit: 2024-01-22 12:51:34 build: 2024-01-23 05:39:16");
                            
                            iVar3 = strcmp(Local_Config_Information->Test_Process, "PT1");
                            if (iVar3 == 0) {
                                memset(response_msg, 0, 100);
                                memset(time_stamp, 0, 100);
                                memset(board_sn, 0, 20);
                                strncpy(board_sn, g_pt1.board_sn, 0x14);
                                
                                iVar3 = get_pt1_test_permission(board_sn, response_msg);
                                if (iVar3 == 0) {
                                    uVar5 = Single_Board_PT1_Test();
                                    memset(time_stamp, 0, 100);
                                    memset(response_msg, 0, 100);
                                    iVar3 = send_pt1_test_result(board_sn, uVar5 == 0, uVar5 & 0xff, (uVar5 & 0xff00) >> 8, time_stamp, response_msg);
                                    
                                    if (iVar3 == 0) {
                                        printf("%s : SN:%s %s result:%d send ok to mes\n", "main", board_sn, "PT1", (uint)(uVar5 == 0));
                                    } else if (iVar3 != 0x1234) {
                                        printf("%s : SN:%s %s result:%d send failed to mes, response_msg:%s\n", "main", board_sn, "PT1", (uint)(uVar5 == 0), response_msg);
                                        lcd_clear_result();
                                        lcd_show(0, "send mes failed");
                                        memset(lcd_data, 0, 0x10);
                                        snprintf(lcd_data, 0x10, "%s", response_msg);
                                        lcd_show(1, lcd_data);
                                        if (iVar3 != 1 && iVar3 != -2) {
                                            save_mes_submit_result(true, time_stamp);
                                        }
                                    }
                                } else {
                                    printf("%s : SN:%s %s test is disallowed by MES, response_msg:%s\n", "main", board_sn, "PT1", response_msg);
                                    lcd_clear_result();
                                    lcd_show(0, "mes disallowed");
                                    memset(lcd_data_1, 0, 0x10);
                                    snprintf(lcd_data_1, 0x10, "%s", response_msg);
                                    lcd_show(1, lcd_data_1);
                                }
                            } 
                            else {
                                iVar3 = strcmp(Local_Config_Information->Test_Process, "PT1new");
                                if (iVar3 == 0) {
                                    start_new_log("PT1new", g_pt1.board_sn, Local_Config_Information->Hash_Board.Miner_Type, Local_Config_Information->Hash_Board.Board_Name);
                                    memset(response_msg, 0, 100);
                                    memset(time_stamp, 0, 100);
                                    memset(board_sn_1, 0, 20);
                                    strncpy(board_sn_1, g_pt1.board_sn, 0x14);
                                    
                                    iVar3 = get_pt1_test_permission(board_sn_1, response_msg);
                                    if (iVar3 == 0) {
                                        uVar5 = Single_Board_PT1_New_Test();
                                        memset(time_stamp, 0, 100);
                                        memset(response_msg, 0, 100);
                                        iVar3 = send_pt1_test_result(board_sn_1, uVar5 == 0, uVar5 & 0xff, (uVar5 & 0xff00) >> 8, time_stamp, response_msg);
                                        
                                        if (iVar3 == 0) {
                                            printf("%s : SN:%s %s result:%d send ok to mes\n", "main", board_sn_1, "PT1new", (uint)(uVar5 == 0));
                                        } else if (iVar3 != 0x1234) {
                                            printf("%s : SN:%s %s result:%d send failed to mes, response_msg:%s\n", "main", board_sn_1, "PT1new", (uint)(uVar5 == 0), response_msg);
                                            lcd_clear_result();
                                            lcd_show(0, "send mes failed");
                                            memset(lcd_data_2, 0, 0x10);
                                            snprintf(lcd_data_2, 0x10, "%s", response_msg);
                                            lcd_show(1, lcd_data_2);
                                            if (iVar3 != 1 && iVar3 != -2) {
                                                save_mes_submit_result(true, time_stamp);
                                            }
                                        }
                                    } else {
                                        printf("%s : SN:%s %s test is disallowed by MES, response_msg:%s\n", "main", board_sn_1, "PT1new", response_msg);
                                        lcd_clear_result();
                                        lcd_show(0, "mes disallowed");
                                        memset(lcd_data_3, 0, 0x10);
                                        snprintf(lcd_data_3, 0x10, "%s", response_msg);
                                        lcd_show(1, lcd_data_3);
                                    }
                                }
                                else {
                                    iVar3 = strcmp(Local_Config_Information->Test_Process, "PT1+");
                                    if (iVar3 == 0) {
                                        memset(response_msg, 0, 100);
                                        memset(time_stamp, 0, 100);
                                        memset(board_sn_2, 0, 20);
                                        strncpy(board_sn_2, g_pt1.board_sn, 0x14);
                                        
                                        iVar3 = get_pt1_test_permission(board_sn_2, response_msg);
                                        if (iVar3 == 0) {
                                            uVar5 = Single_Board_PT1_Plus_Test();
                                            memset(time_stamp, 0, 100);
                                            memset(response_msg, 0, 100);
                                            iVar3 = send_pt1_test_result(board_sn_2, uVar5 == 0, uVar5 & 0xff, (uVar5 & 0xff00) >> 8, time_stamp, response_msg);
                                            
                                            if (iVar3 == 0) {
                                                printf("%s : SN:%s %s result:%d send ok to mes\n", "main", board_sn_2, "PT1+", (uint)(uVar5 == 0));
                                            } else if (iVar3 != 0x1234) {
                                                printf("%s : SN:%s %s result:%d send failed to mes, response_msg:%s\n", "main", board_sn_2, "PT1+", (uint)(uVar5 == 0), response_msg);
                                                lcd_clear_result();
                                                lcd_show(0, "send mes failed");
                                                memset(lcd_data_4, 0, 0x10);
                                                snprintf(lcd_data_4, 0x10, "%s", response_msg);
                                                lcd_show(1, lcd_data_4);
                                                if (iVar3 != 1 && iVar3 != -2) {
                                                    save_mes_submit_result(true, time_stamp);
                                                }
                                            }
                                        } else {
                                            printf("%s : SN:%s %s test is disallowed by MES, response_msg:%s\n", "main", board_sn_2, "PT1+", response_msg);
                                            lcd_clear_result();
                                            lcd_show(0, "mes disallowed");
                                            memset(lcd_data_5, 0, 0x10);
                                            snprintf(lcd_data_5, 0x10, "%s", response_msg);
                                            lcd_show(1, lcd_data_5);
                                        }
                                    }
                                    else {
                                        iVar3 = strcmp(Local_Config_Information->Test_Process, "PT2");
                                        if (iVar3 == 0) {
                                            start_new_log("PT2", g_pt1.board_sn, Local_Config_Information->Hash_Board.Miner_Type, Local_Config_Information->Hash_Board.Board_Name);
                                            memset(response_msg, 0, 100);
                                            memset(time_stamp, 0, 100);
                                            memset(board_sn_3, 0, 20);
                                            strncpy(board_sn_3, g_pt1.board_sn, 0x14);
                                            
                                            iVar3 = get_pt2_test_permission(board_sn_3, response_msg);
                                            if (iVar3 == 0) {
                                                uVar5 = Single_Board_PT2_Test();
                                                memset(time_stamp, 0, 100);
                                                memset(response_msg, 0, 100);
                                                iVar3 = send_pt2_test_result(board_sn_3, uVar5 == 0, uVar5 & 0xff, (uVar5 & 0xff00) >> 8, time_stamp, response_msg);
                                                
                                                if (iVar3 == 0) {
                                                    printf("%s : SN:%s %s result:%d send ok to mes\n", "main", board_sn_3, "PT2", (uint)(uVar5 == 0));
                                                } else if (iVar3 != 0x1234) {
                                                    printf("%s : SN:%s %s result:%d send failed to mes, response_msg:%s\n", "main", board_sn_3, "PT2", (uint)(uVar5 == 0), response_msg);
                                                    lcd_clear_result();
                                                    lcd_show(0, "send mes failed");
                                                    memset(lcd_data_6, 0, 0x10);
                                                    snprintf(lcd_data_6, 0x10, "%s", response_msg);
                                                    lcd_show(1, lcd_data_6);
                                                    if (iVar3 != 1 && iVar3 != -2) {
                                                        save_mes_submit_result(false, time_stamp);
                                                    }
                                                }
                                            } else {
                                                printf("%s : SN:%s %s test is disallowed by MES, response_msg:%s\n", "main", board_sn_3, "PT2", response_msg);
                                                lcd_clear_result();
                                                lcd_show(0, "mes disallowed");
                                                memset(lcd_data_7, 0, 0x10);
                                                snprintf(lcd_data_7, 0x10, "%s", response_msg);
                                                lcd_show(1, lcd_data_7);
                                            }
                                        }
                                        else {
                                            iVar3 = strcmp(Local_Config_Information->Test_Process, "SWEEP");
                                            if (iVar3 == 0) {
                                                if (g_eeprom_version == 4) {
                                                    memset(response_msg, 0, 100);
                                                    memset(time_stamp, 0, 100);
                                                    memset(board_sn_4, 0, 20);
                                                    strncpy(board_sn_4, g_pt1.board_sn, 0x14);
                                                    
                                                    iVar3 = get_pt2_test_permission(board_sn_4, response_msg);
                                                    if (iVar3 == 0) {
                                                        uVar5 = Single_Board_PT2_Test();
                                                        memset(time_stamp, 0, 100);
                                                        memset(response_msg, 0, 100);
                                                        iVar3 = send_pt2_test_result(board_sn_4, uVar5 == 0, uVar5 & 0xff, (uVar5 & 0xff00) >> 8, time_stamp, response_msg);
                                                        
                                                        if (iVar3 == 0) {
                                                            printf("%s : SN:%s %s result:%d send ok to mes\n", "main", board_sn_4, "SWEEP", (uint)(uVar5 == 0));
                                                        } else if (iVar3 != 0x1234) {
                                                            printf("%s : SN:%s %s result:%d send failed to mes, response_msg:%s\n", "main", board_sn_4, "SWEEP", (uint)(uVar5 == 0), response_msg);
                                                            lcd_clear_result();
                                                            lcd_show(0, "send mes failed");
                                                            memset(lcd_data_8, 0, 0x10);
                                                            snprintf(lcd_data_8, 0x10, "%s", response_msg);
                                                            lcd_show(1, lcd_data_8);
                                                            if (iVar3 != 1 && iVar3 != -2) {
                                                                save_mes_submit_result(false, time_stamp);
                                                            }
                                                        }
                                                    } else {
                                                        printf("%s : SN:%s %s test is disallowed by MES, response_msg:%s\n", "main", board_sn_4, "SWEEP", response_msg);
                                                        lcd_clear_result();
                                                        lcd_show(0, "mes disallowed");
                                                        memset(lcd_data_9, 0, 0x10);
                                                        snprintf(lcd_data_9, 0x10, "%s", response_msg);
                                                        lcd_show(1, lcd_data_9);
                                                    }
                                                } else {
                                                    memset(response_msg, 0, 100);
                                                    memset(time_stamp, 0, 100);
                                                    memset(board_sn_5, 0, 20);
                                                    strncpy(board_sn_5, g_pt1.board_sn, 0x14);
                                                    
                                                    iVar3 = get_pt2_test_permission(board_sn_5, response_msg);
                                                    if (iVar3 == 0) {
                                                        uVar5 = Single_Board_Sweep_Test();
                                                        memset(time_stamp, 0, 100);
                                                        memset(response_msg, 0, 100);
                                                        iVar3 = send_pt2_test_result(board_sn_5, uVar5 == 0, uVar5 & 0xff, (uVar5 & 0xff00) >> 8, time_stamp, response_msg);
                                                        
                                                        if (iVar3 == 0) {
                                                            printf("%s : SN:%s %s result:%d send ok to mes\n", "main", board_sn_5, "SWEEP", (uint)(uVar5 == 0));
                                                        } else if (iVar3 != 0x1234) {
                                                            printf("%s : SN:%s %s result:%d send failed to mes, response_msg:%s\n", "main", board_sn_5, "SWEEP", (uint)(uVar5 == 0), response_msg);
                                                            lcd_clear_result();
                                                            lcd_show(0, "send mes failed");
                                                            memset(lcd_data_10, 0, 0x10);
                                                            snprintf(lcd_data_10, 0x10, "%s", response_msg);
                                                            lcd_show(1, lcd_data_10);
                                                            if (iVar3 != 1 && iVar3 != -2) {
                                                                save_mes_submit_result(false, time_stamp);
                                                            }
                                                        }
                                                    } else {
                                                        printf("%s : SN:%s %s test is disallowed by MES, response_msg:%s\n", "main", board_sn_5, "SWEEP", response_msg);
                                                        lcd_clear_result();
                                                        lcd_show(0, "mes disallowed");
                                                        memset(lcd_data_11, 0, 0x10);
                                                        snprintf(lcd_data_11, 0x10, "%s", response_msg);
                                                        lcd_show(1, lcd_data_11);
                                                    }
                                                    sweep_exit();
                                                }
                                            }
                                            else {
                                                iVar3 = strcmp(Local_Config_Information->Test_Process, "PT3");
                                                if (iVar3 == 0) {
                                                    if (g_eeprom_version == 4) {
                                                        memset(response_msg, 0, 100);
                                                        memset(time_stamp, 0, 100);
                                                        memset(board_sn_6, 0, 20);
                                                        strncpy(board_sn_6, g_pt1.board_sn, 0x14);
                                                        
                                                        iVar3 = get_pt2_test_permission(board_sn_6, response_msg);
                                                        if (iVar3 == 0) {
                                                            uVar5 = Single_Board_PT2_Test();
                                                            memset(time_stamp, 0, 100);
                                                            memset(response_msg, 0, 100);
                                                            iVar3 = send_pt2_test_result(board_sn_6, uVar5 == 0, uVar5 & 0xff, (uVar5 & 0xff00) >> 8, time_stamp, response_msg);
                                                            
                                                            if (iVar3 == 0) {
                                                                printf("%s : SN:%s %s result:%d send ok to mes\n", "main", board_sn_6, "PT3", (uint)(uVar5 == 0));
                                                            } else if (iVar3 != 0x1234) {
                                                                printf("%s : SN:%s %s result:%d send failed to mes, response_msg:%s\n", "main", board_sn_6, "PT3", (uint)(uVar5 == 0), response_msg);
                                                                lcd_clear_result();
                                                                lcd_show(0, "send mes failed");
                                                                memset(lcd_data_12, 0, 0x10);
                                                                snprintf(lcd_data_12, 0x10, "%s", response_msg);
                                                                lcd_show(1, lcd_data_12);
                                                                if (iVar3 != 1 && iVar3 != -2) {
                                                                    save_mes_submit_result(false, time_stamp);
                                                                }
                                                            }
                                                        } else {
                                                            printf("%s : SN:%s %s test is disallowed by MES, response_msg:%s\n", "main", board_sn_6, "PT3", response_msg);
                                                            lcd_clear_result();
                                                            lcd_show(0, "mes disallowed");
                                                            memset(lcd_data_13, 0, 0x10);
                                                            snprintf(lcd_data_13, 0x10, "%s", response_msg);
                                                            lcd_show(1, lcd_data_13);
                                                        }
                                                    } else {
                                                        memset(response_msg, 0, 100);
                                                        memset(time_stamp, 0, 100);
                                                        memset(board_sn_7, 0, 20);
                                                        strncpy(board_sn_7, g_pt1.board_sn, 0x14);
                                                        
                                                        iVar3 = get_pt2_test_permission(board_sn_7, response_msg);
                                                        if (iVar3 == 0) {
                                                            uVar5 = Single_Board_PT3_Test();
                                                            memset(time_stamp, 0, 100);
                                                            memset(response_msg, 0, 100);
                                                            iVar3 = send_pt2_test_result(board_sn_7, uVar5 == 0, uVar5 & 0xff, (uVar5 & 0xff00) >> 8, time_stamp, response_msg);
                                                            
                                                            if (iVar3 == 0) {
                                                                printf("%s : SN:%s %s result:%d send ok to mes\n", "main", board_sn_7, "PT3", (uint)(uVar5 == 0));
                                                            } else if (iVar3 != 0x1234) {
                                                                printf("%s : SN:%s %s result:%d send failed to mes, response_msg:%s\n", "main", board_sn_7, "PT3", (uint)(uVar5 == 0), response_msg);
                                                                lcd_clear_result();
                                                                lcd_show(0, "send mes failed");
                                                                memset(lcd_data_14, 0, 0x10);
                                                                snprintf(lcd_data_14, 0x10, "%s", response_msg);
                                                                lcd_show(1, lcd_data_14);
                                                                if (iVar3 != 1 && iVar3 != -2) {
                                                                    save_mes_submit_result(false, time_stamp);
                                                                }
                                                            }
                                                        } else {
                                                            printf("%s : SN:%s %s test is disallowed by MES, response_msg:%s\n", "main", board_sn_7, "PT3", response_msg);
                                                            lcd_clear_result();
                                                            lcd_show(0, "mes disallowed");
                                                            memset(lcd_data_15, 0, 0x10);
                                                            snprintf(lcd_data_15, 0x10, "%s", response_msg);
                                                            lcd_show(1, lcd_data_15);
                                                        }
                                                        sweep_exit();
                                                    }
                                                }
                                                else {
                                                    iVar3 = strcmp(Local_Config_Information->Test_Process, "Reset_Low");
                                                    if (iVar3 == 0) {
                                                        Single_Board_Reset_Low_Test();
                                                    }
                                                    else {
                                                        iVar3 = strcmp(Local_Config_Information->Test_Process, "DropLevel");
                                                        if (iVar3 == 0) {
                                                            start_new_log("DropLevel", g_pt1.board_sn, Local_Config_Information->Hash_Board.Miner_Type, Local_Config_Information->Hash_Board.Board_Name);
                                                            memset(response_msg, 0, 100);
                                                            memset(time_stamp, 0, 100);
                                                            memset(board_sn_8, 0, 20);
                                                            strncpy(board_sn_8, g_pt1.board_sn, 0x14);
                                                            
                                                            iVar3 = get_pt2_test_permission(board_sn_8, response_msg);
                                                            if (iVar3 == 0) {
                                                                uVar5 = Single_Board_DropLevel_Test();
                                                                memset(time_stamp, 0, 100);
                                                                memset(response_msg, 0, 100);
                                                                iVar3 = send_pt2_test_result(board_sn_8, uVar5 == 0, uVar5 & 0xff, (uVar5 & 0xff00) >> 8, time_stamp, response_msg);
                                                                
                                                                if (iVar3 == 0) {
                                                                    printf("%s : SN:%s %s result:%d send ok to mes\n", "main", board_sn_8, "DropLevel", (uint)(uVar5 == 0));
                                                                } else if (iVar3 != 0x1234) {
                                                                    printf("%s : SN:%s %s result:%d send failed to mes, response_msg:%s\n", "main", board_sn_8, "DropLevel", (uint)(uVar5 == 0), response_msg);
                                                                    lcd_clear_result();
                                                                    lcd_show(0, "send mes failed");
                                                                    memset(lcd_data_16, 0, 0x10);
                                                                    snprintf(lcd_data_16, 0x10, "%s", response_msg);
                                                                    lcd_show(1, lcd_data_16);
                                                                    if (iVar3 != 1 && iVar3 != -2) {
                                                                        save_mes_submit_result(false, time_stamp);
                                                                    }
                                                                }
                                                            } else {
                                                                printf("%s : SN:%s %s test is disallowed by MES, response_msg:%s\n", "main", board_sn_8, "DropLevel", response_msg);
                                                                lcd_clear_result();
                                                                lcd_show(0, "mes disallowed");
                                                                memset(lcd_data_17, 0, 0x10);
                                                                snprintf(lcd_data_17, 0x10, "%s", response_msg);
                                                                lcd_show(1, lcd_data_17);
                                                            }
                                                            sweep_exit();
                                                        }
                                                        else {
                                                            lcd_common_show("Test Process", "error", nullptr);
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                        } while (true); 
                    }
                }
            }
        }
    }
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s : TEST OVER...\n", "main");
    strncpy(tmp1, "TEST OVER...\0", 0xc);
    log_to_file(tmp1, time_stamp);
    
    submit_cur_log();
    gCheck_chain_status = true;
    
    if (g_user_chain != 0xff) {
        printf("%s : g_user_chain:%d,exit test.\n", "main", g_user_chain);
        sleep(10);
        return 0;
    }
    
    lcd_clear_result();
    lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
    lcd_show(1, "eeprom/pic error");
    printf("%s : can't decode eeprom data, maybe pic lost firmware\n", "main");
    
    if (g_user_chain != 0xff) {
        lcd_show(2, "try PT1 again");
        return -1;
    }
    
    lcd_show(3, "try another board");
    gScanCodeGun_data_ready = 0;
    gCheck_chain_status = true;
    
    while (gCheck_chain_status) {
        check_whether_hashboard_plug_in_again();
        if (gHashBoard_plug_in_again) {
            sleep(1);
            display_main_page();
            gCheck_chain_status = false;
            gHashBoard_plug_in_again = false;
        }
    }
    
    return 0;
}