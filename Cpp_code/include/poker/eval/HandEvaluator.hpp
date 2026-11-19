#pragma once

#include "poker/core/Card.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace poker {

enum class HandCategory : std::uint8_t {
    HighCard = 0,
    OnePair,
    TwoPair,
    ThreeOfAKind,
    Straight,
    Flush,
    FullHouse,
    FourOfAKind,
    StraightFlush
};

std::string categoryName(HandCategory category);

struct HandValue {
    HandCategory category = HandCategory::HighCard;
    std::array<int, 5> ranks{};
    std::uint32_t score = 0;

    std::string describe() const;

    friend bool operator<(const HandValue& a, const HandValue& b) { return a.score < b.score; }
    friend bool operator>(const HandValue& a, const HandValue& b) { return a.score > b.score; }
    friend bool operator==(const HandValue& a, const HandValue& b) { return a.score == b.score; }
    friend bool operator!=(const HandValue& a, const HandValue& b) { return a.score != b.score; }
    friend bool operator<=(const HandValue& a, const HandValue& b) { return a.score <= b.score; }
    friend bool operator>=(const HandValue& a, const HandValue& b) { return a.score >= b.score; }
};

HandValue evaluateHand(const Card* cards, std::size_t count);
HandValue evaluateHand(const std::vector<Card>& cards);
HandValue evaluateHand(const HoleCards& hole, const std::vector<Card>& board);

std::vector<Card> bestFiveCards(const HoleCards& hole, const std::vector<Card>& board);

}
