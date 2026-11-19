#include "TestFramework.hpp"

#include "poker/game/PotManager.hpp"

using namespace poker;

namespace {

int total(const std::vector<Pot>& pots) {
    int sum = 0;
    for (const auto& p : pots) sum += p.amount;
    return sum;
}

}

TEST_CASE(single_pot_when_everyone_equal) {
    const auto pots = buildPots({100, 100, 100}, {false, false, false});
    CHECK_EQ(pots.size(), std::size_t{1});
    CHECK_EQ(pots[0].amount, 300);
    CHECK_EQ(pots[0].eligible.size(), std::size_t{3});
}

TEST_CASE(side_pot_for_short_all_in) {
    const auto pots = buildPots({50, 100, 100}, {false, false, false});
    CHECK_EQ(pots.size(), std::size_t{2});
    CHECK_EQ(pots[0].amount, 150);
    CHECK_EQ(pots[0].eligible.size(), std::size_t{3});
    CHECK_EQ(pots[1].amount, 100);
    CHECK_EQ(pots[1].eligible.size(), std::size_t{2});
    CHECK_EQ(pots[1].eligible[0], 1);
}

TEST_CASE(folded_chips_go_to_pot_but_cannot_win) {
    const auto pots = buildPots({100, 50, 200}, {true, false, false});
    CHECK_EQ(total(pots), 350);
    CHECK_EQ(pots[0].amount, 150);
    CHECK_EQ(pots[0].eligible.size(), std::size_t{2});
    CHECK_EQ(pots[1].amount, 200);
    CHECK_EQ(pots[1].eligible.size(), std::size_t{1});
    CHECK_EQ(pots[1].eligible[0], 2);
}

TEST_CASE(multiple_all_ins_create_multiple_side_pots) {
    const auto pots = buildPots({25, 75, 150, 150}, {false, false, false, false});
    CHECK_EQ(pots.size(), std::size_t{3});
    CHECK_EQ(pots[0].amount, 100);
    CHECK_EQ(pots[1].amount, 150);
    CHECK_EQ(pots[2].amount, 150);
    CHECK_EQ(total(pots), 400);
}

TEST_CASE(mismatched_sizes_throw) { CHECK_THROWS(buildPots({1, 2}, {false})); }
