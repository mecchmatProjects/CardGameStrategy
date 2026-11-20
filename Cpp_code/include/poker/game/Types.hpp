#pragma once

#include <cstdint>
#include <string>

namespace poker {

enum class Street : std::uint8_t { Preflop, Flop, Turn, River, Showdown };

enum class Position : std::uint8_t {
    Early,
    Middle,
    Late,
    Blinds
};

std::string streetName(Street street);
std::string positionName(Position position);

Position positionFromButton(int indexFromButton, int playersDealt);

}
