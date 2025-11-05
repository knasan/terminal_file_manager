#ifndef FNV1A_HPP
#define FNV1A_HPP

#include "ihashcalculator.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdint>

/**
 * @brief Implementation of FNV-1a (Fowler-Noll-Vo) hash algorithm
 * 
 * FNV-1a is a non-cryptographic hash function designed for fast hash table lookup.
 * This implementation uses the 64-bit version of the algorithm with:
 * - FNV prime: 2^40 + 2^8 + 0xb3 (1099511628211)
 * - FNV offset basis: 14695981039346656037
 * 
 * @note Inherits from IHashCalculator interface
 * 
 * @see http://www.isthe.com/chongo/tech/comp/fnv/
 */
class FNV1A : public IHashCalculator {
public:
  std::string calculateHash(const std::string &filePath) const override {
    const uint64_t FNV_prime = 1099511628211u;
    uint64_t hash = 1469598103934665603u;

    std::ifstream file(filePath, std::ios::binary);
    
    if (!file)
      return "";

    char c;
    while (file.get(c)) {
      hash ^= static_cast<unsigned char>(c);
      hash *= FNV_prime;
    }
    
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setw(16) << std::setfill('0') << hash;

    return ss.str();
  }
};

#endif // FNV1A_HPP