#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstddef> // size_t
#include <functional>
#include <optional>
#include <vector>

template <typename T>

const T *safe_at(const std::vector<T> &vec, int index) {
  if (index < 0 || static_cast<size_t>(index) >= vec.size())
    return nullptr;
  return &vec[static_cast<size_t>(index)];
}

/**
 * @brief Format bytes to human-readable string
 */
static std::string formatBytes(long long bytes) {
  if (bytes == 0)
    return "0 B";

  const char *units[] = {"B", "KB", "MB", "GB", "TB"};
  int unit = 0;
  double size = static_cast<double>(bytes);

  while (size >= 1024.0 && unit < 4) {
    size /= 1024.0;
    unit++;
  }

  char buf[32];
  snprintf(buf, sizeof(buf), "%.1f %s", size, units[unit]);
  return std::string(buf);
}

#endif // UTILS_HPP