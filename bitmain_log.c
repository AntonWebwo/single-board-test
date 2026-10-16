#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

// --- Типы данных (на основе использования в псевдокоде) ---

struct log_info_t {
    char path[128];
    char compressed_path[128];
    char encoded_path[128];
    char *data;
    size_t origin_size;
    size_t filter_size;
    bool compressed;
    bool encode;
};

struct test_info_t {
    char file_path[128];
    char test_type[128];
    char miner_type[128];
    char boardname[128];
    char sn[128];
    char fac[128]; // Factory code extracted from SN
};

// Внешние зависимости
extern struct Local_Config_Info* Local_Config_Information;
extern void hmac_sha256(uint8_t *out, const uint8_t *data, size_t data_len, const uint8_t *key, size_t key_len);
extern int32_t get_ip(char *ip);
extern char * post_request(char *url, char *post_str);
extern void print_time_stamp(char *buf, int len);
extern void log_to_file(char *msg, char *time);
extern void _applog(int level, const char* msg, bool newline);

// Заглушки для внешних библиотек (LZ4, Base64, JSON)
extern int compress_file(FILE *in, FILE *out);
extern int base64_encode_file(FILE *in, FILE *out);
// Jansson functions assumed to be available via jansson.h included elsewhere or linked
extern void* json_object();
extern void* json_string(const char *value);
extern void json_object_set(void *object, const char *key, void *value);
extern char* json_dumps(void *json, size_t flags);
extern void json_delete(void *json);
extern void free(void *ptr);

// --- Глобальные переменные ---

static pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t upload_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t upload_cond = PTHREAD_COND_INITIALIZER;

static const char* LOG_PREFIX_LOCAL = "local_";
static const char* LOG_PREFIX_UPLOADED = "uploaded_";
static const char* LOG_DIR = "/mnt/card/logs/";

static FILE *g_log_file = nullptr;
static char g_cur_log_path[128] = {};

static char buff[1024] = {}; // Buffer for log_to_file
static char buff_x[1024] = {}; // Buffer for log_to_filex

// --- Функции ---

/**
 * @brief Получение размера файла.
 */
size_t get_file_size(char *filename) {
    struct stat statbuf;
    if (filename == nullptr) {
        return 0;
    }
    if (stat(filename, &statbuf) != 0) {
        return 0;
    }
    return statbuf.st_size;
}

/**
 * @brief Получение форматированного времени.
 */
void get_format_time(char *time_str) {
    time_t rawtime;
    struct tm *info;
    time(&rawtime);
    info = localtime(&rawtime);
    // В псевдокоде есть printf для отладки месяца, сохраняем логику
    printf("month:%d\n", info->tm_mon); 
    strftime(time_str, 0x18, "%m-%d-%H-%M-%S-%Z", info);
}

/**
 * @brief Разбиение массива строк (Quicksort partition).
 */
int paritition(char (*logs)[128], int left, int right) {
    char pivot[128];
    strncpy(pivot, logs[left], 0x80);
    
    int l = left;
    int r = right;
    
    while (l < r) {
        while (l < r && strncmp(pivot, logs[r], 0x80) > -1) { // Note: logic in pseudocode seems to sort ascending/descending based on strncmp result
             // Pseudocode: iVar1 = strncmp(pivot,logs[right_local],0x80), -1 < iVar1
             // This means while pivot >= logs[r], move r left
            r--;
        }
        if (l != r) {
            strncpy(logs[l], logs[r], 0x80);
        }
        
        while (l < r && strncmp(pivot, logs[l], 0x80) < 1) {
             // Pseudocode: iVar1 = strncmp(pivot,logs[left_local],0x80), iVar1 < 1
             // This means while pivot > logs[l], move l right
            l++;
        }
        if (l != r) {
            strncpy(logs[r], logs[l], 0x80);
        }
    }
    strncpy(logs[l], pivot, 0x80);
    return l;
}

