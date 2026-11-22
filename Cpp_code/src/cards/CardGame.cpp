#include "poker/cards/CardGame.hpp"

#include <sstream>
#include <stdexcept>
#include <utility>

namespace poker::cards {

CardGame::CardGame(std::string name, DeckType deck, int decks, int minPlayers, int maxPlayers, GameRules rules,
                   std::map<int, int> weights, std::map<int, int> points, int minRankOverride)
    : name_(std::move(name)),
      deck_(deck),
      decks_(decks),
      minPlayers_(minPlayers),
      maxPlayers_(maxPlayers),
      rules_(std::move(rules)),
      weights_(std::move(weights)),
      points_(std::move(points)),
      minRankOverride_(minRankOverride) {
    if (decks_ < 1 || minPlayers_ < 1 || maxPlayers_ < minPlayers_) throw std::invalid_argument("Некоректні параметри гри");
}

int CardGame::minRank() const { return minRankOverride_ > 0 ? minRankOverride_ : deckMinRank(deck_); }

int CardGame::deckSize() const { return decks_ * (4 * (15 - minRank()) + deckJokers(deck_)); }

int CardGame::weight(int code) const {
    const int rank = Decks::rankOf(code);
    const auto it = weights_.find(rank);
    return it != weights_.end() ? it->second : rank;
}

int CardGame::points(int code) const {
    const auto it = points_.find(Decks::rankOf(code));
    return it != points_.end() ? it->second : 0;
}

bool CardGame::beats(int a, int b, int trumpSuit) const {
    if (Decks::suitOf(a) == Decks::suitOf(b)) return weight(a) > weight(b);
    return trumpSuit != 0 && Decks::suitOf(a) == trumpSuit;
}

Decks CardGame::makeDecks(std::uint64_t seed) const { return Decks(minRank(), deckJokers(deck_), decks_, seed); }

DealResult CardGame::deal(int N, int players, std::uint64_t seed) const {
    if (players < minPlayers_ || players > maxPlayers_) {
        throw std::invalid_argument(name_ + ": гравців має бути " + std::to_string(minPlayers_) + ".." +
                                    std::to_string(maxPlayers_));
    }
    Decks decks = makeDecks(seed);
    return decks.deal(N, players, rules_.cardsPerPlayer);
}

std::string CardGame::describe() const {
    std::ostringstream os;
    os << name_ << "\n";
    os << "  Колода: ";
    if (minRankOverride_ > 0) os << "скорочена, від " << rankName(static_cast<Rank>(minRankOverride_));
    else os << deckTypeName(deck_);
    if (decks_ > 1) os << " × " << decks_;
    os << " — разом " << deckSize() << " " << cardsWord(deckSize()) << "\n";
    os << "  Гравців: " << minPlayers_;
    if (maxPlayers_ != minPlayers_) os << "–" << maxPlayers_;
    os << ", карт кожному: " << rules_.cardsPerPlayer;
    if (rules_.talon > 0) os << ", прикуп/стіл: " << rules_.talon;
    os << (rules_.trumps ? ", є козирі" : ", без козирів") << "\n";
    if (!weights_.empty()) {
        os << "  Старшинство:";
        std::multimap<int, int> byWeight;
        for (const auto& [rank, w] : weights_) byWeight.emplace(w, rank);
        for (const auto& [w, rank] : byWeight) os << " " << rankName(static_cast<Rank>(rank));
        os << "\n";
    }
    if (!points_.empty()) {
        os << "  Очки:";
        for (const auto& [rank, p] : points_) os << " " << rankName(static_cast<Rank>(rank)) << "=" << p;
        os << "\n";
    }
    for (const auto& line : rules_.description) os << "  • " << line << "\n";
    return os.str();
}

CardGame CardGame::texasHoldem() {
    return CardGame("Техаський холдем", DeckType::Standard, 1, 2, 10,
                    GameRules{2, 5, false,
                              {"2 закриті карти кожному, 5 спільних карт на столі",
                               "Перемагає найкраща комбінація з 5 карт із 7",
                               "Масті рівноправні, туз — найстарша карта (і молодша в стріті A-2-3-4-5)"}});
}

CardGame CardGame::preferans() {
    return CardGame("Преферанс", DeckType::Short, 1, 3, 3,
                    GameRules{10, 2, true,
                              {"3 гравці по 10 карт, 2 карти — прикуп",
                               "Розігрувач бере прикуп, скидає 2 карти й оголошує козир та кількість взяток",
                               "Треба класти карту масті ходу; якщо її немає — козир; якщо й козиря немає — будь-яку",
                               "Взятку бере старша карта масті ходу або старший козир"}},
                    {{7, 1}, {8, 2}, {9, 3}, {10, 4}, {11, 5}, {12, 6}, {13, 7}, {14, 8}});
}

CardGame CardGame::durak() {
    return CardGame("Дурень (підкидний)", DeckType::Middle, 1, 2, 6,
                    GameRules{6, 0, true,
                              {"Кожному по 6 карт, наступна карта колоди визначає козир",
                               "Карту відбивають старшою тієї ж масті або козирем",
                               "Хто першим позбувся карт — виграв; останній — \"дурень\""}},
                    {{6, 1}, {7, 2}, {8, 3}, {9, 4}, {10, 5}, {11, 6}, {12, 7}, {13, 8}, {14, 9}});
}

CardGame CardGame::blackjack() {
    return CardGame("Блекджек", DeckType::Standard, 6, 1, 7,
                    GameRules{2, 0, false,
                              {"Мета — набрати суму очок ближче до 21, ніж у дилера, не перебравши",
                               "Туз — 11 або 1 очко, картинки — по 10",
                               "Грають кількома колодами (тут — 6)"}},
                    {}, {{2, 2}, {3, 3}, {4, 4}, {5, 5}, {6, 6}, {7, 7}, {8, 8}, {9, 9}, {10, 10},
                         {11, 10}, {12, 10}, {13, 10}, {14, 11}});
}

CardGame CardGame::thousand() {
    return CardGame("Тисяча", DeckType::Short, 1, 3, 3,
                    GameRules{7, 3, true,
                              {"Колода з 24 карт (від дев'ятки), 3 гравці по 7 карт, 3 карти — прикуп",
                               "Десятка старша за короля; марьяж (король + дама) оголошує козир",
                               "Гра до 1000 очок"}},
                    {{9, 1}, {11, 2}, {12, 3}, {13, 4}, {10, 5}, {14, 6}},
                    {{9, 0}, {11, 2}, {12, 3}, {13, 4}, {10, 10}, {14, 11}}, 9);
}

CardGame CardGame::klondike() {
    return CardGame("Пасьянс \"Косинка\" (Klondike)", DeckType::Standard, 1, 1, 1,
                    GameRules{28, 24, false,
                              {"28 карт викладаються у 7 стовпців, решта 24 — колода добору",
                               "Карти перекладаються на старшу на 1 протилежного кольору",
                               "Мета — зібрати 4 стопки від туза до короля"}},
                    {{14, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}, {6, 6}, {7, 7}, {8, 8}, {9, 9}, {10, 10},
                     {11, 11}, {12, 12}, {13, 13}});
}

std::vector<CardGame> CardGame::all() {
    return {texasHoldem(), preferans(), durak(), blackjack(), thousand(), klondike()};
}

}
