#pragma once

#include "poker/game/Game.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace poker::gui {

struct SeatState {
    std::string name;
    std::string strategy;
    int chips = 0;
    int streetBet = 0;
    int contribution = 0;
    bool folded = false;
    bool allIn = false;
    bool sittingOut = false;
    bool human = false;
    bool hasCards = false;
    HoleCards hole{};
    OpponentInfo stats;

    bool inHand() const { return hasCards && !folded && !sittingOut; }
    bool canAct() const { return inHand() && !allIn; }
};

enum class EventType {
    HandStart,
    HoleCards,
    Action,
    Street,
    Showdown,
    PotAwarded,
    HandEnd,
    DecisionRequest,
    GameOver
};

struct GameEvent {
    EventType type = EventType::HandStart;
    int handNumber = 0;
    int dealer = -1;
    std::vector<SeatState> seats;
    std::vector<Card> board;
    int pot = 0;
    Street street = Street::Preflop;

    int seat = -1;
    Action action;
    std::vector<ShowdownEntry> showdown;
    PotResult potResult;
    DecisionContext decision;
    bool humanWon = false;
};

struct GameSettings {
    int opponents = 3;
    int difficulty = 1;
    int startingChips = 1000;
    int smallBlind = 10;
    int bigBlind = 20;
    std::uint64_t seed = 0;
};

class GameBridge {
public:
    explicit GameBridge(GameSettings settings);
    ~GameBridge();

    GameBridge(const GameBridge&) = delete;
    GameBridge& operator=(const GameBridge&) = delete;

    void start();
    void stop();

    bool pollEvent(GameEvent& out);

    void submitAction(const Action& action);
    void continueToNextHand();

    const GameSettings& settings() const { return settings_; }

    void publish(GameEvent event);
    Action waitForHumanAction(const DecisionContext& ctx);
    std::vector<SeatState> snapshot() const;

private:
    void run();

    GameSettings settings_;
    std::thread thread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<GameEvent> events_;
    std::optional<Action> pendingAction_;
    bool continueRequested_ = false;
    std::atomic<bool> stop_{false};
    const Game* game_ = nullptr;
};

}
