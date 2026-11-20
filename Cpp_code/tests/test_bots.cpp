#include "TestFramework.hpp"

#include "poker/ai/Bots.hpp"
#include "poker/game/Game.hpp"

using namespace poker;

namespace {

DecisionContext facingBet() {
    DecisionContext ctx;
    ctx.hole = {Card::fromString("9c"), Card::fromString("4d")};
    ctx.board = parseCards("AhKsQd");
    ctx.street = Street::Flop;
    ctx.pot = 200;
    ctx.toCall = 100;
    ctx.highestBet = 100;
    ctx.minRaiseTo = 200;
    ctx.stack = 900;
    ctx.bigBlind = 20;
    ctx.activeOpponents = 2;
    ctx.playersInHand = 3;
    return ctx;
}

}

TEST_CASE(factory_creates_every_bot_type) {
    for (const BotType type : allBotTypes()) {
        const auto bot = makeBot(type, 1, 200);
        CHECK(bot != nullptr);
        CHECK_EQ(bot->name(), botTypeName(type));
        CHECK(!bot->isHuman());
        CHECK(!botTypeDescription(type).empty());
    }
}

TEST_CASE(calling_station_never_folds) {
    CallingStationBot bot;
    DecisionContext ctx = facingBet();
    CHECK(bot.decide(ctx).type == ActionType::Call);
    ctx.toCall = 0;
    CHECK(bot.decide(ctx).type == ActionType::Check);
}

TEST_CASE(tag_folds_weak_hand_to_big_bet) {
    TightAggressiveBot bot(5, 1000);
    CHECK(bot.decide(facingBet()).type == ActionType::Fold);
}

TEST_CASE(random_bot_raises_are_at_least_minimum) {
    RandomBot bot(9);
    const DecisionContext ctx = facingBet();
    for (int i = 0; i < 500; ++i) {
        const Action a = bot.decide(ctx);
        if (a.type == ActionType::Raise || a.type == ActionType::Bet) CHECK(a.amount >= ctx.minRaiseTo);
    }
}

#include "poker/analysis/ExpertAdvisor.hpp"

TEST_CASE(difficulty_lineups) {
    for (const BotType t : lineupFor(Difficulty::Easy)) {
        CHECK(t == BotType::Novice || t == BotType::CallingStation || t == BotType::Random);
    }
    for (const BotType t : lineupFor(Difficulty::Hard)) CHECK(t == BotType::Expert || t == BotType::ExpertLag);
    CHECK_EQ(lineupFor(Difficulty::Medium).size(), std::size_t{5});
    CHECK_EQ(difficultyName(Difficulty::Hard), std::string("Складний"));
}

namespace {

DecisionContext riverSpot(const char* hole, double foldToBet) {
    DecisionContext ctx;
    const auto h = parseCards(hole);
    ctx.hole = {h[0], h[1]};
    ctx.board = parseCards("Ks9d4c2h7s");
    ctx.street = Street::River;
    ctx.position = Position::Late;
    ctx.pot = 200;
    ctx.stack = ctx.effectiveStack = 1000;
    ctx.bigBlind = 20;
    ctx.minRaiseTo = 20;
    ctx.activeOpponents = 1;
    ctx.playersInHand = 2;
    OpponentInfo opp;
    opp.seat = 1;
    opp.hands = 200;
    opp.foldToBet = foldToBet;
    opp.aggression = 0.3;
    ctx.opponents.push_back(opp);
    ctx.history.push_back({1, Street::River, ActionType::Check, 0, false, 200, 0});
    return ctx;
}

int bluffs(double foldToBet) {
    int count = 0;
    for (int seed = 0; seed < 12; ++seed) {
        ExpertAdvisor advisor(100 + seed, 1500, 0.0);
        if (advisor.advise(riverSpot("Qh8h", foldToBet)).action.isAggressive()) ++count;
    }
    return count;
}

}

TEST_CASE(expert_bluffs_more_against_folders) {
    CHECK(bluffs(0.75) > bluffs(0.15));
    CHECK_EQ(bluffs(0.15), 0);
}