/**
 * @brief Сортировка файлов по времени (имени).
 */
void sort_files_by_time(char (*logs)[128], int left, int right) {
    if (left < right) {
        int pivot = paritition(logs, left, right);
        sort_files_by_time(logs, left, pivot - 1);
        sort_files_by_time(logs, pivot + 1, right);
    }
}

/**
 * @brief Обновление списка логов.
 */
int refresh_log_list(char (*logs)[128], char *prefix) {
    DIR *dir;
    struct dirent *ent;
    int log_cnt = 0;
    
    dir = opendir(LOG_DIR);
    if (dir != nullptr) {
        while ((ent = readdir(dir)) != nullptr && log_cnt < 400) {
            size_t n = strlen(prefix);
            // Проверка префикса
            if (strncmp(ent->d_name, prefix, n) == 0) {
                // Исключение сжатых/закодированных файлов
                if (strstr(ent->d_name, ".lz4") == nullptr && strstr(ent->d_name, ".64") == nullptr) {
                    strncpy(logs[log_cnt], ent->d_name, 0x7f);
                    logs[log_cnt][0x7f] = '\0';
                    log_cnt++;
                }
            }
        }
        closedir(dir);
    }
    
    if (log_cnt > 0) {
        sort_files_by_time(logs, 0, log_cnt - 1);
    }
    
    return log_cnt;
}

/**
 * @brief Очистка кэш-файлов (сжатых/закодированных).
 */
void clear_cache_file(void) {
    DIR *dir;
    struct dirent *ent;
    char full_name[128];
    
    dir = opendir(LOG_DIR);
    if (dir != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            if (strstr(ent->d_name, ".lz4") != nullptr || strstr(ent->d_name, ".64") != nullptr) {
                snprintf(full_name, 0x80, "%s%s", LOG_DIR, ent->d_name);
                remove(full_name);
                unlink(full_name);
            }
        }
        closedir(dir);
    }
}

/**
 * @brief Очистка старых не загруженных логов (хранит максимум 200).
 */
void clear_unupload_logs(void) {
    char full_name[128];
    char logs[400][128];
    memset(logs, 0, sizeof(logs));
    
    int i = refresh_log_list(logs, const_cast<char*>(LOG_PREFIX_LOCAL));
    if (i > 200) {
        while (i > 200) {
            i--;
            snprintf(full_name, 0x80, "%s%s", LOG_DIR, logs[i]);
            remove(full_name);
            unlink(full_name);
        }
    }
}

/**
 * @brief Очистка старых загруженных логов (хранит максимум 100).
 */
void clear_uploaded_logs(void) {
    char full_name[128];
    char logs[200][128];
    memset(logs, 0, sizeof(logs));
    
    int i = refresh_log_list(logs, const_cast<char*>(LOG_PREFIX_UPLOADED));
    if (i > 100) {
        while (i > 100) {
            i--;
            snprintf(full_name, 0x80, "%s%s", LOG_DIR, logs[i]);
            remove(full_name);
            unlink(full_name);
        }
    }
}

/**
 * @brief Создание нового лог-файла.
 */
