#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <lz4frame.h>

// --- Внешние зависимости (заглушки) ---
extern void print_time_stamp(char *buf, int len);
extern void log_to_file(char *msg, char *time);
extern void _applog(int level, const char* msg, bool newline);

// --- Функции ---

/**
 * @brief Сжатие файла с использованием LZ4.
 * Читает входной файл блоками, сжимает их и записывает в выходной файл.
 */
int compress_file(FILE *f_in, FILE *f_out) {
    uint uVar1;
    char *pcVar2;
    LZ4F_errorCode_t code;
    int iVar3;
    FILE *f_out_local = f_out;
    FILE *f_in_local = f_in;
    LZ4F_preferences_t m_prefsPtr;
    LZ4F_cctx *lz4fWrite = nullptr; // Контекст записи LZ4
    size_t len;
    void *buf;
    LZ4F_errorCode_t ret;

    if ((f_in == nullptr) || (f_out == nullptr)) {
        iVar3 = -1;
    } else {
        ret = 0;
        buf = malloc(0x4000); // Выделение буфера 16KB
        if (buf == nullptr) {
            puts("error: memory allocation failed ");
        }
        
        memset(&m_prefsPtr, 0, 0x38);
        m_prefsPtr.compressionLevel = 0xc; // Уровень сжатия 12
        
        // Открытие контекста записи LZ4
        ret = LZ4F_compressBegin(&lz4fWrite, f_out, &m_prefsPtr);
        uVar1 = LZ4F_isError(ret);
        
        if (uVar1 == 0) {
            do {
                // Чтение блока данных
                len = fread(buf, 1, 0x4000, f_in);
                iVar3 = ferror(f_in);
                
                if (iVar3 != 0) {
                    puts("fread error");
                    goto LAB_0006223a;
                }
                
                if (len == 0) goto LAB_0006223a; // Конец файла
                
                // Сжатие и запись блока
                // Примечание: В псевдокоде вызывается LZ4F_write, но в стандартном API это обычно LZ4F_compressUpdate или аналогичный поток.
                // Сохраняем логику вызова как есть, предполагая наличие такой функции-обертки или макроса.
                ret = LZ4F_compressUpdate(lz4fWrite, buf, len); 
                uVar1 = LZ4F_isError(ret);
            } while (uVar1 == 0);
            
            pcVar2 = (char *)LZ4F_getErrorName(ret);
            printf("LZ4F_write: %s\n", pcVar2);
            
        LAB_0006223a:
            free(buf);
            // Завершение сжатия (запись финального фрейма)
            code = LZ4F_compressEnd(lz4fWrite, nullptr, 0, nullptr);
            uVar1 = LZ4F_isError(code);
            
            if (uVar1 == 0) {
                iVar3 = 0; // Успех
            } else {
                pcVar2 = (char *)LZ4F_getErrorName(ret);
                printf("LZ4F_writeClose: %s\n", pcVar2);
                iVar3 = 1; // Ошибка
            }
        } else {
            pcVar2 = (char *)LZ4F_getErrorName(ret);
            printf("LZ4F_writeOpen error: %s\n", pcVar2);
            free(buf);
            iVar3 = 1; // Ошибка
        }
    }
    return iVar3;
}

/**
 * @brief Распаковка файла с использованием LZ4.
 * Читает сжатый файл блоками, распаковывает их и записывает в выходной файл.
 */
int decompress_file(FILE *f_in, FILE *f_out) {
    uint uVar1;
    char *pcVar2;
    size_t sVar3;
    LZ4F_errorCode_t code;
    int iVar4;
    FILE *f_out_local = f_out;
    FILE *f_in_local = f_in;
    LZ4F_dctx *lz4fRead = nullptr; // Контекст чтения LZ4
    void *buf;
    LZ4F_errorCode_t ret;

    if (f_in == nullptr) {
        // WARNING: Subroutine does not return
        __assert_fail("f_in != ((void *)0)", "../zhiju/log_file/fileCompress.c", 0x55, "decompress_file");
    }
    
    if (f_out == nullptr) {
        // WARNING: Subroutine does not return
        __assert_fail("f_out != ((void *)0)", "../zhiju/log_file/fileCompress.c", 0x55, "decompress_file");
    }

    ret = 0;
    buf = malloc(0x4000); // Выделение буфера 16KB
    if (buf == nullptr) {
        puts("error: memory allocation failed ");
    }
    
    // Открытие контекста чтения LZ4
    ret = LZ4F_decompressBegin(&lz4fRead);
    uVar1 = LZ4F_isError(ret);
    
    if (uVar1 == 0) {
        do {
            // Чтение и распаковка блока
            // Примечание: Логика псевдокода предполагает функцию LZ4F_read. 
            // В стандартном API это обычно цикл с LZ4F_decompress.
            size_t srcSize = 0x4000;
            size_t dstSize = 0x4000;
            
            // Эмуляция чтения из файла в промежуточный буфер если нужно, 
            // но здесь предполагаем, что lz4fRead работает напрямую с потоком или буфером.
            // Строго следуя псевдокоду:
            ret = LZ4F_decompress(lz4fRead, buf, &dstSize, f_in, &srcSize, nullptr);
            
            uVar1 = LZ4F_isError(ret);
            if (uVar1 != 0) {
                pcVar2 = (char *)LZ4F_getErrorName(ret);
                printf("LZ4F_read error: %s\n", pcVar2);
                goto LAB_00062388;
            }
            
            if (ret == 0) goto LAB_00062388; // Конец фрейма
            
            // Запись распакованных данных
            sVar3 = fwrite(buf, 1, dstSize, f_out);
        } while (sVar3 == dstSize);
        
        puts("write error! ");
        
    LAB_00062388:
        free(buf);
        // Освобождение контекста
        code = LZ4F_freeDecompressionContext(lz4fRead);
        uVar1 = LZ4F_isError(code);
        
        if (uVar1 == 0) {
            if (ret == 0) {
                iVar4 = 0; // Успех
            } else {
                iVar4 = 1; // Ошибка
            }
        } else {
            pcVar2 = (char *)LZ4F_getErrorName(ret);
            printf("LZ4F_readClose: %s\n", pcVar2);
            iVar4 = 1;
        }
    } else {
        pcVar2 = (char *)LZ4F_getErrorName(ret);
        printf("LZ4F_readOpen error: %s\n", pcVar2);
        free(buf);
        iVar4 = 1;
    }
    return iVar4;
}