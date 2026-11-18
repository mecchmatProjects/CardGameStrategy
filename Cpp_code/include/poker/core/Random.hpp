#pragma once

#include <cstdint>
#include <random>

namespace poker {

/// Обгортка над генератором випадкових чисел.
/// Один і той самий seed дає однакову послідовність — зручно для тестів і відтворення роздач.
class Rng {
public:
    explicit Rng(std::uint64_t seed = std::random_device{}()) : engine_(seed) {}

    /// Ціле число в діапазоні [lo, hi] включно.
    int uniformInt(int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(engine_); }

    /// Дійсне число в діапазоні [0, 1).
    double uniform01() { return std::uniform_real_distribution<double>(0.0, 1.0)(engine_); }

    /// true з імовірністю p.
    bool chance(double p) { return uniform01() < p; }

    /// Нове 64-бітне число (наприклад, seed для іншого генератора).
    std::uint64_t next() { return engine_(); }

    std::mt19937_64& engine() { return engine_; }

private:
    std::mt19937_64 engine_;
};

}  // namespace poker
