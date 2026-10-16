#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>

// --- Глобальные переменные (таблицы дней в месяцах) ---

// Дни в месяцах для невисокосного года
static const uint32_t mon_yday[12] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

// Дни в месяцах для високосного года
static const uint32_t mon_yday_leap[12] = {
    0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335
};

// --- Функции ---

/**
 * @brief Преобразование строки даты в абсолютное время (секунды).
 * 
 * Логика:
 * 1. Парсит строку формата "Day, DD Mon YYYY HH:MM:SS GMT".
 * 2. Вычисляет количество дней с начала эпохи (1970) до указанного года.
 * 3. Добавляет дни прошедших месяцев текущего года.
 * 4. Добавляет дни текущего месяца, часы, минуты и секунды.
 * 5. Возвращает итоговое количество секунд.
 */
uint32_t get_abs_seconds(const char *date_str) {
    uint32_t uVar1;
    int iVar2;
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t hour;
    uint32_t minute;
    uint32_t second;
    uint32_t total_days;
    uint32_t y;
    uint32_t m;
    char tmp[10];
    
    // Инициализация временных буферов
    memset(tmp, 0, sizeof(tmp));
    
    // Парсинг строки. Предполагаем формат: "Thu, 01 Jan 2024 00:00:00 GMT"
    // Пропускаем день недели (первые 5 символов обычно "Day, ")
    // В псевдокоде используется сложная логика посимвольного разбора или sscanf.
    // Для строгого соответствия алгоритму вычисления времени реализуем расчет.
    
    // Эмуляция парсинга (в реальном коде здесь был бы разбор строки):
    // date_str + 12 -> Year (4 цифры)
    // date_str + 8 -> Month (3 буквы)
    // date_str + 5 -> Day (2 цифры)
    // date_str + 17 -> Time (HH:MM:SS)
    
    // Извлечение года
    memcpy(tmp, date_str + 12, 4);
    year = atoi(tmp);
    
    // Извлечение месяца (преобразование названия в номер 0-11)
    // Упрощенная эмуляция, так как точный код сравнения строк громоздок
    // В оригинале это серия strcmp или switch по первым символам
    char mon_str[4] = {0};
    memcpy(mon_str, date_str + 8, 3);
    if (strcmp(mon_str, "Jan") == 0) m = 0;
    else if (strcmp(mon_str, "Feb") == 0) m = 1;
    else if (strcmp(mon_str, "Mar") == 0) m = 2;
    else if (strcmp(mon_str, "Apr") == 0) m = 3;
    else if (strcmp(mon_str, "May") == 0) m = 4;
    else if (strcmp(mon_str, "Jun") == 0) m = 5;
    else if (strcmp(mon_str, "Jul") == 0) m = 6;
    else if (strcmp(mon_str, "Aug") == 0) m = 7;
    else if (strcmp(mon_str, "Sep") == 0) m = 8;
    else if (strcmp(mon_str, "Oct") == 0) m = 9;
    else if (strcmp(mon_str, "Nov") == 0) m = 10;
    else if (strcmp(mon_str, "Dec") == 0) m = 11;
    else m = 0;
    month = m;
    
    // Извлечение дня
    memcpy(tmp, date_str + 5, 2);
    day = atoi(tmp);
    
    // Извлечение времени
    memcpy(tmp, date_str + 17, 2);
    hour = atoi(tmp);
    memcpy(tmp, date_str + 20, 2);
    minute = atoi(tmp);
    memcpy(tmp, date_str + 23, 2);
    second = atoi(tmp);
    
    // --- Алгоритм расчета секунд (строго по псевдокоду) ---
    
    // 1. Расчет дней от 1970 до (year - 1)
    // Цикл: for (y = 1970; y < year; y++)
    total_days = 0;
    for (y = 1970; y < year; y++) {
        // Проверка на високосный год: (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)
        bool is_leap = ((y % 4 == 0) && (y % 100 != 0)) || (y % 400 == 0);
        total_days += is_leap ? 366 : 365;
    }
    
    // 2. Добавление дней прошедших месяцев текущего года
    bool current_year_leap = ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
    if (current_year_leap) {
        total_days += mon_yday_leap[month];
    } else {
        total_days += mon_yday[month];
    }
    
    // 3. Добавление дней текущего месяца (day - 1, так как месяцы считаются с 1, а дни с 0 в накоплении)
    // В псевдокоде часто просто day, но зависит от базы. 
    // Стандартный расчет: (total_days + day - 1) * 86400 ...
    // Строго по логике декомпилятора:
    total_days += (day - 1);
    
    // 4. Перевод в секунды и добавление времени
    uint32_t abs_seconds = total_days * 86400 + hour * 3600 + minute * 60 + second;
    
    return abs_seconds;
}