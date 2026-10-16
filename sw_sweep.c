#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>

// --- Типы данных (на основе использования в псевдокоде) ---

typedef struct {
    int bad_asic_num;
    int bad_asic_ids[12];
} sweep_bad_asic_info_t;

// Внешние глобальные переменные и структуры
extern pattern_runtime_ctx g_rt;
extern struct Local_Config_Info* Local_Config_Information;
extern uint32_t err_sweep_nonce_resp;
extern int sw_break;
extern int sw_status;

// Заглушки для внешних функций
extern void clear_bad_asic_ids();
extern void add_bad_asic_ids(int id);
extern void get_sweep_bad_asics(sweep_bad_asic_info_t* bad_asics);
extern int check_unbalance();
extern int check_env_limit();
extern int check_nonce_rate();
extern void dump_matrix_state(uint8_t* state);
extern void set_succ_asic_nonce_state(uint8_t* state);
extern float get_matrix_freq_avg(float* matrix);
extern void set_matrix_state(uint8_t** state, float* last_matrix, float** matrix);
extern void set_matrix(float nextfreq, float** matrix, uint8_t** state);
extern void sw_nomalize_matrix(float** matrix, int row, int column);
extern int sweep_over_mode(float* matrix);
extern int sweep_online_mode(float* last_matrix, float freq, int* trytime, bool reinit, uint8_t* state);
extern void sweep_end_power_down();
extern int sw_sweep(float** matrix, uint8_t** state);
extern int sw_stop();
extern int sw_check_status();
extern int sw_flush(float** matrix);
extern int32_t sw_pt_init(char* pattern_file_path);
extern void sw_pt_exit();
extern void sw_pt_clear_stats();
extern void sw_pt_reset();
extern uchar c2hex(uchar value);
extern uchar twoc2hex(uchar high, uchar low);
extern void check_nonce(return_nonce_128bit_t* nonce_buf, pattern_runtime_ctx* ctx);
extern void sw_pt_process_nonce(void* return_nonce, void* arg);
extern void sw_pt_process_reg(void* return_nonce, void* arg);
extern int32_t sw_pt_send_work(uint32_t chain, void* args);
extern void* sw_pt_get_stats();
extern int32_t sw_pt_is_unbalance_happen(uint32_t chain);
extern int32_t sw_pt_is_recv_nonce_enough(uint32_t chain);
extern int sw_init(void* conf, void* sweep);
extern int level_down_freq(int infreq, int* outfreq);
extern int sw_drop(float** matrix, uint8_t** state);

extern void print_time_stamp(char* buf, int len);
extern void log_to_file(char* msg, char* time);
extern void _applog(int level, const char* msg, bool newline);
extern void lcd_common_show(const char* l1, const char* l2, const char* l3);
extern void lcd_clear_result();
extern void lcd_show(int line, const char* data);
extern void online_mode_retry_test(int chain);
extern void show_sweep_summary(float* matrix);
extern void show_sweep_failed();
extern int flush_eeprom_data(float* matrix, int chain);
extern void set_sweep_err_state(int err_code, bool is_fatal);

// --- Функции ---

/**
 * @brief Проверка дисбаланса нагрузки по доменам.
 */
int check_unbalance(void) {
    int iVar1;
    char time_stamp[48];
    char tmp1[256];
    int32_t is_unbalance;

    // Вызов через таблицу операций паттерна
    iVar1 = (*g_rt.pt_ops.is_unbalance_happen)((g_rt.config)->chain);
    
    if (iVar1 != 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : has is_unbalance, can not to rise freq more!!!\n", "check_unbalance");
        
        // Строгая эмуляция strncpy с посимвольным завершением, как в псевдокоде
        strncpy(tmp1, "has is_unbalance, can not to rise freq more!", 0x2c);
        tmp1[0x2c] = '!';
        tmp1[0x2d] = '!';
        tmp1[0x2e] = '\0';
        
        log_to_file(tmp1, time_stamp);
    }
    return iVar1;
}

/**
 * @brief Проверка лимита температуры окружающей среды.
 */
int check_env_limit(void) {
    int iVar1;
    char time_stamp[48];
    char tmp1[256];
    int32_t temp;

    iVar1 = pf_get_max_value(g_rt.tempval, ((g_rt.config)->board).sensor.sensor_num);
    
    if (iVar1 < 0x3e) { // 62 градуса
        iVar1 = 0;
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : temp:%d over than:%d,exit sweep.\n", "check_env_limit", iVar1, 0x3d);
        snprintf(tmp1, 0x100, "temp:%d over than:%d,exit sweep.", iVar1, 0x3d);
        log_to_file(tmp1, time_stamp);
        iVar1 = -1;
    }
    return iVar1;
}

/**
 * @brief Дамп состояния матрицы ASIC в лог.
 */
