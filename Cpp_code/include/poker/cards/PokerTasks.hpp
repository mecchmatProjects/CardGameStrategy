#pragma once

#include "poker/cards/Decks.hpp"

#include <cmath>
#include <functional>
#include <vector>

namespace poker::cards {

struct Estimate {
    long long trials = 0;
    long long hits = 0;
    double p() const { return trials > 0 ? static_cast<double>(hits) / static_cast<double>(trials) : 0.0; }
    double stdError() const { return trials > 0 ? std::sqrt(p() * (1.0 - p()) / static_cast<double>(trials)) : 0.0; }
};

bool hasFourOfAKind(const int* hand, int n);
bool isStraightFlush(const int* hand, int n);

struct PokerTaskResult {
    Estimate quads;
    Estimate straightFlush;
    double exactQuadsOneHand = 0.0;
    double exactStraightFlushOneHand = 0.0;
    int handsChecked = 0;
    double approxAny(double oneHand) const { return 1.0 - std::pow(1.0 - oneHand, handsChecked); }
};

PokerTaskResult pokerHandsRandom(Decks& decks, long long N, int m = 4, int n = 5, int batch = 100000);

PokerTaskResult pokerHandsFixed(Decks& decks, long long N, const std::vector<int>& firstHand, int m = 4,
                                int n = 5, int batch = 100000);

double exactOneHand(const std::vector<int>& cards, int n, const std::function<bool(const int*, int)>& predicate);

}
