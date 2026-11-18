#pragma once

#include <array>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace poker {

/// Масть карти.
enum class Suit : std::uint8_t { Clubs = 0, Diamonds = 1, Hearts = 2, Spades = 3 };

/// Ранг карти: 2..14 (14 — туз).
enum class Rank : std::uint8_t {
    Two = 2, Three, Four, Five, Six, Seven, Eight, Nine, Ten, Jack, Queen, King, Ace
};

inline constexpr int kNumSuits = 4;
inline constexpr int kNumRanks = 13;
inline constexpr int kDeckSize = 52;

char rankToChar(Rank rank);         ///< 'A', 'K', ..., 'T', '9', ..., '2'
char suitToChar(Suit suit);         ///< 'c', 'd', 'h', 's'
std::string suitSymbol(Suit suit);  ///< "♣", "♦", "♥", "♠"

/// Набір карт у вигляді бітової маски (біт № Card::index()).
using CardMask = std::uint64_t;

/// Одна гральна карта. Легкий value-тип (2 байти), можна вільно копіювати.
class Card {
public:
    constexpr Card() = default;
    constexpr Card(Rank rank, Suit suit) : rank_(rank), suit_(suit) {}

    /// Розбір карти з тексту: "Ah", "Td", "10c", "2s" (регістр не важливий).
    /// @throws std::invalid_argument якщо текст некоректний
    static Card fromString(std::string_view text);

    /// Карта за індексом 0..51.
    static Card fromIndex(int index);

    constexpr Rank rank() const { return rank_; }
    constexpr Suit suit() const { return suit_; }
    constexpr int rankValue() const { return static_cast<int>(rank_); }
    constexpr int index() const { return (rankValue() - 2) * kNumSuits + static_cast<int>(suit_); }
    constexpr CardMask mask() const { return CardMask{1} << index(); }
    constexpr bool isRed() const { return suit_ == Suit::Hearts || suit_ == Suit::Diamonds; }

    std::string toString() const;  ///< Коротка нотація: "Ah"
    std::string pretty() const;    ///< З символом масті: "A♥"

    friend constexpr bool operator==(Card a, Card b) { return a.index() == b.index(); }
    friend constexpr bool operator!=(Card a, Card b) { return !(a == b); }
    friend constexpr bool operator<(Card a, Card b) { return a.index() < b.index(); }

private:
    Rank rank_ = Rank::Two;
    Suit suit_ = Suit::Clubs;
};

/// Дві закриті карти гравця.
using HoleCards = std::array<Card, 2>;

std::ostream& operator<<(std::ostream& os, Card card);

/// Розбір списку карт: "AhKd 7c", "AhKd7c", "10h,9h". Повтори заборонені.
/// @throws std::invalid_argument
std::vector<Card> parseCards(std::string_view text);

std::string cardsToString(const std::vector<Card>& cards);
CardMask toMask(const std::vector<Card>& cards);

}  // namespace poker