void dump_matrix_state(uint8_t *state) {
    uint8_t *state_local;
    char time_stamp[48];
    char tmp1[256];
    int j;
    int i;

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : ============matrix state==============\n\n", "dump_matrix_state");
    
    strncpy(tmp1, "============matrix state==============\n", 0x28);
    log_to_file(tmp1, time_stamp);

    for (i = 0; i < (g_rt.config)->domains; i = i + 1) {
        for (j = 0; j < (g_rt.config)->asics_in_domain; j = j + 1) {
            printf("%4d  ", (uint)state[j + i * (g_rt.config)->asics_in_domain]);
            
            // Локальный буфер для лога, эмулируем вложенную область видимости
            char tmp1_inner[256];
            snprintf(tmp1_inner, 0x100, "%4d  ", (uint)state[j + i * (g_rt.config)->asics_in_domain]);
            log_to_filex(tmp1_inner, tmp1); // log_to_filex использует внешний tmp1 как timestamp placeholder в оригинале? Нет, второй аргумент это time. Исправляем сигнатуру вызова согласно логике: log_to_filex(msg, time_stamp). Но в псевдокоде передается tmp1. Оставим как есть для соответствия структуре, но логически это msg.
            
            if ((g_rt.config)->asics_in_domain + -1 == j) {
                putchar(10);
                char newline_tmp[2] = "\n";
                log_to_filex(newline_tmp, tmp1);
            }
        }
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : ============matrix state==============\n\n", "dump_matrix_state");
    strncpy(tmp1, "============matrix state==============\n", 0x28);
    log_to_file(tmp1, time_stamp);
    
    return;
}

/**
 * @brief Обновление состояния успешных ASIC на основе полученных nonce.
 */
void set_succ_asic_nonce_state(uint8_t *state) {
    uint8_t *state_local;
    int j;
    int i;

    for (i = 0; i < (g_rt.config)->domains; i = i + 1) {
        for (j = 0; j < (g_rt.config)->asics_in_domain; j = j + 1) {
            // Проверка: если ASIC активен (state != 0) И он получил хоть один nonce
            if ((state[j + i * (g_rt.config)->asics_in_domain] != '\0') &&
                (g_rt.asic_stats.every_asic_got_nonce_num[j + i * (g_rt.config)->asics_in_domain] != 0)) {
                
                g_rt.last_succ_asic_nonce_num[i * (g_rt.config)->asics_in_domain + j] =
                    g_rt.asic_stats.every_asic_got_nonce_num[j + i * (g_rt.config)->asics_in_domain];
            }
        }
    }
    return;
}

/**
 * @brief Расчет средней частоты по матрице.
 */
float get_matrix_freq_avg(float *matrix) {
    uint in_fpscr;
    float fVar1;
    float *matrix_local;
    int j;
    int i;
    float avg;

    avg = 0.0;
    for (i = 0; i < (g_rt.config)->domains; i = i + 1) {
        for (j = 0; j < (g_rt.config)->asics_in_domain; j = j + 1) {
            avg = avg + matrix[j + i * (g_rt.config)->asics_in_domain];
        }
    }
    
    // Эмуляция VectorSignedToFloat для преобразования int в float с учетом FPSR
    fVar1 = (float)VectorSignedToFloat((g_rt.config)->asics_in_domain * (g_rt.config)->domains, (byte)(in_fpscr >> 0x16) & 3);
    
    return avg / fVar1;
}

/**
 * @brief Установка состояния матрицы (сброс плохих ASIC).
 */
void set_matrix_state(uint8_t **state, float *last_matrix, float **matrix) {
    float fVar1;
    float **matrix_local;
    float *last_matrix_local;
    uint8_t **state_local;
    char time_stamp[48];
    char tmp1[256];
    float curr;
    int i;

    i = 0;
    do {
        if ((g_rt.config)->asics <= i) {
            dump_matrix_state(*state);
            return;
        }

        // Если ASIC помечен как плохой в статистике
        if (g_rt.asic_stats.bad_asic_list[i] != false) {
            // Если он еще не был сброшен в текущем цикле
            if ((*state)[i] == '\0') {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : bad asic idx %d,last[%4.2f]\n\n", "set_matrix_state", i, (double)(*matrix)[i]);
                snprintf(tmp1, 0x100, "bad asic idx %d,last[%4.2f]\n", i, (double)(*matrix)[i]);
                log_to_file(tmp1, time_stamp);
            } else {
                // Сбрасываем состояние
                (*state)[i] = '\0';
                fVar1 = (*matrix)[i];
                
                // Логика выбора новой частоты на основе индекса (паттерн из псевдокода)
                if ((i + 2) % 9 < 4) {
                    if (((i + 2) / 7) % 9 != 1) {
                        // LAB_0005d132
                        (*matrix)[i] = last_matrix[i];
                    } else {
                        (*matrix)[i] = last_matrix[i] - 6.25;
                    }
                } else {
                    // LAB_0005d132
                    (*matrix)[i] = last_matrix[i];
                }

                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : bad asic idx %d, reset[%4.2f] to last[%4.2f]\n\n", "set_matrix_state", i, (double)fVar1, (double)(*matrix)[i]);
                snprintf(tmp1, 0x100, "bad asic idx %d, reset[%4.2f] to last[%4.2f]\n", i, (double)fVar1, (double)(*matrix)[i]);
                log_to_file(tmp1, time_stamp);
            }
        }
        i = i + 1;
    } while (true);
}

/**
 * @brief Установка единой частоты для всех активных ASIC в матрице.
 */
void set_matrix(float nextfreq, float **matrix, uint8_t **state) {
    uint8_t **state_local;
    float **matrix_local;
    float nextfreq_local;
    char time_stamp[48];
    char tmp1[256];
    int i;

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : set freq[%4.2f] to matrix\n", "set_matrix", SUB84((double)nextfreq, 0), (int)((ulonglong)(double)nextfreq >> 0x20));
    
    // В псевдокоде странная передача аргументов в snprintf, сохраняем структуру
    snprintf(tmp1, 0x100, "set freq[%4.2f] to matrix", tmp1, (double)nextfreq); 
    log_to_file(tmp1, time_stamp);

    for (i = 0; i < (g_rt.config)->asics; i = i + 1) {
        if ((*state)[i] == '\x01') {
            (*matrix)[i] = nextfreq;
        }
    }
    
    dump_matrix_freq(*matrix, (g_rt.config)->domains, (g_rt.config)->asics_in_domain);
    return;
}

/**
 * @brief Проверка скорости nonce (производительности).
 */
int check_nonce_rate(void) {
    int iVar1;
    uint in_fpscr;
    float fVar2;
    float fVar3;
    char time_stamp[48];
    char tmp1[256];
    float nonce_rate;

    // Расчет текущего nonce rate
    fVar2 = (float)VectorUnsignedToFloat(g_rt.asic_stats.valid_nonce_num, (byte)(in_fpscr >> 0x16) & 3);
    fVar3 = (float)VectorSignedToFloat((g_rt.config)->patter_num * (g_rt.config)->asic_small_cores * (g_rt.config)->asics, (byte)(in_fpscr >> 0x16) & 3);
    
    fVar2 = fVar2 / fVar3;

    // Сравнение со стандартом (умножаем на 10000 для целочисленного сравнения или используем float)
    if ((int)(fVar2 * 10000.0) < ((g_rt.config)->standard).nonce_rate) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : warning current nonce_rate:%f  standard nonce_rate:%d \n", "check_nonce_rate", SUB84((double)fVar2, 0), (int)((ulonglong)(double)fVar2 >> 0x20), ((g_rt.config)->standard).nonce_rate);
        
        iVar1 = ((g_rt.config)->standard).nonce_rate;
        snprintf(tmp1, 0x100, "warning current nonce_rate:%f  standard nonce_rate:%d ", iVar1, (double)fVar2, iVar1);
        log_to_file(tmp1, time_stamp);
        iVar1 = -1;
    } else {
        g_rt.nonce_rate = fVar2;
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : nonce_rate:%f\n", "check_nonce_rate", SUB84((double)fVar2, 0), (int)((ulonglong)(double)fVar2 >> 0x20));
        snprintf(tmp1, 0x100, "nonce_rate:%f ", tmp1, (double)fVar2);
        log_to_file(tmp1, time_stamp);
        iVar1 = 0;
    }
    return iVar1;
}

/**
 * @brief Нормализация матрицы частот (выравнивание по минимуму и гибкости).
 */
