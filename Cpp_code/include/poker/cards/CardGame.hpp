#pragma once

#include "poker/cards/Decks.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace poker::cards {

struct GameRules {
    int cardsPerPlayer = 0;
    int talon = 0;
    bool trumps = false;
    std::vector<std::string> description;
};

class CardGame {
public:
    CardGame(std::string name, DeckType deck, int decks, int minPlayers, int maxPlayers, GameRules rules,
             std::map<int, int> weights = {}, std::map<int, int> points = {}, int minRankOverride = 0);

    const std::string& name() const { return name_; }
    DeckType deckType() const { return deck_; }
    int decks() const { return decks_; }
    int minPlayers() const { return minPlayers_; }
    int maxPlayers() const { return maxPlayers_; }
    const GameRules& rules() const { return rules_; }
    int minRank() const;
    int deckSize() const;

    int weight(int code) const;
    int points(int code) const;
    bool beats(int a, int b, int trumpSuit = 0) const;

    Decks makeDecks(std::uint64_t seed) const;
    DealResult deal(int N, int players, std::uint64_t seed) const;

    std::string describe() const;

    static CardGame texasHoldem();
    static CardGame preferans();
    static CardGame durak();
    static CardGame blackjack();
    static CardGame thousand();
    static CardGame klondike();
    static std::vector<CardGame> all();

private:
    std::string name_;
    DeckType deck_;
    int decks_;
    int minPlayers_;
    int maxPlayers_;
    GameRules rules_;
    std::map<int, int> weights_;
    std::map<int, int> points_;
    int minRankOverride_;
};

}
