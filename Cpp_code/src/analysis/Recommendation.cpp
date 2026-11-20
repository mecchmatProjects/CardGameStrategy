#include "poker/analysis/Recommendation.hpp"

#include <algorithm>
#include <cmath>

namespace poker {

Action sizedBet(const DecisionContext& ctx, int targetTotal) {
    if (targetTotal >= ctx.maxRaiseTo()) return Action::allIn();
    const int target = std::max(targetTotal, ctx.minRaiseTo);
    if (target >= ctx.maxRaiseTo()) return Action::allIn();
    const int behind = ctx.maxRaiseTo() - target;
    const int potAfter = ctx.pot + (target - ctx.currentBet) * 2;
    if (ctx.board.empty() ? target * 10 >= ctx.maxRaiseTo() * 4 : behind * 4 < potAfter) return Action::allIn();
    return ctx.highestBet == 0 ? Action::bet(target) : Action::raiseTo(target);
}

Action potFractionBet(const DecisionContext& ctx, double fraction) {
    const int size = std::max(ctx.bigBlind, static_cast<int>(std::lround(ctx.pot * fraction)));
    return sizedBet(ctx, ctx.highestBet + size);
}

std::string optionLabel(const Action& action, const DecisionContext& ctx) {
    switch (action.type) {
        case ActionType::Fold: return "фолд";
        case ActionType::Check: return "чек";
        case ActionType::Call: return "кол " + std::to_string(ctx.toCall);
        case ActionType::Bet: return "бет " + std::to_string(action.amount);
        case ActionType::Raise: return "рейз до " + std::to_string(action.amount);
        case ActionType::AllIn: return "ол-ін " + std::to_string(ctx.maxRaiseTo());
        case ActionType::PostBlind: return "блайнд";
    }
    return "?";
}

}