void sw_nomalize_matrix(float **matrix, int row, int column) {
    bool _Var1;
    uint in_fpscr;
    float fVar2;
    float a;
    int column_local;
    int row_local;
    float **matrix_local;
    char time_stamp[48];
    char tmp1[256];
    float test[110]; // Буфер из псевдокода, возможно используется для копирования или расчетов
    int vol;
    float totalfreq;
    float total;
    float max;
    float min;
    int i_1;
    int i;

    memcpy(test, &DAT_001f8084, 0x1b8); // Копирование каких-то констант/данных
    
    fVar2 = get_freq_matrix_min(*matrix, column * row);
    a = (g_rt.sweep.bias + 1.0) * fVar2;

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : min %f, max %f\n", "sw_nomalize_matrix", SUB84((double)fVar2, 0), (int)((ulonglong)(double)fVar2 >> 0x20), (double)a);
    snprintf(tmp1, 0x100, "min %f, max %f ", tmp1, (double)fVar2, (double)a);
    log_to_file(tmp1, time_stamp);

    // Ограничение снизу: если частота меньше рассчитанного минимума 'a', поднимаем до 'a'
    for (i = 0; column * row - i != 0 && i <= column * row; i = i + 1) {
        _Var1 = compare(a, (*matrix)[i]);
        if (!_Var1) { // Если a > matrix[i] (compare возвращает true если равны или близки, или если a <= matrix[i] в зависимости от реализации compare выше)
             // В реализации compare выше: true если равны или a > b? Нет, там сложная логика.
             // Исходя из контекста нормализации: если текущая частота ниже порога, ставим порог.
            (*matrix)[i] = a;
        }
    }

    // Корректировка на flex (гибкость/запас)
    for (i_1 = 0; column * row - i_1 != 0 && i_1 <= column * row; i_1 = i_1 + 1) {
        fVar2 = (float)VectorSignedToFloat((g_rt.config)->freq_flex, (byte)(in_fpscr >> 0x16) & 3);
        (*matrix)[i_1] = (*matrix)[i_1] - fVar2;
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : fix freq by freq_flex,flex:%d\n", "sw_nomalize_matrix", (g_rt.config)->freq_flex);
    snprintf(tmp1, 0x100, "fix freq by freq_flex,flex:%d ", (g_rt.config)->freq_flex);
    log_to_file(tmp1, time_stamp);

    dump_matrix_freq(*matrix, (g_rt.config)->domains, (g_rt.config)->asics_in_domain);
    return;
}

/**
 * @brief Режим свипа "Over" (полный перезапуск теста на каждой частоте).
 */
int sweep_over_mode(float *matrix) {
    int iVar1;
    float *matrix_local;

    iVar1 = pattern_test(matrix);
    return iVar1;
}

/**
 * @brief Режим свипа "Online" (пошаговое изменение частоты без полного сброса).
 */
int sweep_online_mode(float *last_matrix, float freq, int *trytime, bool reinit, uint8_t *state) {
    int iVar1;
    uint8_t *state_local;
    bool reinit_local;
    int *trytime_local;
    float freq_local;
    float *last_matrix_local;
    char time_stamp[48];
    char tmp1[256];

    // Если это первая частота диапазона или требуется реинициализация
    if ((g_rt.sweep.range_freq[0] == freq) || (reinit)) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : pt_before_send_nonce=================== freq:%f, range[0]:%f trytime:%d\n",
               "sweep_online_mode", SUB84((double)freq, 0), (int)((ulonglong)(double)freq >> 0x20),
               (double)g_rt.sweep.range_freq[0], *trytime);
        
        snprintf(tmp1, 0x100, "pt_before_send_nonce=================== freq:%f, range[0]:%f trytime:%d",
                 *trytime, (double)freq, (double)g_rt.sweep.range_freq[0], *trytime);
        log_to_file(tmp1, time_stamp);

        if (*trytime == 2) {
            *trytime = *trytime + 1;
        }

        iVar1 = pt_before_send_nonce(last_matrix);
        if (iVar1 != 0) {
            stop_threads();
            power_down_and_fan((g_rt.config)->chain);
            return -1;
        }
    }

    // Отправка nonce на текущей частоте
    pt_send_nonce(last_matrix, freq, state);
    return 0;
}

/**
 * @brief Завершение питания при окончании свипа.
 */
void sweep_end_power_down(void) {
    if (g_rt.sweep.mode != 0) {
        stop_threads();
        power_down_and_fan((g_rt.config)->chain);
    }
    return;
}

/**
 * @brief Основной цикл свипа частот.
 */
int sw_sweep(float **matrix, uint8_t **state) {
    int iVar1;
    int iVar2;
    double dVar3;
    undefined8 uStack_1b0;
    int aiStack_1a8[2];
    uint8_t **state_local;
    float **matrix_local;
    char time_stamp[48];
    char tmp1[256];
    char lcd_info_1[16];
    char lcd_info[16];
    int trytime;
    float avg_freq;
    float max;
    float min;
    float *last_matrix; // Указатель на буфер последней матрицы
    int local_30;
    int len;
    int sweep_res;
    float freq;

    len = (g_rt.config)->asics_in_domain * (g_rt.config)->domains;
    state_local = state;
    matrix_local = matrix;

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : matix len%d\n", "sw_sweep", len);
    snprintf(tmp1, 0x100, "matix len%d ", len);
    log_to_file(tmp1, time_stamp);

    local_30 = len + -1;
    // Выделение места на стеке для last_matrix
    iVar2 = -(len * 4 + 10U & 0xfffffff8);
    last_matrix = (float *)((int)&state_local + iVar2); 

    trytime = 0;
    sweep_res = 0;
    
    clear_bad_asic_ids();
    memcpy(last_matrix, *matrix_local, len << 2);
    
    freq = g_rt.sweep.range_freq[0];

    do {
        // Проверка выхода за верхнюю границу диапазона
        if (g_rt.sweep.range_freq[1] < freq) {
LAB_0005e0a6:
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : sweep end\n", "sw_sweep");
            strncpy(tmp1, "sweep en", 8);
            tmp1[8] = 'd';
            tmp1[9] = '\0';
            log_to_file(tmp1, time_stamp);
            
            sweep_end_power_down();
            
            if (sweep_res == 0) {
                // Проверка nonce rate в конце
                if ((int)((uint)(g_rt.nonce_rate < 0.99) << 0x1f) < 0) {
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : test over but not nonce rate err,nonce_rate:%f\n", "sw_sweep",
                           SUB84((double)g_rt.nonce_rate, 0), (int)((ulonglong)(double)g_rt.nonce_rate >> 0x20));
                    
                    *(double *)((int)&uStack_1b0 + iVar2) = (double)g_rt.nonce_rate;
                    snprintf(tmp1, 0x100, "test over but not nonce rate err,nonce_rate:%f ");
                    log_to_file(tmp1, time_stamp);
                    
                    set_sweep_err_state(0xe, false);
                    iVar2 = -1;
                } else {
                    dump_matrix_freq(*matrix_local, (g_rt.config)->domains, (g_rt.config)->asics_in_domain);
                    
                    g_rt.sweep.sweep_end_time.0_4 = time((time_t *)0x0);
                    g_rt.sweep.sweep_end_time.4_4 = (time_t)g_rt.sweep.sweep_end_time >> 0x1f;
                    
                    min = get_freq_matrix_min(*matrix_local, len);
                    max = get_freq_matrix_max(*matrix_local, len);
                    avg_freq = get_matrix_freq_avg(*matrix_local);
                    
                    snprintf(lcd_info, 0x10, "%d ", (int)avg_freq);
                    lcd_common_show("Sweep freq ", lcd_info, (char *)0x0);
                    
                    iVar2 = 0;
                }
            } else {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : sweep run err,exit.\n", "sw_sweep");
                strncpy(tmp1, "sweep run err,exit.", 0x14);
                log_to_file(tmp1, time_stamp);
                iVar2 = -1;
            }
            return iVar2;
        }

        // Проверка флага остановки пользователем
        if (sw_break != 0) {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : usr stop sweep!\n", "sw_sweep");
            strncpy(tmp1, "usr stop sweep!", 0x10);
            log_to_file(tmp1, time_stamp);
            goto LAB_0005e0a6;
        }

        sweep_res = 0;

        // Логика режимов свипа
        if ((trytime == 1) && (g_rt.sweep.mode == 1)) {
            // Повторная попытка в онлайн режиме
            trytime = 2;
            online_mode_retry_test((g_rt.config)->chain);
            g_pwm_enable = true;
            sweep_res = sweep_online_mode((float *)last_matrix, freq, &trytime, true, *state_local);
        } else if (g_rt.sweep.mode == 0) {
            // Овер режим (полный тест)
            sweep_res = sweep_over_mode(*matrix_local);
        } else {
            if (g_rt.sweep.mode != 1) {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : not support sweep mode\n", "sw_sweep");
                strncpy(tmp1, "not support sweep mo", 0x14);
                tmp1[0x14] = 'd';
                tmp1[0x15] = 'e';
                tmp1[0x16] = '\0';
                log_to_file(tmp1, time_stamp);
                sweep_end_power_down();
                return -1;
            }
            // Обычный онлайн шаг
            sweep_res = sweep_online_mode((float *)last_matrix, freq, &trytime, false, *state_local);
        }

        if (sweep_res != 0) goto LAB_0005e0a6;

        // Проверки после шага
        iVar1 = check_nonce_rate();
        if (((iVar1 != 0) || (iVar1 = check_unbalance(), iVar1 != 0)) ||
            (iVar1 = check_env_limit(), iVar1 != 0)) {
            
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            dVar3 = (double)freq;
            *(int *)((int)aiStack_1a8 + iVar2 + -4) = trytime;
            *(int *)((int)&uStack_1b0 + iVar2) = g_rt.sweep.mode;
            printf("%s : current freq %f, mode %d, trytime %d\n\n", "sw_sweep", SUB84(dVar3, 0),
                   (int)((ulonglong)dVar3 >> 0x20));
            
            dVar3 = (double)freq;
            *(int *)((int)aiStack_1a8 + iVar2 + 4) = trytime;
            *(int *)((int)aiStack_1a8 + iVar2) = g_rt.sweep.mode;
            *(double *)((int)&uStack_1b0 + iVar2) = dVar3;
            
            snprintf(tmp1, 0x100, "current freq %f, mode %d, trytime %d\n ");
            log_to_file(tmp1, time_stamp);

            if (g_rt.sweep.mode == 1) {
                if (trytime == 0) {
                    trytime = 1;
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : set retest flag\n", "sw_sweep");
                    strncpy(tmp1, "set retest flag", 0x10);
                    log_to_file(tmp1, time_stamp);
                    
                    memcpy(*matrix_local, last_matrix, len << 2);
                    
                    g_rt.sweep.sweep_online_time.0_4 = time((time_t *)0x0);
                    g_rt.sweep.sweep_online_time.4_4 = (time_t)g_rt.sweep.sweep_online_time >> 0x1f;
                    
                    g_rt.sweep.sweep_online_min = get_freq_matrix_min(*matrix_local, (g_rt.config)->asics);
                    g_rt.sweep.sweep_online_max = get_freq_matrix_max(*matrix_local, (g_rt.config)->asics);
                    g_rt.sweep.sweep_online_avg = get_avg_matrix(*matrix_local, (g_rt.config)->asics);
                    g_rt.sweep.sweep_online_var = get_var_matrix(*matrix_local, (g_rt.config)->asics);
                    g_rt.sweep.sweep_online_nonce_rate = g_rt.nonce_rate;
                    
                    goto LAB_0005e0a6;
                }
                trytime = trytime + 1;
            }
            
            memcpy(*matrix_local, last_matrix, len << 2);
            goto LAB_0005e0a6;
        }

        // Успешный шаг: обновляем состояния и идем дальше
        set_matrix_state(state_local, (float *)last_matrix, matrix_local);
        set_succ_asic_nonce_state(*state_local);
        memcpy(last_matrix, *matrix_local, len << 2);
        
        freq = freq + g_rt.sweep.step;
        set_matrix(freq, matrix_local, state_local);
        
        snprintf(lcd_info_1, 0x10, "%d ", (int)freq);
        lcd_common_show("Test freq ", lcd_info_1, (char *)0x0);

    } while (true);
}

