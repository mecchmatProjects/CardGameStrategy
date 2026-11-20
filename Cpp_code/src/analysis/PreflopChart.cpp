#include "poker/analysis/PreflopChart.hpp"

#include <algorithm>
#include <cmath>

namespace poker {

namespace {

double cardPoints(int rank) {
    switch (rank) {
        case 14: return 10.0;
        case 13: return 8.0;
        case 12: return 7.0;
        case 11: return 6.0;
        default: return rank / 2.0;
    }
}

}

std::string tierName(PreflopTier tier) {
    switch (tier) {
        case PreflopTier::Premium: return "Преміум";
        case PreflopTier::Strong: return "Сильна";
        case PreflopTier::Playable: return "Іграбельна";
        case PreflopTier::Speculative: return "Спекулятивна";
        case PreflopTier::Trash: return "Слабка";
    }
    return "?";
}

double chenScore(Rank first, Rank second, bool suited) {
    const int high = std::max(static_cast<int>(first), static_cast<int>(second));
    const int low = std::min(static_cast<int>(first), static_cast<int>(second));

    double score = 0.0;
    if (high == low) {
        score = std::max(5.0, cardPoints(high) * 2.0);
    } else {
        score = cardPoints(high);
        if (suited) score += 2.0;
        const int gap = high - low - 1;
        if (gap == 1) score -= 1.0;
        else if (gap == 2) score -= 2.0;
        else if (gap == 3) score -= 4.0;
        else if (gap >= 4) score -= 5.0;
        if (gap <= 1 && high < 12) score += 1.0;
    }
    return std::ceil(score);
}

double chenScore(const HoleCards& hole) {
    return chenScore(hole[0].rank(), hole[1].rank(), hole[0].suit() == hole[1].suit());
}

PreflopTier tierFromScore(double chen) {
    if (chen >= 12) return PreflopTier::Premium;
    if (chen >= 10) return PreflopTier::Strong;
    if (chen >= 8) return PreflopTier::Playable;
    if (chen >= 5) return PreflopTier::Speculative;
    return PreflopTier::Trash;
}

PreflopTier preflopTier(const HoleCards& hole) { return tierFromScore(chenScore(hole)); }

std::string handCode(const HoleCards& hole) {
    Card high = hole[0];
    Card low = hole[1];
    if (low.rankValue() > high.rankValue()) std::swap(high, low);
    std::string code{rankToChar(high.rank()), rankToChar(low.rank())};
    if (high.rank() != low.rank()) code += high.suit() == low.suit() ? 's' : 'o';
    return code;
}

bool shouldOpen(PreflopTier tier, Position position) {
    switch (position) {
        case Position::Early: return tier <= PreflopTier::Strong;
        case Position::Middle: return tier <= PreflopTier::Playable;
        case Position::Late: return tier <= PreflopTier::Speculative;
        case Position::Blinds: return tier <= PreflopTier::Playable;
    }
    return false;
}

PreflopChart buildPreflopChart() {
    PreflopChart chart{};
    for (int row = 0; row < kNumRanks; ++row) {
        for (int col = 0; col < kNumRanks; ++col) {
            const auto rowRank = static_cast<Rank>(14 - row);
            const auto colRank = static_cast<Rank>(14 - col);
            const bool suited = row < col;
            ChartCell& cell = chart[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
            const Rank high = row <= col ? rowRank : colRank;
            const Rank low = row <= col ? colRank : rowRank;
            cell.code = std::string{rankToChar(high), rankToChar(low)};
            if (row != col) cell.code += suited ? 's' : 'o';
            cell.chen = chenScore(high, low, suited);
            cell.tier = tierFromScore(cell.chen);
        }
    }
    return chart;
}

}
