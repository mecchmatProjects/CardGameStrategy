#pragma once

#include "poker/core/Card.hpp"
#include "poker/game/Action.hpp"
#include "poker/game/Types.hpp"

#include <vector>

namespace poker {

struct ActionRecord {
    int seat = -1;
    Street street = Street::Preflop;
    ActionType type = ActionType::Fold;
    int amount = 0;
    bool raised = false;
    int potBefore = 0;
    int toCall = 0;
};

struct OpponentInfo {
    int seat = -1;
    int chips = 0;
    int streetBet = 0;
    int hands = 0;
    double vpip = 0.28;
    double pfr = 0.18;
    double aggression = 0.4;
    double foldToBet = 0.45;
};

struct DecisionContext {
    HoleCards hole{};
    std::vector<Card> board;
    Street street = Street::Preflop;
    Position position = Position::Middle;

    int pot = 0;
    int toCall = 0;
    int currentBet = 0;
    int highestBet = 0;
    int minRaiseTo = 0;
    int stack = 0;
    int bigBlind = 0;
    int activeOpponents = 0;
    int playersInHand = 0;

    int raisesThisStreet = 0;
    int preflopRaises = 0;
    bool preflopAggressor = false;
    int effectiveStack = 0;

    double opponentsVpip = 0.28;
    double opponentsAggression = 0.40;

    int seat = -1;
    std::vector<ActionRecord> history;
    std::vector<OpponentInfo> opponents;

    bool canCheck() const { return toCall == 0; }
    int maxRaiseTo() const { return currentBet + stack; }
};

}