void create_new_log_file(char *test_type, char *sn, char *miner_type, char *board_name) {
    if (g_log_file == nullptr) {
        char datetime[24];
        char file_path[128];
        get_format_time(datetime);
        
        mode_t old_mask = umask(0);
        mkdir(LOG_DIR, 0x1ff);
        umask(old_mask);
        
        memset(file_path, 0, 0x80);
        snprintf(file_path, 0x80, "%s%s%s%s%s_%s", LOG_DIR, LOG_PREFIX_LOCAL, test_type, miner_type, board_name, datetime);
        
        // Проверка типа теста (возвращает строку или NULL)
        // В псевдокоде вызывается get_test_type, но результат не используется для ветвления, только проверка на NULL?
        // Нет, в псевдокоде: pcVar1 = get_test_type(file_path); if (pcVar1 != 0) ...
        // Но get_test_type ищет подстроки в имени файла. Имя файла еще не создано.
        // Вероятно, проверка избыточна или предназначена для валидации параметров.
        // Реализуем как в псевдокоде.
        
        // Примечание: get_test_type ожидает имя файла, содержащее тип теста.
        // Так как мы только что сформировали file_path, проверим его.
        char* type_check = nullptr; 
        // Эмуляция логики get_test_type внутри create_new_log_file не требуется, если просто открыть файл.
        // Но следуем псевдокоду:
        // В псевдокоде get_test_type ищет "PT2", "PT1new" и т.д. в строке.
        // Если найден, возвращает указатель.
        
        // Для упрощения и соответствия логике создания файла:
        g_log_file = fopen(file_path, "a+");
        if (g_log_file) {
            strcpy(g_cur_log_path, file_path);
        }
    }
}

/**
 * @brief Начало новой сессии логирования.
 */
void start_new_log(char *test_type, char *sn, char *miner_type, char *board_name) {
    pthread_mutex_lock(&file_mutex);
    if (g_log_file != nullptr) {
        fflush(g_log_file);
        fclose(g_log_file);
        g_log_file = nullptr;
    }
    pthread_mutex_unlock(&file_mutex);
    
    pthread_mutex_lock(&upload_mutex);
    memset(g_cur_log_path, 0, 0x80);
    create_new_log_file(test_type, sn, miner_type, board_name);
    pthread_mutex_unlock(&upload_mutex);
    
    pthread_cond_signal(&upload_cond);
}

/**
 * @brief Запись в лог-файл (стандартная).
 */
void log_to_file(char *str, char *str_time) {
    if (g_log_file != nullptr) {
        pthread_mutex_lock(&file_mutex);
        snprintf(buff, 0x3ff, "%s%s\n", str_time, str);
        size_t len = strlen(buff);
        size_t written = fwrite(buff, 1, len, g_log_file);
        fflush(g_log_file);
        
        if (written != len) {
            printf("warning:write log failed,write:%zu,target:%zu\n", written, len);
        }
        pthread_mutex_unlock(&file_mutex);
    }
}

/**
 * @brief Запись в лог-файл (расширенная/сырая).
 */
void log_to_filex(char *str, char *str_time) {
    if (g_log_file != nullptr) {
        pthread_mutex_lock(&file_mutex);
        // В псевдокоде format string is just "%s", but args are str, str_time. 
        // Likely meant to concatenate or just write str. 
        // Pseudocode: snprintf(..., "%s", str, str_time); -> effectively writes str.
        snprintf(buff_x, 0x3ff, "%s", str); 
        size_t len = strlen(buff_x);
        size_t written = fwrite(buff_x, 1, len, g_log_file);
        fflush(g_log_file);
        
        if (written != len) {
            printf("warning:write log failed,write:%zu,target:%zu\n", written, len);
        }
        pthread_mutex_unlock(&file_mutex);
    }
}

/**
 * @brief Чтение данных из файла со смещением.
 */
int read_data(FILE *file, uint32_t offset, uint32_t len, uint8_t *data) {
    if (file != nullptr) {
        fseek(file, 0, SEEK_END);
        uint32_t file_size = ftell(file);
        
        uint32_t read_len = len;
        if (file_size < offset + len) {
            read_len = file_size - offset;
        }
        
        fseek(file, offset, SEEK_SET);
        fread(data, 1, read_len, file);
    }
    return len;
}

/**
 * @brief Чтение и декодирование лог-файла (Base64).
 */
bool read_encode_log_file(log_info_t *p_info) {
    snprintf(p_info->encoded_path, 0x80, "%s.64", p_info->path);
    
    FILE *f_in = fopen(p_info->path, "rb");
    FILE *f_out = fopen(p_info->encoded_path, "wb");
    
    if (!f_in || !f_out) {
        if (f_in) fclose(f_in);
        if (f_out) fclose(f_out);
        return false;
    }
    
    int res = base64_encode_file(f_in, f_out);
    fclose(f_in);
    fclose(f_out);
    
    if (res == 0) {
        p_info->encode = true;
        bool ret = read_log_file(p_info); // Read from encoded path
        remove(p_info->encoded_path);
        return ret;
    }
    return false;
}

