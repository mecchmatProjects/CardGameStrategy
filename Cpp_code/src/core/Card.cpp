#include "poker/core/Card.hpp"

#include <cctype>
#include <stdexcept>

namespace poker {

namespace {

constexpr char kRankChars[] = "23456789TJQKA";
constexpr char kSuitChars[] = "cdhs";

int parseRank(char c) {
    const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    for (int i = 0; i < kNumRanks; ++i) {
        if (kRankChars[i] == upper) return i + 2;
    }
    return -1;
}

int parseSuit(char c) {
    const char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    for (int i = 0; i < kNumSuits; ++i) {
        if (kSuitChars[i] == lower) return i;
    }
    return -1;
}

}

char rankToChar(Rank rank) { return kRankChars[static_cast<int>(rank) - 2]; }

char suitToChar(Suit suit) { return kSuitChars[static_cast<int>(suit)]; }

std::string suitSymbol(Suit suit) {
    switch (suit) {
        case Suit::Clubs: return "♣";
        case Suit::Diamonds: return "♦";
        case Suit::Hearts: return "♥";
        case Suit::Spades: return "♠";
    }
    return "?";
}

Card Card::fromString(std::string_view text) {
    int rank = -1;
    int suit = -1;
    if (text.size() == 2) {
        rank = parseRank(text[0]);
        suit = parseSuit(text[1]);
    } else if (text.size() == 3 && text[0] == '1' && text[1] == '0') {
        rank = 10;
        suit = parseSuit(text[2]);
    }
    if (rank < 0 || suit < 0) {
        throw std::invalid_argument("Некоректна карта: '" + std::string(text) + "'");
    }
    return Card(static_cast<Rank>(rank), static_cast<Suit>(suit));
}

Card Card::fromIndex(int index) {
    if (index < 0 || index >= kDeckSize) {
        throw std::out_of_range("Індекс карти має бути в межах 0..51");
    }
    return Card(static_cast<Rank>(index / kNumSuits + 2), static_cast<Suit>(index % kNumSuits));
}

std::string Card::toString() const { return std::string{rankToChar(rank_), suitToChar(suit_)}; }

std::string Card::pretty() const {
    const std::string rank = rank_ == Rank::Ten ? "10" : std::string(1, rankToChar(rank_));
    return rank + suitSymbol(suit_);
}

std::ostream& operator<<(std::ostream& os, Card card) { return os << card.toString(); }

std::vector<Card> parseCards(std::string_view text) {
    std::vector<Card> cards;
    CardMask seen = 0;
    std::size_t i = 0;
    while (i < text.size()) {
        const char c = text[i];
        if (std::isspace(static_cast<unsigned char>(c)) || c == ',' || c == ';') {
            ++i;
            continue;
        }
        const bool isTen = c == '1' && i + 1 < text.size() && text[i + 1] == '0';
        const std::size_t length = isTen ? 3 : 2;
        if (i + length > text.size()) {
            throw std::invalid_argument("Неповна карта: '" + std::string(text.substr(i)) + "'");
        }
        const Card card = Card::fromString(text.substr(i, length));
        if (seen & card.mask()) {
            throw std::invalid_argument("Карта повторюється: " + card.toString());
        }
        seen |= card.mask();
        cards.push_back(card);
        i += length;
    }
    return cards;
}

std::string cardsToString(const std::vector<Card>& cards) {
    std::string result;
    for (std::size_t i = 0; i < cards.size(); ++i) {
        if (i > 0) result += ' ';
        result += cards[i].toString();
    }
    return result;
}

CardMask toMask(const std::vector<Card>& cards) {
    CardMask mask = 0;
    for (const Card card : cards) mask |= card.mask();
    return mask;
}

}
