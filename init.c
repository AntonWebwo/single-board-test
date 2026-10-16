#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

// --- Заглушки для внешних зависимостей ---
extern int32_t platform_init();
extern void platform_uninit();
extern int32_t miner_loop(); // Основной цикл работы майнера
extern void _applog(int level, const char* msg, bool newline);

// --- Глобальные переменные ---
static volatile sig_atomic_t g_quit = 0;

// --- Функции ---

/**
 * @brief Обработчик сигналов завершения (SIGTERM, SIGINT).
 * Устанавливает флаг выхода из основного цикла.
 */
void sig_handler(int signo) {
    if (signo == SIGTERM || signo == SIGINT) {
        g_quit = 1;
    }
}

/**
 * @brief Точка входа в программу.
 * Инициализирует платформу, настраивает обработчики сигналов и запускает майнинг.
 */
int main(int argc, char** argv) {
    int32_t ret = 0;
    
    // Настройка обработчиков сигналов
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    // Инициализация платформы (FPGA, GPIO, UART и т.д.)
    ret = platform_init();
    if (ret != 0) {
        _applog(0, "Platform init failed!\n", false);
        return -1;
    }

    _applog(3, "Platform initialized successfully.\n", false);

    // Основной цикл работы
    while (!g_quit) {
        ret = miner_loop();
        if (ret != 0) {
            _applog(0, "Miner loop error occurred.\n", false);
            break;
        }
        
        // Небольшая задержка, чтобы не нагружать CPU в холостом цикле
        usleep(100000); 
    }

    // Деинициализация платформы перед выходом
    platform_uninit();
    _applog(3, "System shutdown complete.\n", false);

    return 0;
}