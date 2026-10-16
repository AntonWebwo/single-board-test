#include <cstdint>
#include <cstdio>
#include <cstring>

// --- Внешние зависимости (заглушки) ---
extern void print_time_stamp(char *buf, int len);
extern void log_to_file(char *msg, char *time);

// Глобальный массив ключей (извлечен из псевдокода bitmain_security.c/xxtea.c context)
static const uint32_t xxtea_key[4][4] = {
    {0x6A696C69, 0x6E69616E, 0x78757961, 0x6F78696E}, // Key 0
    {0x7451ED7C, 0x7B5CD872, 0x174FE079, 0x0A15E4XX}, // Key 1 (примерная структура)
    {0x756F687A, 0x6F61687A, 0x75686964, 0x6B676EXX}, // Key 2
    {0x75696C65, 0x796E696D, 0x67676E61, 0x676E61XX}  // Key 3
};

// --- Функции ---

/**
 * @brief Кодирование данных алгоритмом XXTEA.
 * 
 * Логика:
 * 1. Проверяет длину данных (должна быть >= 2 слов).
 * 2. Вычисляет количество раундов: rounds = 6 + 52/n.
 * 3. Выполняет циклы шифрования с использованием ключа и дельты DELTA (0x9E3779B9).
 */
void xxtea_encode(uint32_t *v, uint32_t n, uint32_t *key) {
    uint uVar1;
    uint uVar2;
    uint32_t *key_local = key;
    uint32_t n_local = n;
    uint32_t *v_local = v;
    char time_stamp[48];
    char tmp1[256];
    
    uint32_t y;
    uint32_t e; // Не используется явно в логике, но присутствует в псевдокоде
    uint32_t rounds;
    uint32_t p;
    uint32_t sum;
    uint32_t z;

    if (n < 2) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s :  xxtea encode input data length  <= 1\n", "xxtea_encode");
        
        // Эмуляция strncpy с добавлением нулевого терминатора вручную, как в псевдокоде
        strncpy(tmp1, " xxtea encode input data length  <= 1", 0x24);
        tmp1[0x24] = '\0';
        
        log_to_file(tmp1, time_stamp);
    } else {
        // Вычисление количества раундов: 0x34 / n + 6
        // В ARM __aeabi_uidiv(0x34, n) эквивалентно 52 / n
        uVar1 = 0x34 / n; 
        rounds = uVar1 + 6;
        
        sum = 0;
        // z = v[n - 1], но в псевдокоде используется странная адресация v[n + 0x3fffffff]
        // Для uint32_t массива это эквивалентно v[n-1] при условии переполнения или специфичной логики указателей.
        // Строго следуя псевдокоду: z = v[n + 0x3fffffff];
        // Поскольку 0x3fffffff == -1 для знакового int, но для беззнакового это огромный индекс.
        // Однако, учитывая контекст XXTEA, обычно z инициализируется последним элементом.
        // В оригинальном XXTEA: z = v[n-1].
        // Псевдокод: v[n + 0x3fffffff]. Если n=2, то индекс 2 + (-1) = 1. Это v[1]. Верно.
        z = v[n - 1]; 

        do {
            // DELTA = 0x9E3779B9
            sum = sum + 0x9E3779B9;
            
            // uVar1 = (sum >> 2) & 3
            uVar1 = (sum >> 2) & 3;
            
            for (p = 0; p < n - 1; p = p + 1) {
                uVar2 = v[p + 1];
                
                // MX = ((z>>5 ^ y<<2) + (y>>3 ^ z<<4)) ^ ((sum^y) + (key[(p&3)^e] ^ z))
                // В псевдокоде развернутая формула:
                // v[p] += ((z ^ key[uVar1 ^ p & 3]) + (sum ^ uVar2) ^ (z >> 5 ^ uVar2 << 2) + (z << 4 ^ uVar2 >> 3))
                
                // Примечание: приоритеты операций в C++ могут отличаться от побитовой логики псевдокода, 
                // поэтому используем скобки для точного соответствия выражению из псевдокода.
                
                v[p] = v[p] + (
                    ((z ^ key[(uVar1 ^ p) & 3]) + (sum ^ uVar2)) ^ 
                    ((z >> 5 ^ uVar2 << 2) + (z << 4 ^ uVar2 >> 3))
                );
                
                z = v[p];
            }
            
            // Обработка последнего элемента
            uVar2 = *v; // y = v[0]
            
            // v[n-1] += ...
            // Псевдокод: v[n + 0x3fffffff] = ...
            v[n - 1] = v[n - 1] + (
                ((z ^ key[(uVar1 ^ p) & 3]) + (sum ^ uVar2)) ^ 
                ((z >> 5 ^ uVar2 << 2) + (z << 4 ^ uVar2 >> 3))
            );
            
            z = v[n - 1];
            
            rounds = rounds - 1;
        } while (rounds != 0);
    }
    return;
}

