#include "TestFramework.hpp"

#include "poker/ai/Bots.hpp"
#include "poker/game/Game.hpp"

#include <memory>
#include <utility>
#include <vector>

using namespace poker;

namespace {

class ScriptedStrategy : public Strategy {
public:
    explicit ScriptedStrategy(std::vector<Action> script, DecisionContext* lastContext = nullptr)
        : script_(std::move(script)), lastContext_(lastContext) {}

    Action decide(const DecisionContext& ctx) override {
        if (lastContext_) *lastContext_ = ctx;
        if (next_ < script_.size()) return script_[next_++];
        return ctx.canCheck() ? Action::check() : Action::call();
    }
    std::string name() const override { return "Сценарій"; }

private:
    std::vector<Action> script_;
    std::size_t next_ = 0;
    DecisionContext* lastContext_;
};

class RecordingObserver : public GameObserver {
public:
    void onAction(const Player& player, const Action& action, int) override {
        actions.emplace_back(player.seat(), action);
    }
    void onStreet(Street, const std::vector<Card>& board, int) override { boardSize = board.size(); }
    std::vector<std::pair<int, Action>> actions;
    std::size_t boardSize = 0;
};

std::unique_ptr<Strategy> script(std::vector<Action> actions, DecisionContext* ctx = nullptr) {
    return std::make_unique<ScriptedStrategy>(std::move(actions), ctx);
}

}

TEST_CASE(heads_up_small_blind_fold) {
    Game game(GameConfig{10, 20, 1000}, 1);
    game.addPlayer("A", script({Action::fold()}));
    game.addPlayer("B", script({}));
    CHECK(game.playHand());
    CHECK_EQ(game.players()[0].chips(), 990);
    CHECK_EQ(game.players()[1].chips(), 1010);
}

TEST_CASE(decision_context_is_correct_for_first_action) {
    DecisionContext seen;
    Game game(GameConfig{10, 20, 1000}, 2);
    game.addPlayer("A", script({Action::fold()}, &seen));
    game.addPlayer("B", script({}));
    game.playHand();
    CHECK_EQ(seen.pot, 30);
    CHECK_EQ(seen.toCall, 10);
    CHECK_EQ(seen.highestBet, 20);
    CHECK_EQ(seen.minRaiseTo, 40);
    CHECK_EQ(seen.stack, 990);
    CHECK_EQ(seen.activeOpponents, 1);
    CHECK(seen.street == Street::Preflop);
    CHECK(seen.position == Position::Late);
}

TEST_CASE(too_small_raise_is_bumped_to_minimum) {
    RecordingObserver recorder;
    Game game(GameConfig{10, 20, 1000}, 3);
    game.addPlayer("A", script({Action::raiseTo(25)}));
    game.addPlayer("B", script({Action::fold()}));
    game.setObserver(&recorder);
    game.playHand();

    CHECK_EQ(recorder.actions.size(), std::size_t{4});
    CHECK(recorder.actions[2].second.type == ActionType::Raise);
    CHECK_EQ(recorder.actions[2].second.amount, 40);
    CHECK_EQ(game.players()[0].chips(), 1020);
}

TEST_CASE(check_down_reaches_showdown_and_conserves_chips) {
    RecordingObserver recorder;
    Game game(GameConfig{10, 20, 1000}, 4);
    for (int i = 0; i < 3; ++i) game.addPlayer("P" + std::to_string(i), script({}));
    game.setObserver(&recorder);
    game.playHand();
    CHECK_EQ(recorder.boardSize, std::size_t{5});
    CHECK_EQ(game.totalChips(), 3000);
}

TEST_CASE(all_in_with_different_stacks_creates_side_pots) {
    Game game(GameConfig{10, 20, 1000}, 5);
    for (int i = 0; i < 3; ++i) game.addPlayer("P" + std::to_string(i), script({Action::allIn()}));
    game.rebuy(1, 500);
    game.rebuy(2, 1000);
    CHECK(game.playHand());
    CHECK_EQ(game.board().size(), std::size_t{5});
    CHECK_EQ(game.totalChips(), 4500);
    CHECK(game.players()[2].chips() >= 500);
}

TEST_CASE(bots_play_many_hands_without_losing_chips) {
    Game game(GameConfig{10, 20, 1000}, 6);
    game.addPlayer("TAG", makeBot(BotType::TightAggressive, 1, 150));
    game.addPlayer("LAG", makeBot(BotType::LooseAggressive, 2, 150));
    game.addPlayer("Колер", makeBot(BotType::CallingStation, 3));
    game.addPlayer("Рандом", makeBot(BotType::Random, 4));

    int hands = 0;
    for (int i = 0; i < 300 && game.playHand(); ++i) {
        ++hands;
        CHECK_EQ(game.totalChips(), 4000);
        for (const auto& p : game.players()) CHECK(p.chips() >= 0);
    }
    CHECK(hands > 0);
}

TEST_CASE(game_stops_when_one_player_has_chips) {
    Game game(GameConfig{10, 20, 1000}, 7);
    game.addPlayer("A", script(std::vector<Action>(100, Action::allIn())));
    game.addPlayer("B", script(std::vector<Action>(100, Action::allIn())));
    int hands = 0;
    while (game.playHand()) ++hands;
    CHECK(hands >= 1);
    CHECK_EQ(game.playersWithChips(), 1);
    CHECK_EQ(game.totalChips(), 2000);
}

TEST_CASE(hole_cards_sorted_high_first) {
    Game game(GameConfig{10, 20, 1000}, 8);
    for (int i = 0; i < 4; ++i) game.addPlayer("P" + std::to_string(i), script({}));
    for (int h = 0; h < 20 && game.playHand(); ++h) {
        for (const auto& p : game.players()) CHECK(p.hole()[0].rankValue() >= p.hole()[1].rankValue());
    }
}

TEST_CASE(context_contains_history_and_opponent_profile) {
    DecisionContext seen;
    Game game(GameConfig{10, 20, 1000}, 9);
    game.addPlayer("A", script({Action::raiseTo(60)}));
    game.addPlayer("B", script({Action::fold()}, &seen));
    game.playHand();
    CHECK_EQ(seen.preflopRaises, 1);
    CHECK_EQ(seen.raisesThisStreet, 1);
    CHECK_EQ(seen.effectiveStack, 980);
    CHECK(seen.opponentsVpip > 0.0 && seen.opponentsVpip < 1.0);
}