/**
 * @brief Чтение, сжатие и кодирование лог-файла.
 */
bool read_compress_log_file(log_info_t *p_info) {
    snprintf(p_info->compressed_path, 0x80, "%s.lz4", p_info->path);
    snprintf(p_info->encoded_path, 0x80, "%s.64", p_info->path);
    
    FILE *f_in = fopen(p_info->path, "rb");
    FILE *f_comp = fopen(p_info->compressed_path, "wb");
    
    if (!f_in || !f_comp) {
        if (f_in) fclose(f_in);
        if (f_comp) fclose(f_comp);
        return false;
    }
    
    int res = compress_file(f_in, f_comp);
    fclose(f_in);
    fclose(f_comp);
    
    if (res == 0) {
        f_in = fopen(p_info->compressed_path, "rb");
        FILE *f_enc = fopen(p_info->encoded_path, "wb");
        
        if (!f_in || !f_enc) {
            if (f_in) fclose(f_in);
            if (f_enc) fclose(f_enc);
            return false;
        }
        
        res = base64_encode_file(f_in, f_enc);
        fclose(f_in);
        fclose(f_enc);
        
        if (res == 0) {
            p_info->compressed = true;
            p_info->encode = true;
            bool ret = read_log_file(p_info); // Read from encoded path
            remove(p_info->compressed_path);
            remove(p_info->encoded_path);
            return ret;
        }
    }
    return false;
}

/**
 * @brief Чтение содержимого лог-файла в память.
 */
bool read_log_file(log_info_t *p_info) {
    char *path_str = p_info->path;
    if (p_info->encode) {
        path_str = p_info->encoded_path;
    } else if (p_info->compressed) {
        path_str = p_info->encoded_path; // Usually encoded path is used if compressed too
    }
    
    FILE *file = fopen(path_str, "r");
    if (!file) return false;
    
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (size == 0) {
        fclose(file);
        return false;
    }
    
    p_info->data = (char *)malloc(size);
    if (!p_info->data) {
        fclose(file);
        return false;
    }
    
    memset(p_info->data, 0, size);
    size_t read = fread(p_info->data, 1, size, file);
    
    if (read == size) {
        p_info->origin_size = size;
        p_info->filter_size = size;
        fclose(file);
        return true;
    } else {
        free(p_info->data);
        p_info->data = nullptr;
        fclose(file);
        return false;
    }
}

/**
 * @brief Определение типа теста по имени файла.
 */
char * get_test_type(char *log_name) {
    if (strstr(log_name, "PT2")) return const_cast<char*>("PT2");
    if (strstr(log_name, "PT1new")) return const_cast<char*>("PT1new");
    if (strstr(log_name, "SWEEP")) return const_cast<char*>("SWEEP");
    if (strstr(log_name, "PT3")) return const_cast<char*>("PT3");
    return nullptr;
}

/**
 * @brief Извлечение информации о тесте из имени файла.
 */
