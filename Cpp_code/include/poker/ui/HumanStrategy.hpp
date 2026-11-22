#pragma once

#include "poker/analysis/ExpertAdvisor.hpp"
#include "poker/game/Strategy.hpp"

#include <cstdint>

namespace poker::ui {

class HumanStrategy : public Strategy {
public:
    explicit HumanStrategy(std::uint64_t seed);

    Action decide(const DecisionContext& ctx) override;
    std::string name() const override { return "Людина"; }
    bool isHuman() const override { return true; }

private:
    ExpertAdvisor advisor_;
};

}
