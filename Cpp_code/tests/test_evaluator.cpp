#include "TestFramework.hpp"

#include "poker/eval/HandEvaluator.hpp"

#include <algorithm>
#include <iterator>

using namespace poker;

namespace {

HandValue eval(const char* text) { return evaluateHand(parseCards(text)); }

bool is(const char* text, HandCategory category) { return eval(text).category == category; }

}

TEST_CASE(detects_every_category) {
    CHECK(is("AhKhQhJhTh", HandCategory::StraightFlush));
    CHECK(is("9c9d9h9s2c", HandCategory::FourOfAKind));
    CHECK(is("KhKdKc7s7h", HandCategory::FullHouse));
    CHECK(is("Ad9d7d4d2d", HandCategory::Flush));
    CHECK(is("9h8c7d6s5h", HandCategory::Straight));
    CHECK(is("7c7d7hKsQd", HandCategory::ThreeOfAKind));
    CHECK(is("JhJsTcTd4h", HandCategory::TwoPair));
    CHECK(is("AcAh9s6d3c", HandCategory::OnePair));
    CHECK(is("AdQc9h6s3d", HandCategory::HighCard));
}

TEST_CASE(wheel_straight_and_steel_wheel) {
    const HandValue wheel = eval("Ah2c3d4s5h");
    CHECK(wheel.category == HandCategory::Straight);
    CHECK_EQ(wheel.ranks[0], 5);
    CHECK(eval("6c5d4h3s2c") > wheel);
    CHECK(is("As2s3s4s5s", HandCategory::StraightFlush));
    CHECK(is("QhKdAc2s3h", HandCategory::HighCard));
}

TEST_CASE(royal_flush_description) {
    CHECK_EQ(eval("AsKsQsJsTs").describe(), std::string("Роял-флеш"));
}

TEST_CASE(best_five_of_seven) {
    CHECK(is("Ah9h8h7h2h6c5d", HandCategory::Flush));
    const HandValue fh = eval("KhKdKc7s7h7d2c");
    CHECK(fh.category == HandCategory::FullHouse);
    CHECK_EQ(fh.ranks[0], 13);
    CHECK_EQ(fh.ranks[1], 7);
    const HandValue tp = eval("AhAdKhKdQhQd2c");
    CHECK(tp.category == HandCategory::TwoPair);
    CHECK_EQ(tp.ranks[2], 12);
}

TEST_CASE(kickers_decide_ties) {
    CHECK(eval("AhAdKc7s2h") > eval("AsAcQd7c2d"));
    CHECK(eval("9h9d9c9sAh") > eval("9h9d9c9sKh"));
    CHECK(eval("AhKd8c6s3h") == eval("AsKc8d6h3c"));
    CHECK(eval("2c2d3h4s6h") > eval("AhKdQcJs9h"));
}

TEST_CASE(category_order_is_correct) {
    const char* hands[] = {"AdQc9h6s3d", "AcAh9s6d3c", "JhJsTcTd4h", "7c7d7hKsQd", "9h8c7d6s5h",
                           "Ad9d7d4d2d", "KhKdKc7s7h", "9c9d9h9s2c", "9h8h7h6h5h"};
    for (std::size_t i = 1; i < std::size(hands); ++i) CHECK(eval(hands[i]) > eval(hands[i - 1]));
}

TEST_CASE(partial_hands_are_supported) {
    CHECK(is("AhAd", HandCategory::OnePair));
    CHECK(is("7h7d7c", HandCategory::ThreeOfAKind));
    CHECK(is("AhKhQhJh", HandCategory::HighCard));
    CHECK_THROWS(evaluateHand(std::vector<Card>{}));
}

TEST_CASE(best_five_cards_matches_best_hand) {
    const HoleCards hole{Card::fromString("Ah"), Card::fromString("Kh")};
    const auto board = parseCards("Qh Jh 2c Th 3d");
    const auto five = bestFiveCards(hole, board);
    CHECK_EQ(five.size(), std::size_t{5});
    CHECK(evaluateHand(five).category == HandCategory::StraightFlush);
    CHECK(std::find(five.begin(), five.end(), Card::fromString("2c")) == five.end());
    CHECK_THROWS(bestFiveCards(hole, parseCards("2c3d")));
}

TEST_CASE(hole_and_board_overload) {
    const HoleCards hole{Card::fromString("Ah"), Card::fromString("Kh")};
    const auto board = parseCards("QhJhTh2c3d");
    CHECK(evaluateHand(hole, board).category == HandCategory::StraightFlush);
}