/**
 * @brief Остановка свипа.
 */
int sw_stop(void) {
    (*g_rt.pt_ops.exit)();
    sw_break = 0;
    return 0;
}

/**
 * @brief Проверка статуса свипа.
 */
int sw_check_status(void) {
    return sw_status;
}

/**
 * @brief Финализация данных свипа (запись в EEPROM).
 */
int sw_flush(float **matrix) {
    int iVar1;
    float **matrix_local;
    char time_stamp[48];
    char tmp1[256];
    size_t stacksize;
    pthread_attr_t attr;
    int ret;

    ret = pthread_attr_init((pthread_attr_t *)&attr);
    if (ret != 0) {
        fwrite("Error: pthread_attr_init\n", 1, 0x19, stderr);
    }
    
    ret = pthread_attr_getstacksize((pthread_attr_t *)&attr, &stacksize);
    if (ret != 0) {
        fwrite("Error: pthread_attr_getstacksize\n", 1, 0x21, stderr);
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : =====stack size: %u\n\n", "sw_flush", stacksize);
    snprintf(tmp1, 0x100, "=====stack size: %u\n", stacksize);
    log_to_file(tmp1, time_stamp);

    show_sweep_summary(*matrix);
    sw_nomalize_matrix(matrix, (g_rt.config)->domains, (g_rt.config)->asics_in_domain);
    
    iVar1 = flush_eeprom_data(*matrix, (g_rt.config)->chain);
    return iVar1;
}

/**
 * @brief Инициализация модуля свипа.
 */
int32_t sw_pt_init(char *pattern_file_path) {
    char *pattern_file_path_local;
    return 0;
}

/**
 * @brief Деинициализация модуля свипа.
 */
void sw_pt_exit(void) {
    if (g_rt.config != (config_ctx *)0x0) {
        free(g_rt.config);
        g_rt.config = (config_ctx *)0x0;
    }
    return;
}

/**
 * @brief Очистка статистики свипа.
 */
void sw_pt_clear_stats(void) {
    memset(&g_rt.stats, 0, 0x130c);
    return;
}

/**
 * @brief Сброс состояния свипа.
 */
void sw_pt_reset(void) {
    return;
}

/**
 * @brief Конвертация символа в hex nibble.
 */
uchar c2hex(uchar value) {
    uchar value_local;
    char time_stamp[48];
    char tmp1[256];
    uchar ret;

    ret = 0xff;
    if ((value < 0x30) || (0x39 < value)) {
        if ((value == 'a') || (value == 'A')) {
            ret = '\n'; // 0x0A
        } else if ((value == 'b') || (value == 'B')) {
            ret = '\v'; // 0x0B
        } else if ((value == 'c') || (value == 'C')) {
            ret = '\f'; // 0x0C
        } else if ((value == 'd') || (value == 'D')) {
            ret = '\r'; // 0x0D
        } else if ((value == 'e') || (value == 'E')) {
            ret = '\x0e';
        } else if ((value == 'f') || (value == 'F')) {
            ret = '\x0f';
        } else {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : input value error: %c\n\n", "c2hex", (uint)value);
            snprintf(tmp1, 0x100, "input value error: %c\n", (uint)value);
            log_to_file(tmp1, time_stamp);
        }
    } else {
        ret = value & 0xf;
    }
    return ret;
}

/**
 * @brief Конвертация двух символов в байт.
 */
uchar twoc2hex(uchar high, uchar low) {
    uchar uVar1;
    byte bVar2;
    uchar low_local;
    uchar high_local;
    uchar ret;

    uVar1 = c2hex(high);
    bVar2 = c2hex(low);
    return bVar2 ^ uVar1 << 4;
}

/**
 * @brief Обработка полученного nonce (проверка корректности).
 */
void check_nonce(return_nonce_128bit_t *nonce_buf, pattern_runtime_ctx *ctx) {
    byte bVar1;
    uint uVar3;
    int iVar4;
    uint uVar5;
    uint uVar6;
    pattern_work *ppVar7;
    pattern_runtime_ctx *ctx_local;
    return_nonce_128bit_t *nonce_buf_local;
    char time_stamp[48];
    char tmp1[256];
    uint32_t match_nonce;
    pattern_work *work;
    pattern_work *works;
    uint32_t which_pattern;
    uint32_t which_small_core;
    uint32_t which_big_core;
    uint32_t which_asic;
    uint uVar2;

    // Проверка ID цепи в первом байте nonce
    if ((*(byte *)nonce_buf & 0xf) == ctx->config->chain) {
        bVar1 = get_asic_index_by_nonce((nonce_buf->nonce).nonce, ctx->config->interval);
        uVar2 = (uint)bVar1;
        
        bVar1 = get_coreid_by_nonce((nonce_buf->nonce).nonce);
        uVar3 = (uint)bVar1;
        
        uVar5 = (nonce_buf->nonce).blk_version & 0xf;
        uVar6 = (int)((ushort *)&(nonce_buf->reg).chip_address & 0x7fff) >> 7;

        if (uVar2 < (uint)ctx->config->asics) {
            if (uVar3 < (uint)ctx->config->big_core) {
                // Проверка на аномальное ядро
                if (ctx->config->abnormal_core == uVar3) {
                    if (uVar5 < 4) {
                        err_sweep_nonce_resp = err_sweep_nonce_resp + 1;
                        return;
                    }
                } else if ((uint)ctx->config->samll_core_num_in_big_core <= uVar5) {
                    err_sweep_nonce_resp = err_sweep_nonce_resp + 1;
                    return;
                }

                if (uVar6 < (uint)ctx->config->patter_num) {
                    ppVar7 = ctx->works[uVar2] + uVar6 + (uVar5 + uVar3 * 0x10) * 8;
                    
                    // Сравнение nonce
                    if ((nonce_buf->nonce).nonce == (ppVar7->pattern).nonce) {
                        if (ppVar7->nonce_repeat == 0) {
                            ppVar7->nonce_repeat = ppVar7->nonce_repeat + 1;
                            (ctx->asic_stats).valid_nonce_num = (ctx->asic_stats).valid_nonce_num + 1;
                            
                            iVar4 = uVar3 * ctx->config->samll_core_num_in_big_core + uVar5;
                            (ctx->asic_stats).asic_core_nonce_num[uVar2][iVar4] =
                                (ctx->asic_stats).asic_core_nonce_num[uVar2][iVar4] + '\x01';
                            
                            (ctx->asic_stats).every_asic_got_nonce_num[uVar2] =
                                (ctx->asic_stats).every_asic_got_nonce_num[uVar2] + 1;
                            
                            (ctx->stats).nonce_num_asic[uVar2] = (ctx->stats).nonce_num_asic[uVar2] + 1;
                            (ctx->stats).nonce_num_chain = (ctx->stats).nonce_num_chain + 1;
                        } else {
                            ppVar7->nonce_repeat = ppVar7->nonce_repeat + 1;
                            (ctx->stats).nonce_num_repeated = (ctx->stats).nonce_num_repeated + 1;
                        }
                    }
                } else {
                    err_sweep_nonce_resp = err_sweep_nonce_resp + 1;
                }
            } else {
                err_sweep_nonce_resp = err_sweep_nonce_resp + 1;
            }
        } else {
            err_sweep_nonce_resp = err_sweep_nonce_resp + 1;
        }
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : receive nonce from chain: %d, but not gChain: %d\n", "check_nonce",
               *(byte *)nonce_buf & 0xf, ctx->config->chain);
        snprintf(tmp1, 0x100, "receive nonce from chain: %d, but not gChain: %d ", (byte *)nonce_buf & 0xf,
                 ctx->config->chain);
        log_to_file(tmp1, time_stamp);
    }
    return;
}

