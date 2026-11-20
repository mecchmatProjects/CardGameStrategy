#pragma once

#include <cstdint>
#include <string>

namespace poker {

enum class ActionType : std::uint8_t {
    Fold,
    Check,
    Call,
    Bet,
    Raise,
    AllIn,
    PostBlind
};

struct Action {
    ActionType type = ActionType::Fold;
    int amount = 0;

    static Action fold() { return {ActionType::Fold, 0}; }
    static Action check() { return {ActionType::Check, 0}; }
    static Action call() { return {ActionType::Call, 0}; }
    static Action bet(int total) { return {ActionType::Bet, total}; }
    static Action raiseTo(int total) { return {ActionType::Raise, total}; }
    static Action allIn() { return {ActionType::AllIn, 0}; }

    bool isAggressive() const {
        return type == ActionType::Bet || type == ActionType::Raise || type == ActionType::AllIn;
    }

    std::string toString() const;
};

}
