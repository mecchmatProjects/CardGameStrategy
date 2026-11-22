#pragma once

#include "poker/core/Random.hpp"
#include "poker/gui/AnalysisPanel.hpp"
#include "poker/gui/GameBridge.hpp"

#include "raylib.h"

#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace poker::gui {

class PokerApp {
public:
    explicit PokerApp(std::uint64_t seed, bool demo = false, int level = 1);
    ~PokerApp();

    void run();

private:
    enum class Screen { Menu, Table };

    void updateCamera();
    void drawBackground() const;
    void handleKeys();

    void drawMenu();
    void startGame();
    void stopGame();

    void processEvents(float dt);
    void applyEvent(const GameEvent& e);
    float delayFor(const GameEvent& e) const;
    void resetHandState();

    void drawTopBar();
    void drawTable();
    void drawBoard();
    void drawSeat(std::size_t seat);
    void drawActionBar();
    void drawLog();
    void drawBanner();
    void drawGameOver();
    void drawStatsTooltip();
    Vector2 seatPosition(std::size_t seat) const;

    void submit(const Action& action);
    void autoPlay();
    int presetRaise(double potFraction) const;
    void clampRaise();

    Screen screen_ = Screen::Menu;
    Camera2D camera_{};
    Rng rng_;
    GameSettings settings_;
    int speedIndex_ = 1;
    std::unique_ptr<GameBridge> bridge_;
    AnalysisPanel analysis_;
    bool chartOpen_ = false;
    bool chartJustOpened_ = false;

    std::vector<SeatState> seats_;
    std::vector<Card> board_;
    int pot_ = 0;
    int dealer_ = -1;
    int handNumber_ = 0;
    Street street_ = Street::Preflop;
    std::vector<Action> lastAction_;
    std::vector<bool> hasAction_;
    std::vector<ActionType> actionType_;
    std::vector<float> cardsDealtAt_;
    std::vector<float> boardDealtAt_;
    int lastActor_ = -1;
    int hoveredSeat_ = -1;
    float lastActorAt_ = 0.0f;

    std::optional<DecisionContext> decision_;
    int raiseAmount_ = 0;
    bool draggingSlider_ = false;

    bool handOver_ = false;
    bool gameOver_ = false;
    bool humanWon_ = false;
    float gameOverAt_ = 0.0f;
    bool showdown_ = false;
    std::vector<std::string> handDescription_;
    std::vector<int> wonAmount_;
    std::vector<Card> winningCards_;
    struct BannerWin {
        int seat = 0;
        int amount = 0;
        bool sidePot = false;
    };
    std::vector<BannerWin> bannerWins_;
    std::string bannerDescription_;
    std::vector<std::string> bannerLines() const;
    std::vector<double> runoutEquity_;
    struct LogEntry {
        std::string text;
        int seat = -1;
        Action action;
        int won = -1;
    };
    std::deque<LogEntry> log_;
    void addLog(LogEntry entry);

    bool panelOpen_ = true;
    float panelAnim_ = 1.0f;
    bool bigBlindUnits_ = false;

    bool autoPlay_ = false;
    float decisionAt_ = 0.0f;
    float handOverAt_ = 0.0f;

    float eventTimer_ = 0.0f;
    float now_ = 0.0f;
    int startingChips_ = 0;
};

}
