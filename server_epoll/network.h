//
// Created by nikita on 12/27/19.
//

#ifndef SERVER_NETWORK_H
#define SERVER_NETWORK_H

#include <type_traits>
#include <cstddef>
// TODO tests

template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
T to_network_byteorder(T number) {
    T res = 0;
    for (size_t i = 0; i < sizeof(T); ++i) {
        size_t shift = i * 8;
        res |= ((res & (0xFF << shift)) >> (shift)) << 8 * (sizeof(T) - i - 1);
    }
    return res;
}

#endif //SERVER_NETWORK_H