void get_test_info(char *log_name, test_info_t *test_info) {
    char m_log_name[128];
    memset(m_log_name, 0, 0x80);
    strncpy(m_log_name, log_name, 0x7f);
    strncpy(test_info->file_path, log_name, 0x7f);
    
    // Tokenize by space or underscore? Pseudocode uses strtok with " " (space).
    // But filenames usually don't have spaces. Let's assume delimiter is implicit or specific.
    // Pseudocode: token = strtok(m_log_name, " ");
    // Then it skips tokens until it finds LOG_PREFIX_LOCAL.
    
    char *token = strtok(m_log_name, "_"); // Changed to underscore as typical for these filenames, or keep space if strictly following pseudocode which might imply a formatted string. 
    // Strictly following pseudocode:
    token = strtok(m_log_name, " "); 
    
    while (token != nullptr && strstr(token, LOG_PREFIX_LOCAL) == nullptr) {
        token = strtok(nullptr, " ");
    }
    
    // Next tokens should be TestType, MinerType, BoardName, SN
    token = strtok(nullptr, " ");
    if (token) strncpy(test_info->test_type, token, 0x7f);
    
    token = strtok(nullptr, " ");
    if (token) strncpy(test_info->miner_type, token, 0x7f);
    
    token = strtok(nullptr, " ");
    if (token) strncpy(test_info->boardname, token, 0x7f);
    
    token = strtok(nullptr, " ");
    if (token) {
        strncpy(test_info->sn, token, 0x7f);
        strncpy(test_info->fac, token, 4); // First 4 chars of SN
    }
}

/**
 * @brief Проверка UTF-8 строки.
 */
int utf8_check_string_x(char *string, int length, int *p_err_pos) {
    if (length == -1) {
        length = strlen(string);
    }
    
    for (int i = 0; i < length; i++) {
        unsigned char c = string[i];
        int bytes = 1;
        
        if ((c & 0x80) != 0) {
            if ((c & 0xE0) == 0xC0) bytes = 2;
            else if ((c & 0xF0) == 0xE0) bytes = 3;
            else if ((c & 0xF8) == 0xF0) bytes = 4;
            else {
                *p_err_pos = -1;
                return 0;
            }
            
            if (i + bytes > length) {
                *p_err_pos = -1;
                return 0;
            }
            
            for (int j = 1; j < bytes; j++) {
                if ((string[i+j] & 0xC0) != 0x80) {
                    *p_err_pos = i;
                    return 0;
                }
            }
            i += bytes - 1;
        }
    }
    return 1;
}

/**
 * @brief Загрузка лога на сервер MES.
 */
bool upload_logs(char *file_path) {
    log_info_t m_info;
    memset(&m_info, 0, sizeof(m_info));
    strcpy(m_info.path, file_path);
    
    pthread_mutex_lock(&file_mutex);
    bool read_res = read_compress_log_file(&m_info);
    pthread_mutex_unlock(&file_mutex);
    
    if (!read_res || m_info.data == nullptr) {
        return false;
    }
    
    test_info_t m_test_info;
    memset(&m_test_info, 0, sizeof(m_test_info));
    get_test_info(file_path, &m_test_info);
    
    // Generate Nonce (timestamp in ms)
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    char nonce[20];
    snprintf(nonce, 0x14, "%llu", (unsigned long long)tv.tv_sec * 1000 + tv.tv_usec / 1000);
    
    // Signature generation
    uint8_t signature[32];
    char sig_message[100];
    memset(sig_message, 0, 100);
    
    strncat(sig_message, m_test_info.fac, 99);
    strncat(sig_message, "f5ece2e3-9b84-4052-a34a-645d3b5900a6", 99 - strlen(sig_message));
    strncat(sig_message, nonce, 99 - strlen(sig_message));
    
    const char *SECRET_KEY = "7796B3FFF1ECBA69A3CCB913C48BBD45";
    hmac_sha256(signature, (uint8_t*)sig_message, strlen(sig_message), (uint8_t*)SECRET_KEY, strlen(SECRET_KEY));
    
    char str_signature[100];
    for (int i = 0; i < 32; i++) {
        sprintf(str_signature + i * 2, "%02X", signature[i]);
    }
    
    // Get IP
    char ip[32];
    memset(ip, 0, 32);
    if (get_ip(ip) != 0) {
        strncpy(ip, "55.55.55.55", 0x1f);
    }
    
    // Build JSON
    void *object = json_object();
    json_object_set(object, "userId", json_string(m_test_info.fac));
    json_object_set(object, "nonce", json_string(nonce));
    json_object_set(object, "signature", json_string(str_signature));
    json_object_set(object, "minerType", json_string(m_test_info.miner_type));
    json_object_set(object, "boardName", json_string(m_test_info.boardname));
    json_object_set(object, "sn", json_string(m_test_info.sn));
    json_object_set(object, "ip", json_string(ip));
    json_object_set(object, "isCompress", json_string("1"));
    
    // Handle UTF-8 check for log data
    int err_pos;
    if (utf8_check_string_x(m_info.data, -1, &err_pos)) {
        json_object_set(object, "log", json_string(m_info.data));
    } else {
        json_object_set(object, "log", json_string_nocheck(m_info.data));
    }
    
    char *post_str = json_dumps(object, 0x200);
    json_delete(object);
    
    bool upload_succ = false;
    if (post_str) {
        char *resp = nullptr;
        
        // Determine URL based on test type
        if (strcmp(m_test_info.test_type, "PT2") == 0 || 
            strcmp(m_test_info.test_type, "SWEEP") == 0 || 
            strcmp(m_test_info.test_type, "PT3") == 0) {
            resp = post_request(const_cast<char*>("https://mtcs.bitmain.vip:443/public/api/mtc/uploadPtTwo.json"), post_str);
        } else if (strcmp(m_test_info.test_type, "PT1new") == 0) {
            resp = post_request(const_cast<char*>("https://mtcs.bitmain.vip:443/public/api/mtc/uploadPtOne.json"), post_str);
        }
        
        if (resp) {
            upload_succ = true;
            free(resp);
        }
        free(post_str);
    }
    
    free(m_info.data);
    return upload_succ;
}

