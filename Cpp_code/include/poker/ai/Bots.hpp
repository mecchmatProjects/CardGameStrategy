#pragma once

#include "poker/analysis/Advisor.hpp"
#include "poker/analysis/BasicAdvisor.hpp"
#include "poker/analysis/ExpertAdvisor.hpp"
#include "poker/core/Random.hpp"
#include "poker/game/Strategy.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace poker {

enum class BotType {
    Expert,
    ExpertLag,
    TightAggressive,
    LooseAggressive,
    Basic,
    Novice,
    CallingStation,
    Random
};

enum class Difficulty { Easy, Medium, Hard };

std::string difficultyName(Difficulty difficulty);
std::vector<BotType> lineupFor(Difficulty difficulty);

std::string botTypeName(BotType type);
std::string botTypeDescription(BotType type);
std::vector<BotType> allBotTypes();

std::unique_ptr<Strategy> makeBot(BotType type, std::uint64_t seed, int equityIterations = 1500);

class TightAggressiveBot : public Strategy {
public:
    TightAggressiveBot(std::uint64_t seed, int iterations);
    Action decide(const DecisionContext& ctx) override;
    std::string name() const override { return botTypeName(BotType::TightAggressive); }

private:
    Advisor advisor_;
};

class LooseAggressiveBot : public Strategy {
public:
    LooseAggressiveBot(std::uint64_t seed, int iterations);
    Action decide(const DecisionContext& ctx) override;
    std::string name() const override { return botTypeName(BotType::LooseAggressive); }

private:
    Advisor advisor_;
};

class ExpertBot : public Strategy {
public:
    ExpertBot(std::uint64_t seed, int iterations, bool loose = false);
    Action decide(const DecisionContext& ctx) override { return advisor_.advise(ctx).action; }
    std::string name() const override { return botTypeName(loose_ ? BotType::ExpertLag : BotType::Expert); }

private:
    ExpertAdvisor advisor_;
    bool loose_;
};

class NoviceBot : public Strategy {
public:
    NoviceBot(std::uint64_t seed, int iterations);
    Action decide(const DecisionContext& ctx) override;
    std::string name() const override { return botTypeName(BotType::Novice); }

private:
    BasicAdvisor advisor_;
    Rng rng_;
};

class BasicBot : public Strategy {
public:
    BasicBot(std::uint64_t seed, int iterations);
    Action decide(const DecisionContext& ctx) override { return advisor_.advise(ctx).action; }
    std::string name() const override { return botTypeName(BotType::Basic); }

private:
    BasicAdvisor advisor_;
};

class CallingStationBot : public Strategy {
public:
    Action decide(const DecisionContext& ctx) override;
    std::string name() const override { return botTypeName(BotType::CallingStation); }
};

class RandomBot : public Strategy {
public:
    explicit RandomBot(std::uint64_t seed);
    Action decide(const DecisionContext& ctx) override;
    std::string name() const override { return botTypeName(BotType::Random); }

private:
    Rng rng_;
};

}
