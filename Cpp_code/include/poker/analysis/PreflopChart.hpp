#pragma once

#include "poker/core/Card.hpp"
#include "poker/game/Types.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace poker {

enum class PreflopTier : std::uint8_t { Premium, Strong, Playable, Speculative, Trash };

std::string tierName(PreflopTier tier);

double chenScore(Rank first, Rank second, bool suited);
double chenScore(const HoleCards& hole);

PreflopTier tierFromScore(double chen);
PreflopTier preflopTier(const HoleCards& hole);

std::string handCode(const HoleCards& hole);

bool shouldOpen(PreflopTier tier, Position position);

struct ChartCell {
    std::string code;
    double chen = 0.0;
    PreflopTier tier = PreflopTier::Trash;
};

using PreflopChart = std::array<std::array<ChartCell, kNumRanks>, kNumRanks>;
PreflopChart buildPreflopChart();

}
