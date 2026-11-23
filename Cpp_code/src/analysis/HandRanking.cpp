#include "poker/analysis/HandRanking.hpp"

#include "poker/analysis/EquityCalculator.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>

namespace poker {

namespace {

int classIndex(int highRank, int lowRank, bool suited) {
    const int hi = 14 - highRank;
    const int lo = 14 - lowRank;
    if (hi == lo) return hi * 13 + hi;
    return suited ? hi * 13 + lo : lo * 13 + hi;
}

struct Entry {
    int index;
    double equity;
    int combos;
    int high;
    int low;
    bool suited;
};

struct Ranking {
    std::array<double, 169> percentile{};
    std::vector<Entry> sorted;
    std::vector<int> cumulative;
};

Ranking buildRanking() {
    std::vector<Entry> entries;
    EquityCalculator calculator(20251113);

    for (int high = 14; high >= 2; --high) {
        for (int low = high; low >= 2; --low) {
            for (const bool suited : {true, false}) {
                if (high == low && suited) continue;
                const Card a(static_cast<Rank>(high), Suit::Spades);
                const Card b(static_cast<Rank>(low), suited ? Suit::Spades : Suit::Hearts);
                const double eq = calculator.vsRandom({a, b}, {}, 1, 6000).equity;
                const int combos = high == low ? 6 : (suited ? 4 : 12);
                entries.push_back({classIndex(high, low, suited), eq, combos, high, low, suited});
            }
        }
    }

    std::sort(entries.begin(), entries.end(), [](const Entry& x, const Entry& y) { return x.equity > y.equity; });
    Ranking ranking;
    int cumulative = 0;
    for (const Entry& e : entries) {
        ranking.percentile[static_cast<std::size_t>(e.index)] = (cumulative + e.combos / 2.0) / 1326.0;
        cumulative += e.combos;
        ranking.cumulative.push_back(cumulative);
    }
    ranking.sorted = std::move(entries);
    return ranking;
}

const Ranking& ranking() {
    static const Ranking r = buildRanking();
    return r;
}

}

double handPercentile(const HoleCards& hole) {
    const auto& table = ranking().percentile;
    const int a = hole[0].rankValue();
    const int b = hole[1].rankValue();
    return table[static_cast<std::size_t>(
        classIndex(std::max(a, b), std::min(a, b), hole[0].suit() == hole[1].suit()))];
}

std::optional<HoleCards> sampleFromTop(double topFraction, CardMask dead, Rng& rng) {
    const Ranking& r = ranking();
    const int limit = std::clamp(static_cast<int>(topFraction * 1326.0), 6, 1326);
    const auto it = std::lower_bound(r.cumulative.begin(), r.cumulative.end(), limit);
    const std::size_t classes = static_cast<std::size_t>(std::distance(r.cumulative.begin(), it)) + 1;
    const int total = r.cumulative[std::min(classes, r.cumulative.size()) - 1];

    for (int attempt = 0; attempt < 60; ++attempt) {
        const int pick = rng.uniformInt(1, total);
        const auto cls = std::lower_bound(r.cumulative.begin(), r.cumulative.begin() + static_cast<std::ptrdiff_t>(classes), pick);
        const Entry& e = r.sorted[static_cast<std::size_t>(std::distance(r.cumulative.begin(), cls))];

        int s1 = rng.uniformInt(0, 3);
        int s2 = rng.uniformInt(0, 3);
        if (e.suited) s2 = s1;
        else if (s1 == s2) continue;
        const Card a(static_cast<Rank>(e.high), static_cast<Suit>(s1));
        const Card b(static_cast<Rank>(e.low), static_cast<Suit>(s2));
        if ((dead & a.mask()) || (dead & b.mask())) continue;
        return HoleCards{a, b};
    }
    return std::nullopt;
}

double openingRange(Position position, int playersDealt) {
    double base = 0.0;
    switch (position) {
        case Position::Early: base = 0.12; break;
        case Position::Middle: base = 0.18; break;
        case Position::Late: base = 0.32; break;
        case Position::Blinds: base = 0.26; break;
    }
    const double factor = std::sqrt(6.0 / std::clamp(playersDealt, 2, 10));
    return std::min(0.85, base * factor);
}

bool shouldOpenHand(const HoleCards& hole, Position position, int playersDealt, double looseness) {
    return handPercentile(hole) <= openingRange(position, playersDealt) * looseness;
}

}
