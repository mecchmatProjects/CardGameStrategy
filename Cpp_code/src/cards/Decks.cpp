#include "poker/cards/Decks.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace poker::cards {

namespace {

std::string asciiLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

int parseSuit(const std::string& text) {
    static const std::pair<const char*, int> names[] = {
        {"піка", 1},  {"піки", 1},  {"Піка", 1},  {"Піки", 1},  {"♠", 1},  {"spades", 1},
        {"трефа", 2}, {"трефи", 2}, {"Трефа", 2}, {"Трефи", 2}, {"♣", 2},  {"clubs", 2},
        {"бубна", 3}, {"бубни", 3}, {"Бубна", 3}, {"Бубни", 3}, {"♦", 3},  {"diamonds", 3},
        {"чирва", 4}, {"чирви", 4}, {"Чирва", 4}, {"Чирви", 4}, {"черва", 4}, {"черви", 4},
        {"Черва", 4}, {"Черви", 4}, {"♥", 4},     {"hearts", 4},
    };
    const std::string lower = asciiLower(text);
    for (const auto& [name, code] : names) {
        if (text == name || lower == name) return code;
    }
    throw std::invalid_argument("Невідома масть: '" + text + "'");
}

int parseRank(const std::string& text) {
    const std::string s = asciiLower(text);
    if (s == "jack" || s == "j" || text == "валет" || text == "Валет") return 11;
    if (s == "queen" || s == "q" || text == "дама" || text == "Дама") return 12;
    if (s == "king" || s == "k" || text == "король" || text == "Король") return 13;
    if (s == "ace" || s == "a" || text == "туз" || text == "Туз") return 14;
    try {
        std::size_t used = 0;
        const int value = std::stoi(s, &used);
        if (used == s.size() && value >= 2 && value <= 10) return value;
    } catch (const std::exception&) {
    }
    throw std::invalid_argument("Невідома гідність: '" + text + "'");
}

}

std::string deckTypeName(DeckType type) {
    switch (type) {
        case DeckType::Standard: return "Standard (52 карти)";
        case DeckType::Great: return "Great (52 карти + 2 джокери)";
        case DeckType::Middle: return "Middle (36 карт, від шістки)";
        case DeckType::Short: return "Short (32 карти, від сімки)";
    }
    return "?";
}

std::string cardsWord(int count) {
    const int n = count % 100;
    if (n >= 11 && n <= 14) return "карт";
    switch (n % 10) {
        case 1: return "карта";
        case 2:
        case 3:
        case 4: return "карти";
        default: return "карт";
    }
}

int deckMinRank(DeckType type) {
    switch (type) {
        case DeckType::Middle: return 6;
        case DeckType::Short: return 7;
        default: return 2;
    }
}

int deckJokers(DeckType type) { return type == DeckType::Great ? 2 : 0; }

Decks::Decks(int minRank, int jokers, int decks, std::uint64_t seed) : minRank_(minRank), rng_(seed) {
    if (minRank < 2 || minRank > 14) throw std::invalid_argument("Мінімальна гідність має бути 2..14");
    if (jokers < 0 || jokers > 2) throw std::invalid_argument("Джокерів може бути 0..2");
    if (decks < 1 || decks > 8) throw std::invalid_argument("Колод може бути 1..8");
    for (int d = 0; d < decks; ++d) {
        for (const Suit s : kAllSuits) {
            for (int r = minRank; r <= 14; ++r) cards_.push_back(static_cast<int>(s) * 100 + r);
        }
        if (jokers >= 1) cards_.push_back(kRedJoker);
        if (jokers >= 2) cards_.push_back(kBlackJoker);
    }
}

Decks::Decks(DeckType type, std::uint64_t seed, int decks)
    : Decks(deckMinRank(type), deckJokers(type), decks, seed) {}

int Decks::toInternal(const ExternalCard& card) {
    const std::string rank = asciiLower(card.first);
    if (rank == "joker" || card.first == "джокер" || card.first == "Джокер") {
        return (card.second == "чорний" || asciiLower(card.second) == "black") ? kBlackJoker : kRedJoker;
    }
    return parseSuit(card.second) * 100 + parseRank(card.first);
}

