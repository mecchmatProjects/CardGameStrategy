#pragma once

#include "poker/analysis/EquityCalculator.hpp"
#include "poker/analysis/Recommendation.hpp"
#include "poker/core/Random.hpp"

#include <cstdint>
#include <random>
#include <string>

namespace poker {

struct AdvisorStyle {
    double openLooseness = 1.0;
    double bluffConfidence = 1.0;
    double temperature = 0.0;
};

class Advisor {
public:
    explicit Advisor(std::uint64_t seed = std::random_device{}(), int iterations = 2000,
                     AdvisorStyle style = {});

    Recommendation advise(const DecisionContext& ctx);

    RangeModel opponentRange(const DecisionContext& ctx) const;

private:
    double foldProbability(const DecisionContext& ctx, int heroInvests, int callAmount) const;
    double realization(const DecisionContext& ctx, bool aggressor) const;
    void evaluateOptions(const DecisionContext& ctx, Recommendation& rec, const RangeModel& range);
    void preflopOpen(const DecisionContext& ctx, Recommendation& rec);
    void choose(const DecisionContext& ctx, Recommendation& rec);

    EquityCalculator calculator_;
    Rng rng_;
    int iterations_;
    AdvisorStyle style_;
};

}