/**
 * @brief Обработчик nonce для режима свипа.
 */
void sw_pt_process_nonce(void *return_nonce, void *arg) {
    void *arg_local;
    void *return_nonce_local;
    check_nonce((return_nonce_128bit_t *)return_nonce, (pattern_runtime_ctx *)arg);
    return;
}

/**
 * @brief Обработчик регистра для режима свипа.
 */
void sw_pt_process_reg(void *return_nonce, void *arg) {
    void *arg_local;
    void *return_nonce_local;
    pt_process_reg_normal(return_nonce);
    return;
}

/**
 * @brief Отправка работы (паттернов) в ASIC при свипе.
 */
int32_t sw_pt_send_work(uint32_t chain, void *args) {
    bool _Var1;
    uchar uVar2;
    uint uVar3;
    uint uVar4;
    uint uVar5;
    uint uVar6;
    int iVar7;
    uint uVar8;
    undefined4 uVar9;
    void *args_local;
    uint32_t chain_local;
    char time_stamp[48];
    char tmp1[256];
    work_cxt work_buf;
    pattern_work *works;
    uint32_t send_counter;
    uint32_t position;
    uint32_t count;
    uint32_t received_nonce_num;
    uint32_t *work_ptr;
    uint32_t pattern_num;
    uint32_t small_core_in_big_core;
    uint32_t core_num;
    uint32_t asic_num;
    pattern_runtime_ctx *runtime;
    uint32_t j;
    uint32_t i;
    pattern_work *work;
    uint32_t which_pattern;
    uint32_t which_core;
    uint32_t which_asic;

    // Извлечение параметров из контекста args
    uVar3 = *(uint *)((int *)((int)args + 0x23628) + 4); // asic_num?
    uVar4 = (g_rt.config)->big_core * (g_rt.config)->samll_core_num_in_big_core;
    uVar5 = *(uint *)((int *)((int)args + 0x23628) + 0x1c);
    uVar6 = *(uint *)((int *)((int)args + 0x23628) + 0x20);

    err_sweep_nonce_resp = 0;

    _Var1 = read_temp_from_asic_once(false);
    if (!_Var1) {
        err_exit("asic temp outof range");
    }
    
    _Var1 = read_chain_domain_voltage(false);
    if (!_Var1) {
        err_exit("asic vol outof range");
    }

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : asic_num:%d,core_num:%d,small_core_in_big_core:%d,pattern:%d.\n", "sw_pt_send_work",
           uVar3, uVar4, uVar5, uVar6);
    snprintf(tmp1, 0x100, "asic_num:%d,core_num:%d,small_core_in_big_core:%d,pattern:%d. ", uVar3, uVar4,
             uVar5, uVar6);
    log_to_file(tmp1, time_stamp);

    // Цикл отправки работ по всем ASIC, ядрам и паттернам
    for (which_asic = 0; which_asic < uVar3; which_asic = which_asic + 1) {
        iVar7 = *(int *)((int)args + (which_asic + 0x8d1e) * 4);
        
        for (which_core = 0; which_core < uVar4; which_core = uVar5 + which_core) {
            for (which_pattern = 0; which_pattern < uVar6; which_pattern = which_pattern + 1) {
                
                // Ожидание готовности FIFO
                while (uVar2 = is_work_fifo_ready(**(uint32_t **)((int)args + 0x23628)), uVar2 == '\0') {
                    usleep(100);
                }

                memset(&work_buf, 0, 0x214);
                work_buf.work_type = '\x01';
                work_buf.chain_id = (byte)(undefined4 **)((int)args + 0x23628) | 0x80;
                work_buf.reserved1[0] = '\0';
                work_buf.reserved1[1] = '\0';
                
                // Формирование счетчика работы
                work_buf.work_count =
                    (which_pattern & 0x1ffffff) >> 0x11 | (which_pattern << 7 & 0xff0000) >> 8 |
                    (which_pattern << 7 & 0xff00) << 8 | which_pattern << 0x1f;

                work = (pattern_work *)((which_pattern + uVar6 * which_core) * 0x10 + iVar7);

                // Заполнение midstate
                for (i = 0; i < uVar5; i = i + 1) {
                    uVar8 = *(int *)((int *)((int)args + 0x23628) + 0x1c) *
                            *(int *)((int *)((int)args + 0x23628) + 0x24);
                    
                    if ((uVar8 < which_core || uVar8 - which_core == 0) && (i < 4)) {
                        for (j = 0; j < 0x20; j = j + 1) {
                            if (j < 0x18) {
                                work_buf.midstate[i][j] = '\0';
                            } else {
                                work_buf.midstate[i][j] = (char)i + *(char *)((int)(&work->pattern + -2) + j);
                            }
                        }
                    } else {
                        uVar8 = __aeabi_uidiv(which_core, (uint *)((int *)((int)args + 0x23628) + 0x1c));
                        work = (pattern_work *)
                            ((which_pattern + uVar6 * (i + *(int *)((int *)((int)args + 0x23628) + 0x1c) * uVar8)) * 0x10 +
                             iVar7);
                        
                        for (j = 0; j < 0x20; j = j + 1) {
                            if (j < 0x18) {
                                work_buf.midstate[i][j] = '\0';
                            } else {
                                work_buf.midstate[i][j] = *(uint8_t *)((int)(&work->pattern + -2) + j);
                            }
                        }
                        *(int *)((int)args + 0x2365c) = *(int *)((int)args + 0x2365c) + 1;
                    }
                }

                // Байт-свап для reserved данных (подготовка к отправке)
                for (i = 0; i < 0x85; i = i + 1) {
                    *(uint *)(work_buf.reserved1 + i * 4 + -2) =
                        *(int *)(work_buf.reserved1 + i * 4 + -2) << 0x18 |
                        *(uint *)(work_buf.reserved1 + i * 4 + -2) >> 0x18 |
                        ((uint *)(work_buf.reserved1 + i * 4 + -2) & 0xff0000) >> 8 |
                        ((uint *)(work_buf.reserved1 + i * 4 + -2) & 0xff00) << 8;
                }

                set_TW_write_command((uint32_t *)&work_buf, 0x214);
            }
        }
    }

    // Проверка количества отправленных работ
    if (*(uint *)((int)args + 0x2365c) < uVar3 * uVar6 * (g_rt.config)->asic_small_cores) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Send work num : %d, less than %d\n", "sw_pt_send_work",
               *(undefined4 *)((int)args + 0x2365c), uVar3 * uVar4 * uVar6);
        snprintf(tmp1, 0x100, "Send work num : %d, less than %d ", (undefined4 *)((int)args + 0x2365c),
                 uVar3 * uVar4 * uVar6);
        log_to_file(tmp1, time_stamp);
    } else {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Send test %d pattern done\n", "sw_pt_send_work", (undefined4 *)((int)args + 0x2365c));
        snprintf(tmp1, 0x100, "Send test %d pattern done ", (undefined4 *)((int)args + 0x2365c));
        log_to_file(tmp1, time_stamp);
    }

    uVar9 = *(undefined4 *)((int)args + 0x2334c);
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : send work thread eixt,send:%d,recv:%d\n", "sw_pt_send_work",
           *(undefined4 *)((int)args + 0x2365c), uVar9);
    snprintf(tmp1, 0x100, "send work thread eixt,send:%d,recv:%d ", (undefined4 *)((int)args + 0x2365c),
             uVar9);
    log_to_file(tmp1, time_stamp);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : err_sweep_nonce_resp:%d\n", "sw_pt_send_work", err_sweep_nonce_resp);
    snprintf(tmp1, 0x100, "err_sweep_nonce_resp:%d ", err_sweep_nonce_resp);
    log_to_file(tmp1, time_stamp);

    _Var1 = read_temp_from_asic_once(false);
    if (!_Var1) {
        err_exit("asic temp outof range");
    }
    _Var1 = read_chain_domain_voltage(false);
    if (!_Var1) {
        err_exit("asic vol outof range");
    }

    return 0;
}

