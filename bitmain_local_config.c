#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>

// --- Типы данных (строго по структуре из псевдокода) ---

// Заглушка для структуры Local_Config_Info, так как её полное определение не предоставлено,
// но она активно используется. В реальном проекте это будет заголовочный файл.
struct Local_Config_Info {
    char Test_Process[16];
    struct {
        bool Bypass_Scan_Code_Gun;
        bool Factory_Mode;
        int Freq_Flex;
        bool Find_Stupid_Asic;
        int Max_PT1_Board_Poweron_Time;
        struct {
            bool Software_Pattern;
            bool Super_Software_Pattern;
            bool Hardware_Pattern;
            bool Autogen_Pattern;
            uint32_t Midstate_Number;
            bool Capture_Pattern;
        } Test_Method;
        struct {
            bool Force_Reset_Power_Mode;
            bool Force_PT2_Before_Sweep;
            bool Data_Reuse_Mode;
            int Freq_Float_Percent;
            bool Force_Reset_Freq_Range;
            int Sweep_Min_Freq;
            int Sweep_Max_Freq;
            bool Force_Local_Cfg_Level;
        } Sweep_Cfg;
        struct {
            uint32_t Pattern_Number;
            uint32_t Invalid_Core_Number;
            uint32_t Least_Nonce_Per_Core;
            uint32_t Nonce_Rate;
            uint32_t Most_HW_Num;
            uint32_t Asic_Nonce_Rate;
            struct {
                uint32_t Level;
                uint32_t Pre_Open_Core_Voltage;
                uint32_t Voltage;
                uint32_t Frequence;
            } Test_Loop[50]; // Предполагаемый размер массива
        } Test_Standard;
        struct {
            uint32_t Baudrate;
            uint32_t Timeout;
        } Test_Speed;
        struct {
            uint32_t CCdly_Sel;
            uint32_t Pwth_Sel;
            uint32_t Swpf_Mode;
            uint32_t Pulse_Mode;
            uint32_t Clk_Sel;
            uint32_t RO_Relay_En;
            uint32_t CO_Relay_En;
            uint32_t Diode_Vdd_Mux_Sel;
        } Asic_Register;
        struct {
            int32_t Start_Temp_Gap_Pic;
            uint32_t Max_Wait_Cooling_Down_Time;
            uint32_t Time_For_Cooling_Hashboard;
            int32_t Max_Board_Temp;
        } Temperature;
        struct {
            uint32_t Fan_Speed;
            uint32_t Fan_Limit;
            uint32_t Work_Fan_Speed;
        } Fan;
        struct {
            bool Display_result_Voltage_Domain;
            bool Display_result_Every_Core;
        } Display;
        char Test_Config_Data_Version[32];
        uint32_t Voltage_Accuracy;
    } Test_Info;
    struct {
        char Miner_Type[32];
        char Board_Name[32];
        char Asic_Type[32];
        uint32_t Asic_Num;
        uint32_t Voltage_Domain;
        uint32_t Asic_Num_Per_Voltage_Domain;
        bool Has_Pic;
        bool Smooth_Voltage;
        uint32_t Inc_Freq_Delay;
        uint32_t Voltage_Adjust_Step;
        uint32_t Wait_Board_Temp;
        struct {
            bool Read_Temperature_From_Pic;
            bool Read_Temperature_From_Asic;
            bool Read_Temperature_From_CtrlBoard;
            struct {
                char Pic_Sensor_Model[32];
            } Pic_Sensor;
            struct {
                char CtrlBoard_Sensor_Model[32];
                uint32_t CtrlBoard_Sensor_Addr[2];
            } CtrlBoard_Sensor;
        } Sensor_Info;
    } Hash_Board;
    struct {
        uint32_t Close_Power_Delay;
        bool Clear_EEPROM_Data;
        bool Enable_Repair;
    } Repair_Mode;
};

// --- Глобальные переменные (строго по адресам и именам из псевдокода) ---

static Local_Config_Info* Local_Config_Information = nullptr; 
static uint32_t gPic_sensor_num = 0;
static uint8_t gPic_sensor_low_3_bits_addr[4] = {};
static uint8_t gPic_temperature_reg_addr = 0;
static uint32_t gCtrlBoard_sensor_num = 0;
static uint8_t gCtrlBoard_sensor_low_3_bits_addr[2] = {};
static uint32_t gTest_loop = 0;

// Внешние функции для работы с JSON (jansson)
extern void* json_load_file(const char* path, int flags, void* error);
extern void* json_object_get(void* object, const char* key);
extern const char* json_string_value(void* string);
extern int64_t json_integer_value(void* integer);
extern int json_array_size(void* array);
extern void* json_array_get(void* array, size_t index);
extern void json_decref(void* json);

// Внешние функции логирования и LCD
extern void print_time_stamp(char* buf, int len);
extern void log_to_file(char* msg, char* time);
extern void _applog(int level, const char* msg, bool newline);
extern void lcd_clear_result();
extern int32_t lcd_show(int line, const char* data);

// Внешние функции проверки сенсоров
extern bool check_pic_sensor_type(const char* model);
extern bool check_ctrlboard_sensor_type(const char* model);
extern bool check_config_information(const char* asic_type);

// --- Функции ---

/**
 * @brief Выделение памяти под структуру конфигурации.
 */
bool malloc_for_local_config_information(void) {
    char time_stamp[48];
    char tmp1[256];

    if (Local_Config_Information == nullptr) {
        Local_Config_Information = static_cast<Local_Config_Info*>(malloc(0x4b0));
        if (Local_Config_Information == nullptr) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : malloc for local config information fail!\n", "malloc_for_local_config_information");
            strncpy(tmp1, "malloc for local config information fail!\0", 0x2a);
            log_to_file(tmp1, time_stamp);
            return false;
        }
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : local config information already malloced\n", "malloc_for_local_config_information");
        strncpy(tmp1, "local config information already malloced\0", 0x2a);
        log_to_file(tmp1, time_stamp);
    }

    memset(Local_Config_Information, 0, 0x4b0);
    
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : malloc for local config information success\n", "malloc_for_local_config_information");
    strncpy(tmp1, "malloc for local config information success\0", 0x2c);
    log_to_file(tmp1, time_stamp);
    
    return true;
}

/**
 * @brief Парсинг JSON файла конфигурации.
 * Строго следует логике вложенных проверок и извлечения значений.
 */
