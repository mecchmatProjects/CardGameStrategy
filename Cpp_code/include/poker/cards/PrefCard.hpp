#pragma once

#include <string>

namespace poker::cards {

    enum class Suit : int {
        Spades = 1,
        Clubs = 2,
        Diamonds = 3,
        Hearts = 4
    };

    enum class Rank : int {
        Two = 2, Three, Four, Five, Six, Seven, Eight, Nine, Ten,
        Jack = 11,
        Queen = 12,
        King = 13,
        Ace = 14
    };

    inline constexpr Suit kAllSuits[] = {Suit::Spades, Suit::Clubs, Suit::Diamonds, Suit::Hearts};

    std::string suitName(Suit suit);
    std::string rankName(Rank rank);
    std::string suitSymbol(Suit suit);

    struct Card {
        Rank rank = Rank::Two;
        Suit suit = Suit::Spades;

        int code() const { return static_cast<int>(suit) * 100 + static_cast<int>(rank); }
        static Card fromCode(int code);

        std::string toString() const;

        friend bool operator==(Card a, Card b) { return a.rank == b.rank && a.suit == b.suit; }
        friend bool operator!=(Card a, Card b) { return !(a == b); }
    };

    bool beat(Card x, Card y, Suit trump);

}
