#pragma once

#include "poker/game/GameObserver.hpp"

namespace poker::ui {

class ConsoleObserver : public GameObserver {
public:
    explicit ConsoleObserver(bool revealAll = false) : revealAll_(revealAll) {}

    void onHandStart(int handNumber, const std::vector<Player>& players, int dealerSeat) override;
    void onHoleCards(const Player& player) override;
    void onAction(const Player& player, const Action& action, int pot) override;
    void onStreet(Street street, const std::vector<Card>& board, int pot) override;
    void onShowdown(const std::vector<ShowdownEntry>& entries,
                    const std::vector<Player>& players) override;
    void onPotAwarded(const PotResult& result, const std::vector<Player>& players) override;

private:
    bool revealAll_;
};

}
