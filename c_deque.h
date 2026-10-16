#ifndef C_DEQUE_H
#define C_DEQUE_H

#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>

// --- Типы данных ---

// Функтор для сравнения элементов (аналог clib_compare)
typedef std::function<int(const void*, const void*)> clib_compare_fn;

// Функтор для уничтожения/очистки элементов (аналог clib_destroy)
typedef std::function<void(void*)> clib_destroy_fn;

/**
 * @brief Структура, эмулирующая c_deque из оригинальной библиотеки.
 * Использует std::deque для хранения элементов.
 */
template <typename T = void*>
struct CDeque {
    std::deque<T> elements;
    int no_max_elements;      // Максимальное количество элементов (0 = без ограничений)
    int head;                 // Индекс головы (в std::deque не используется напрямую, но сохранен для совместимости логики)
    int tail;                 // Индекс хвоста
    int no_of_elements;       // Текущее количество элементов
    
    clib_compare_fn compare_fn;   // Функция сравнения
    clib_destroy_fn destruct_fn;  // Функция очистки памяти элемента

    // Мьютекс для потокобезопасности (если требуется, как в других модулях)
    mutable std::mutex mutex;

    /**
     * @brief Конструктор по умолчанию.
     */
    CDeque() : no_max_elements(0), head(0), tail(0), no_of_elements(0), compare_fn(nullptr), destruct_fn(nullptr) {}

    /**
     * @brief Конструктор с настройками.
     * @param max_elems Максимальное количество элементов.
     * @param cmp_fn Функция сравнения.
     * @param dest_fn Функция очистки.
     */
    CDeque(int max_elems, clib_compare_fn cmp_fn, clib_destroy_fn dest_fn)
        : no_max_elements(max_elems), head(0), tail(0), no_of_elements(0), 
          compare_fn(cmp_fn), destruct_fn(dest_fn) {}

    /**
     * @brief Добавление элемента в конец дека.
     * Если достигнут лимит no_max_elements, удаляет элемент с начала.
     */
    int push_back(T element) {
        std::lock_guard<std::mutex> lock(mutex);
        
        if (no_max_elements > 0 && no_of_elements >= no_max_elements) {
            // Если есть функция очистки, вызываем её для удаляемого элемента
            if (destruct_fn && !elements.empty()) {
                destruct_fn(elements.front());
            }
            elements.pop_front();
            no_of_elements--;
        }
        
        elements.push_back(element);
        no_of_elements++;
        tail++;
        
        return 0;
    }

    /**
     * @brief Добавление элемента в начало дека.
     */
    int push_front(T element) {
        std::lock_guard<std::mutex> lock(mutex);

        if (no_max_elements > 0 && no_of_elements >= no_max_elements) {
            if (destruct_fn && !elements.empty()) {
                destruct_fn(elements.back());
            }
            elements.pop_back();
            no_of_elements--;
        }

        elements.push_front(element);
        no_of_elements++;
        head--;
        
        return 0;
    }

    /**
     * @brief Извлечение элемента с начала дека.
     */
    T pop_front() {
        std::lock_guard<std::mutex> lock(mutex);
        
        if (elements.empty()) {
            return nullptr;
        }
        
        T elem = elements.front();
        elements.pop_front();
        no_of_elements--;
        head++;
        
        return elem;
    }

    /**
     * @brief Извлечение элемента с конца дека.
     */
    T pop_back() {
        std::lock_guard<std::mutex> lock(mutex);
        
        if (elements.empty()) {
            return nullptr;
        }
        
        T elem = elements.back();
        elements.pop_back();
        no_of_elements--;
        tail--;
        
        return elem;
    }

    /**
     * @brief Получение текущего количества элементов.
     */
    int size() const {
        std::lock_guard<std::mutex> lock(mutex);
        return no_of_elements;
    }

    /**
     * @brief Проверка на пустоту.
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex);
        return elements.empty();
    }

    /**
     * @brief Очистка всего дека с вызовом destruct_fn для каждого элемента.
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        
        if (destruct_fn) {
            for (auto& elem : elements) {
                destruct_fn(elem);
            }
        }
        
        elements.clear();
        no_of_elements = 0;
        head = 0;
        tail = 0;
    }
    
    /**
     * @brief Доступ к элементу по индексу (без проверки границ).
     */
    T at(int index) {
        std::lock_guard<std::mutex> lock(mutex);
        if (index >= 0 && index < static_cast<int>(elements.size())) {
            return elements[index];
        }
        return nullptr;
    }
};

#endif // C_DEQUE_H