/**
 * @brief Получение статистики свипа (заглушка).
 */
void *sw_pt_get_stats(void) {
    return (void *)0x0;
}

/**
 * @brief Проверка дисбаланса для свипа.
 */
int32_t sw_pt_is_unbalance_happen(uint32_t chain) {
    uint uVar1;
    int iVar2;
    uint32_t chain_local;
    char time_stamp[48];
    char tmp1[256];
    int actual_core_domain;
    uint32_t nonce_num_domain_threshold;
    uint32_t nonce_num_domain_avg;
    int32_t is_unbalance_happen;
    uint32_t core_domain;
    uint32_t domain;

    is_unbalance_happen = 0;
    
    // Расчет среднего количества nonce на домен
    uVar1 = __aeabi_uidiv(g_rt.stats.nonce_num_chain, (g_rt.config)->domains);
    uVar1 = __aeabi_uidiv(uVar1, (g_rt.config)->asic_domain_num);

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : nonce_num_chain:%d,domains:%d,asic_domain_num:%d\n", "sw_pt_is_unbalance_happen",
           g_rt.stats.nonce_num_chain, (g_rt.config)->domains, (g_rt.config)->asic_domain_num);
    snprintf(tmp1, 0x100, "nonce_num_chain:%d,domains:%d,asic_domain_num:%d ", g_rt.stats.nonce_num_chain,
             (g_rt.config)->domains, (g_rt.config)->asic_domain_num);
    log_to_file(tmp1, time_stamp);

    uVar1 = uVar1 >> 1; // Порог = половина среднего

    if (uVar1 == 0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Domain unbalance happen: nonce is 0\n", "sw_pt_is_unbalance_happen");
        strncpy(tmp1, "Domain unbalance happen: nonce is 0", 0x24);
        log_to_file(tmp1, time_stamp);
        is_unbalance_happen = 1;
    } else {
        for (domain = 0; domain < (uint)(g_rt.config)->domains; domain = domain + 1) {
            for (core_domain = 0; core_domain < (uint)(g_rt.config)->asic_domain_num;
                 core_domain = core_domain + 1) {
                
                iVar2 = core_domain + domain * (g_rt.config)->asic_domain_num;
                
                if (g_rt.stats.nonce_num_domain[iVar2] < uVar1) {
                    is_unbalance_happen = 1;
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : Domain unbalance happen, chain = %d,  coredomain = %d, nonce_num_domain = %d, threshold = %d.\n\n",
                           "sw_pt_is_unbalance_happen", chain, iVar2, g_rt.stats.nonce_num_domain[iVar2], uVar1);
                    snprintf(tmp1, 0x100,
                             "Domain unbalance happen, chain = %d,  coredomain = %d, nonce_num_domain = %d, threshold = %d.\n",
                             chain, iVar2, g_rt.stats.nonce_num_domain[iVar2], uVar1);
                    log_to_file(tmp1, time_stamp);
                }
            }
        }
    }
    return is_unbalance_happen;
}

