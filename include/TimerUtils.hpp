#ifndef TIMER_UTILS_HPP
#define TIMER_UTILS_HPP

#include <chrono>
#include <atomic>

/**
 * Отримує поточний час із використанням бар'єрів пам'яті.
 * Це гарантує, що вимірювання часу не "запливе" всередину або зовні коду, що тестується.
 */
inline std::chrono::high_resolution_clock::time_point get_current_time_fenced()
{
    std::atomic_thread_fence(std::memory_order_seq_cst); // Бар'єр "до"
    auto res_time = std::chrono::high_resolution_clock::now();
    std::atomic_thread_fence(std::memory_order_seq_cst); // Бар'єр "після"
    return res_time;
}

/**
 * Перетворює тривалість у мікросекунди.
 */
template<class D>
inline long long to_us(const D& d)
{
    return std::chrono::duration_cast<std::chrono::microseconds>(d).count();
}

#endif