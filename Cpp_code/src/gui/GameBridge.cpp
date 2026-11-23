#include "poker/gui/GameBridge.hpp"

#include "poker/ai/Bots.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <utility>

namespace poker::gui {

namespace {

class GuiHumanStrategy : public Strategy {
public:
    explicit GuiHumanStrategy(GameBridge& bridge) : bridge_(bridge) {}
    Action decide(const DecisionContext& ctx) override { return bridge_.waitForHumanAction(ctx); }
    std::string name() const override { return "Людина"; }
    bool isHuman() const override { return true; }

private:
    GameBridge& bridge_;
};

class BridgeObserver : public GameObserver {
public:
    BridgeObserver(GameBridge& bridge, const Game& game) : bridge_(bridge), game_(game) {}

    void onHandStart(int handNumber, const std::vector<Player>&, int dealerSeat) override {
        GameEvent e = make(EventType::HandStart);
        e.handNumber = handNumber;
        e.dealer = dealerSeat;
        street_ = Street::Preflop;
        e.street = street_;
        bridge_.publish(std::move(e));
    }

    void onHoleCards(const Player& player) override {
        GameEvent e = make(EventType::HoleCards);
        e.seat = player.seat();
        bridge_.publish(std::move(e));
    }

    void onAction(const Player& player, const Action& action, int pot) override {
        GameEvent e = make(EventType::Action);
        e.seat = player.seat();
        e.action = action;
        e.pot = pot;
        bridge_.publish(std::move(e));
    }

    void onStreet(Street street, const std::vector<Card>&, int) override {
        street_ = street;
        bridge_.publish(make(EventType::Street));
    }

    void onShowdown(const std::vector<ShowdownEntry>& entries, const std::vector<Player>&) override {
        GameEvent e = make(EventType::Showdown);
        e.showdown = entries;
        bridge_.publish(std::move(e));
    }

    void onPotAwarded(const PotResult& result, const std::vector<Player>&) override {
        GameEvent e = make(EventType::PotAwarded);
        e.potResult = result;
        bridge_.publish(std::move(e));
    }

    void onHandEnd(const std::vector<Player>&) override { bridge_.publish(make(EventType::HandEnd)); }

private:
    GameEvent make(EventType type) const {
        GameEvent e;
        e.type = type;
        e.handNumber = game_.handNumber();
        e.dealer = game_.dealerSeat();
        e.seats = bridge_.snapshot();
        e.board = game_.board();
        e.pot = game_.pot();
        e.street = street_;
        return e;
    }

    GameBridge& bridge_;
    const Game& game_;
    Street street_ = Street::Preflop;
};

}

GameBridge::GameBridge(GameSettings settings) : settings_(settings) {}

GameBridge::~GameBridge() { stop(); }

void GameBridge::start() {
    if (thread_.joinable()) return;
    stop_ = false;
    thread_ = std::thread([this] { run(); });
}

void GameBridge::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    cv_.notify_all();
    if (thread_.joinable()) thread_.join();
}

bool GameBridge::pollEvent(GameEvent& out) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (events_.empty()) return false;
    out = std::move(events_.front());
    events_.pop_front();
    return true;
}

void GameBridge::submitAction(const Action& action) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingAction_ = action;
    }
    cv_.notify_all();
}

void GameBridge::continueToNextHand() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        continueRequested_ = true;
    }
    cv_.notify_all();
}

void GameBridge::publish(GameEvent event) {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.push_back(std::move(event));
}

std::vector<SeatState> GameBridge::snapshot() const {
    std::vector<SeatState> seats;
    if (!game_) return seats;
    for (const auto& p : game_->players()) {
        SeatState s;
        s.name = p.name();
        s.strategy = p.strategy().name();
        s.chips = p.chips();
        s.streetBet = p.streetBet();
        s.contribution = p.handContribution();
        s.folded = p.hasFolded();
        s.allIn = p.isAllIn();
        s.sittingOut = p.isSittingOut();
        s.human = p.isHuman();
        s.hole = p.hole();
        s.hasCards = !p.isSittingOut() && p.hole()[0] != p.hole()[1];
        s.stats = game_->playerStats(p.seat());
        seats.push_back(std::move(s));
    }
    return seats;
}

Action GameBridge::waitForHumanAction(const DecisionContext& ctx) {
    GameEvent e;
    e.type = EventType::DecisionRequest;
    e.handNumber = game_->handNumber();
    e.dealer = game_->dealerSeat();
    e.seats = snapshot();
    e.board = game_->board();
    e.pot = game_->pot();
    e.street = ctx.street;
    e.seat = 0;
    e.decision = ctx;

    std::unique_lock<std::mutex> lock(mutex_);
    pendingAction_.reset();
    events_.push_back(std::move(e));
    cv_.wait(lock, [this] { return pendingAction_.has_value() || stop_; });
    if (stop_) return Action::fold();
    const Action action = *pendingAction_;
    pendingAction_.reset();
    return action;
}

void GameBridge::run() {
    GameConfig config;
    config.smallBlind = settings_.smallBlind;
    config.bigBlind = settings_.bigBlind;
    config.startingChips = settings_.startingChips;

    Game game(config, settings_.seed);
    Rng rng(settings_.seed ^ 0xC0FFEEULL);
    game.addPlayer("Ви", std::make_unique<GuiHumanStrategy>(*this));

    const std::array<const char*, 5> names = {"Олег", "Марія", "Степан", "Ірина", "Тарас"};
    const auto types = lineupFor(static_cast<Difficulty>(std::clamp(settings_.difficulty, 0, 2)));
    for (int i = 0; i < settings_.opponents && i < 5; ++i) {
        const auto idx = static_cast<std::size_t>(i);
        game.addPlayer(names[idx], makeBot(types[idx], rng.next(), 1500));
    }

    BridgeObserver observer(*this, game);
    game.setObserver(&observer);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        game_ = &game;
    }

    while (!stop_) {
        const bool humanBusted = game.players().front().chips() == 0;
        if (humanBusted || game.playersWithChips() < 2) {
            GameEvent e;
            e.type = EventType::GameOver;
            e.seats = snapshot();
            e.handNumber = game.handNumber();
            e.humanWon = !humanBusted;
            publish(std::move(e));
            break;
        }

        game.playHand();

        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return continueRequested_ || stop_; });
        continueRequested_ = false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    game_ = nullptr;
}

}
