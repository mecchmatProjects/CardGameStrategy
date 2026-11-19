#pragma once

#include <array>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace poker {

enum class Suit : std::uint8_t { Clubs = 0, Diamonds = 1, Hearts = 2, Spades = 3 };

enum class Rank : std::uint8_t {
    Two = 2, Three, Four, Five, Six, Seven, Eight, Nine, Ten, Jack, Queen, King, Ace
};

inline constexpr int kNumSuits = 4;
inline constexpr int kNumRanks = 13;
inline constexpr int kDeckSize = 52;

char rankToChar(Rank rank);
char suitToChar(Suit suit);
std::string suitSymbol(Suit suit);

using CardMask = std::uint64_t;

class Card {
public:
    constexpr Card() = default;
    constexpr Card(Rank rank, Suit suit) : rank_(rank), suit_(suit) {}

    static Card fromString(std::string_view text);

    static Card fromIndex(int index);

    constexpr Rank rank() const { return rank_; }
    constexpr Suit suit() const { return suit_; }
    constexpr int rankValue() const { return static_cast<int>(rank_); }
    constexpr int index() const { return (rankValue() - 2) * kNumSuits + static_cast<int>(suit_); }
    constexpr CardMask mask() const { return CardMask{1} << index(); }
    constexpr bool isRed() const { return suit_ == Suit::Hearts || suit_ == Suit::Diamonds; }

    std::string toString() const;
    std::string pretty() const;

    friend constexpr bool operator==(Card a, Card b) { return a.index() == b.index(); }
    friend constexpr bool operator!=(Card a, Card b) { return !(a == b); }
    friend constexpr bool operator<(Card a, Card b) { return a.index() < b.index(); }

private:
    Rank rank_ = Rank::Two;
    Suit suit_ = Suit::Clubs;
};

using HoleCards = std::array<Card, 2>;

std::ostream& operator<<(std::ostream& os, Card card);

std::vector<Card> parseCards(std::string_view text);

std::string cardsToString(const std::vector<Card>& cards);
CardMask toMask(const std::vector<Card>& cards);

}