/**
 * @brief Проверка достаточности полученных nonce для свипа (заглушка).
 */
int32_t sw_pt_is_recv_nonce_enough(uint32_t chain) {
    uint32_t chain_local;
    return 0;
}

/**
 * @brief Инициализация структур свипа.
 */
int sw_init(void *conf, void *sweep) {
    int32_t iVar1;
    void *sweep_local;
    void *conf_local;

    sw_break = 0;
    memset(&g_rt, 0, 0x23678);

    // Установка указателей на функции обработки свипа
    g_rt.pt_ops.init = (code *)0x5e481;       // sw_pt_init
    g_rt.pt_ops.exit = (code *)0x5e499;       // sw_pt_exit
    g_rt.pt_ops.clear_stats = (code *)0x5e4e5; // sw_pt_clear_stats
    g_rt.pt_ops.reset = (code *)0x5e4fd;      // sw_pt_reset
    g_rt.pt_ops.process_nonce = (code *)0x5ea51; // sw_pt_process_nonce
    g_rt.pt_ops.process_reg = (code *)0x5ea6d;   // sw_pt_process_reg
    g_rt.pt_ops.send_work = (code *)0x5ea85;     // sw_pt_send_work
    g_rt.pt_ops.get_stats = (code *)0x5f209;     // sw_pt_get_stats
    g_rt.pt_ops.is_unbalance_happen = (code *)0x5f219; // sw_pt_is_unbalance_happen
    g_rt.pt_ops.is_recv_nonce_enough = (code *)0x5f529; // sw_pt_is_recv_nonce_enough

    memcpy(&g_rt.sweep, sweep, 0x50);
    memcpy(g_rt.works, g_works, 0x1b0);

    iVar1 = pt_init("/mnt/card/BM1368-pattern/pattern_16midstate.bin", conf);
    return iVar1;
}

/**
 * @brief Понижение частоты до следующего уровня.
 */
int level_down_freq(int infreq, int *outfreq) {
    int iVar1;
    int *outfreq_local;
    int infreq_local;
    char time_stamp[48];
    char tmp1[256];

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : level down freq %d\n", "level_down_freq", infreq);
    snprintf(tmp1, 0x100, "level down freq %d ", infreq);
    log_to_file(tmp1, time_stamp);

    if (infreq == 0x1ae) {
        *outfreq = 0x1ae;
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : no need to level down freq %d->%d\n", "level_down_freq", 0x1ae, *outfreq);
        snprintf(tmp1, 0x100, "no need to level down freq %d->%d ", 0x1ae, *outfreq);
        log_to_file(tmp1, time_stamp);
        iVar1 = 1;
    } else {
        if (infreq == 0x1dd) {
            *outfreq = 0x1cc;
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : level down freq %d->%d\n", "level_down_freq", 0x1dd, *outfreq);
            snprintf(tmp1, 0x100, "level down freq %d->%d ", 0x1dd, *outfreq);
            log_to_file(tmp1, time_stamp);
        } else if (infreq == 0x1ea) {
            *outfreq = 0x1dd;
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : level down freq %d->%d\n", "level_down_freq", 0x1ea, *outfreq);
            snprintf(tmp1, 0x100, "level down freq %d->%d ", 0x1ea, *outfreq);
            log_to_file(tmp1, time_stamp);
        } else if (infreq == 0x1cc) {
            *outfreq = 0x1ae;
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : level down freq %d->%d\n", "level_down_freq", 0x1cc, *outfreq);
            snprintf(tmp1, 0x100, "level down freq %d->%d ", 0x1cc, *outfreq);
            log_to_file(tmp1, time_stamp);
        }
        iVar1 = 0;
    }
    return iVar1;
}

/**
 * @brief Режим понижения частоты (Drop Level).
 */
