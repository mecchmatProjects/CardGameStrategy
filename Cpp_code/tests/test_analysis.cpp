#include "TestFramework.hpp"

#include "poker/analysis/Advisor.hpp"
#include "poker/analysis/EquityCalculator.hpp"
#include "poker/analysis/Odds.hpp"
#include "poker/analysis/PreflopChart.hpp"

#include <algorithm>

using namespace poker;

namespace {

HoleCards hole(const char* text) {
    const auto cards = parseCards(text);
    return {cards.at(0), cards.at(1)};
}

DecisionContext context(const char* holeText, const char* boardText, int pot, int toCall) {
    DecisionContext ctx;
    ctx.hole = hole(holeText);
    ctx.board = parseCards(boardText);
    ctx.street = ctx.board.empty() ? Street::Preflop : Street::Flop;
    ctx.position = Position::Middle;
    ctx.pot = pot;
    ctx.toCall = toCall;
    ctx.highestBet = ctx.board.empty() ? std::max(toCall, 20) : toCall;
    ctx.currentBet = ctx.highestBet - toCall;
    ctx.minRaiseTo = ctx.highestBet + std::max(20, toCall);
    ctx.stack = 1000;
    ctx.bigBlind = 20;
    ctx.activeOpponents = 1;
    ctx.playersInHand = 2;
    return ctx;
}

}

TEST_CASE(aces_vs_kings_exact_equity) {
    EquityCalculator calc(1);
    const auto report = calc.calculate({hole("AhAs"), hole("KdKc")}, {});
    CHECK(report.exact);
    CHECK_EQ(report.trials, combinations(48, 5));
    CHECK_NEAR(report.players[0].equity, 0.82, 0.015);
    CHECK_NEAR(report.players[0].equity + report.players[1].equity, 1.0, 1e-9);
}

TEST_CASE(complete_board_is_deterministic) {
    EquityCalculator calc(2);
    const auto report = calc.calculate({hole("AhKh"), hole("QsQd")}, parseCards("2c7d9hJs3c"));
    CHECK_EQ(report.trials, 1LL);
    CHECK_NEAR(report.players[1].equity, 1.0, 1e-12);
}

TEST_CASE(board_royal_flush_splits_pot) {
    EquityCalculator calc(3);
    const auto report = calc.calculate({hole("2c3d"), hole("4h5h")}, parseCards("AsKsQsJsTs"));
    CHECK_NEAR(report.players[0].tie, 1.0, 1e-12);
    CHECK_NEAR(report.players[0].equity, 0.5, 1e-12);
}

TEST_CASE(duplicate_cards_are_rejected) {
    EquityCalculator calc(4);
    CHECK_THROWS(calc.calculate({hole("AhKh"), hole("AhQd")}, {}));
    CHECK_THROWS(calc.vsRandom(hole("AhKh"), parseCards("Ah2c3d"), 1));
}

TEST_CASE(aces_vs_random_hand) {
    EquityCalculator calc(5);
    const auto result = calc.vsRandom(hole("AhAs"), {}, 1, 20000);
    CHECK_NEAR(result.equity, 0.85, 0.02);
    const auto multiway = calc.vsRandom(hole("AhAs"), {}, 4, 20000);
    CHECK(multiway.equity < result.equity);
}

TEST_CASE(pot_odds_and_ev) {
    CHECK_NEAR(potOdds(50, 100), 1.0 / 3.0, 1e-12);
    CHECK_NEAR(potOdds(0, 100), 0.0, 1e-12);
    CHECK_NEAR(callEV(0.5, 100, 50), 25.0, 1e-12);
    CHECK_NEAR(bluffBreakEven(50, 100), 1.0 / 3.0, 1e-12);
    CHECK_EQ(percentString(0.6321), std::string("63.2%"));
}

TEST_CASE(flush_draw_with_overcards_has_15_outs) {
    const OutsInfo info = countOuts(hole("AhKh"), parseCards("7h2h9c"));
    CHECK_EQ(info.outs.size(), std::size_t{15});
    CHECK_EQ(info.unseen, 47);
    CHECK_NEAR(info.byRiver, 1.0 - 496.0 / 1081.0, 1e-9);
    CHECK_EQ(info.ruleOf2And4, 60);
}

TEST_CASE(board_pair_is_not_an_out) {
    const OutsInfo info = countOuts(hole("AhKd"), parseCards("7c2s9d"));
    for (const Card c : info.outs) CHECK(c.rank() == Rank::Ace || c.rank() == Rank::King);
}

TEST_CASE(chen_formula_values) {
    CHECK_NEAR(chenScore(hole("AhAs")), 20.0, 1e-9);
    CHECK_NEAR(chenScore(hole("KhKs")), 16.0, 1e-9);
    CHECK_NEAR(chenScore(hole("AhKh")), 12.0, 1e-9);
    CHECK_NEAR(chenScore(hole("2c2d")), 5.0, 1e-9);
    CHECK_NEAR(chenScore(hole("7h2c")), -1.0, 1e-9);
    CHECK(preflopTier(hole("AhAs")) == PreflopTier::Premium);
    CHECK(preflopTier(hole("7h2c")) == PreflopTier::Trash);
}

