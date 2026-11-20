#pragma once

#include "poker/core/Card.hpp"
#include "poker/eval/HandEvaluator.hpp"
#include "poker/game/Action.hpp"
#include "poker/game/Player.hpp"
#include "poker/game/Types.hpp"

#include <string>
#include <vector>

namespace poker {

struct ShowdownEntry {
    int seat = 0;
    HoleCards hole{};
    HandValue value;
};

struct PotWinner {
    int seat = 0;
    int amount = 0;
};

struct PotResult {
    int index = 0;
    int amount = 0;
    std::vector<PotWinner> winners;
    std::string description;
};

class GameObserver {
public:
    virtual ~GameObserver() = default;

    virtual void onHandStart(int  , const std::vector<Player>&  ,
                             int  ) {}
    virtual void onHoleCards(const Player&  ) {}
    virtual void onAction(const Player&  , const Action&  , int  ) {}
    virtual void onStreet(Street  , const std::vector<Card>&  , int  ) {}
    virtual void onShowdown(const std::vector<ShowdownEntry>&  ,
                            const std::vector<Player>&  ) {}
    virtual void onPotAwarded(const PotResult&  , const std::vector<Player>&  ) {}
    virtual void onHandEnd(const std::vector<Player>&  ) {}
};

}
