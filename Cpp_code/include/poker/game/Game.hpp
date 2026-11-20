#pragma once

#include "poker/core/Deck.hpp"
#include "poker/core/Random.hpp"
#include "poker/game/DecisionContext.hpp"
#include "poker/game/GameObserver.hpp"
#include "poker/game/Player.hpp"

#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace poker {

struct GameConfig {
    int smallBlind = 10;
    int bigBlind = 20;
    int startingChips = 1000;
};

class Game {
public:
    explicit Game(GameConfig config, std::uint64_t seed = std::random_device{}());

    int addPlayer(std::string name, std::unique_ptr<Strategy> strategy);

    void setObserver(GameObserver* observer);

    bool playHand();

    void rebuy(int seat, int amount);

    int playersWithChips() const;
    int totalChips() const;
    int pot() const;
    int handNumber() const { return handNumber_; }
    int dealerSeat() const { return dealer_; }
    const GameConfig& config() const { return config_; }
    const std::vector<Player>& players() const { return players_; }
    const std::vector<Card>& board() const { return board_; }

private:
    GameObserver& observer() { return observer_ ? *observer_ : silentObserver_; }

    int nextSeat(int from, bool requireInHand) const;
    int countInHand() const;
    int countCanAct() const;
    void prepareSeatOrder();
    Position positionOf(int seat) const;

    void postBlinds();
    void postBlind(int seat, int amount);
    void dealHoleCards();
    void dealBoard(int count);
    void bettingRound(Street street);
    Action execute(int seat, const Action& requested);
    DecisionContext makeContext(int seat, Street street) const;
    void awardUncontested();
    void showdown();

    GameConfig config_;
    Rng rng_;
    Deck deck_;
    std::vector<Player> players_;
    std::vector<Card> board_;
    std::vector<int> order_;

    GameObserver silentObserver_;
    GameObserver* observer_ = nullptr;

    int dealer_ = -1;
    int bigBlindSeat_ = -1;
    int handNumber_ = 0;
    int highestBet_ = 0;
    int minRaise_ = 0;

    int raisesThisStreet_ = 0;
    int preflopRaises_ = 0;
    int preflopAggressor_ = -1;

    struct PlayerStats {
        int hands = 0;
        int vpip = 0;
        int pfr = 0;
        int aggressive = 0;
        int passive = 0;
        int facedBet = 0;
        int foldedToBet = 0;
    };
    std::vector<PlayerStats> stats_;
    std::vector<bool> vpipCounted_;
    std::vector<bool> pfrCounted_;
    std::vector<ActionRecord> history_;

    double smoothedVpip(int seat) const;
    double smoothedAggression(int seat) const;
    OpponentInfo opponentInfo(const Player& p) const;
    void recordAction(int seat, Street street, const Action& resolved, bool raised, int potBefore, int toCall);

public:
    OpponentInfo playerStats(int seat) const { return opponentInfo(players_.at(static_cast<std::size_t>(seat))); }
};

}
