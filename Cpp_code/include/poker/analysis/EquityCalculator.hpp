#pragma once

#include "poker/core/Card.hpp"
#include "poker/core/Random.hpp"

#include <cstdint>
#include <random>
#include <vector>

namespace poker {

struct EquityResult {
    double win = 0.0;
    double tie = 0.0;
    double equity = 0.0;
};

struct EquityReport {
    std::vector<EquityResult> players;
    long long trials = 0;
    bool exact = false;
};

struct RangeModel {
    double top = 1.0;
    bool showsStrength = false;
    double bluffShare = 0.35;
};

struct OpponentRange {
    double top = 1.0;
    int bets = 0;
    int raises = 0;
    int calls = 0;
    int bigCalls = 0;
    bool checkedThisStreet = false;
    double bluffShare = 0.35;
    double aggressive = 0.0;
};

class EquityCalculator {
public:
    explicit EquityCalculator(std::uint64_t seed = std::random_device{}());

    EquityReport calculate(const std::vector<HoleCards>& hands, const std::vector<Card>& board,
                           int iterations = 50000, long long exactLimit = 2'000'000);

    EquityResult vsRandom(const HoleCards& hero, const std::vector<Card>& board, int opponents,
                          int iterations = 3000);

    EquityResult vsRange(const HoleCards& hero, const std::vector<Card>& board, int opponents,
                         const RangeModel& range, int iterations = 3000);

    EquityResult vsOpponents(const HoleCards& hero, const std::vector<Card>& board,
                             const std::vector<OpponentRange>& opponents, int iterations = 3000);

private:
    Rng rng_;
};

long long combinations(int n, int k);

}
