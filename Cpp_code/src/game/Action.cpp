#include "poker/game/Action.hpp"

namespace poker {

std::string Action::toString() const {
    const std::string value = std::to_string(amount);
    switch (type) {
        case ActionType::Fold: return "фолд";
        case ActionType::Check: return "чек";
        case ActionType::Call: return "кол " + value;
        case ActionType::Bet: return "бет " + value;
        case ActionType::Raise: return "рейз до " + value;
        case ActionType::AllIn: return "ОЛ-ІН (" + value + ")";
        case ActionType::PostBlind: return "блайнд " + value;
    }
    return "?";
}

}