TEST_CASE(hand_codes) {
    CHECK_EQ(handCode(hole("AhKh")), std::string("AKs"));
    CHECK_EQ(handCode(hole("JdQc")), std::string("QJo"));
    CHECK_EQ(handCode(hole("7c7d")), std::string("77"));
}

TEST_CASE(preflop_chart_layout) {
    const PreflopChart chart = buildPreflopChart();
    CHECK_EQ(chart[0][0].code, std::string("AA"));
    CHECK_EQ(chart[0][1].code, std::string("AKs"));
    CHECK_EQ(chart[1][0].code, std::string("AKo"));
    CHECK_EQ(chart[12][12].code, std::string("22"));
    CHECK(chart[0][0].tier == PreflopTier::Premium);
    CHECK(shouldOpen(PreflopTier::Premium, Position::Early));
    CHECK(!shouldOpen(PreflopTier::Speculative, Position::Early));
    CHECK(shouldOpen(PreflopTier::Speculative, Position::Late));
}

TEST_CASE(advisor_raises_aces_preflop) {
    Advisor advisor(10, 1000);
    const auto rec = advisor.advise(context("AhAs", "", 30, 20));
    CHECK(rec.action.isAggressive());
    CHECK(!rec.reasoning.empty());
}

TEST_CASE(advisor_folds_trash_against_raise) {
    Advisor advisor(11, 1000);
    const auto rec = advisor.advise(context("7h2c", "", 90, 60));
    CHECK(rec.action.type == ActionType::Fold);
}

TEST_CASE(advisor_bets_the_nuts_on_flop) {
    Advisor advisor(12, 1000);
    const auto rec = advisor.advise(context("AhKh", "QhJhTh", 100, 0));
    CHECK(rec.action.isAggressive());
    CHECK(rec.equity > 0.95);
}

TEST_CASE(advisor_folds_air_against_big_bet) {
    Advisor advisor(13, 1000);
    const auto rec = advisor.advise(context("3c2d", "AhKsQd", 300, 300));
    CHECK(rec.action.type == ActionType::Fold);
    CHECK(rec.callEv < 0.0);
}

TEST_CASE(sized_bet_respects_limits) {
    DecisionContext ctx = context("AhKh", "QhJh2c", 100, 0);
    ctx.stack = 50;
    CHECK(sizedBet(ctx, 500).type == ActionType::AllIn);
    ctx.stack = 1000;
    const Action small = sizedBet(ctx, 5);
    CHECK(small.type == ActionType::Bet);
    CHECK_EQ(small.amount, ctx.minRaiseTo);
}

#include "poker/analysis/HandRanking.hpp"

TEST_CASE(hand_percentile_orders_hands) {
    CHECK(handPercentile(hole("AhAs")) < 0.01);
    CHECK(handPercentile(hole("KhKs")) < handPercentile(hole("QhQs")));
    CHECK(handPercentile(hole("AhKh")) < handPercentile(hole("AhKd")));
    CHECK(handPercentile(hole("7h2c")) > 0.9);
}

TEST_CASE(sample_from_top_respects_range_and_dead_cards) {
    Rng rng(3);
    const CardMask dead = toMask(parseCards("AhAs"));
    for (int i = 0; i < 300; ++i) {
        const auto h = sampleFromTop(0.05, dead, rng);
        CHECK(h.has_value());
        CHECK(handPercentile(*h) <= 0.06);
        CHECK(((*h)[0].mask() & dead) == 0);
        CHECK(((*h)[1].mask() & dead) == 0);
    }
}

TEST_CASE(equity_vs_tight_range_is_lower) {
    EquityCalculator calc(9);
    RangeModel any;
    RangeModel tight;
    tight.top = 0.05;
    const double vsAny = calc.vsRange(hole("QhJh"), {}, 1, any, 8000).equity;
    const double vsTight = calc.vsRange(hole("QhJh"), {}, 1, tight, 8000).equity;
    CHECK(vsTight + 0.1 < vsAny);
}

TEST_CASE(advisor_lists_ev_of_options) {
    Advisor advisor(21, 1500);
    const auto rec = advisor.advise(context("AhKh", "As7c2d", 200, 100));
    CHECK(rec.options.size() >= 3);
    CHECK(rec.action.type != ActionType::Fold);
}

TEST_CASE(short_stack_plays_all_in_or_fold) {
    Advisor advisor(22, 1500);
    DecisionContext strong = context("AhKd", "", 30, 20);
    strong.stack = strong.effectiveStack = 160;
    CHECK(advisor.advise(strong).action.type == ActionType::AllIn);
    DecisionContext weak = context("8c3d", "", 30, 20);
    weak.stack = weak.effectiveStack = 160;
    weak.activeOpponents = 3;
    CHECK(advisor.advise(weak).action.type == ActionType::Fold);
}

TEST_CASE(advisor_does_not_bluff_calling_stations_with_air) {
    Advisor advisor(23, 2000);
    DecisionContext ctx = context("3c2d", "AhKsQd", 200, 0);
    ctx.opponentsVpip = 0.9;
    ctx.activeOpponents = 2;
    const Action a = advisor.advise(ctx).action;
    CHECK(a.type == ActionType::Check);
}