bool parse_local_config_file(char* config_file_path) {
    void* json = nullptr;
    void* pjVar4 = nullptr;
    void* pjVar6 = nullptr;
    void* pjVar7 = nullptr;
    void* pjVar8 = nullptr;
    
    char time_stamp[48];
    char tmp1[256];
    char* pcVar5 = nullptr;
    
    bool ret = true;
    uint32_t index = 0;
    int64_t jVar11 = 0;
    bool _Var3 = false;

    // Массивы для временного хранения указателей на JSON элементы
    void* Pic_Sensor_Addr_array[4] = {};
    void* CtrlBoard_Sensor_Addr_array[2] = {};
    void* Test_Loop_array[50] = {};

    memset(Pic_Sensor_Addr_array, 0, sizeof(Pic_Sensor_Addr_array));
    memset(CtrlBoard_Sensor_Addr_array, 0, sizeof(CtrlBoard_Sensor_Addr_array));
    memset(Test_Loop_array, 0, sizeof(Test_Loop_array));

    json = json_load_file(config_file_path, 0, nullptr);
    if ((json == nullptr) /* || (json->type != JSON_OBJECT) */) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Can't read out local config file : %s\n", "parse_local_config_file", config_file_path);
        snprintf(tmp1, 0x100, "Can't read out local config file : %s\0", config_file_path);
        log_to_file(tmp1, time_stamp);
        ret = false;
    } else {
        pjVar4 = json_object_get(json, "Test_Process");
        if ((pjVar4 == nullptr) /* || (pjVar4->type != JSON_STRING) */) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Can't read out Test_Process in %s\n", "parse_local_config_file", config_file_path);
            snprintf(tmp1, 0x100, "Can't read out Test_Process in %s\0", config_file_path);
            log_to_file(tmp1, time_stamp);
            ret = false;
        } else {
            pcVar5 = const_cast<char*>(json_string_value(pjVar4));
            strcpy(Local_Config_Information->Test_Process, pcVar5);
            
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : Test_Process : %s\n", "parse_local_config_file", Local_Config_Information->Test_Process);
            snprintf(tmp1, 0x100, "Test_Process : %s\0", Local_Config_Information->Test_Process);
            log_to_file(tmp1, time_stamp);

            pjVar4 = json_object_get(json, "Hash_Board");
            if ((pjVar4 == nullptr) /* || (pjVar4->type != JSON_OBJECT) */) {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : Can't read out Hash_Board in %s\n", "parse_local_config_file", config_file_path);
                snprintf(tmp1, 0x100, "Can't read out Hash_Board in %s\0", config_file_path);
                log_to_file(tmp1, time_stamp);
                ret = false;
            } else {
                pjVar6 = json_object_get(pjVar4, "Miner_Type");
                if ((pjVar6 == nullptr) /* || (pjVar6->type != JSON_STRING) */) {
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : Can't read out Miner_Type in %s\n", "parse_local_config_file", config_file_path);
                    snprintf(tmp1, 0x100, "Can't read out Miner_Type in %s\0", config_file_path);
                    log_to_file(tmp1, time_stamp);
                    ret = false;
                } else {
                    pcVar5 = const_cast<char*>(json_string_value(pjVar6));
                    strcpy(Local_Config_Information->Hash_Board.Miner_Type, pcVar5);
                    
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : Miner_Type : %s\n", "parse_local_config_file", Local_Config_Information->Hash_Board.Miner_Type);
                    snprintf(tmp1, 0x100, "Miner_Type : %s\0", Local_Config_Information->Hash_Board.Miner_Type);
                    log_to_file(tmp1, time_stamp);

                    pjVar6 = json_object_get(pjVar4, "Board_Name");
                    if ((pjVar6 == nullptr) /* || (pjVar6->type != JSON_STRING) */) {
                        print_time_stamp(time_stamp, 0x30);
                        printf("%s ", time_stamp);
                        printf("%s : Can't read out Board_Name in %s\n", "parse_local_config_file", config_file_path);
                        snprintf(tmp1, 0x100, "Can't read out Board_Name in %s\0", config_file_path);
                        log_to_file(tmp1, time_stamp);
                        ret = false;
                    } else {
                        pcVar5 = const_cast<char*>(json_string_value(pjVar6));
                        strcpy(Local_Config_Information->Hash_Board.Board_Name, pcVar5);
                        
                        print_time_stamp(time_stamp, 0x30);
                        printf("%s ", time_stamp);
                        printf("%s : Board_Name : %s\n", "parse_local_config_file", Local_Config_Information->Hash_Board.Board_Name);
                        snprintf(tmp1, 0x100, "Board_Name : %s\0", Local_Config_Information->Hash_Board.Board_Name);
                        log_to_file(tmp1, time_stamp);

                        pjVar6 = json_object_get(pjVar4, "Asic_Type");
                        if ((pjVar6 == nullptr) /* || (pjVar6->type != JSON_STRING) */) {
                            print_time_stamp(time_stamp, 0x30);
                            printf("%s ", time_stamp);
                            printf("%s : Can't read out Asic_Type in %s\n", "parse_local_config_file", config_file_path);
                            snprintf(tmp1, 0x100, "Can't read out Asic_Type in %s\0", config_file_path);
                            log_to_file(tmp1, time_stamp);
                            ret = false;
                        } else {
                            pcVar5 = const_cast<char*>(json_string_value(pjVar6));
                            strcpy(Local_Config_Information->Hash_Board.Asic_Type, pcVar5);
                            
                            print_time_stamp(time_stamp, 0x30);
                            printf("%s ", time_stamp);
                            printf("%s : Asic_Type : %s\n", "parse_local_config_file", Local_Config_Information->Hash_Board.Asic_Type);
                            snprintf(tmp1, 0x100, "Asic_Type : %s\0", Local_Config_Information->Hash_Board.Asic_Type);
                            log_to_file(tmp1, time_stamp);

                            pjVar6 = json_object_get(pjVar4, "Asic_Num");
                            if ((pjVar6 == nullptr) /* || (pjVar6->type != JSON_INTEGER) */) {
                                print_time_stamp(time_stamp, 0x30);
                                printf("%s ", time_stamp);
                                printf("%s : Can't read out Asic_Num in %s\n", "parse_local_config_file", config_file_path);
                                snprintf(tmp1, 0x100, "Can't read out Asic_Num in %s\0", config_file_path);
                                log_to_file(tmp1, time_stamp);
                                ret = false;
                            } else {
                                jVar11 = json_integer_value(pjVar6);
                                Local_Config_Information->Hash_Board.Asic_Num = static_cast<uint32_t>(jVar11);
                                
                                print_time_stamp(time_stamp, 0x30);
                                printf("%s ", time_stamp);
                                printf("%s : Asic_Num : %d\n", "parse_local_config_file", Local_Config_Information->Hash_Board.Asic_Num);
                                snprintf(tmp1, 0x100, "Asic_Num : %d\0", Local_Config_Information->Hash_Board.Asic_Num);
                                log_to_file(tmp1, time_stamp);

                                pjVar6 = json_object_get(pjVar4, "Voltage_Domain");
                                if ((pjVar6 == nullptr) /* || (pjVar6->type != JSON_INTEGER) */) {
                                    print_time_stamp(time_stamp, 0x30);
                                    printf("%s ", time_stamp);
                                    printf("%s : Can't read out Voltage_Domain in %s\n", "parse_local_config_file", config_file_path);
                                    snprintf(tmp1, 0x100, "Can't read out Voltage_Domain in %s\0", config_file_path);
                                    log_to_file(tmp1, time_stamp);
                                    ret = false;
                                } else {
                                    jVar11 = json_integer_value(pjVar6);
                                    Local_Config_Information->Hash_Board.Voltage_Domain = static_cast<uint32_t>(jVar11);
                                    
                                    print_time_stamp(time_stamp, 0x30);
                                    printf("%s ", time_stamp);
                                    printf("%s : Voltage_Domain : %d\n", "parse_local_config_file", Local_Config_Information->Hash_Board.Voltage_Domain);
                                    snprintf(tmp1, 0x100, "Voltage_Domain : %d\0", Local_Config_Information->Hash_Board.Voltage_Domain);
                                    log_to_file(tmp1, time_stamp);

                                    pjVar6 = json_object_get(pjVar4, "Asic_Num_Per_Voltage_Domain");
                                    if ((pjVar6 == nullptr) /* || (pjVar6->type != JSON_INTEGER) */) {
                                        print_time_stamp(time_stamp, 0x30);
                                        printf("%s ", time_stamp);
                                        printf("%s : Can't read out Asic_Num_Per_Voltage_Domain in %s\n", "parse_local_config_file", config_file_path);
                                        snprintf(tmp1, 0x100, "Can't read out Asic_Num_Per_Voltage_Domain in %s\0", config_file_path);
                                        log_to_file(tmp1, time_stamp);
                                        ret = false;
                                    } else {
                                        jVar11 = json_integer_value(pjVar6);
                                        Local_Config_Information->Hash_Board.Asic_Num_Per_Voltage_Domain = static_cast<uint32_t>(jVar11);
                                        
                                        print_time_stamp(time_stamp, 0x30);
                                        printf("%s ", time_stamp);
                                        printf("%s : Asic_Num_Per_Voltage_Domain : %d\n", "parse_local_config_file", Local_Config_Information->Hash_Board.Asic_Num_Per_Voltage_Domain);
                                        snprintf(tmp1, 0x100, "Asic_Num_Per_Voltage_Domain : %d\0", Local_Config_Information->Hash_Board.Asic_Num_Per_Voltage_Domain);
                                        log_to_file(tmp1, time_stamp);

                                        pjVar6 = json_object_get(pjVar4, "Has_Pic");
                                        if (((pjVar6 == nullptr) /* || (pjVar6->type != JSON_TRUE) */) &&
                                            ((pjVar6 == nullptr /* || (pjVar6->type != JSON_FALSE) */))) {
                                            print_time_stamp(time_stamp, 0x30);
                                            printf("%s ", time_stamp);
                                            printf("%s : Can't read out Has_Pic in %s,will set default value\n", "parse_local_config_file", config_file_path);
                                            snprintf(tmp1, 0x100, "Can't read out Has_Pic in %s,will set default value\0", config_file_path);
                                            log_to_file(tmp1, time_stamp);
                                            Local_Config_Information->Hash_Board.Has_Pic = true;
                                        } else {
                                            if ((pjVar6 == nullptr) /* || (pjVar6->type != JSON_TRUE) */) {
                                                _Var3 = false;
                                            } else {
                                                _Var3 = true;
                                            }
                                            Local_Config_Information->Hash_Board.Has_Pic = _Var3;
                                            
                                            print_time_stamp(time_stamp, 0x30);
                                            printf("%s ", time_stamp);
                                            pcVar5 = Local_Config_Information->Hash_Board.Has_Pic ? "true" : "false";
                                            printf("%s : Has_Pic : %s\n", "parse_local_config_file", pcVar5);
                                            snprintf(tmp1, 0x100, "Has_Pic : %s\0", pcVar5);
                                            log_to_file(tmp1, time_stamp);
                                        }

                                        pjVar6 = json_object_get(pjVar4, "Smooth_Voltage");
                                        if (((pjVar6 == nullptr) /* || (pjVar6->type != JSON_TRUE) */) &&
                                            ((pjVar6 == nullptr /* || (pjVar6->type != JSON_FALSE) */))) {
                                            print_time_stamp(time_stamp, 0x30);
                                            printf("%s ", time_stamp);
                                            printf("%s : Can't read out Smooth_Voltage in %s,will set default value\n", "parse_local_config_file", config_file_path);
                                            snprintf(tmp1, 0x100, "Can't read out Smooth_Voltage in %s,will set default value\0", config_file_path);
                                            log_to_file(tmp1, time_stamp);
                                            Local_Config_Information->Hash_Board.Smooth_Voltage = false;
                                        } else {
                                            if ((pjVar6 == nullptr) /* || (pjVar6->type != JSON_TRUE) */) {
                                                _Var3 = false;
                                            } else {
                                                _Var3 = true;
                                            }
                                            Local_Config_Information->Hash_Board.Smooth_Voltage = _Var3;
                                            
                                            print_time_stamp(time_stamp, 0x30);
                                            printf("%s ", time_stamp);
                                            pcVar5 = Local_Config_Information->Hash_Board.Smooth_Voltage ? "true" : "false";
                                            printf("%s : Smooth_Voltage : %s\n", "parse_local_config_file", pcVar5);
                                            snprintf(tmp1, 0x100, "Smooth_Voltage : %s\0", pcVar5);
                                            log_to_file(tmp1, time_stamp);
                                        }

                                        pjVar6 = json_object_get(pjVar4, "Inc_Freq_Delay");
                                        if ((pjVar6 == nullptr) /* || (pjVar6->type != JSON_INTEGER) */) {
                                            print_time_stamp(time_stamp, 0x30);
                                            printf("%s ", time_stamp);
                                            printf("%s : Can't read out Inc_Freq_Delay in %s,will set default value\n", "parse_local_config_file", config_file_path);
                                            snprintf(tmp1, 0x100, "Can't read out Inc_Freq_Delay in %s,will set default value\0", config_file_path);
                                            log_to_file(tmp1, time_stamp);
                                            Local_Config_Information->Hash_Board.Inc_Freq_Delay = 400;
                                            
                                            print_time_stamp(time_stamp, 0x30);
                                            printf("%s ", time_stamp);
                                            printf("%s : Inc_Freq_Delay : %d\n", "parse_local_config_file", Local_Config_Information->Hash_Board.Inc_Freq_Delay);
                                            snprintf(tmp1, 0x100, "Inc_Freq_Delay : %d\0", Local_Config_Information->Hash_Board.Inc_Freq_Delay);
                                            log_to_file(tmp1, time_stamp);
                                        } else {
                                            jVar11 = json_integer_value(pjVar6);
                                            Local_Config_Information->Hash_Board.Inc_Freq_Delay = static_cast<uint32_t>(jVar11);
                                            
                                            print_time_stamp(time_stamp, 0x30);
                                            printf("%s ", time_stamp);
                                            printf("%s : Inc_Freq_Delay : %d\n", "parse_local_config_file", Local_Config_Information->Hash_Board.Inc_Freq_Delay);
                                            snprintf(tmp1, 0x100, "Inc_Freq_Delay : %d\0", Local_Config_Information->Hash_Board.Inc_Freq_Delay);
                                            log_to_file(tmp1, time_stamp);
                                        }

                                        pjVar6 = json_object_get(pjVar4, "Voltage_Adjust_Step");
                                        if ((pjVar6 == nullptr) /* || (pjVar6->type != JSON_INTEGER) */) {
                                            print_time_stamp(time_stamp, 0x30);
                                            printf("%s ", time_stamp);
                                            printf("%s : Can't read out Voltage_Adjust_Step in %s,will set default value\n", "parse_local_config_file", config_file_path);
                                            snprintf(tmp1, 0x100, "Can't read out Voltage_Adjust_Step in %s,will set default value\0", config_file_path);
                                            log_to_file(tmp1, time_stamp);
                                            Local_Config_Information->Hash_Board.Voltage_Adjust_Step = 5;
                                            
                                            print_time_stamp(time_stamp, 0x30);
                                            printf("%s ", time_stamp);
                                            printf("%s : Voltage_Adjust_Step : %d\n", "parse_local_config_file", Local_Config_Information->Hash_Board.Voltage_Adjust_Step);
                                            snprintf(tmp1, 0x100, "Voltage_Adjust_Step : %d\0", Local_Config_Information->Hash_Board.Voltage_Adjust_Step);
                                            log_to_file(tmp1, time_stamp);
                                        } else {
                                            jVar11 = json_integer_value(pjVar6);
                                            Local_Config_Information->Hash_Board.Voltage_Adjust_Step = static_cast<uint32_t>(jVar11);
                                            
                                            print_time_stamp(time_stamp, 0x30);
                                            printf("%s ", time_stamp);
                                            printf("%s : Voltage_Adjust_Step : %d\n", "parse_local_config_file", Local_Config_Information->Hash_Board.Voltage_Adjust_Step);
                                            snprintf(tmp1, 0x100, "Voltage_Adjust_Step : %d\0", Local_Config_Information->Hash_Board.Voltage_Adjust_Step);
                                            log_to_file(tmp1, time_stamp);
                                        }

                                        pjVar6 = json_object_get(pjVar4, "Wait_Board_Temp");
                                        if ((pjVar6 == nullptr) /* || (pjVar6->type != JSON_INTEGER) */) {
                                            print_time_stamp(time_stamp, 0x30);
                                            printf("%s ", time_stamp);
                                            printf("%s : Can't read out Wait_Board_Temp in %s,will set default value\n", "parse_local_config_file", config_file_path);
                                            snprintf(tmp1, 0x100, "Can't read out Wait_Board_Temp in %s,will set default value\0", config_file_path);
                                            log_to_file(tmp1, time_stamp);
                                            Local_Config_Information->Hash_Board.Wait_Board_Temp = 0;
                                            
                                            print_time_stamp(time_stamp, 0x30);
                                            printf("%s ", time_stamp);
                                            printf("%s : Wait_Board_Temp : %d\n", "parse_local_config_file", Local_Config_Information->Hash_Board.Wait_Board_Temp);
                                            snprintf(tmp1, 0x100, "Wait_Board_Temp : %d\0", Local_Config_Information->Hash_Board.Wait_Board_Temp);
                                            log_to_file(tmp1, time_stamp);
                                        } else {
                                            jVar11 = json_integer_value(pjVar6);
                                            Local_Config_Information->Hash_Board.Wait_Board_Temp = static_cast<uint32_t>(jVar11);
                                            
                                            print_time_stamp(time_stamp, 0x30);
                                            printf("%s ", time_stamp);
                                            printf("%s : Wait_Board_Temp : %d\n", "parse_local_config_file", Local_Config_Information->Hash_Board.Wait_Board_Temp);
                                            snprintf(tmp1, 0x100, "Wait_Board_Temp : %d\0", Local_Config_Information->Hash_Board.Wait_Board_Temp);
                                            log_to_file(tmp1, time_stamp);
                                        }

                                        pjVar6 = json_object_get(pjVar4, "Sensor_Info");
                                        if ((pjVar6 == nullptr) /* || (pjVar6->type != JSON_OBJECT) */) {
                                            print_time_stamp(time_stamp, 0x30);
                                            printf("%s ", time_stamp);
                                            printf("%s : Can't read out Sensor_Info in %s\n", "parse_local_config_file", config_file_path);
                                            snprintf(tmp1, 0x100, "Can't read out Sensor_Info in %s\0", config_file_path);
                                            log_to_file(tmp1, time_stamp);
                                            ret = false;
                                        } else {
                                            pjVar7 = json_object_get(pjVar6, "Read_Temperature_From_Pic");
                                            if (((pjVar7 == nullptr) /* || (pjVar7->type != JSON_TRUE) */) &&
                                                ((pjVar7 == nullptr /* || (pjVar7->type != JSON_FALSE) */))) {
                                                print_time_stamp(time_stamp, 0x30);
                                                printf("%s ", time_stamp);
                                                printf("%s : Can't read out Read_Temperature_From_Pic in %s\n", "parse_local_config_file", config_file_path);
                                                snprintf(tmp1, 0x100, "Can't read out Read_Temperature_From_Pic in %s\0", config_file_path);
                                                log_to_file(tmp1, time_stamp);
                                                ret = false;
                                            } else {
                                                if ((pjVar7 == nullptr) /* || (pjVar7->type != JSON_TRUE) */) {
                                                    _Var3 = false;
                                                } else {
                                                    _Var3 = true;
                                                }
                                                Local_Config_Information->Hash_Board.Sensor_Info.Read_Temperature_From_Pic = _Var3;
                                                
                                                print_time_stamp(time_stamp, 0x30);
                                                printf("%s ", time_stamp);
                                                pcVar5 = Local_Config_Information->Hash_Board.Sensor_Info.Read_Temperature_From_Pic ? "true" : "false";
                                                printf("%s : Read_Temperature_From_Pic : %s\n", "parse_local_config_file", pcVar5);
                                                snprintf(tmp1, 0x100, "Read_Temperature_From_Pic : %s\0", pcVar5);
                                                log_to_file(tmp1, time_stamp);

                                                pjVar7 = json_object_get(pjVar6, "Read_Temperature_From_Asic");
                                                if (((pjVar7 == nullptr) /* || (pjVar7->type != JSON_TRUE) */) &&
                                                    ((pjVar7 == nullptr /* || (pjVar7->type != JSON_FALSE) */))) {
                                                    print_time_stamp(time_stamp, 0x30);
                                                    printf("%s ", time_stamp);
                                                    printf("%s : Can't read out Read_Temperature_From_Asic in %s\n", "parse_local_config_file", config_file_path);
                                                    snprintf(tmp1, 0x100, "Can't read out Read_Temperature_From_Asic in %s\0", config_file_path);
                                                    log_to_file(tmp1, time_stamp);
                                                    ret = false;
                                                } else {
                                                    if ((pjVar7 == nullptr) /* || (pjVar7->type != JSON_TRUE) */) {
                                                        _Var3 = false;
                                                    } else {
                                                        _Var3 = true;
                                                    }
                                                    Local_Config_Information->Hash_Board.Sensor_Info.Read_Temperature_From_Asic = _Var3;
                                                    
                                                    print_time_stamp(time_stamp, 0x30);
                                                    printf("%s ", time_stamp);
                                                    pcVar5 = Local_Config_Information->Hash_Board.Sensor_Info.Read_Temperature_From_Asic ? "true" : "false";
                                                    printf("%s : Read_Temperature_From_Asic : %s\n", "parse_local_config_file", pcVar5);
                                                    snprintf(tmp1, 0x100, "Read_Temperature_From_Asic : %s\0", pcVar5);
                                                    log_to_file(tmp1, time_stamp);

                                                    pjVar7 = json_object_get(pjVar6, "Read_Temperature_From_CtrlBoard");
                                                    if (((pjVar7 == nullptr) /* || (pjVar7->type != JSON_TRUE) */) &&
                                                        ((pjVar7 == nullptr /* || (pjVar7->type != JSON_FALSE) */))) {
                                                        print_time_stamp(time_stamp, 0x30);
                                                        printf("%s ", time_stamp);
                                                        printf("%s : Can't read out Read_Temperature_From_CtrlBoard in %s,will set default value\n", "parse_local_config_file", config_file_path);
                                                        snprintf(tmp1, 0x100, "Can't read out Read_Temperature_From_CtrlBoard in %s,will set default value\0", config_file_path);
                                                        log_to_file(tmp1, time_stamp);
                                                        Local_Config_Information->Hash_Board.Sensor_Info.Read_Temperature_From_CtrlBoard = false;
                                                    } else {
                                                        if ((pjVar7 == nullptr) /* || (pjVar7->type != JSON_TRUE) */) {
                                                            _Var3 = false;
                                                        } else {
                                                            _Var3 = true;
                                                        }
                                                        Local_Config_Information->Hash_Board.Sensor_Info.Read_Temperature_From_CtrlBoard = _Var3;
                                                        
                                                        print_time_stamp(time_stamp, 0x30);
                                                        printf("%s ", time_stamp);
                                                        pcVar5 = Local_Config_Information->Hash_Board.Sensor_Info.Read_Temperature_From_CtrlBoard ? "true" : "false";
                                                        printf("%s : Read_Temperature_From_CtrlBoard : %s\n", "parse_local_config_file", pcVar5);
                                                        snprintf(tmp1, 0x100, "Read_Temperature_From_CtrlBoard : %s\0", pcVar5);
                                                        log_to_file(tmp1, time_stamp);
                                                    }

                                                    pjVar7 = json_object_get(pjVar6, "Pic_Sensor");
                                                    if ((pjVar7 == nullptr) /* || (pjVar7->type != JSON_OBJECT) */) {
                                                        print_time_stamp(time_stamp, 0x30);
                                                        printf("%s ", time_stamp);
                                                        printf("%s : Can't read out Pic_Sensor in %s\n", "parse_local_config_file", config_file_path);
                                                        snprintf(tmp1, 0x100, "Can't read out Pic_Sensor in %s\0", config_file_path);
                                                        log_to_file(tmp1, time_stamp);
                                                        ret = false;
                                                    } else {
                                                        pjVar8 = json_object_get(pjVar7, "Pic_Sensor_Model");
                                                        if ((pjVar8 == nullptr) /* || (pjVar8->type != JSON_STRING) */) {
                                                            print_time_stamp(time_stamp, 0x30);
                                                            printf("%s ", time_stamp);
                                                            printf("%s : Can't read out Pic_Sensor_Model in %s\n", "parse_local_config_file", config_file_path);
                                                            snprintf(tmp1, 0x100, "Can't read out Pic_Sensor_Model in %s\0", config_file_path);
                                                            log_to_file(tmp1, time_stamp);
                                                            ret = false;
                                                        } else {
                                                            pcVar5 = const_cast<char*>(json_string_value(pjVar8));
                                                            strcpy(Local_Config_Information->Hash_Board.Sensor_Info.Pic_Sensor.Pic_Sensor_Model, pcVar5);
                                                            
                                                            print_time_stamp(time_stamp, 0x30);
                                                            printf("%s ", time_stamp);
                                                            printf("%s : Pic_Sensor_Model : %s\n", "parse_local_config_file", Local_Config_Information->Hash_Board.Sensor_Info.Pic_Sensor.Pic_Sensor_Model);
                                                            snprintf(tmp1, 0x100, "Pic_Sensor_Model : %s\0", Local_Config_Information->Hash_Board.Sensor_Info.Pic_Sensor.Pic_Sensor_Model);
                                                            log_to_file(tmp1, time_stamp);

                                                            _Var3 = check_pic_sensor_type(Local_Config_Information->Hash_Board.Sensor_Info.Pic_Sensor.Pic_Sensor_Model);
                                                            if (!_Var3) {
                                                                if (json != nullptr) json_decref(json);
                                                                return false;
                                                            }

                                                            pjVar7 = json_object_get(pjVar7, "Pic_Sensor_Addr");
                                                            if ((pjVar7 == nullptr) /* || (pjVar7->type != JSON_ARRAY) */) {
                                                                print_time_stamp(time_stamp, 0x30);
                                                                printf("%s ", time_stamp);
                                                                printf("%s : Can't read out Pic_Sensor_Addr in %s\n", "parse_local_config_file", config_file_path);
                                                                snprintf(tmp1, 0x100, "Can't read out Pic_Sensor_Addr in %s\0", config_file_path);
                                                                log_to_file(tmp1, time_stamp);
                                                                ret = false;
                                                            } else {
                                                                gPic_sensor_num = json_array_size(pjVar7);
                                                                if (gPic_sensor_num < 5) {
                                                                    print_time_stamp(time_stamp, 0x30);
                                                                    printf("%s ", time_stamp);
                                                                    printf("%s : gPic_sensor_num : %d\n", "parse_local_config_file", gPic_sensor_num);
                                                                    snprintf(tmp1, 0x100, "gPic_sensor_num : %d\0", gPic_sensor_num);
                                                                    log_to_file(tmp1, time_stamp);

                                                                    memset(gPic_sensor_low_3_bits_addr, 0, 4);
                                                                    for (index = 0; index < gPic_sensor_num; index++) {
                                                                        pjVar8 = json_array_get(pjVar7, index);
                                                                        Pic_Sensor_Addr_array[index] = pjVar8;
                                                                        if ((Pic_Sensor_Addr_array[index] == nullptr) /* || (Pic_Sensor_Addr_array[index]->type != JSON_INTEGER) */) {
                                                                            print_time_stamp(time_stamp, 0x30);
                                                                            printf("%s ", time_stamp);
                                                                            printf("%s : Can't find Pic_Sensor_Addr_array[%d]\n", "parse_local_config_file", index);
                                                                            snprintf(tmp1, 0x100, "Can't find Pic_Sensor_Addr_array[%d]\0", index);
                                                                            log_to_file(tmp1, time_stamp);
                                                                            ret = false;
                                                                            goto LAB_EXIT;
                                                                        }
                                                                        jVar11 = json_integer_value(Pic_Sensor_Addr_array[index]);
                                                                        gPic_sensor_low_3_bits_addr[index] = static_cast<uint8_t>(jVar11);
                                                                        
                                                                        print_time_stamp(time_stamp, 0x30);
                                                                        printf("%s ", time_stamp);
                                                                        printf("%s : gPic_sensor_low_3_bits_addr[%d] : %d\n", "parse_local_config_file", index, gPic_sensor_low_3_bits_addr[index]);
                                                                        snprintf(tmp1, 0x100, "gPic_sensor_low_3_bits_addr[%d] : %d\0", index, gPic_sensor_low_3_bits_addr[index]);
                                                                        log_to_file(tmp1, time_stamp);
                                                                    }

                                                                    pjVar6 = json_object_get(pjVar6, "CtrlBoard_Sensor");
                                                                    if ((pjVar6 == nullptr) /* || (pjVar6->type != JSON_OBJECT) */) {
                                                                        print_time_stamp(time_stamp, 0x30);
                                                                        printf("%s ", time_stamp);
                                                                        printf("%s : Can't read out CtrlBoard_Sensor in %s\n", "parse_local_config_file", config_file_path);
                                                                        snprintf(tmp1, 0x100, "Can't read out CtrlBoard_Sensor in %s\0", config_file_path);
                                                                        log_to_file(tmp1, time_stamp);
                                                                        ret = false;
                                                                    } else {
                                                                        pjVar7 = json_object_get(pjVar6, "CtrlBoard_Sensor_Model");
                                                                        if ((pjVar7 == nullptr) /* || (pjVar7->type != JSON_STRING) */) {
                                                                            print_time_stamp(time_stamp, 0x30);
                                                                            printf("%s ", time_stamp);
                                                                            printf("%s : Can't read out CtrlBoard_Sensor_Model in %s\n", "parse_local_config_file", config_file_path);
                                                                            snprintf(tmp1, 0x100, "Can't read out CtrlBoard_Sensor_Model in %s\0", config_file_path);
                                                                            log_to_file(tmp1, time_stamp);
                                                                            ret = false;
                                                                        } else {
                                                                            pcVar5 = const_cast<char*>(json_string_value(pjVar7));
                                                                            strcpy(Local_Config_Information->Hash_Board.Sensor_Info.CtrlBoard_Sensor.CtrlBoard_Sensor_Model, pcVar5);
                                                                            
                                                                            print_time_stamp(time_stamp, 0x30);
                                                                            printf("%s ", time_stamp);
                                                                            printf("%s : CtrlBoard_Sensor_Model : %s\n", "parse_local_config_file", Local_Config_Information->Hash_Board.Sensor_Info.CtrlBoard_Sensor.CtrlBoard_Sensor_Model);
                                                                            snprintf(tmp1, 0x100, "CtrlBoard_Sensor_Model : %s\0", Local_Config_Information->Hash_Board.Sensor_Info.CtrlBoard_Sensor.CtrlBoard_Sensor_Model);
                                                                            log_to_file(tmp1, time_stamp);

                                                                            _Var3 = check_ctrlboard_sensor_type(Local_Config_Information->Hash_Board.Sensor_Info.CtrlBoard_Sensor.CtrlBoard_Sensor_Model);
                                                                            if (!_Var3) {
                                                                                if (json != nullptr) json_decref(json);
                                                                                return false;
                                                                            }

                                                                            pjVar6 = json_object_get(pjVar6, "CtrlBoard_Sensor_Addr");
                                                                            if ((pjVar6 == nullptr) /* || (pjVar6->type != JSON_ARRAY) */) {
                                                                                print_time_stamp(time_stamp, 0x30);
                                                                                printf("%s ", time_stamp);
                                                                                printf("%s : Can't read out CtrlBoard_Sensor_Addr in %s\n", "parse_local_config_file", config_file_path);
                                                                                snprintf(tmp1, 0x100, "Can't read out CtrlBoard_Sensor_Addr in %s\0", config_file_path);
                                                                                log_to_file(tmp1, time_stamp);
                                                                                ret = false;
                                                                            } else {
                                                                                gCtrlBoard_sensor_num = json_array_size(pjVar6);
                                                                                if (gCtrlBoard_sensor_num < 3) {
                                                                                    print_time_stamp(time_stamp, 0x30);
                                                                                    printf("%s ", time_stamp);
                                                                                    printf("%s : gCtrlBoard_sensor_num : %d\n", "parse_local_config_file", gCtrlBoard_sensor_num);
                                                                                    snprintf(tmp1, 0x100, "gCtrlBoard_sensor_num : %d\0", gCtrlBoard_sensor_num);
                                                                                    log_to_file(tmp1, time_stamp);

                                                                                    memset(gCtrlBoard_sensor_low_3_bits_addr, 0, 2);
                                                                                    for (index = 0; index < gCtrlBoard_sensor_num; index++) {
                                                                                        pjVar7 = json_array_get(pjVar6, index);
                                                                                        CtrlBoard_Sensor_Addr_array[index] = pjVar7;
                                                                                        if ((CtrlBoard_Sensor_Addr_array[index] == nullptr) /* || (CtrlBoard_Sensor_Addr_array[index]->type != JSON_INTEGER) */) {
                                                                                            print_time_stamp(time_stamp, 0x30);
                                                                                            printf("%s ", time_stamp);
                                                                                            printf("%s : Can't find CtrlBoard_Sensor_Addr_array[%d]\n", "parse_local_config_file", index);
                                                                                            snprintf(tmp1, 0x100, "Can't find CtrlBoard_Sensor_Addr_array[%d]\0", index);
                                                                                            log_to_file(tmp1, time_stamp);
                                                                                            ret = false;
                                                                                            goto LAB_EXIT;
                                                                                        }
                                                                                        jVar11 = json_integer_value(CtrlBoard_Sensor_Addr_array[index]);
                                                                                        gCtrlBoard_sensor_low_3_bits_addr[index] = static_cast<uint8_t>(jVar11);
                                                                                        
                                                                                        print_time_stamp(time_stamp, 0x30);
                                                                                        printf("%s ", time_stamp);
                                                                                        printf("%s : gCtrlBoard_sensor_low_3_bits_addr[%d] : %d\n", "parse_local_config_file", index, gCtrlBoard_sensor_low_3_bits_addr[index]);
                                                                                        snprintf(tmp1, 0x100, "gCtrlBoard_sensor_low_3_bits_addr[%d] : %d\0", index, gCtrlBoard_sensor_low_3_bits_addr[index]);
                                                                                        log_to_file(tmp1, time_stamp);
                                                                                        
                                                                                        Local_Config_Information->Hash_Board.Sensor_Info.CtrlBoard_Sensor.CtrlBoard_Sensor_Addr[index] = static_cast<uint32_t>(gCtrlBoard_sensor_low_3_bits_addr[index]);
                                                                                    }
                                                                                } else {
                                                                                    print_time_stamp(time_stamp, 0x30);
                                                                                    printf("%s ", time_stamp);
                                                                                    printf("%s : There are %d ctrlboard sensors in local config file, but LOCAL_CONFIG_MAX_CTRLBOARD_SENSOR_NUM is %d\n", "parse_local_config_file", gCtrlBoard_sensor_num, 2);
                                                                                    snprintf(tmp1, 0x100, "There are %d ctrlboard sensors in local config file, but LOCAL_CONFIG_MAX_CTRLBOARD_SENSOR_NUM is %d\0", gCtrlBoard_sensor_num, 2);
                                                                                    log_to_file(tmp1, time_stamp);
                                                                                    ret = false;
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                } else {
                                                                    print_time_stamp(time_stamp, 0x30);
                                                                    printf("%s ", time_stamp);
                                                                    printf("%s : There are %d pic sensors in local config file, but LOCAL_CONFIG_MAX_PIC_SENSOR_NUM is %d\n", "parse_local_config_file", gPic_sensor_num, 4);
                                                                    snprintf(tmp1, 0x100, "There are %d pic sensors in local config file, but LOCAL_CONFIG_MAX_PIC_SENSOR_NUM is %d\0", gPic_sensor_num, 4);
                                                                    log_to_file(tmp1, time_stamp);
                                                                    ret = false;
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

LAB_EXIT:
    if (json != nullptr) {
        json_decref(json);
    }
    return ret;
}

/**
 * @brief Печать локальной конфигурации после парсинга.
 */
void print_local_config_after_parse_MES_system_information(void) {
    char time_stamp[48];
    char tmp1[256];
    char* pcVar1 = nullptr;
    uint32_t index = 0;

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : \n", "print_local_config_after_parse_MES_system_information");
    tmp1[0] = ' ';
    tmp1[1] = '\0';
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Test_Process : %s\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Process);
    snprintf(tmp1, 0x100, "Test_Process : %s\0", Local_Config_Information->Test_Process);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Miner_Type : %s\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Hash_Board.Miner_Type);
    snprintf(tmp1, 0x100, "Miner_Type : %s\0", Local_Config_Information->Hash_Board.Miner_Type);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Board_Name : %s\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Hash_Board.Board_Name);
    snprintf(tmp1, 0x100, "Board_Name : %s\0", Local_Config_Information->Hash_Board.Board_Name);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Asic_Type : %s\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Hash_Board.Asic_Type);
    snprintf(tmp1, 0x100, "Asic_Type : %s\0", Local_Config_Information->Hash_Board.Asic_Type);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Asic_Num : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Hash_Board.Asic_Num);
    snprintf(tmp1, 0x100, "Asic_Num : %d\0", Local_Config_Information->Hash_Board.Asic_Num);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Voltage_Domain : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Hash_Board.Voltage_Domain);
    snprintf(tmp1, 0x100, "Voltage_Domain : %d\0", Local_Config_Information->Hash_Board.Voltage_Domain);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Asic_Num_Per_Voltage_Domain : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Hash_Board.Asic_Num_Per_Voltage_Domain);
    snprintf(tmp1, 0x100, "Asic_Num_Per_Voltage_Domain : %d\0", Local_Config_Information->Hash_Board.Asic_Num_Per_Voltage_Domain);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    pcVar1 = Local_Config_Information->Hash_Board.Sensor_Info.Read_Temperature_From_Pic ? "true" : "false";
    printf("%s : Read_Temperature_From_Pic : %s\n", "print_local_config_after_parse_MES_system_information", pcVar1);
    snprintf(tmp1, 0x100, "Read_Temperature_From_Pic : %s\0", pcVar1);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    pcVar1 = Local_Config_Information->Hash_Board.Sensor_Info.Read_Temperature_From_Asic ? "true" : "false";
    printf("%s : Read_Temperature_From_Asic : %s\n", "print_local_config_after_parse_MES_system_information", pcVar1);
    snprintf(tmp1, 0x100, "Read_Temperature_From_Asic : %s\0", pcVar1);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    pcVar1 = Local_Config_Information->Hash_Board.Sensor_Info.Read_Temperature_From_CtrlBoard ? "true" : "false";
    printf("%s : Read_Temperature_From_CtrlBoard : %s\n", "print_local_config_after_parse_MES_system_information", pcVar1);
    snprintf(tmp1, 0x100, "Read_Temperature_From_CtrlBoard : %s\0", pcVar1);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Pic_Sensor_Model : %s\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Hash_Board.Sensor_Info.Pic_Sensor.Pic_Sensor_Model);
    snprintf(tmp1, 0x100, "Pic_Sensor_Model : %s\0", Local_Config_Information->Hash_Board.Sensor_Info.Pic_Sensor.Pic_Sensor_Model);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : gPic_sensor_num : %d\n", "print_local_config_after_parse_MES_system_information", gPic_sensor_num);
    snprintf(tmp1, 0x100, "gPic_sensor_num : %d\0", gPic_sensor_num);
    log_to_file(tmp1, time_stamp);

    for (index = 0; index < gPic_sensor_num; index++) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : gPic_sensor_low_3_bits_addr[%d] : %d\n", "print_local_config_after_parse_MES_system_information", index, gPic_sensor_low_3_bits_addr[index]);
        snprintf(tmp1, 0x100, "gPic_sensor_low_3_bits_addr[%d] : %d\0", index, gPic_sensor_low_3_bits_addr[index]);
        log_to_file(tmp1, time_stamp);
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : CtrlBoard_Sensor_Model : %s\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Hash_Board.Sensor_Info.CtrlBoard_Sensor.CtrlBoard_Sensor_Model);
    snprintf(tmp1, 0x100, "CtrlBoard_Sensor_Model : %s\0", Local_Config_Information->Hash_Board.Sensor_Info.CtrlBoard_Sensor.CtrlBoard_Sensor_Model);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : gCtrlBoard_sensor_num : %d\n", "print_local_config_after_parse_MES_system_information", gCtrlBoard_sensor_num);
    snprintf(tmp1, 0x100, "gCtrlBoard_sensor_num : %d\0", gCtrlBoard_sensor_num);
    log_to_file(tmp1, time_stamp);

    for (index = 0; index < gCtrlBoard_sensor_num; index++) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : gCtrlBoard_sensor_low_3_bits_addr[%d] : %d\n", "print_local_config_after_parse_MES_system_information", index, gCtrlBoard_sensor_low_3_bits_addr[index]);
        snprintf(tmp1, 0x100, "gCtrlBoard_sensor_low_3_bits_addr[%d] : %d\0", index, gCtrlBoard_sensor_low_3_bits_addr[index]);
        log_to_file(tmp1, time_stamp);
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    pcVar1 = Local_Config_Information->Test_Info.Factory_Mode ? "true" : "false";
    printf("%s : Factory_Mode : %s\n", "print_local_config_after_parse_MES_system_information", pcVar1);
    snprintf(tmp1, 0x100, "Factory_Mode : %s\0", pcVar1);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    pcVar1 = Local_Config_Information->Test_Info.Bypass_Scan_Code_Gun ? "true" : "false";
    printf("%s : Bypass_Scan_Code_Gun : %s\n", "print_local_config_after_parse_MES_system_information", pcVar1);
    snprintf(tmp1, 0x100, "Bypass_Scan_Code_Gun : %s\0", pcVar1);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Max_PT1_Board_Poweron_Time : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Max_PT1_Board_Poweron_Time);
    snprintf(tmp1, 0x100, "Max_PT1_Board_Poweron_Time : %d\0", Local_Config_Information->Test_Info.Max_PT1_Board_Poweron_Time);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    pcVar1 = Local_Config_Information->Test_Info.Test_Method.Software_Pattern ? "true" : "false";
    printf("%s : Software_Pattern : %s\n", "print_local_config_after_parse_MES_system_information", pcVar1);
    snprintf(tmp1, 0x100, "Software_Pattern : %s\0", pcVar1);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    pcVar1 = Local_Config_Information->Test_Info.Test_Method.Super_Software_Pattern ? "true" : "false";
    printf("%s : Super_Software_Pattern : %s\n", "print_local_config_after_parse_MES_system_information", pcVar1);
    snprintf(tmp1, 0x100, "Super_Software_Pattern : %s\0", pcVar1);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    pcVar1 = Local_Config_Information->Test_Info.Test_Method.Hardware_Pattern ? "true" : "false";
    printf("%s : Hardware_Pattern : %s\n", "print_local_config_after_parse_MES_system_information", pcVar1);
    snprintf(tmp1, 0x100, "Hardware_Pattern : %s\0", pcVar1);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    pcVar1 = Local_Config_Information->Test_Info.Test_Method.Autogen_Pattern ? "true" : "false";
    printf("%s : Autogen_Pattern : %s\n", "print_local_config_after_parse_MES_system_information", pcVar1);
    snprintf(tmp1, 0x100, "Autogen_Pattern : %s\0", pcVar1);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Midstate_Number : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Test_Method.Midstate_Number);
    snprintf(tmp1, 0x100, "Midstate_Number : %d\0", Local_Config_Information->Test_Info.Test_Method.Midstate_Number);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Pattern_Number : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Test_Standard.Pattern_Number);
    snprintf(tmp1, 0x100, "Pattern_Number : %d\0", Local_Config_Information->Test_Info.Test_Standard.Pattern_Number);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Invalid_Core_Number : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Test_Standard.Invalid_Core_Number);
    snprintf(tmp1, 0x100, "Invalid_Core_Number : %d\0", Local_Config_Information->Test_Info.Test_Standard.Invalid_Core_Number);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Least_Nonce_Per_Core : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Test_Standard.Least_Nonce_Per_Core);
    snprintf(tmp1, 0x100, "Least_Nonce_Per_Core : %d\0", Local_Config_Information->Test_Info.Test_Standard.Least_Nonce_Per_Core);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Asic_Nonce_Rate : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Test_Standard.Asic_Nonce_Rate);
    snprintf(tmp1, 0x100, "Asic_Nonce_Rate : %d\0", Local_Config_Information->Test_Info.Test_Standard.Asic_Nonce_Rate);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Nonce_Rate : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Test_Standard.Nonce_Rate);
    snprintf(tmp1, 0x100, "Nonce_Rate : %d\0", Local_Config_Information->Test_Info.Test_Standard.Nonce_Rate);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Most_HW_Num : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Test_Standard.Most_HW_Num);
    snprintf(tmp1, 0x100, "Most_HW_Num : %d\0", Local_Config_Information->Test_Info.Test_Standard.Most_HW_Num);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : In local config file, gTest_loop : %d\n", "print_local_config_after_parse_MES_system_information", gTest_loop);
    snprintf(tmp1, 0x100, "In local config file, gTest_loop : %d\0", gTest_loop);
    log_to_file(tmp1, time_stamp);

    for (index = 0; index < gTest_loop; index++) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Test_Loop[%d]->Level : %d\n", "print_local_config_after_parse_MES_system_information", index, Local_Config_Information->Test_Info.Test_Standard.Test_Loop[index].Level);
        snprintf(tmp1, 0x100, "Test_Loop[%d]->Level : %d\0", index, Local_Config_Information->Test_Info.Test_Standard.Test_Loop[index].Level);
        log_to_file(tmp1, time_stamp);

        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Test_Loop[%d]->Pre_Open_Core_Voltage : %d\n", "print_local_config_after_parse_MES_system_information", index, Local_Config_Information->Test_Info.Test_Standard.Test_Loop[index].Pre_Open_Core_Voltage);
        snprintf(tmp1, 0x100, "Test_Loop[%d]->Pre_Open_Core_Voltage : %d\0", index, Local_Config_Information->Test_Info.Test_Standard.Test_Loop[index].Pre_Open_Core_Voltage);
        log_to_file(tmp1, time_stamp);

        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Test_Loop[%d]->Voltage : %d\n", "print_local_config_after_parse_MES_system_information", index, Local_Config_Information->Test_Info.Test_Standard.Test_Loop[index].Voltage);
        snprintf(tmp1, 0x100, "Test_Loop[%d]->Voltage : %d\0", index, Local_Config_Information->Test_Info.Test_Standard.Test_Loop[index].Voltage);
        log_to_file(tmp1, time_stamp);

        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Test_Loop[%d]->Frequence : %d\n", "print_local_config_after_parse_MES_system_information", index, Local_Config_Information->Test_Info.Test_Standard.Test_Loop[index].Frequence);
        snprintf(tmp1, 0x100, "Test_Loop[%d]->Frequence : %d\0", index, Local_Config_Information->Test_Info.Test_Standard.Test_Loop[index].Frequence);
        log_to_file(tmp1, time_stamp);
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Baudrate : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Test_Speed.Baudrate);
    snprintf(tmp1, 0x100, "Baudrate : %d\0", Local_Config_Information->Test_Info.Test_Speed.Baudrate);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Timeout : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Test_Speed.Timeout);
    snprintf(tmp1, 0x100, "Timeout : %d\0", Local_Config_Information->Test_Info.Test_Speed.Timeout);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : CCdly_Sel : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Asic_Register.CCdly_Sel);
    snprintf(tmp1, 0x100, "CCdly_Sel : %d\0", Local_Config_Information->Test_Info.Asic_Register.CCdly_Sel);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Pwth_Sel : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Asic_Register.Pwth_Sel);
    snprintf(tmp1, 0x100, "Pwth_Sel : %d\0", Local_Config_Information->Test_Info.Asic_Register.Pwth_Sel);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Swpf_Mode : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Asic_Register.Swpf_Mode);
    snprintf(tmp1, 0x100, "Swpf_Mode : %d\0", Local_Config_Information->Test_Info.Asic_Register.Swpf_Mode);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Pulse_Mode : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Asic_Register.Pulse_Mode);
    snprintf(tmp1, 0x100, "Pulse_Mode : %d\0", Local_Config_Information->Test_Info.Asic_Register.Pulse_Mode);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Clk_Sel : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Asic_Register.Clk_Sel);
    snprintf(tmp1, 0x100, "Clk_Sel : %d\0", Local_Config_Information->Test_Info.Asic_Register.Clk_Sel);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : RO_Relay_En : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Asic_Register.RO_Relay_En);
    snprintf(tmp1, 0x100, "RO_Relay_En : %d\0", Local_Config_Information->Test_Info.Asic_Register.RO_Relay_En);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : CO_Relay_En : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Asic_Register.CO_Relay_En);
    snprintf(tmp1, 0x100, "CO_Relay_En : %d\0", Local_Config_Information->Test_Info.Asic_Register.CO_Relay_En);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Diode_Vdd_Mux_Sel : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Asic_Register.Diode_Vdd_Mux_Sel);
    snprintf(tmp1, 0x100, "Diode_Vdd_Mux_Sel : %d\0", Local_Config_Information->Test_Info.Asic_Register.Diode_Vdd_Mux_Sel);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Start_Temp_Gap_Pic : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Temperature.Start_Temp_Gap_Pic);
    snprintf(tmp1, 0x100, "Start_Temp_Gap_Pic : %d\0", Local_Config_Information->Test_Info.Temperature.Start_Temp_Gap_Pic);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Time_For_Cooling_Hashboard : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Temperature.Time_For_Cooling_Hashboard);
    snprintf(tmp1, 0x100, "Time_For_Cooling_Hashboard : %d\0", Local_Config_Information->Test_Info.Temperature.Time_For_Cooling_Hashboard);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Fan_Speed : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Fan.Fan_Speed);
    snprintf(tmp1, 0x100, "Fan_Speed : %d\0", Local_Config_Information->Test_Info.Fan.Fan_Speed);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    pcVar1 = Local_Config_Information->Test_Info.Display.Display_result_Voltage_Domain ? "true" : "false";
    printf("%s : Display_result_Voltage_Domain : %s\n", "print_local_config_after_parse_MES_system_information", pcVar1);
    snprintf(tmp1, 0x100, "Display_result_Voltage_Domain : %s\0", pcVar1);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    pcVar1 = Local_Config_Information->Test_Info.Display.Display_result_Every_Core ? "true" : "false";
    printf("%s : Display_result_Every_Core : %s\n", "print_local_config_after_parse_MES_system_information", pcVar1);
    snprintf(tmp1, 0x100, "Display_result_Every_Core : %s\0", pcVar1);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Test_Config_Data_Version : %s\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Test_Config_Data_Version);
    snprintf(tmp1, 0x100, "Test_Config_Data_Version : %s\0", Local_Config_Information->Test_Info.Test_Config_Data_Version);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Voltage_Accuracy : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Test_Info.Voltage_Accuracy);
    snprintf(tmp1, 0x100, "Voltage_Accuracy : %d\0", Local_Config_Information->Test_Info.Voltage_Accuracy);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : Close_Power_Delay : %d\n", "print_local_config_after_parse_MES_system_information", Local_Config_Information->Repair_Mode.Close_Power_Delay);
    snprintf(tmp1, 0x100, "Close_Power_Delay : %d\0", Local_Config_Information->Repair_Mode.Close_Power_Delay);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    pcVar1 = Local_Config_Information->Repair_Mode.Clear_EEPROM_Data ? "true" : "false";
    printf("%s : Clear_EEPROM_Data : %s\n", "print_local_config_after_parse_MES_system_information", pcVar1);
    snprintf(tmp1, 0x100, "Clear_EEPROM_Data : %s\0", pcVar1);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : \n", "print_local_config_after_parse_MES_system_information");
    tmp1[0] = ' ';
    tmp1[1] = '\0';
    log_to_file(tmp1, time_stamp);
}

/**
 * @brief Подготовка информации о локальной конфигурации.
 */
bool prepare_local_config_information(void) {
    bool _Var1;
    char time_stamp[48];
    char tmp1[256];

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : \n", "prepare_local_config_information");
    tmp1[0] = ' ';
    tmp1[1] = '\0';
    log_to_file(tmp1, time_stamp);

    _Var1 = malloc_for_local_config_information();
    if (_Var1) {
        _Var1 = parse_local_config_file("/mnt/card/Config.ini");
        if (_Var1) {
            _Var1 = check_config_information(Local_Config_Information->Hash_Board.Asic_Type);
            if (_Var1) {
                _Var1 = true;
            } else {
                _Var1 = false;
            }
        } else {
            lcd_clear_result();
            lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
            lcd_show(1, "parse local ");
            lcd_show(2, "config file ");
            lcd_show(3, "fail ");
            _Var1 = false;
        }
    } else {
        lcd_clear_result();
        lcd_show(0, Local_Config_Information->Hash_Board.Board_Name);
        lcd_show(1, "malloc local ");
        lcd_show(2, "config file ");
        lcd_show(3, "fail ");
        _Var1 = false;
    }
    return _Var1;
}