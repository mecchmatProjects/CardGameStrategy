#pragma once

#include "poker/core/Card.hpp"
#include "poker/game/Strategy.hpp"

#include <memory>
#include <string>

namespace poker {

class Player {
public:
    Player(int seat, std::string name, int chips, std::unique_ptr<Strategy> strategy);

    int seat() const { return seat_; }
    const std::string& name() const { return name_; }
    int chips() const { return chips_; }

    Strategy& strategy() { return *strategy_; }
    const Strategy& strategy() const { return *strategy_; }
    bool isHuman() const { return strategy_->isHuman(); }

    const HoleCards& hole() const { return hole_; }
    void setHole(const HoleCards& hole) { hole_ = hole; }

    bool hasFolded() const { return folded_; }
    bool isAllIn() const { return allIn_; }
    bool isSittingOut() const { return sittingOut_; }
    bool inHand() const { return !folded_ && !sittingOut_; }
    bool canAct() const { return inHand() && !allIn_; }

    int streetBet() const { return streetBet_; }
    int handContribution() const { return contribution_; }

    void startHand();
    void startStreet();

    int commit(int amount);

    void fold() { folded_ = true; }
    void award(int amount) { chips_ += amount; }
    void addChips(int amount) { chips_ += amount; }

private:
    int seat_;
    std::string name_;
    int chips_;
    std::unique_ptr<Strategy> strategy_;

    HoleCards hole_{};
    bool folded_ = false;
    bool allIn_ = false;
    bool sittingOut_ = false;
    int streetBet_ = 0;
    int contribution_ = 0;
};

}
