#include "poker/cards/PrefCard.hpp"

#include <stdexcept>

namespace poker::cards {

    std::string suitName(Suit suit) {
        switch (suit) {
            case Suit::Spades: return "піка";
            case Suit::Clubs: return "трефа";
            case Suit::Diamonds: return "бубна";
            case Suit::Hearts: return "чирва";
        }
        return "?";
    }

    std::string suitSymbol(Suit suit) {
        switch (suit) {
            case Suit::Spades: return "♠";
            case Suit::Clubs: return "♣";
            case Suit::Diamonds: return "♦";
            case Suit::Hearts: return "♥";
        }
        return "?";
    }

    std::string rankName(Rank rank) {
        switch (rank) {
            case Rank::Jack: return "Jack";
            case Rank::Queen: return "Queen";
            case Rank::King: return "King";
            case Rank::Ace: return "Ace";
            default: return std::to_string(static_cast<int>(rank));
        }
    }

    Card Card::fromCode(int code) {
        const int s = code / 100;
        const int r = code % 100;
        if (s < 1 || s > 4 || r < 2 || r > 14) {
            throw std::invalid_argument("Некоректний код карти: " + std::to_string(code));
        }
        return Card{static_cast<Rank>(r), static_cast<Suit>(s)};
    }

    std::string Card::toString() const { return rankName(rank) + " " + suitName(suit); }

    bool beat(Card x, Card y, Suit trump) {
        if (x.suit == y.suit) return static_cast<int>(x.rank) > static_cast<int>(y.rank);
        return x.suit == trump;
    }

}
