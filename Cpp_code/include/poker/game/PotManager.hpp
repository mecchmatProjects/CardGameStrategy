#pragma once

#include <vector>

namespace poker {

struct Pot {
    int amount = 0;
    std::vector<int> eligible;
};

std::vector<Pot> buildPots(const std::vector<int>& contributions, const std::vector<bool>& folded);

}