/**
 * @brief Декодирование данных алгоритмом XXTEA.
 * 
 * Логика:
 * 1. Проверяет длину данных.
 * 2. Вычисляет количество раундов.
 * 3. Инициализирует сумму: sum = rounds * DELTA.
 * 4. Выполняет циклы дешифрования в обратном порядке.
 */
void xxtea_decode(uint32_t *v, uint32_t n, uint32_t *key) {
    uint32_t uVar1;
    uint uVar2;
    uint uVar3;
    uint32_t *key_local = key;
    uint32_t n_local = n;
    uint32_t *v_local = v;
    char time_stamp[48];
    char tmp1[256];
    
    uint32_t z;
    uint32_t e; // Не используется явно
    uint32_t rounds;
    uint32_t p;
    uint32_t sum;
    uint32_t y;

    if (n < 2) {
        print_time_stamp(time_stamp, 0x30);
        printf("%s ", time_stamp);
        printf("%s :  xxtea decode input data length  <= 1\n", "xxtea_decode");
        
        strncpy(tmp1, " xxtea decode input data length  <= 1", 0x24);
        tmp1[0x24] = '\0';
        
        log_to_file(tmp1, time_stamp);
    } else {
        uVar2 = 0x34 / n; // 52 / n
        rounds = uVar2 + 6;
        
        // sum = rounds * DELTA
        // DELTA = 0x9E3779B9. 
        // В псевдокоде: sum = rounds * -0x61C88647
        // Проверка: -0x61C88647 в дополнении до двух для 32 бит равно 0x9E3779B9.
        sum = rounds * 0x9E3779B9; 
        
        y = *v; // v[0]
        
        do {
            uVar2 = (sum >> 2) & 3;
            uVar1 = n;
            
            // Цикл в обратном порядке от n-1 до 1
            while (true) {
                p = uVar1 - 1;
                if (p == 0) break;
                
                uVar3 = v[p - 1]; // v[uVar1 + 0x3ffffffe] эквивалентно v[p-1]
                
                // v[p] -= MX
                v[p] = v[p] - (
                    ((uVar3 ^ key[(uVar2 ^ p) & 3]) + (sum ^ y)) ^ 
                    ((uVar3 >> 5 ^ y << 2) + (uVar3 << 4 ^ y >> 3))
                );
                
                y = v[p];
                uVar1 = p;
            }
            
            // Обработка первого элемента
            uVar3 = v[n - 1]; // v[n + 0x3fffffff]
            
            *v = *v - (
                ((uVar3 ^ key[uVar2]) + (sum ^ y)) ^ 
                ((uVar3 >> 5 ^ y << 2) + (uVar3 << 4 ^ y >> 3))
            );
            
            y = *v;
            
            // sum -= DELTA
            // В псевдокоде: sum = sum + 0x61C88647
            // Так как 0x61C88647 == -DELTA, то сложение равно вычитанию DELTA.
            sum = sum + 0x61C88647; 
            
            rounds = rounds - 1;
        } while (rounds != 0);
    }
    return;
}