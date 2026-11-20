#include "poker/ai/Bots.hpp"

#include <algorithm>
#include <stdexcept>

namespace poker {

std::string difficultyName(Difficulty difficulty) {
    switch (difficulty) {
        case Difficulty::Easy: return "Легкий";
        case Difficulty::Medium: return "Середній";
        case Difficulty::Hard: return "Складний";
    }
    return "?";
}

std::vector<BotType> lineupFor(Difficulty difficulty) {
    switch (difficulty) {
        case Difficulty::Easy:
            return {BotType::Novice, BotType::CallingStation, BotType::Novice, BotType::Random, BotType::Novice};
        case Difficulty::Medium:
            return {BotType::TightAggressive, BotType::LooseAggressive, BotType::Basic, BotType::TightAggressive,
                    BotType::LooseAggressive};
        case Difficulty::Hard:
            return {BotType::ExpertLag, BotType::Expert, BotType::ExpertLag, BotType::Expert, BotType::ExpertLag};
    }
    return {};
}

std::string botTypeName(BotType type) {
    switch (type) {
        case BotType::Expert: return "Профі";
        case BotType::ExpertLag: return "Профі-LAG";
        case BotType::Novice: return "Новачок";
        case BotType::TightAggressive: return "TAG";
        case BotType::LooseAggressive: return "LAG";
        case BotType::Basic: return "Базовий";
        case BotType::CallingStation: return "Колер";
        case BotType::Random: return "Рандом";
    }
    return "?";
}

std::string botTypeDescription(BotType type) {
    switch (type) {
        case BotType::Expert:
            return "профі: діапазон кожного суперника, його статистика, implied odds";
        case BotType::ExpertLag:
            return "профі-LAG: та сама математика, ширші відкриття й більше тиску";
        case BotType::Novice:
            return "новачок: колить, щоб подивитись карту, мало ставить";
        case BotType::TightAggressive:
            return "тайтово-агресивний: діапазони суперників, максимум EV";
        case BotType::LooseAggressive:
            return "лузово-агресивний: ширші діапазони, блефи, змішані дії";
        case BotType::Basic:
            return "перша версія: еквіті проти випадкових рук";
        case BotType::CallingStation:
            return "пасивний: завжди чек або кол";
        case BotType::Random:
            return "випадкові дії";
    }
    return "?";
}

std::vector<BotType> allBotTypes() {
    return {BotType::Expert,  BotType::ExpertLag, BotType::TightAggressive, BotType::LooseAggressive,
            BotType::Basic,   BotType::Novice,    BotType::CallingStation,  BotType::Random};
}

std::unique_ptr<Strategy> makeBot(BotType type, std::uint64_t seed, int equityIterations) {
    switch (type) {
        case BotType::Expert:
            return std::make_unique<ExpertBot>(seed, equityIterations);
        case BotType::ExpertLag:
            return std::make_unique<ExpertBot>(seed, equityIterations, true);
        case BotType::Novice:
            return std::make_unique<NoviceBot>(seed, equityIterations);
        case BotType::TightAggressive:
            return std::make_unique<TightAggressiveBot>(seed, equityIterations);
        case BotType::LooseAggressive:
            return std::make_unique<LooseAggressiveBot>(seed, equityIterations);
        case BotType::Basic:
            return std::make_unique<BasicBot>(seed, equityIterations);
        case BotType::CallingStation:
            return std::make_unique<CallingStationBot>();
        case BotType::Random:
            return std::make_unique<RandomBot>(seed);
    }
    throw std::invalid_argument("Невідомий тип бота");
}

TightAggressiveBot::TightAggressiveBot(std::uint64_t seed, int iterations)
    : advisor_(seed, iterations) {}

Action TightAggressiveBot::decide(const DecisionContext& ctx) { return advisor_.advise(ctx).action; }

LooseAggressiveBot::LooseAggressiveBot(std::uint64_t seed, int iterations)
    : advisor_(seed, iterations, AdvisorStyle{1.5, 1.25, 0.8}) {}

Action LooseAggressiveBot::decide(const DecisionContext& ctx) { return advisor_.advise(ctx).action; }

BasicBot::BasicBot(std::uint64_t seed, int iterations) : advisor_(seed, iterations) {}

ExpertBot::ExpertBot(std::uint64_t seed, int iterations, bool loose)
    : advisor_(seed, std::max(iterations, 1200), loose ? 0.8 : 0.15, loose ? 1.5 : 1.0, 1.0),
      loose_(loose) {}

NoviceBot::NoviceBot(std::uint64_t seed, int iterations) : advisor_(seed, iterations), rng_(seed ^ 0xF15EULL) {}

Action NoviceBot::decide(const DecisionContext& ctx) {
    Action action = advisor_.advise(ctx).action;
    const double r = rng_.uniform01();
    if (action.type == ActionType::Fold) {
        const bool cheap = ctx.board.empty() ? ctx.toCall <= 3 * ctx.bigBlind : ctx.toCall * 2 <= ctx.pot;
        if (cheap && r < 0.5) return Action::call();
    } else if (action.isAggressive() && action.type != ActionType::AllIn && r < 0.4) {
        return ctx.canCheck() ? Action::check() : Action::call();
    }
    if (r > 0.97) return sizedBet(ctx, ctx.minRaiseTo);
    return action;
}

Action CallingStationBot::decide(const DecisionContext& ctx) {
    return ctx.canCheck() ? Action::check() : Action::call();
}

RandomBot::RandomBot(std::uint64_t seed) : rng_(seed) {}

Action RandomBot::decide(const DecisionContext& ctx) {
    const double r = rng_.uniform01();
    if (r < 0.01) return Action::allIn();
    if (r < 0.20) {
        const int low = ctx.minRaiseTo;
        const int high = std::max(low, ctx.highestBet + ctx.pot);
        return sizedBet(ctx, rng_.uniformInt(low, high));
    }
    if (r < 0.85 || ctx.canCheck()) return ctx.canCheck() ? Action::check() : Action::call();
    return Action::fold();
}

}
