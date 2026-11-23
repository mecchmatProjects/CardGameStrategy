#include "poker/game/Types.hpp"

namespace poker {

std::string streetName(Street street) {
    switch (street) {
        case Street::Preflop: return "Префлоп";
        case Street::Flop: return "Флоп";
        case Street::Turn: return "Терн";
        case Street::River: return "Рівер";
        case Street::Showdown: return "Шоудаун";
    }
    return "?";
}

std::string positionName(Position position) {
    switch (position) {
        case Position::Early: return "рання";
        case Position::Middle: return "середня";
        case Position::Late: return "пізня";
        case Position::Blinds: return "блайнди";
    }
    return "?";
}

Position positionFromButton(int k, int n) {
    if (n == 2) return k == 0 ? Position::Late : Position::Blinds;
    if (k == 0) return Position::Late;
    if (k <= 2) return Position::Blinds;
    if (k == n - 1) return Position::Late;
    const int middleSeats = n - 4;
    return (k - 3) < (middleSeats + 1) / 2 ? Position::Early : Position::Middle;
}

}
