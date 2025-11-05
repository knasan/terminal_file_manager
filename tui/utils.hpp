#ifndef UTILS_HPP
#define UTILS_HPP

#include <vector>
#include <optional>
#include <functional>
#include <cstddef> // size_t

template <typename T>
/*inline const T* safe_at(const std::vector<T>& vec, int index) {
    if (index < 0 || static_cast<size_t>(index) >= vec.size())
        return nullptr;
    return &vec[static_cast<size_t>(index)];
}*/

const T* safe_at(const std::vector<T>& vec, int index) {
    if (index < 0 || static_cast<size_t>(index) >= vec.size())
        return nullptr;
    return &vec[static_cast<size_t>(index)];
}

#endif // UTILS_HPP