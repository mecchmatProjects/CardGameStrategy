#pragma once

#include "poker/cards/Decks.hpp"
#include "poker/cards/PokerTasks.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace poker::cards {

using SuitMasks = std::array<std::uint8_t, 4>;

SuitMasks toMasks(const int* cards, int count);
std::string describeHand(const int* cards, int count);

bool strongTrump(std::uint8_t mask);
bool topCards(std::uint8_t mask);
bool guaranteesAllTricks(const SuitMasks& hand);

bool misereSafeSuit(std::uint8_t mask);
bool misereSafe(const SuitMasks& hand);

bool withTalon(const int* hand10, const int* talon2, bool (*check)(const SuitMasks&));

struct PreferansResult {
    Estimate tenTricksNoTalon;
    Estimate tenTricksWithTalon;
    Estimate misereNoTalon;
    Estimate misereWithTalon;
};

PreferansResult preferansLayouts(Decks& decks, long long N, int batch = 50000);

struct TrumpSplitResult {
    long long trials = 0;
    int remainingTrumps = 0;
    std::vector<long long> counts;
    std::vector<double> exact;
};

TrumpSplitResult trumpSplit(Decks& decks, long long N, const std::vector<int>& hand10,
                            const std::vector<int>& discards, Suit trump, int batch = 50000);

}
