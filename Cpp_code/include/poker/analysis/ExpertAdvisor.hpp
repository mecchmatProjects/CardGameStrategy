#pragma once

#include "poker/analysis/EquityCalculator.hpp"
#include "poker/analysis/Recommendation.hpp"
#include "poker/core/Random.hpp"

#include <cstdint>
#include <random>
#include <vector>

namespace poker {

class ExpertAdvisor {
public:
    explicit ExpertAdvisor(std::uint64_t seed = std::random_device{}(), int iterations = 2500,
                           double temperature = 0.15, double openLooseness = 1.0, double pressure = 1.0);

    Recommendation advise(const DecisionContext& ctx);

    std::vector<OpponentRange> opponentRanges(const DecisionContext& ctx) const;

private:
    struct Response {
        double fold = 0.0;
        OpponentRange calling;
    };
    Response respond(const DecisionContext& ctx, const OpponentInfo& opp, const OpponentRange& range,
                     int callAmount) const;
    double realization(const DecisionContext& ctx, bool aggressor) const;
    void preflopOpen(const DecisionContext& ctx, Recommendation& rec);
    void evaluate(const DecisionContext& ctx, Recommendation& rec, const std::vector<OpponentRange>& ranges);

    EquityCalculator calculator_;
    Rng rng_;
    int iterations_;
    double temperature_;
    double openLooseness_;
    double pressure_;
};

}