/**
 * @brief Сигнал для потока загрузки.
 */
void test_singal(void) {
    pthread_cond_signal(&upload_cond);
}

/**
 * @brief Завершение текущего лога.
 */
void submit_cur_log(void) {
    pthread_mutex_lock(&file_mutex);
    if (g_log_file != nullptr) {
        fflush(g_log_file);
        fclose(g_log_file);
        g_log_file = nullptr;
    }
    pthread_mutex_unlock(&file_mutex);
    
    pthread_mutex_lock(&upload_mutex);
    memset(g_cur_log_path, 0, 0x80);
    pthread_mutex_unlock(&upload_mutex);
    
    pthread_cond_signal(&upload_cond);
}

/**
 * @brief Поток загрузки логов.
 */
void * upload_logs_func(void *arg) {
    char logs[400][128];
    
    while (true) {
        memset(logs, 0, sizeof(logs));
        int log_cnt = refresh_log_list(logs, const_cast<char*>(LOG_PREFIX_LOCAL));
        
        for (int i = log_cnt - 1; i >= 0; i--) {
            char full_name[128];
            snprintf(full_name, 0x80, "%s%s", LOG_DIR, logs[i]);
            
            pthread_mutex_lock(&upload_mutex);
            bool is_cur_log = (g_cur_log_path[0] != '\0' && strncmp(g_cur_log_path, full_name, 0x80) == 0);
            pthread_mutex_unlock(&upload_mutex);
            
            if (!is_cur_log) {
                if (upload_logs(full_name)) {
                    char uploaded_name[128];
                    snprintf(uploaded_name, 0x80, "%s%s%s", LOG_DIR, LOG_PREFIX_UPLOADED, logs[i]);
                    rename(full_name, uploaded_name);
                    clear_uploaded_logs();
                }
            }
        }
        
        pthread_mutex_lock(&upload_mutex);
        pthread_cond_wait(&upload_cond, &upload_mutex);
        pthread_mutex_unlock(&upload_mutex);
    }
    return nullptr;
}

/**
 * @brief Запуск системы загрузки логов.
 */
void start_upload_logs(void) {
    pthread_t log_thread;
    clear_cache_file();
    pthread_create(&log_thread, nullptr, upload_logs_func, nullptr);
}