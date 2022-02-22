
#ifndef _TYPE_H_INCLUDED_
#define _TYPE_H_INCLUDED_

#include <atomic>
#include <cstdint>
#include <limits>
#include <type_traits>

template<typename T, typename U>
static inline T sign_extend(U x) {
    static_assert(std::is_unsigned<T>::value && std::is_unsigned<U>::value,
                  "sign_extend expects unsigned integral types");
    typename std::make_signed<U>::type y = x;
    typename std::make_signed<T>::type sy = y;
    return (T)sy;
}

template<typename T, typename U>
static inline T zero_extend(U x) {
    static_assert(std::is_unsigned<T>::value && std::is_unsigned<U>::value,
                  "zero_extend expects unsigned integral types");
    return (T)x;
}

template<typename T, typename U>
static inline T clamp(U x) {
    static_assert(std::numeric_limits<T>::max() <=
                      std::numeric_limits<U>::max() &&
                  std::numeric_limits<T>::min() >=
                      std::numeric_limits<U>::min(),
                  "clamp expects that the range of values of the return type"
                  " be included in the range of values of the input type");

    return x > std::numeric_limits<T>::max() ? std::numeric_limits<T>::max() :
           x < std::numeric_limits<T>::min() ? std::numeric_limits<T>::min() :
           (T)x;
}

template<typename T>
static inline T read_be(uint8_t const *bytes, unsigned nr_bytes = sizeof(T)) {
    static_assert(std::is_unsigned<T>::value,
                  "read_be expects an unsigned integral type");
    T val = 0;
    for (unsigned n = 0; n < nr_bytes; n++) {
        val = (val << 8) | (T)bytes[n];
    }
    return val;
}

template<typename T>
static inline void write_be(uint8_t *bytes, T val, unsigned nr_bytes = sizeof(T)) {
    static_assert(std::is_unsigned<T>::value,
                  "read_be expects an unsigned integral type");
    for (unsigned n = 0; n < nr_bytes; n++) {
        bytes[n] = val >> (8 * (nr_bytes - n - 1));
    }
}

#endif /* _TYPE_H_INCLUDED_ */
