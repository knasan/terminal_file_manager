/**
 * @file utils.hpp
 * @brief Utility functions and helpers for the terminal file manager
 *
 * This header provides common utility functions used throughout the file
 * manager application, including safe array access and byte formatting.
 *
 * Key utilities:
 * - safe_at: Bounds-checked vector element access
 * - formatBytes: Human-readable file size formatting
 *
 * @see safe_at()
 * @see formatBytes()
 */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstddef> // size_t
#include <functional>
#include <optional>
#include <vector>

/**
 * @brief Safely accesses a vector element with bounds checking
 *
 * Provides safe access to vector elements by checking bounds before returning
 * a pointer to the element. Returns nullptr if the index is out of bounds,
 * preventing undefined behavior from invalid array access.
 *
 * This is particularly useful in UI code where indices may come from user
 * input or external sources and need validation.
 *
 * @tparam T The type of elements stored in the vector
 * @param vec The vector to access
 * @param index The index to access (can be negative or out of bounds)
 *
 * @return const T* Pointer to the element at the specified index, or nullptr
 *         if the index is out of bounds (negative or >= vec.size())
 *
 * @note Returns a const pointer to prevent modification of the vector element
 * @note This is a template function that works with any vector type
 *
 * Example usage:
 * @code
 * std::vector<FileInfo> files = {...};
 * const FileInfo* file = safe_at(files, selected_index);
 * if (file) {
 *     // Safe to use file pointer
 *     std::cout << file->getPath();
 * }
 * @endcode
 */
template <typename T>
const T *safe_at(const std::vector<T> &vec, int index) {
  if (index < 0 || static_cast<size_t>(index) >= vec.size())
    return nullptr;
  return &vec[static_cast<size_t>(index)];
}

/**
 * @brief Formats byte count into human-readable size string
 *
 * Converts a byte count into a human-readable string with appropriate units
 * and one decimal place of precision. Automatically selects the most appropriate
 * unit (B, KB, MB, GB, TB) based on the size.
 *
 * The function uses binary units (1024 bytes = 1 KB) rather than decimal units.
 *
 * Formatting rules:
 * - 0 bytes: Returns "0 B"
 * - < 1024 bytes: Returns "X.X B"
 * - < 1 MB: Returns "X.X KB"
 * - < 1 GB: Returns "X.X MB"
 * - < 1 TB: Returns "X.X GB"
 * - >= 1 TB: Returns "X.X TB"
 *
 * @param bytes The number of bytes to format (can be any non-negative value)
 *
 * @return std::string Human-readable formatted size string with one decimal
 *         place and appropriate unit (e.g., "1.5 KB", "3.2 MB")
 *
 * @note Uses binary (1024-based) units, not decimal (1000-based) units
 * @note Always displays one decimal place for consistency
 * @note Maximum unit is TB (terabytes)
 *
 * Example outputs:
 * - formatBytes(0) → "0 B"
 * - formatBytes(512) → "512.0 B"
 * - formatBytes(1024) → "1.0 KB"
 * - formatBytes(1536) → "1.5 KB"
 * - formatBytes(1048576) → "1.0 MB"
 * - formatBytes(1073741824) → "1.0 GB"
 *
 * @see FileInfo::getFileSize()
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