TEST_CASE(expert_value_bets_the_nuts) {
    ExpertAdvisor advisor(7, 1500, 0.0);
    const auto rec = advisor.advise(riverSpot("KhKd", 0.3));
    CHECK(rec.action.isAggressive());
    CHECK(rec.equity > 0.85);
}

TEST_CASE(expert_folds_air_to_big_bet) {
    ExpertAdvisor advisor(8, 1500, 0.0);
    DecisionContext ctx = riverSpot("Jc3d", 0.4);
    ctx.history.back() = {1, Street::River, ActionType::Bet, 300, true, 200, 0};
    ctx.pot = 500;
    ctx.toCall = ctx.highestBet = 300;
    ctx.minRaiseTo = 600;
    ctx.raisesThisStreet = 1;
    CHECK(advisor.advise(ctx).action.type == ActionType::Fold);
}

TEST_CASE(expert_and_novice_bots_play_full_games) {
    Game game(GameConfig{10, 20, 1000}, 31);
    game.addPlayer("Профі", makeBot(BotType::Expert, 1, 300));
    game.addPlayer("Профі-LAG", makeBot(BotType::ExpertLag, 2, 300));
    game.addPlayer("Новачок", makeBot(BotType::Novice, 3, 300));
    for (int i = 0; i < 60 && game.playHand(); ++i) CHECK_EQ(game.totalChips(), 3000);
}

namespace {

DecisionContext committedSpot() {
    DecisionContext ctx;
    ctx.hole = {Card::fromString("Ah"), Card::fromString("6c")};
    ctx.street = Street::Preflop;
    ctx.position = Position::Late;
    ctx.pot = 1864;
    ctx.toCall = 420;
    ctx.currentBet = 456;
    ctx.highestBet = 876;
    ctx.minRaiseTo = 1300;
    ctx.stack = 420;
    ctx.effectiveStack = 420;
    ctx.bigBlind = 20;
    ctx.activeOpponents = 1;
    ctx.playersInHand = 5;
    ctx.raisesThisStreet = ctx.preflopRaises = 4;
    return ctx;
}

}

TEST_CASE(expert_calls_shove_with_good_pot_odds) {
    ExpertAdvisor expert(1, 1500, 0.0);
    const ActionType a = expert.advise(committedSpot()).action.type;
    CHECK(a == ActionType::Call || a == ActionType::AllIn);
}

TEST_CASE(medium_calls_shove_with_good_pot_odds) {
    Advisor advisor(2, 1500);
    const ActionType a = advisor.advise(committedSpot()).action.type;
    CHECK(a == ActionType::Call || a == ActionType::AllIn);
}

TEST_CASE(big_preflop_raise_becomes_all_in) {
    DecisionContext ctx;
    ctx.pot = 300;
    ctx.highestBet = 200;
    ctx.toCall = 200;
    ctx.minRaiseTo = 300;
    ctx.stack = 900;
    ctx.bigBlind = 20;
    CHECK(sizedBet(ctx, 460).type == ActionType::AllIn);
    CHECK(sizedBet(ctx, 300).type == ActionType::Raise);
}

TEST_CASE(expert_does_not_jam_deep_into_small_pot) {
    for (int seed = 0; seed < 6; ++seed) {
        ExpertAdvisor expert(40 + seed, 1200, 0.0);
        DecisionContext ctx;
        ctx.hole = {Card::fromString("Jh"), Card::fromString("Td")};
        ctx.board = parseCards("6s Tc 3h");
        ctx.street = Street::Flop;
        ctx.position = Position::Late;
        ctx.pot = 60;
        ctx.stack = ctx.effectiveStack = 1860;
        ctx.bigBlind = 20;
        ctx.minRaiseTo = 20;
        ctx.activeOpponents = 1;
        ctx.playersInHand = 2;
        CHECK(expert.advise(ctx).action.type != ActionType::AllIn);
    }
}
