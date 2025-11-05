#ifndef IHASHCALCULATOR_HPP
#define IHASHCALCULATOR_HPP

#include <string>

class IHashCalculator {
public:
    virtual std::string calculateHash(const std::string& filePath) const = 0;
    virtual ~IHashCalculator() = default;
};

#endif // IHASHCALCULATOR_HPP
