#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <unistd.h>

// --- Типы данных (на основе использования в псевдокоде) ---

struct PID {
    double max_limit;
    double min_limit;
    double target;
    double feedback;
    double startup_Kp;
    double Kp;
    double Ki;
    double Kd;
    double e_0;
    double e_1;
    double e_2;
};

// --- Глобальные переменные ---

static PID gPID = {};
static int32_t last_pwm = 0;
static bool is_init_pid_parms = false;
static float pwm_percent_pid = -10000.0f; // Инициализация специальным значением для проверки
static uint64_t counter = 0;
static bool flag = false;
static int32_t pwm_ctx = 0;

// Внешние зависимости
extern int32_t get_curr_pwm();
extern void set_fan_pwm(uint8_t pwm_percent);
extern int32_t pwm_init(int type, int freq);
extern int32_t pwm_set(int ctx, uint32_t duty);
extern void print_time_stamp(char *buf, int len);
extern void log_to_file(char *msg, char *time);
extern void _applog(int level, const char *msg, bool newline);

// Заглушка для аппаратной конвертации (в оригинале используется векторная инструкция)
// Эмулируем поведение: преобразование целого числа в float с учетом знака
static inline float VectorSignedToFloat(int val, uint8_t mode) {
    return static_cast<float>(val);
}

// --- Функции ---

/**
 * @brief Установка ШИМ в зависимости от температуры.
 * Если температура ниже порога, используется PID, иначе максимальная скорость.
 */
void set_pwm_by_temp(int temp) {
    if (!is_init_pid_parms) {
        init_pid_parms();
    }

    if (temp < 0x3b) { // 59 градусов
        set_pwm_by_pid(temp);
    } else {
        set_pwm_in_high_temp(temp);
    }
}

/**
 * @brief Инициализация параметров PID регулятора.
 */
void init_pid_parms(void) {
    gPID.startup_Kp = -9.0;
    gPID.Kp = -9.5;
    gPID.Ki = -1.0;
    gPID.Kd = 0.0;
    gPID.target = 56.0;
    gPID.min_limit = 50.0;
    gPID.max_limit = 100.0;
    is_init_pid_parms = true;
}

/**
 * @brief Получение текущего значения PWM.
 */
int32_t get_curr_pwm(void) {
    return last_pwm;
}

/**
 * @brief Расчет PWM на основе PID регулятора.
 */
void set_pwm_by_pid(int temp) {
    uint32_t uVar5;
    
    // Проверка флага инициализации pwm_percent_pid через битовые операции (эмуляция логики декомпилятора)
    // В C++ проще проверить значение напрямую, но сохраняем логику:
    // Если pwm_percent_pid равно специальному значению -10000.0, берем текущее значение из железа
    if (std::isnan(pwm_percent_pid) || pwm_percent_pid == -10000.0f) {
        int32_t iVar4 = get_curr_pwm();
        pwm_percent_pid = VectorSignedToFloat(iVar4, 0);
    }

    gPID.feedback = VectorSignedToFloat(temp, 0);
    
    float fVar6 = pid_calc(&gPID, static_cast<int>(pwm_percent_pid));
    pwm_percent_pid = fVar6 + pwm_percent_pid;

    char time_stamp[48];
    char tmp1[256];
    print_time_stamp(time_stamp, 0x30);
    printf("%s ", time_stamp);
    printf("%s : feedback: %f, input: %f\n\n", "set_pwm_by_pid", gPID.feedback, pwm_percent_pid);
    snprintf(tmp1, 0x100, "feedback: %f, input: %f\n", gPID.feedback, pwm_percent_pid);
    log_to_file(tmp1, time_stamp);

    int32_t iVar3 = static_cast<int32_t>(pwm_percent_pid + 0.5f);
    int32_t iVar4 = get_curr_pwm();

    if (iVar4 != iVar3) {
        float fVar7 = VectorSignedToFloat(iVar3, 0);
        
        // Ограничение снизу (min_limit)
        float limit_val = gPID.min_limit;
        if (gPID.min_limit <= fVar7) {
            limit_val = fVar7;
        }
        
        // Ограничение сверху (max_limit)
        if (limit_val > gPID.max_limit) {
            limit_val = gPID.max_limit;
        }

        uint8_t local_148 = 0;
        if (limit_val > 0.0f) {
            local_148 = static_cast<uint8_t>(limit_val);
        } else {
            local_148 = 0;
        }

        set_fan_pwm(local_148);
    }
}

/**
 * @brief Установка PWM при высокой температуре (максимальная скорость).
 */
void set_pwm_in_high_temp(int temp) {
    gPID.feedback = VectorSignedToFloat(temp, 0);
    pid_calc(&gPID, 100);
    set_fan_pwm(100); // 'd' в ASCII это 100, но логичнее передать 100 как процент
}

/**
 * @brief Расчет выходного сигнала PID регулятора.
 */
float pid_calc(PID *pid, int pwm_percent_pid_current) {
    pid->e_0 = pid->target - pid->feedback;

    if (!flag) {
        pid->e_1 = pid->e_0;
        pid->e_2 = pid->e_0;
        flag = true;
    }

    // P + I + D составляющие
    float out = pid->Kp * (pid->e_0 - pid->e_1) + 
                pid->Ki * pid->e_0 + 
                pid->Kd * ((pid->e_0 - (pid->e_1 + pid->e_1)) + pid->e_2);

    float fVar4 = VectorSignedToFloat(pwm_percent_pid_current, 0);
    fVar4 = fVar4 + out;

    // Ограничение выхода
    float limit_val = fVar4;
    if (limit_val < pid->min_limit) {
        out = pid->min_limit - fVar4; // Корректировка out для возврата дельты? 
                                      // В оригинале возвращается out, который потом добавляется к текущему значению.
                                      // Здесь логика немного запутана в псевдокоде, но суть - ограничение.
        // Если мы ограничиваем fVar4, то out должен быть таким, чтобы fVar4 не вышел за пределы.
        // Но в конце функции возвращается out.
        // Давайте последуем строго логике присваивания out в ветках else/if
        
        if (fVar4 < pid->min_limit) {
             out = pid->min_limit - VectorSignedToFloat(pwm_percent_pid_current, 0);
        } else {
             out = pid->max_limit - VectorSignedToFloat(pwm_percent_pid_current, 0);
        }
    } else if (fVar4 > pid->max_limit) {
         out = pid->max_limit - VectorSignedToFloat(pwm_percent_pid_current, 0);
    } else {
         out = out; // Без изменений
    }
    
    // Обновление истории ошибок
    pid->e_2 = pid->e_1;
    pid->e_1 = pid->e_0;
    
    counter++;

    return out;
}

/**
 * @brief Установка значения PWM через контекст.
 */
void set_fan_pwm(uint8_t pwm_percent) {
    if (pwm_ctx == 0) {
        // PWM_CTRL_FAN предполагается равным 0 или другому константному значению
        pwm_ctx = pwm_init(0, 100000); 
    }

    if (pwm_ctx > -1 && static_cast<uint32_t>(pwm_percent) != static_cast<uint32_t>(last_pwm)) {
        uint32_t duty = pwm_percent;
        pwm_set(pwm_ctx, duty);
        
        char time_stamp[48];
        char tmp1[256];
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s : Set PWM success , current pwm is : %d\n", "set_fan_pwm", duty);
        snprintf(tmp1, 0x100, "Set PWM success , current pwm is : %d", duty);
        log_to_file(tmp1, time_stamp);
        
        last_pwm = duty;
    }
}