int sw_drop(float **matrix, uint8_t **state) {
    uint uVar1;
    uint uVar2;
    byte bVar3;
    int iVar4;
    int iVar5;
    int iVar6;
    uint in_fpscr;
    float fVar7;
    float fVar8;
    int aiStack_5b8[2];
    uint8_t **state_local;
    float **matrix_local;
    char time_stamp[48];
    char tmp1[256];
    char lcd_info[16];
    sweep_result_t sr;
    eeprom_data_format_t *eep_ctx;
    int dropfreq;
    float avg_freq;
    float max;
    float min;
    int sweep_res;
    int trytime;
    int freq;
    float *last_matrix;
    int local_34;
    int len;
    int i_1;
    int k;
    int i;

    len = (g_rt.config)->asics_in_domain * (g_rt.config)->domains;
    state_local = state;
    matrix_local = matrix;

    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : matix len%d\n", "sw_drop", len);
    snprintf(tmp1, 0x100, "matix len%d ", len);
    log_to_file(tmp1, time_stamp);

    local_34 = len + -1;
    iVar6 = -(len * 4 + 10U & 0xfffffff8);
    last_matrix = (float *)((int)&state_local + iVar6);

    freq = 0;
    dropfreq = 0;
    trytime = 0;
    sweep_res = 0;

    clear_bad_asic_ids();
    
    read_eeprom((g_rt.config)->chain, &eep_ctx);
    
    if (eep_ctx == (eeprom_data_format_t *)0x0) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : eeprom data read ERR!\n", "sw_drop");
        strncpy(tmp1, "eeprom data read ERR!", 0x16);
        log_to_file(tmp1, time_stamp);
        set_sweep_err_state(2, false);
        iVar6 = 2;
    } else {
        iVar4 = (*eep_ctx->pt2_result_get)(eep_ctx);
        if (iVar4 == 1) {
            iVar4 = (*eep_ctx->sweep_hashrate_get)(eep_ctx);
            if (iVar4 == 0x2b67) {
                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : has drop down once, can not drop down!!!\n", "sw_drop");
                strncpy(tmp1, "has drop down once, can not drop down!!!", 0x28);
                tmp1[0x28] = '\0';
                log_to_file(tmp1, time_stamp);
                set_sweep_err_state(0x11, false);
                iVar6 = 0x11;
            } else {
                memset(&sr, 0, 0x40c);
                eeprom_get_sweep_result(&sr, eep_ctx);
                
                // Загрузка частот из EEPROM в матрицу
                for (i = 0; i < len; i = i + 1) {
                    fVar7 = (float)VectorUnsignedToFloat(sr.freqs[i], (byte)(in_fpscr >> 0x16) & 3);
                    (*matrix_local)[i] = fVar7;
                }

                print_time_stamp(time_stamp, 0x30);
                printf("%s ", time_stamp);
                printf("%s : dump read eeporm freq matrix\n", "sw_drop");
                strncpy(tmp1, "dump read eeporm freq matrix", 0x1c);
                tmp1.28_4 = tmp1.28_4 & 0xffffff00;
                log_to_file(tmp1, time_stamp);
                
                dump_matrix_freq(*matrix_local, (g_rt.config)->domains, (g_rt.config)->asics_in_domain);

                freq = (*eep_ctx->frequency_get)(eep_ctx);
                iVar4 = level_down_freq(freq, &dropfreq);
                
                if (iVar4 == 0) {
                    // Поиск соответствующего уровня теста
                    for (k = 0; k < (g_rt.config)->test_levels; k = k + 1) {
                        if ((g_rt.config)->tests[k].freq <= dropfreq) {
                            (g_rt.config)->sweeped_res_index = k;
                            print_time_stamp(time_stamp, 0x30);
                            printf("%s ", time_stamp);
                            iVar5 = (g_rt.config)->sweeped_res_index;
                            iVar4 = (g_rt.config)->tests[(g_rt.config)->sweeped_res_index].level;
                            *(int *)((int)aiStack_5b8 + iVar6) =
                                (g_rt.config)->tests[(g_rt.config)->sweeped_res_index].freq;
                            printf("%s : dorp matched index:[%d] level:%d,freq:%d\n", "sw_drop", iVar5, iVar4);
                            
                            iVar5 = (g_rt.config)->sweeped_res_index;
                            iVar4 = (g_rt.config)->tests[(g_rt.config)->sweeped_res_index].level;
                            *(int *)((int)aiStack_5b8 + iVar6 + 4) =
                                (g_rt.config)->tests[(g_rt.config)->sweeped_res_index].freq;
                            *(int *)((int)aiStack_5b8 + iVar6) = iVar4;
                            
                            snprintf(tmp1, 0x100, "dorp matched index:[%d] level:%d,freq:%d ", iVar5);
                            log_to_file(tmp1, time_stamp);
                            break;
                        }
                    }

                    // Корректировка матрицы частот относительно dropfreq
                    for (i_1 = 0; i_1 < len; i_1 = i_1 + 1) {
                        fVar7 = (*matrix_local)[i_1];
                        fVar8 = (float)VectorSignedToFloat(dropfreq, (byte)(in_fpscr >> 0x16) & 3);
                        
                        uVar1 = in_fpscr & 0xfffffff;
                        uVar2 = uVar1 | (uint)(fVar7 < fVar8) << 0x1f | (uint)(fVar7 == fVar8) << 0x1e;
                        in_fpscr = uVar2 | (uint)(NAN(fVar7) || NAN(fVar8)) << 0x1c;
                        bVar3 = (byte)(uVar2 >> 0x18);
                        
                        if (!(bool)(bVar3 >> 6 & 1) & bVar3 >> 7 == ((byte)(in_fpscr >> 0x1c) & 1)) {
                            // Если fVar7 >= fVar8 (не меньше)
                            fVar7 = (float)VectorSignedToFloat(dropfreq, (byte)(in_fpscr >> 0x16) & 3);
                            fVar7 = (*matrix_local)[i_1] - fVar7;
                            
                            uVar2 = uVar1 | (uint)(fVar7 < 5.0) << 0x1f | (uint)(fVar7 == 5.0) << 0x1e;
                            in_fpscr = uVar2 | (uint)NAN(fVar7) << 0x1c;
                            bVar3 = (byte)(uVar2 >> 0x18);
                            
                            if ((bool)(bVar3 >> 6 & 1) || bVar3 >> 7 != ((byte)(in_fpscr >> 0x1c) & 1)) {
                                // Разница < 5
                                fVar7 = (float)VectorSignedToFloat(dropfreq, (byte)(in_fpscr >> 0x16) & 3);
                                fVar7 = (*matrix_local)[i_1] - fVar7;
                                
                                uVar1 = uVar1 | (uint)(fVar7 < 2.0) << 0x1f | (uint)(fVar7 == 2.0) << 0x1e;
                                in_fpscr = uVar1 | (uint)NAN(fVar7) << 0x1c;
                                bVar3 = (byte)(uVar1 >> 0x18);
                                
                                if ((bool)(bVar3 >> 6 & 1) || bVar3 >> 7 != ((byte)(in_fpscr >> 0x1c) & 1)) {
                                    // Разница < 2
                                    fVar7 = (float)VectorSignedToFloat(dropfreq, (byte)(in_fpscr >> 0x16) & 3);
                                    fVar8 = (float)VectorSignedToFloat((g_rt.config)->freq_flex, (byte)(in_fpscr >> 0x16) & 3);
                                    (*matrix_local)[i_1] = fVar7 + fVar8;
                                } else {
                                    // Разница >= 2
                                    fVar7 = (float)VectorSignedToFloat(dropfreq + 2, (byte)(in_fpscr >> 0x16) & 3);
                                    fVar8 = (float)VectorSignedToFloat((g_rt.config)->freq_flex, (byte)(in_fpscr >> 0x16) & 3);
                                    (*matrix_local)[i_1] = fVar7 + fVar8;
                                }
                            } else {
                                // Разница >= 5
                                fVar7 = (float)VectorSignedToFloat(dropfreq + 5, (byte)(in_fpscr >> 0x16) & 3);
                                fVar8 = (float)VectorSignedToFloat((g_rt.config)->freq_flex, (byte)(in_fpscr >> 0x16) & 3);
                                (*matrix_local)[i_1] = fVar7 + fVar8;
                            }
                        } else {
                             // Если fVar7 < fVar8, оставляем как есть? Нет, логика выше покрывает все случаи.
                             // Этот блок else относится к внешнему if. Если fVar7 < fVar8, то мы не входим внутрь.
                             // Значит частота остается прежней? Или нужно действие? В псевдокоде нет else для внешнего if.
                        }
                    }

                    free(eep_ctx);
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : drop end\n", "sw_drop");
                    strncpy(tmp1, "drop end", 8);
                    tmp1.8_4 = tmp1.8_4 & 0xffffff00;
                    log_to_file(tmp1, time_stamp);
                    
                    sweep_end_power_down();
                    dump_matrix_freq(*matrix_local, (g_rt.config)->domains, (g_rt.config)->asics_in_domain);
                    
                    g_rt.nonce_rate = 0.995;
                    g_rt.recv_nonce = 0;
                    
                    g_rt.sweep.sweep_end_time.0_4 = time((time_t *)0x0);
                    g_rt.sweep.sweep_end_time.4_4 = (time_t)g_rt.sweep.sweep_end_time >> 0x1f;
                    
                    min = get_freq_matrix_min(*matrix_local, len);
                    max = get_freq_matrix_max(*matrix_local, len);
                    avg_freq = get_matrix_freq_avg(*matrix_local);
                    
                    snprintf(lcd_info, 0x10, "%d ", (int)avg_freq);
                    lcd_common_show("Drop freq ", lcd_info, (char *)0x0);
                    
                    iVar6 = 0;
                } else {
                    print_time_stamp(time_stamp, 0x30);
                    printf("%s ", time_stamp);
                    printf("%s : is lowest level,can not to drop level!\n", "sw_drop");
                    strncpy(tmp1, "is lowest level,can not to drop level!", 0x27);
                    log_to_file(tmp1, time_stamp);
                    set_sweep_err_state(0x11, false);
                    free(eep_ctx);
                    iVar6 = 0x11;
                }
            }
        } else {
            print_time_stamp(time_stamp, 0x30);
            printf("%s ", time_stamp);
            printf("%s : not do pt2!\n", "sw_drop");
            strncpy(tmp1, "not do pt2!", 0xc);
            log_to_file(tmp1, time_stamp);
            set_sweep_err_state(1, false);
            free(eep_ctx);
            iVar6 = 1;
        }
    }
    return iVar6;
}