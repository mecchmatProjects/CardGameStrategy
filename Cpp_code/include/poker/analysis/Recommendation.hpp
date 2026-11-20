#pragma once

#include "poker/game/Action.hpp"
#include "poker/game/DecisionContext.hpp"

#include <string>
#include <vector>

namespace poker {

struct OptionEV {
    Action action;
    double ev = 0.0;
    std::string label;
};

struct Recommendation {
    Action action;
    double equity = 0.0;
    double requiredEquity = 0.0;
    double callEv = 0.0;
    int outs = -1;
    double rangeTop = 1.0;
    std::vector<OptionEV> options;
    std::string handSummary;
    std::string reasoning;
};

Action sizedBet(const DecisionContext& ctx, int targetTotal);

Action potFractionBet(const DecisionContext& ctx, double fraction);

std::string optionLabel(const Action& action, const DecisionContext& ctx);

}
