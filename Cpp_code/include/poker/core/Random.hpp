#pragma once

#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>
#pragma once

#include <cstdint>
#include <random>

namespace poker {

    class Rng {
    public:
        explicit Rng(std::uint64_t seed = std::random_device{}()) : engine_(seed) {}

        int uniformInt(int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(engine_); }

        double uniform01() { return std::uniform_real_distribution<double>(0.0, 1.0)(engine_); }

        bool chance(double p) { return uniform01() < p; }

        std::uint64_t next() { return engine_(); }

        std::mt19937_64& engine() { return engine_; }

    private:
        std::mt19937_64 engine_;
    };

}