ExternalCard Decks::toExternal(int code) {
    if (code == kRedJoker) return {"Joker", "червоний"};
    if (code == kBlackJoker) return {"Joker", "чорний"};
    const Card c = Card::fromCode(code);
    return {rankName(c.rank), suitName(c.suit)};
}

std::string Decks::toString(int code) {
    const ExternalCard e = toExternal(code);
    return e.first + " " + e.second;
}

bool Decks::contains(int code) const { return std::find(cards_.begin(), cards_.end(), code) != cards_.end(); }

std::vector<int> Decks::shuffled() {
    std::vector<int> deck = cards_;
    std::shuffle(deck.begin(), deck.end(), rng_.engine());
    return deck;
}

DealResult Decks::deal(int N, int m, int n) {
    if (N < 0 || m < 1 || n < 1) throw std::invalid_argument("deal: некоректні N, m, n");
    if (m * n > size()) throw std::invalid_argument("deal: у колоді лише " + std::to_string(size()) + " карт");

    const auto restSize = static_cast<std::size_t>(size() - m * n);
    DealResult result{Array3D(static_cast<std::size_t>(N), static_cast<std::size_t>(m), static_cast<std::size_t>(n)),
                      Array2D(static_cast<std::size_t>(N), restSize)};
    std::vector<int> deck = cards_;
    const int total = size();

    for (int d = 0; d < N; ++d) {
        for (int i = 0; i < m * n; ++i) {
            const int j = rng_.uniformInt(i, total - 1);
            std::swap(deck[static_cast<std::size_t>(i)], deck[static_cast<std::size_t>(j)]);
        }
        for (int p = 0; p < m; ++p) {
            int* hand = result.hands.hand(static_cast<std::size_t>(d), static_cast<std::size_t>(p));
            std::copy_n(deck.begin() + p * n, n, hand);
        }
        std::copy(deck.begin() + m * n, deck.end(), result.rest.row(static_cast<std::size_t>(d)));
    }
    return result;
}

std::vector<int> Decks::pool(const std::vector<int>& removeA, const std::vector<int>& removeB) const {
    std::vector<int> rest = cards_;
    for (const auto* list : {&removeA, &removeB}) {
        for (const int code : *list) {
            const auto it = std::find(rest.begin(), rest.end(), code);
            if (it == rest.end()) {
                throw std::invalid_argument("Карти " + std::to_string(code) + " немає в колоді (або вона повторюється)");
            }
            rest.erase(it);
        }
    }
    return rest;
}

DealResult Decks::dealFixed(int N, int m, int n, const std::vector<int>& firstHand, const std::vector<int>& excluded) {
    if (static_cast<int>(firstHand.size()) != n) throw std::invalid_argument("dealFixed: на першій руці має бути n карт");
    if (m < 1) throw std::invalid_argument("dealFixed: m ≥ 1");

    std::vector<int> rest = pool(firstHand, excluded);
    const int others = (m - 1) * n;
    const int poolSize = static_cast<int>(rest.size());
    if (others > poolSize) throw std::invalid_argument("dealFixed: не вистачає карт для інших рук");

    DealResult result{Array3D(static_cast<std::size_t>(N), static_cast<std::size_t>(m), static_cast<std::size_t>(n)),
                      Array2D(static_cast<std::size_t>(N), static_cast<std::size_t>(poolSize - others))};
    for (int d = 0; d < N; ++d) {
        std::copy(firstHand.begin(), firstHand.end(), result.hands.hand(static_cast<std::size_t>(d), 0));
        for (int i = 0; i < others; ++i) {
            const int j = rng_.uniformInt(i, poolSize - 1);
            std::swap(rest[static_cast<std::size_t>(i)], rest[static_cast<std::size_t>(j)]);
        }
        for (int p = 1; p < m; ++p) {
            std::copy_n(rest.begin() + (p - 1) * n, n, result.hands.hand(static_cast<std::size_t>(d), static_cast<std::size_t>(p)));
        }
        std::copy(rest.begin() + others, rest.end(), result.rest.row(static_cast<std::size_t>(d)));
    }
    return result;
}

}
