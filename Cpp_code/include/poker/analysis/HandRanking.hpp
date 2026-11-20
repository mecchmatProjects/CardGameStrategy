#pragma once

#include "poker/core/Card.hpp"
#include "poker/core/Random.hpp"
#include "poker/game/Types.hpp"

#include <optional>

namespace poker {

double handPercentile(const HoleCards& hole);

std::optional<HoleCards> sampleFromTop(double topFraction, CardMask dead, Rng& rng);

double openingRange(Position position, int playersDealt);

bool shouldOpenHand(const HoleCards& hole, Position position, int playersDealt, double looseness = 1.0);

}
