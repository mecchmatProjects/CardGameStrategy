#pragma once

#include "poker/game/Action.hpp"
#include "poker/game/DecisionContext.hpp"

#include <string>

namespace poker {

class Strategy {
public:
    virtual ~Strategy() = default;

    virtual Action decide(const DecisionContext& context) = 0;

    virtual std::string name() const = 0;

    virtual bool isHuman() const { return false; }
};

}
