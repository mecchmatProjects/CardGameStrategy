#include "poker/game/PotManager.hpp"

#include <algorithm>
#include <stdexcept>

namespace poker {

std::vector<Pot> buildPots(const std::vector<int>& contributions, const std::vector<bool>& folded) {
    if (contributions.size() != folded.size()) {
        throw std::invalid_argument("buildPots: розміри масивів не збігаються");
    }
    const std::size_t n = contributions.size();

    std::vector<int> levels;
    for (std::size_t i = 0; i < n; ++i) {
        if (!folded[i] && contributions[i] > 0) levels.push_back(contributions[i]);
    }
    std::sort(levels.begin(), levels.end());
    levels.erase(std::unique(levels.begin(), levels.end()), levels.end());

    std::vector<Pot> pots;
    int previous = 0;
    for (const int level : levels) {
        Pot pot;
        for (std::size_t i = 0; i < n; ++i) {
            pot.amount += std::clamp(contributions[i] - previous, 0, level - previous);
            if (!folded[i] && contributions[i] >= level) pot.eligible.push_back(static_cast<int>(i));
        }
        if (pot.amount > 0) pots.push_back(std::move(pot));
        previous = level;
    }

    int leftover = 0;
    for (const int c : contributions) leftover += std::max(0, c - previous);
    if (leftover > 0) {
        if (pots.empty()) {
            pots.push_back({leftover, {}});
        } else {
            pots.back().amount += leftover;
        }
    }
    return pots;
}

}
