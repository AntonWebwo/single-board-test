#ifndef LZ4FRAME_H
#define LZ4FRAME_H

#include <cstdint>
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

// --- Типы данных и Константы (на основе логики использования в bitmain_log.c и псевдокода) ---

// Прозрачная структура контекста LZ4F
typedef struct LZ4F_cctx_s LZ4F_cctx;
typedef struct LZ4F_dctx_s LZ4F_dctx;

// Типы ошибок и коды возврата
typedef enum {
    LZ4F_OK_NoError = 0,
    LZ4F_ERROR_GENERIC = -1,
    LZ4F_ERROR_maxBlockSize_invalid = -2,
    LZ4F_ERROR_blockMode_invalid = -3,
    LZ4F_ERROR_contentChecksumFlag_invalid = -4,
    LZ4F_ERROR_compressionLevel_invalid = -5,
    LZ4F_ERROR_headerVersion_wrong = -6,
    LZ4F_ERROR_blockSize_tooLarge = -7,
    LZ4F_ERROR_maxHeaderSize_wrong = -8,
    LZ4F_ERROR_allocation_failed = -9,
    LZ4F_ERROR_srcPtr_wrong = -10,
    LZ4F_ERROR_dstPtr_wrong = -11,
    LZ4F_ERROR_state_busy = -12,
    LZ4F_ERROR_state_idle = -13,
    LZ4F_ERROR_frameType_unknown = -14,
    LZ4F_ERROR_frameSize_wrong = -15,
    LZ4F_ERROR_srcSize_wrong = -16,
    LZ4F_ERROR_decompressionFailed = -17,
    LZ4F_ERROR_headerChecksum_invalid = -18,
    LZ4F_ERROR_contentChecksum_invalid = -19,
    LZ4F_ERROR_frameDecoding_alreadyStarted = -20,
    LZ4F_ERROR_maxCode = -21
} LZ4F_errorCode_t;

// Режимы блоков
typedef enum {
    LZ4F_blockLinked = 0,
    LZ4F_blockIndependent
} LZ4F_blockMode_t;

// Проверка контента
typedef enum {
    LZ4F_noContentChecksum = 0,
    LZ4F_contentChecksumEnabled
} LZ4F_contentChecksum_t;

// Размер блока
typedef enum {
    LZ4F_max64KB = 4,
    LZ4F_max256KB = 5,
    LZ4F_max1MB = 6,
    LZ4F_max4MB = 7
} LZ4F_blockSizeID_t;

// Тип кадра
typedef enum {
    LZ4F_frame = 0,
    LZ4F_skippableFrame
} LZ4F_frameType_t;

// Предпочтения сжатия
typedef enum {
    LZ4F_fast = 0,
    LZ4F_highCompression
} LZ4F_compressionLevel_t;

// Структура предпочтений
typedef struct {
    LZ4F_blockSizeID_t     blockSizeID;         /* max64KB, max256KB, max1MB, max4MB ; 7 == default */
    LZ4F_blockMode_t       blockMode;           /* LZ4F_blockLinked, LZ4F_blockIndependent ; linked == default */
    LZ4F_contentChecksum_t contentChecksumFlag; /* noContentChecksum, contentChecksumEnabled ; none == default */
    LZ4F_frameType_t       frameType;           /* LZ4F_frame, skippableFrame ; LZ4F_frame == default */
    unsigned long long     contentSize;         /* Size of uncompressed content ; 0 == unknown */
    unsigned               dictID;              /* Dictionary ID, sent by compressor to help decoder select correct dictionary ; 0 == no dictID provided */
    LZ4F_compressionLevel_t compressionLevel;   /* 0 == default (fast mode); values > LZ4HC_CLEVEL_MAX count as LZ4HC_CLEVEL_MAX; values < 0 trigger "fast acceleration" */
} LZ4F_preferences_t;

// Структура информации о кадре
typedef struct {
    LZ4F_blockSizeID_t     blockSizeID;         /* max64KB, max256KB, max1MB, max4MB */
    LZ4F_blockMode_t       blockMode;           /* LZ4F_blockLinked, LZ4F_blockIndependent */
    LZ4F_contentChecksum_t contentChecksumFlag; /* noContentChecksum, contentChecksumEnabled */
    LZ4F_frameType_t       frameType;           /* LZ4F_frame, skippableFrame */
    unsigned long long     contentSize;         /* Size of uncompressed content */
    unsigned               dictID;              /* Dictionary ID */
} LZ4F_frameInfo_t;

// --- Внешние функции (заглушки/объявления) ---

/**
 * @brief Получение строки с описанием ошибки.
 */
const char* LZ4F_getErrorName(LZ4F_errorCode_t code);

/**
 * @brief Создание контекста компрессии.
 */
LZ4F_cctx* LZ4F_createCompressionContext(void);

/**
 * @brief Освобождение контекста компрессии.
 */
unsigned LZ4F_freeCompressionContext(LZ4F_cctx* cctx);

/**
 * @begin_header
 * @brief Начало сжатия кадра. Записывает заголовок в dstBuffer.
 */
size_t LZ4F_compressBegin(LZ4F_cctx* cctx, void* dstBuffer, size_t dstCapacity, const LZ4F_preferences_t* prefsPtr);

/**
 * @brief Сжатие блока данных.
 */
size_t LZ4F_compressUpdate(LZ4F_cctx* cctx, void* dstBuffer, size_t dstCapacity, const void* srcBuffer, size_t srcSize, const LZ4F_compressOptions_t* compressOptionsPtr);

/**
 * @brief Завершение сжатия кадра. Записывает завершающие байты.
 */
size_t LZ4F_compressEnd(LZ4F_cctx* cctx, void* dstBuffer, size_t dstCapacity, const LZ4F_compressOptions_t* compressOptionsPtr);

/**
 * @brief Создание контекста декомпрессии.
 */
LZ4F_dctx* LZ4F_createDecompressionContext(void);

/**
 * @brief Освобождение контекста декомпрессии.
 */
unsigned LZ4F_freeDecompressionContext(LZ4F_dctx* dctx);

/**
 * @brief Декомпрессия кадра.
 */
size_t LZ4F_decompress(LZ4F_dctx* dctx, void* dstBuffer, size_t* dstSizePtr, const void* srcBuffer, size_t* srcSizePtr, const LZ4F_decompressOptions_t* decompressOptionsPtr);

/**
 * @brief Получение информации о кадре из заголовка.
 */
LZ4F_errorCode_t LZ4F_getFrameInfo(LZ4F_dctx* dctx, LZ4F_frameInfo_t* frameInfoPtr, const void* srcBuffer, size_t* srcSizePtr);

#ifdef __cplusplus
}
#endif

#endif // LZ4FRAME_H