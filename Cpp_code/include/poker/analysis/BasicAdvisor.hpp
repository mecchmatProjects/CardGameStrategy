#pragma once

#include "poker/analysis/EquityCalculator.hpp"
#include "poker/analysis/Recommendation.hpp"

#include <cstdint>
#include <random>
#include <string>

namespace poker {

class BasicAdvisor {
public:
    explicit BasicAdvisor(std::uint64_t seed = std::random_device{}(), int iterations = 2000);

    Recommendation advise(const DecisionContext& ctx);

    double estimateEquity(const DecisionContext& ctx);

private:
    void preflop(const DecisionContext& ctx, Recommendation& rec) const;
    void postflop(const DecisionContext& ctx, Recommendation& rec) const;

    EquityCalculator calculator_;
    int iterations_;
};

}
