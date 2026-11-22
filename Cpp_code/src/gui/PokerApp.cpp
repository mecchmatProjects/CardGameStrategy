#include "poker/gui/PokerApp.hpp"

#include "poker/ai/Bots.hpp"
#include "poker/eval/HandEvaluator.hpp"
#include "poker/gui/Render.hpp"

#include "rlgl.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>

namespace poker::gui {

namespace {

constexpr Vector2 kTableCenter{475.0f, 362.0f};
constexpr float kTableRx = 392.0f;
constexpr float kTableRy = 212.0f;

constexpr std::array<const char*, 3> kSpeedNames = {"Повільно", "Нормально", "Швидко"};
constexpr std::array<float, 3> kSpeedFactor = {1.6f, 1.0f, 0.45f};
constexpr std::array<int, 4> kStacks = {500, 1000, 2000, 5000};
constexpr std::array<std::array<int, 2>, 4> kBlinds = {{{5, 10}, {10, 20}, {25, 50}, {50, 100}}};

const std::array<const char*, 5> kBotNames = {"Олег", "Марія", "Степан", "Ірина", "Тарас"};

Color avatarColor(const std::string& strategy) {
    if (strategy == "TAG") return {52, 120, 220, 255};
    if (strategy == "LAG") return {220, 92, 52, 255};
    if (strategy == "Профі") return {200, 60, 70, 255};
    if (strategy == "Профі-LAG") return {170, 40, 120, 255};
    if (strategy == "Новачок") return {120, 170, 80, 255};
    if (strategy == "Колер") return {60, 160, 110, 255};
    if (strategy == "Базовий") return {40, 150, 160, 255};
    if (strategy == "Рандом") return {150, 90, 200, 255};
    return {214, 160, 40, 255};
}

Color actionColor(ActionType type) {
    switch (type) {
        case ActionType::Fold: return {120, 126, 136, 255};
        case ActionType::Check:
        case ActionType::Call: return {88, 200, 120, 255};
        case ActionType::Bet:
        case ActionType::Raise: return {246, 196, 72, 255};
        case ActionType::AllIn: return {236, 80, 90, 255};
        case ActionType::PostBlind: return {110, 160, 230, 255};
    }
    return WHITE;
}

std::string actionText(const Action& a) {
    switch (a.type) {
        case ActionType::Fold: return "Фолд";
        case ActionType::Check: return "Чек";
        case ActionType::Call: return "Кол " + formatChips(a.amount);
        case ActionType::Bet: return "Бет " + formatChips(a.amount);
        case ActionType::Raise: return "Рейз до " + formatChips(a.amount);
        case ActionType::AllIn: return "Ол-ін " + formatChips(a.amount);
        case ActionType::PostBlind: return "Блайнд " + formatChips(a.amount);
    }
    return "";
}

float easeOut(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
}

Vector2 lerp(Vector2 a, Vector2 b, float t) { return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t}; }

}

PokerApp::PokerApp(std::uint64_t seed, bool demo, int level) : rng_(seed), analysis_(seed ^ 0x5EEDULL), autoPlay_(demo) {
    settings_.difficulty = std::clamp(level, 0, 2);
    unsigned int flags = FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT;
    if (std::getenv("HOLDEM_NO_MSAA") == nullptr) flags |= FLAG_MSAA_4X_HINT;
#if defined(__APPLE__)
    flags |= FLAG_WINDOW_HIGHDPI;
#endif
    SetConfigFlags(flags);
    InitWindow(static_cast<int>(kWidth), static_cast<int>(kHeight), "Техаський холдем");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    SetWindowMinSize(800, 500);

    const int monitor = GetCurrentMonitor();
    const float mw = static_cast<float>(GetMonitorWidth(monitor));
    const float mh = static_cast<float>(GetMonitorHeight(monitor));
    if (mw > 0 && mh > 0 && (mw - 40 < kWidth || mh - 90 < kHeight)) {
        const float scale = std::min((mw - 40) / kWidth, (mh - 90) / kHeight);
        SetWindowSize(static_cast<int>(kWidth * scale), static_cast<int>(kHeight * scale));
        SetWindowPosition(static_cast<int>((mw - kWidth * scale) / 2), 30);
    }

    loadFonts(std::max(1.0f, GetWindowScaleDPI().x));
    if (demo) startGame();
}

PokerApp::~PokerApp() {
    stopGame();
    unloadFonts();
    CloseWindow();
}

void PokerApp::run() {
    while (!WindowShouldClose()) {
        const float dt = GetFrameTime();
        now_ = static_cast<float>(GetTime());
        updateCamera();
        SetMouseCursor(MOUSE_CURSOR_DEFAULT);

        if (screen_ == Screen::Table) {
            processEvents(dt);
            autoPlay();
        }

        BeginDrawing();
        ClearBackground(theme::Background);
        BeginMode2D(camera_);
        drawBackground();

        if (screen_ == Screen::Menu) {
            setChipUnits(false, settings_.bigBlind);
            drawMenu();
        } else {
            setChipUnits(bigBlindUnits_, settings_.bigBlind);
            handleKeys();
            panelAnim_ += ((panelOpen_ ? 1.0f : 0.0f) - panelAnim_) * std::min(1.0f, dt * 10.0f);

            const float shift = (1.0f - panelAnim_) * 165.0f;
            const Vector2 realMouse = mouse();
            rlPushMatrix();
            rlTranslatef(shift, 0.0f, 0.0f);
            setVirtualMouse({realMouse.x - shift, realMouse.y});
            hoveredSeat_ = -1;
            drawTable();
            drawLog();
            drawActionBar();
            drawBanner();
            setVirtualMouse(realMouse);
            rlPopMatrix();

            drawTopBar();
            if (panelAnim_ > 0.01f) {
                const float px = 958.0f + (1.0f - panelAnim_) * 330.0f;
                if (analysis_.draw({px, 66, 308, 726}, decision_.has_value(), !bannerWins_.empty())) {
                    chartOpen_ = true;
                    chartJustOpened_ = true;
                    analysis_.openChart();
                }
            }
            if (!chartOpen_) drawStatsTooltip();
            if (chartOpen_ && !chartJustOpened_) chartOpen_ = analysis_.drawPreflopChart();
            chartJustOpened_ = false;
            if (gameOver_ && now_ >= gameOverAt_) drawGameOver();
        }

        EndMode2D();
        EndDrawing();
    }
}

void PokerApp::updateCamera() {
    const float sw = static_cast<float>(GetScreenWidth());
    const float sh = static_cast<float>(GetScreenHeight());
    const float zoom = std::min(sw / kWidth, sh / kHeight);
    camera_.zoom = zoom;
    camera_.offset = {(sw - kWidth * zoom) / 2, (sh - kHeight * zoom) / 2};
    camera_.target = {0, 0};
    camera_.rotation = 0;
    setVirtualMouse(GetScreenToWorld2D(GetMousePosition(), camera_));
}

void PokerApp::drawBackground() const {
    DrawRectangleGradientV(0, 0, static_cast<int>(kWidth), static_cast<int>(kHeight), theme::BackgroundTop,
                           theme::Background);
}

void PokerApp::handleKeys() {
    if (IsKeyPressed(KEY_T) && !chartOpen_) {
        chartOpen_ = true;
        chartJustOpened_ = true;
        analysis_.openChart();
    }
    if (IsKeyPressed(KEY_H)) analysis_.toggleHints();
    if (IsKeyPressed(KEY_TAB)) panelOpen_ = !panelOpen_;
    if (IsKeyPressed(KEY_B)) bigBlindUnits_ = !bigBlindUnits_;
    if (chartOpen_ || gameOver_) return;

    if (decision_) {
        const DecisionContext& ctx = *decision_;
        if (IsKeyPressed(KEY_F)) submit(Action::fold());
        else if (IsKeyPressed(KEY_C) || IsKeyPressed(KEY_SPACE)) submit(ctx.canCheck() ? Action::check() : Action::call());
        else if (IsKeyPressed(KEY_A)) submit(Action::allIn());
        else if (IsKeyPressed(KEY_R) && ctx.maxRaiseTo() > ctx.highestBet) {
            submit(raiseAmount_ >= ctx.maxRaiseTo() ? Action::allIn() : Action::raiseTo(raiseAmount_));
        }
        if (decision_) {
            const int step = std::max(1, settings_.bigBlind);
            const float wheel = GetMouseWheelMove();
            if (IsKeyPressed(KEY_UP) || wheel > 0) raiseAmount_ += step;
            if (IsKeyPressed(KEY_DOWN) || wheel < 0) raiseAmount_ -= step;
            clampRaise();
        }
    } else if (handOver_ && !gameOver_ && (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))) {
        handOver_ = false;
        bridge_->continueToNextHand();
    }
}

void PokerApp::drawMenu() {
    static int opponents = 3, stackIndex = 1, blindIndex = 1, difficulty = settings_.difficulty;

    const std::array<Card, 5> fan = {Card(Rank::Ten, Suit::Spades), Card(Rank::Jack, Suit::Spades),
                                     Card(Rank::Queen, Suit::Spades), Card(Rank::King, Suit::Spades),
                                     Card(Rank::Ace, Suit::Spades)};
    for (std::size_t i = 0; i < fan.size(); ++i) {
        const float fi = static_cast<float>(i);
        const float bob = std::sin(now_ * 1.5f + fi * 0.7f) * 4.0f;
        drawCard(fan[i], {440 + fi * 82, 48 + std::fabs(fi - 2) * 14 + bob, 76, 108});
    }

    drawTextCentered("ТЕХАСЬКИЙ ХОЛДЕМ", {kWidth / 2, 205}, 46, theme::Text, true);
    drawTextCentered("No-Limit Hold'em з аналізом оптимальної стратегії", {kWidth / 2, 246}, 17, theme::Muted);

    const Rectangle panel{340, 268, 600, 420};
    drawPanel(panel, theme::Panel, 0.05f);
    float y = panel.y + 28;
    const float lx = panel.x + 36, cx = panel.x + 420;

    auto stepper = [&](const std::string& label, const std::string& value, int& index, int minV, int maxV) {
        drawText(label, lx, y + 6, 18, theme::Text);
        if (button({cx - 70, y, 40, 36}, "−", ButtonStyle::Normal, index > minV)) --index;
        drawTextCentered(value, {cx + 30, y + 18}, 20, theme::Gold, true);
        if (button({cx + 90, y, 40, 36}, "+", ButtonStyle::Normal, index < maxV)) ++index;
        y += 52;
    };
    stepper("Рівень ботів", difficultyName(static_cast<Difficulty>(difficulty)), difficulty, 0, 2);
    stepper("Суперники", std::to_string(opponents), opponents, 1, 5);
    stepper("Стартовий стек", formatChipsRaw(kStacks[static_cast<std::size_t>(stackIndex)]), stackIndex, 0, 3);
    const auto& blinds = kBlinds[static_cast<std::size_t>(blindIndex)];
    stepper("Блайнди", std::to_string(blinds[0]) + " / " + std::to_string(blinds[1]), blindIndex, 0, 3);
    stepper("Швидкість ботів", kSpeedNames[static_cast<std::size_t>(speedIndex_)], speedIndex_, 0, 2);

    y += 4;
    drawText("За столом:", lx, y, 14, theme::Muted, true);
    float ox = lx + 92;
    const auto lineup = lineupFor(static_cast<Difficulty>(difficulty));
    for (int i = 0; i < opponents; ++i) {
        const std::string type = botTypeName(lineup[static_cast<std::size_t>(i)]);
        DrawCircle(static_cast<int>(ox + 12), static_cast<int>(y + 9), 12, avatarColor(type));
        drawTextCentered(firstLetter(kBotNames[static_cast<std::size_t>(i)]), {ox + 12, y + 9}, 13, WHITE, true);
        drawText(type, ox + 28, y + 1, 13, theme::Text);
        ox += 28 + measureText(type, 13).x + 16;
    }
    y += 34;

    if (button({panel.x + 150, y, 300, 56}, "ПОЧАТИ ГРУ", ButtonStyle::Gold, true, false, 22) ||
        IsKeyPressed(KEY_ENTER)) {
        settings_.opponents = opponents;
        settings_.difficulty = difficulty;
        settings_.startingChips = kStacks[static_cast<std::size_t>(stackIndex)];
        settings_.smallBlind = blinds[0];
        settings_.bigBlind = blinds[1];
        startGame();
    }

    drawTextCentered("Справа під час гри — аналітика: еквіті, шанси банку, аути та оптимальна дія.",
                     {kWidth / 2, 748}, 14, theme::Muted);
    drawTextCentered("Клавіші: F фолд · C кол/чек · R рейз · A ол-ін · T префлоп-таблиця · H підказки",
                     {kWidth / 2, 772}, 14, theme::Muted);
}

void PokerApp::startGame() {
    stopGame();
    settings_.seed = rng_.next();
    startingChips_ = settings_.startingChips;
    bridge_ = std::make_unique<GameBridge>(settings_);
    seats_.clear();
    board_.clear();
    log_.clear();
    gameOver_ = humanWon_ = handOver_ = false;
    decision_.reset();
    analysis_.clearDecision();
    resetHandState();
    screen_ = Screen::Table;
    eventTimer_ = 0;
    bridge_->start();
}

void PokerApp::stopGame() {
    if (bridge_) {
        bridge_->stop();
        bridge_.reset();
    }
    decision_.reset();
    chartOpen_ = false;
}

void PokerApp::processEvents(float dt) {
    if (!bridge_) return;
    eventTimer_ -= dt;
    while (eventTimer_ <= 0.0f) {
        GameEvent e;
        if (!bridge_->pollEvent(e)) {
            eventTimer_ = 0.0f;
            return;
        }
        applyEvent(e);
        eventTimer_ = delayFor(e) * kSpeedFactor[static_cast<std::size_t>(speedIndex_)];
    }
}

float PokerApp::delayFor(const GameEvent& e) const {
    switch (e.type) {
        case EventType::HandStart: return 0.45f;
        case EventType::HoleCards: return 0.09f;
        case EventType::Action:
            if (e.action.type == ActionType::PostBlind) return 0.3f;
            return e.seat == 0 ? 0.25f : 0.8f;
        case EventType::Street: return 0.9f;
        case EventType::Showdown: return 1.0f;
        case EventType::PotAwarded: return 1.1f;
        default: return 0.0f;
    }
}

void PokerApp::resetHandState() {
    const std::size_t n = std::max<std::size_t>(seats_.size(), 6);
    lastAction_.assign(n, Action{});
    hasAction_.assign(n, false);
    actionType_.assign(n, ActionType::Check);
    cardsDealtAt_.assign(n, -10.0f);
    boardDealtAt_.assign(5, -10.0f);
    handDescription_.assign(n, "");
    wonAmount_.assign(n, 0);
    runoutEquity_.clear();
    winningCards_.clear();
    bannerWins_.clear();
    bannerDescription_.clear();
    showdown_ = false;
    lastActor_ = -1;
}

void PokerApp::addLog(LogEntry entry) {
    log_.push_back(std::move(entry));
    while (log_.size() > 6) log_.pop_front();
}

std::vector<std::string> PokerApp::bannerLines() const {
    std::vector<std::string> lines;
    for (const auto& w : bannerWins_) {
        lines.push_back(seats_[static_cast<std::size_t>(w.seat)].name + " виграє " + formatChips(w.amount) +
                        (w.sidePot ? " (сайд-пот)" : ""));
    }
    if (!lines.empty() && !bannerDescription_.empty()) lines.push_back(bannerDescription_);
    return lines;
}

void PokerApp::applyEvent(const GameEvent& e) {
    if (!e.seats.empty()) seats_ = e.seats;

    switch (e.type) {
        case EventType::HandStart:
            resetHandState();
            board_.clear();
            pot_ = 0;
            dealer_ = e.dealer;
            handNumber_ = e.handNumber;
            street_ = Street::Preflop;
            handOver_ = false;
            addLog({"— Роздача #" + std::to_string(e.handNumber) + " —", -1, Action{}, -1});
            analysis_.update(seats_, board_, dealer_);
            break;

        case EventType::HoleCards:
            cardsDealtAt_[static_cast<std::size_t>(e.seat)] = now_;
            if (e.seat == 0) analysis_.update(seats_, board_, dealer_);
            break;

        case EventType::Action: {
            const auto s = static_cast<std::size_t>(e.seat);
            pot_ = e.pot;
            lastAction_[s] = e.action;
            hasAction_[s] = true;
            actionType_[s] = e.action.type;
            lastActor_ = e.seat;
            lastActorAt_ = now_;
            addLog({"", e.seat, e.action, -1});
            analysis_.update(seats_, board_, dealer_);
            break;
        }

        case EventType::Street: {
            for (std::size_t i = board_.size(); i < e.board.size() && i < 5; ++i) {
                boardDealtAt_[i] = now_ + static_cast<float>(i - board_.size()) * 0.12f;
            }
            board_ = e.board;
            pot_ = e.pot;
            street_ = e.street;
            for (std::size_t i = 0; i < hasAction_.size(); ++i) {
                if (actionType_[i] != ActionType::Fold && actionType_[i] != ActionType::AllIn) hasAction_[i] = false;
            }
            std::string cards;
            for (const Card c : e.board) cards += (cards.empty() ? "" : " ") + c.pretty();
            addLog({streetName(e.street) + ": " + cards, -1, Action{}, -1});
            analysis_.update(seats_, board_, dealer_);

            const auto inHand = std::count_if(seats_.begin(), seats_.end(), [](const SeatState& st) { return st.inHand(); });
            const auto canAct = std::count_if(seats_.begin(), seats_.end(), [](const SeatState& st) { return st.canAct(); });
            if (inHand >= 2 && canAct <= 1 && board_.size() < 5) {
                runoutEquity_ = analysis_.runoutEquity(seats_, board_);
            } else {
                runoutEquity_.clear();
            }
            break;
        }

        case EventType::DecisionRequest:
            decision_ = e.decision;
            decisionAt_ = now_;
            pot_ = e.pot;
            analysis_.update(seats_, board_, dealer_);
            analysis_.onDecision(e.decision);
            raiseAmount_ = e.decision.minRaiseTo;
            if (const auto& rec = analysis_.recommendation();
                rec && (rec->action.type == ActionType::Bet || rec->action.type == ActionType::Raise)) {
                raiseAmount_ = rec->action.amount;
            }
            clampRaise();
            break;

        case EventType::Showdown:
            showdown_ = true;
            runoutEquity_.clear();
            for (const auto& entry : e.showdown) {
                std::string text = entry.value.describe();
                if (const auto pos = text.find(", кікер"); pos != std::string::npos) text.erase(pos);
                handDescription_[static_cast<std::size_t>(entry.seat)] = text;
            }
            break;

        case EventType::PotAwarded: {
            const PotResult& r = e.potResult;
            for (const auto& w : r.winners) {
                const auto s = static_cast<std::size_t>(w.seat);
                wonAmount_[s] += w.amount;
                pot_ = std::max(0, pot_ - w.amount);
                bannerWins_.push_back({w.seat, w.amount, r.index > 0});
                addLog({"", w.seat, Action{}, w.amount});
            }
            if (bannerDescription_.empty()) bannerDescription_ = r.description;
            if (winningCards_.empty() && showdown_ && board_.size() >= 3 && !r.winners.empty()) {
                winningCards_ = bestFiveCards(seats_[static_cast<std::size_t>(r.winners.front().seat)].hole, board_);
            }
            break;
        }

        case EventType::HandEnd:
            handOver_ = true;
            handOverAt_ = now_;
            pot_ = 0;
            decision_.reset();
            analysis_.clearDecision();
            gameOverAt_ = now_ + 3.0f;
            if (!seats_.empty() && seats_.front().chips == 0) {
                gameOver_ = true;
                humanWon_ = false;
            } else if (std::count_if(seats_.begin(), seats_.end(), [](const SeatState& s) { return s.chips > 0; }) == 1) {
                gameOver_ = true;
                humanWon_ = true;
            }
            break;

        case EventType::GameOver:
            if (!gameOver_) gameOverAt_ = now_;
            gameOver_ = true;
            humanWon_ = e.humanWon;
            break;
    }
}

void PokerApp::autoPlay() {
    if (!autoPlay_ || chartOpen_) return;
    const float factor = kSpeedFactor[static_cast<std::size_t>(speedIndex_)];
    if (decision_ && now_ - decisionAt_ > 1.4f * factor) {
        const auto& rec = analysis_.recommendation();
        submit(rec ? rec->action : Action::check());
    } else if (handOver_ && !gameOver_ && now_ - handOverAt_ > 2.6f * factor) {
        handOver_ = false;
        bridge_->continueToNextHand();
    } else if (gameOver_ && now_ - gameOverAt_ > 3.0f) {
        startGame();
    }
}

void PokerApp::submit(const Action& action) {
    if (!bridge_ || !decision_) return;
    bridge_->submitAction(action);
    decision_.reset();
    analysis_.clearDecision();
}

int PokerApp::presetRaise(double potFraction) const {
    if (!decision_) return 0;
    const DecisionContext& c = *decision_;
    return c.highestBet + static_cast<int>(std::lround(potFraction * (c.pot + c.toCall)));
}

void PokerApp::clampRaise() {
    if (!decision_) return;
    const int maxTo = decision_->maxRaiseTo();
    const int minTo = std::min(decision_->minRaiseTo, maxTo);
    raiseAmount_ = std::clamp(raiseAmount_, minTo, maxTo);
}

Vector2 PokerApp::seatPosition(std::size_t seat) const {
    static const Vector2 bottom{475, 646}, bottomLeft{118, 520}, left{96, 362}, topLeft{150, 186},
        top{475, 112}, topRight{796, 186}, right{846, 362}, bottomRight{826, 520};
    const std::size_t n = seats_.size();
    std::vector<Vector2> layout;
    switch (n) {
        case 2: layout = {bottom, top}; break;
        case 3: layout = {bottom, topLeft, topRight}; break;
        case 4: layout = {bottom, left, top, right}; break;
        case 5: layout = {bottom, bottomLeft, topLeft, topRight, bottomRight}; break;
        default: layout = {bottom, bottomLeft, topLeft, top, topRight, bottomRight}; break;
    }
    return layout[std::min(seat, layout.size() - 1)];
}

void PokerApp::drawTopBar() {
    DrawRectangle(0, 0, static_cast<int>(kWidth), 54, Color{8, 14, 12, 220});
    DrawRectangle(0, 54, static_cast<int>(kWidth), 1, Color{255, 255, 255, 25});
    drawText("♠", 20, 10, 28, theme::Text, true);
    drawText("♥", 44, 10, 28, theme::CardRed, true);
    drawText("Техаський холдем", 80, 14, 22, theme::Text, true);
    if (handNumber_ > 0) {
        drawText("Роздача #" + std::to_string(handNumber_) + "  ·  Блайнди " + std::to_string(settings_.smallBlind) +
                     "/" + std::to_string(settings_.bigBlind),
                 316, 18, 15, theme::Muted);
    }

    float x = kWidth - 18;
    auto place = [&x](float width) {
        x -= width;
        const Rectangle r{x, 10, width, 34};
        x -= 10;
        return r;
    };
    if (button(place(96), "Меню", ButtonStyle::Ghost, true, false, 14)) {
        stopGame();
        screen_ = Screen::Menu;
    }
    if (button(place(176), "Швидкість: " + std::string(kSpeedNames[static_cast<std::size_t>(speedIndex_)]),
               ButtonStyle::Ghost, true, false, 14)) {
        speedIndex_ = (speedIndex_ + 1) % 3;
    }
    if (button(place(118), autoPlay_ ? "Автогра ✓" : "Автогра", autoPlay_ ? ButtonStyle::Primary : ButtonStyle::Ghost,
               true, false, 14)) {
        autoPlay_ = !autoPlay_;
        decisionAt_ = handOverAt_ = now_;
    }
    if (button(place(126), panelOpen_ ? "Аналітика ▶" : "◀ Аналітика", panelOpen_ ? ButtonStyle::Primary : ButtonStyle::Ghost,
               true, false, 14)) {
        panelOpen_ = !panelOpen_;
    }
    if (button(place(84), bigBlindUnits_ ? "У ББ ✓" : "У ББ", bigBlindUnits_ ? ButtonStyle::Primary : ButtonStyle::Ghost,
               true, false, 14)) {
        bigBlindUnits_ = !bigBlindUnits_;
    }
}

void PokerApp::drawTable() {
    const Vector2 c = kTableCenter;
    DrawEllipse(static_cast<int>(c.x), static_cast<int>(c.y + 14), kTableRx + 34, kTableRy + 34, Color{0, 0, 0, 90});
    DrawEllipse(static_cast<int>(c.x), static_cast<int>(c.y), kTableRx + 30, kTableRy + 30, theme::RimLight);
    DrawEllipse(static_cast<int>(c.x), static_cast<int>(c.y), kTableRx + 22, kTableRy + 22, theme::Rim);
    DrawEllipse(static_cast<int>(c.x), static_cast<int>(c.y), kTableRx + 4, kTableRy + 4, Color{20, 60, 36, 255});
    DrawEllipse(static_cast<int>(c.x), static_cast<int>(c.y), kTableRx, kTableRy, theme::FeltDark);
    for (int i = 0; i < 8; ++i) {
        const float k = 1.0f - static_cast<float>(i) * 0.1f;
        DrawEllipse(static_cast<int>(c.x), static_cast<int>(c.y - 10), kTableRx * k, kTableRy * k,
                    withAlpha(theme::Felt, 0.35f));
    }
    DrawEllipseLines(static_cast<int>(c.x), static_cast<int>(c.y), kTableRx - 34, kTableRy - 34, Color{255, 255, 255, 30});
    drawTextCentered("TEXAS HOLD'EM", {c.x, c.y + 112}, 20, Color{255, 255, 255, 28}, true);

    drawBoard();
    for (std::size_t i = 0; i < seats_.size(); ++i) drawSeat(i);

    if (dealer_ >= 0 && static_cast<std::size_t>(dealer_) < seats_.size()) {
        const Vector2 p = seatPosition(static_cast<std::size_t>(dealer_));
        Vector2 dir{c.x - p.x, c.y - p.y};
        const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        dir = {dir.x / len, dir.y / len};
        const Vector2 d = dealer_ == 0 ? Vector2{p.x + 150, p.y - 52}
                                       : Vector2{p.x + dir.x * 92 - dir.y * 78, p.y + dir.y * 92 + dir.x * 78};
        DrawCircle(static_cast<int>(d.x), static_cast<int>(d.y + 2), 14, Color{0, 0, 0, 90});
        DrawCircle(static_cast<int>(d.x), static_cast<int>(d.y), 14, Color{245, 245, 240, 255});
        DrawCircleLines(static_cast<int>(d.x), static_cast<int>(d.y), 10, Color{180, 180, 180, 255});
        drawTextCentered("D", d, 14, Color{30, 30, 30, 255}, true);
    }
}

void PokerApp::drawBoard() {
    const float cw = 54, ch = 76, gap = 8;
    const float startX = kTableCenter.x - (5 * cw + 4 * gap) / 2;
    const float y = kTableCenter.y - ch / 2 + 6;

    for (std::size_t i = 0; i < 5; ++i) {
        const Rectangle slot{startX + static_cast<float>(i) * (cw + gap), y, cw, ch};
        if (i >= board_.size()) {
            DrawRectangleRoundedLinesEx(slot, 0.16f, 8, 1.5f, Color{255, 255, 255, 30});
            continue;
        }
        const float t = easeOut((now_ - boardDealtAt_[i]) / 0.35f);
        if (t <= 0.0f) continue;
        Rectangle r = slot;
        r.y -= (1.0f - t) * 30;
        const bool winning = std::find(winningCards_.begin(), winningCards_.end(), board_[i]) != winningCards_.end();
        const bool dim = !winningCards_.empty() && !winning;
        drawCard(board_[i], r, dim ? 0.55f * t : t, winning);
    }

    if (pot_ > 0) {
        drawChipStack({kTableCenter.x - 40, y - 26}, pot_, false);
        const std::string text = "Банк: " + formatChips(pot_);
        const Vector2 m = measureText(text, 17, true);
        const Rectangle pill{kTableCenter.x - 10, y - 40, m.x + 22, 26};
        DrawRectangleRounded(pill, 0.5f, 8, Color{0, 0, 0, 120});
        drawText(text, pill.x + 11, pill.y + 4, 17, theme::Text, true);
    }
}

void PokerApp::drawSeat(std::size_t i) {
    const SeatState& s = seats_[i];
    const Vector2 p = seatPosition(i);
    const bool human = i == 0;
    const bool topSide = p.y < kTableCenter.y - 60;
    const bool out = s.sittingOut || (!s.hasCards && handNumber_ > 0 && s.chips == 0);
    const bool inactive = s.folded || out;
    const float alpha = inactive ? 0.5f : 1.0f;

    const float plateW = human ? 236.0f : 192.0f;
    const Rectangle plate{p.x - plateW / 2, p.y - 29, plateW, 58};

    const bool reveal = !human && s.inHand() && (showdown_ || !runoutEquity_.empty());
    float cy = 0.0f, ch = 0.0f;
    if (s.hasCards && !s.folded) {
        const float t = easeOut((now_ - cardsDealtAt_[i]) / 0.3f);
        const float cw = human ? 66.0f : 40.0f;
        ch = human ? 92.0f : 56.0f;
        const float spread = human ? 24.0f : (reveal ? 15.0f : 10.0f);
        const float tilt = human ? 7.0f : 8.0f;
        cy = topSide ? plate.y + plate.height + 6 : plate.y - ch - 8;
        for (std::size_t k = 0; k < 2; ++k) {
            const float dir = k == 0 ? -1.0f : 1.0f;
            const Vector2 target{p.x + dir * spread, cy + ch / 2 + (human ? 4.0f : 2.0f)};
            const Vector2 pos = lerp(kTableCenter, target, t);
            if (t <= 0.0f) continue;
            const float angle = dir * tilt * t;
            if (human || reveal) {
                const bool winning = std::find(winningCards_.begin(), winningCards_.end(), s.hole[k]) != winningCards_.end();
                const bool dim = !winningCards_.empty() && !winning && showdown_;
                drawCardRotated(s.hole[k], pos, {cw, ch}, angle, dim ? 0.6f : 1.0f, winning);
            } else {
                drawCardBackRotated(pos, {cw, ch}, angle);
            }
        }
        if (i < runoutEquity_.size() && runoutEquity_[i] >= 0.0) {
            const std::string eq = formatPercent(runoutEquity_[i]);
            const Vector2 m = measureText(eq, 16, true);
            const Rectangle pill{p.x - m.x / 2 - 10, topSide ? cy + ch + 8 : cy - 32, m.x + 20, 24};
            DrawRectangleRounded(pill, 0.5f, 8, runoutEquity_[i] >= 0.5 ? theme::GoodGreen : Color{60, 70, 84, 255});
            drawText(eq, pill.x + 10, pill.y + 3, 16, WHITE, true);
        }
    }

    const bool myTurn = human && decision_.has_value();
    const bool winner = wonAmount_[i] > 0;
    if (myTurn || winner) {
        const float pulse = 0.5f + 0.5f * std::sin(now_ * 5.0f);
        DrawRectangleRounded({plate.x - 5, plate.y - 5, plate.width + 10, plate.height + 10}, 0.4f, 10,
                             withAlpha(theme::Gold, 0.25f + 0.35f * pulse));
    } else if (static_cast<int>(i) == lastActor_ && now_ - lastActorAt_ < 0.9f) {
        DrawRectangleRounded({plate.x - 4, plate.y - 4, plate.width + 8, plate.height + 8}, 0.4f, 10,
                             withAlpha(actionColor(actionType_[i]), 0.35f));
    }
    if (hovered(plate)) hoveredSeat_ = static_cast<int>(i);
    drawShadow(plate, 0.4f, 4);
    DrawRectangleRounded(plate, 0.4f, 10, withAlpha(Color{14, 18, 24, 235}, alpha + 0.3f));
    DrawRectangleRoundedLinesEx(plate, 0.4f, 10, 1.5f,
                                withAlpha(winner ? theme::Gold : Color{255, 255, 255, 40}, 1.0f));

    const Vector2 avatar{plate.x + 30, p.y};
    const Color ac = avatarColor(human ? "Людина" : s.strategy);
    DrawCircle(static_cast<int>(avatar.x), static_cast<int>(avatar.y), 23, withAlpha(lerpColor(ac, BLACK, 0.3f), alpha));
    DrawCircle(static_cast<int>(avatar.x), static_cast<int>(avatar.y - 1), 21, withAlpha(ac, alpha));
    drawTextCentered(firstLetter(s.name), {avatar.x, avatar.y - 1}, 20, withAlpha(WHITE, alpha), true);

    const float tx = plate.x + 62;
    drawText(s.name, tx, p.y - 22, 16, withAlpha(theme::Text, alpha), true);
    drawText(formatChips(s.chips), tx, p.y + 2, 17, withAlpha(theme::Gold, alpha), true);
    if (s.allIn && s.inHand()) {
        const Rectangle badge{plate.x + plate.width - 68, p.y + 2, 56, 20};
        DrawRectangleRounded(badge, 0.5f, 8, theme::BadRed);
        drawTextCentered("ОЛ-ІН", {badge.x + badge.width / 2, badge.y + 10}, 12, WHITE, true);
    } else if (out) {
        drawTextRight("вибув", plate.x + plate.width - 14, p.y + 4, 13, theme::Muted);
    } else if (!human) {
        drawTextRight(s.strategy, plate.x + plate.width - 14, p.y + 5, 12, withAlpha(theme::Muted, alpha), true);
    }

    const float labelY = topSide ? plate.y - 21 : plate.y + plate.height + 6;
    const std::string actionLabel = hasAction_[i] ? actionText(lastAction_[i]) : std::string();
    std::string label = handDescription_[i].empty() ? actionLabel : handDescription_[i];
    Color labelColor = handDescription_[i].empty() ? actionColor(actionType_[i]) : theme::Gold;
    if (winner) {
        label = "+" + formatChips(wonAmount_[i]) + (handDescription_[i].empty() ? "" : "  " + handDescription_[i]);
        labelColor = theme::Gold;
    }
    if (!label.empty()) {
        const Vector2 m = measureText(label, 14, true);
        const Rectangle pill{p.x - m.x / 2 - 10, labelY, m.x + 20, 24};
        DrawRectangleRounded(pill, 0.5f, 8, Color{0, 0, 0, 170});
        DrawRectangleRoundedLinesEx(pill, 0.5f, 8, 1.0f, withAlpha(labelColor, 0.6f));
        drawText(label, pill.x + 10, pill.y + 4, 14, labelColor, true);
    }

    if (s.streetBet > 0 && bannerWins_.empty()) {
        const Vector2 betPos = human ? Vector2{kTableCenter.x + 150, 478} : lerp(p, kTableCenter, topSide ? 0.57f : 0.43f);
        drawChipStack({betPos.x, betPos.y}, s.streetBet);
    }

}

void PokerApp::drawLog() {
    const Rectangle box{16, 600, 300, 100};
    DrawRectangleRounded(box, 0.12f, 8, Color{0, 0, 0, 90});
    float y = box.y + 8;
    for (const auto& entry : log_) {
        std::string line = entry.text;
        if (entry.seat >= 0 && static_cast<std::size_t>(entry.seat) < seats_.size()) {
            const std::string& name = seats_[static_cast<std::size_t>(entry.seat)].name;
            line = entry.won >= 0 ? name + " виграє " + formatChips(entry.won)
                                  : name + ": " + actionText(entry.action);
        }
        drawText(line, box.x + 12, y, 13, Color{200, 208, 216, 220});
        y += 15;
    }
    drawText("F фолд · C кол · R рейз · A ол-ін · T таблиця · Tab аналітика · B у ББ", 590, 684, 12, theme::Muted);
}

void PokerApp::drawActionBar() {
    const Rectangle bar{16, 710, 926, 82};
    drawPanel(bar, theme::Panel, 0.2f);

    if (!decision_) {
        if (handOver_ && !gameOver_) {
            if (button({bar.x + bar.width / 2 - 170, bar.y + 16, 340, 50}, "Наступна роздача  ▶", ButtonStyle::Gold,
                       true, false, 20)) {
                handOver_ = false;
                bridge_->continueToNextHand();
            }
            drawText("або Пробіл", bar.x + bar.width / 2 + 186, bar.y + 32, 14, theme::Muted);
        } else {
            std::string dots(static_cast<std::size_t>(static_cast<int>(now_ * 2.5f) % 4), '.');
            const bool humanIn = !seats_.empty() && seats_.front().inHand();
            const bool humanAllIn = humanIn && seats_.front().allIn;
            std::string status = humanAllIn ? "Ол-ін! Відкриваємо карти" : humanIn ? "Суперники думають" : "Ви поза роздачею — дивимось";
            if (gameOver_) status = "Гру завершено";
            drawText(status + dots, bar.x + 24, bar.y + 30, 18, theme::Muted);
        }
        return;
    }

    const DecisionContext& ctx = *decision_;
    const auto& rec = analysis_.hintsEnabled() ? analysis_.recommendation() : std::optional<Recommendation>{};
    const ActionType recType = rec ? rec->action.type : ActionType::PostBlind;
    const bool canRaise = ctx.maxRaiseTo() > ctx.highestBet;

    float x = bar.x + 18;
    const float y = bar.y + 18, h = 46;

    if (button({x, y, 120, h}, "Фолд", ButtonStyle::Danger, true, recType == ActionType::Fold)) submit(Action::fold());
    x += 132;

    const std::string callLabel = ctx.canCheck() ? "Чек" : "Кол " + formatChips(ctx.toCall);
    const bool recCall = recType == ActionType::Call || recType == ActionType::Check;
    if (decision_ && button({x, y, 156, h}, callLabel, ButtonStyle::Primary, true, recCall)) {
        submit(ctx.canCheck() ? Action::check() : Action::call());
    }
    x += 168;

    if (decision_) {
        const bool allInRaise = raiseAmount_ >= ctx.maxRaiseTo();
        const std::string raiseLabel = allInRaise ? "Ол-ін " + formatChips(ctx.maxRaiseTo())
                                                  : (ctx.highestBet == 0 ? "Бет " : "Рейз до ") + formatChips(raiseAmount_);
        const bool recRaise = recType == ActionType::Bet || recType == ActionType::Raise;
        if (button({x, y, 196, h}, raiseLabel, ButtonStyle::Gold, canRaise, recRaise)) {
            submit(allInRaise ? Action::allIn() : Action::raiseTo(raiseAmount_));
        }
    }
    x += 208;

    if (decision_ && button({x, y, 104, h}, "Ол-ін", ButtonStyle::Normal, true, recType == ActionType::AllIn)) {
        submit(Action::allIn());
    }
    x += 118;

    if (decision_ && canRaise) {
        const int minTo = std::min(ctx.minRaiseTo, ctx.maxRaiseTo());
        const int maxTo = ctx.maxRaiseTo();
        const Rectangle track{x + 4, y + 6, bar.x + bar.width - x - 24, 8};
        const float t = maxTo > minTo ? static_cast<float>(raiseAmount_ - minTo) / static_cast<float>(maxTo - minTo) : 1.0f;
        DrawRectangleRounded(track, 0.5f, 8, Color{255, 255, 255, 40});
        DrawRectangleRounded({track.x, track.y, track.width * t, track.height}, 0.5f, 8, theme::Gold);
        const Vector2 knob{track.x + track.width * t, track.y + track.height / 2};
        DrawCircleV(knob, 10, WHITE);
        DrawCircleV(knob, 6, theme::Gold);

        const Rectangle hit{track.x - 10, track.y - 12, track.width + 20, track.height + 24};
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hovered(hit)) draggingSlider_ = true;
        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) draggingSlider_ = false;
        if (draggingSlider_ && maxTo > minTo) {
            const float k = std::clamp((mouse().x - track.x) / track.width, 0.0f, 1.0f);
            const int step = std::max(1, settings_.smallBlind);
            raiseAmount_ = minTo + static_cast<int>(std::lround(k * static_cast<float>(maxTo - minTo) / static_cast<float>(step))) * step;
            if (k >= 0.999f) raiseAmount_ = maxTo;
            clampRaise();
        }

        const std::array<std::pair<const char*, double>, 4> presets = {{{"Мін", -1.0}, {"½ банку", 0.5}, {"¾", 0.75}, {"Банк", 1.0}}};
        const float pw = (track.width + 8) / 4.0f - 6;
        for (std::size_t k = 0; k < presets.size(); ++k) {
            const Rectangle pr{track.x - 4 + static_cast<float>(k) * (pw + 6), y + 22, pw, 26};
            if (button(pr, presets[k].first, ButtonStyle::Ghost, true, false, 13)) {
                raiseAmount_ = presets[k].second < 0 ? minTo : presetRaise(presets[k].second);
                clampRaise();
            }
        }
    }
}

void PokerApp::drawStatsTooltip() {
    if (hoveredSeat_ < 0 || static_cast<std::size_t>(hoveredSeat_) >= seats_.size()) return;
    const SeatState& s = seats_[static_cast<std::size_t>(hoveredSeat_)];
    const OpponentInfo& st = s.stats;
    auto pct = [](double v) { return std::to_string(static_cast<int>(std::lround(v * 100))) + "%"; };

    const std::vector<std::pair<std::string, std::string>> rows = {
        {"Роздач зіграно", std::to_string(st.hands)},
        {"VPIP (грає префлоп)", pct(st.vpip)},
        {"PFR (рейзить префлоп)", pct(st.pfr)},
        {"Агресивність", pct(st.aggression)},
        {"Фолд на ставку", pct(st.foldToBet)},
    };
    const std::string title = s.human ? "Так вас бачать боти" : s.name + " · " + s.strategy;
    const Rectangle box{std::min(mouse().x + 16, kWidth - 300), std::min(mouse().y + 16, kHeight - 190), 280, 170};
    DrawRectangleRounded(box, 0.08f, 8, Color{10, 14, 20, 240});
    DrawRectangleRoundedLinesEx(box, 0.08f, 8, 1.0f, withAlpha(theme::Gold, 0.6f));
    drawText(title, box.x + 14, box.y + 12, 15, theme::Gold, true);
    float y = box.y + 40;
    for (const auto& row : rows) {
        drawText(row.first, box.x + 14, y, 13, theme::Muted);
        drawTextRight(row.second, box.x + box.width - 14, y, 13, theme::Text, true);
        y += 21;
    }
    if (st.hands < 15) drawText("мало даних — значення наближені", box.x + 14, y + 2, 12, theme::Muted);
}

void PokerApp::drawBanner() {
    const std::vector<std::string> lines = bannerLines();
    if (lines.empty() || chartOpen_) return;
    const float y0 = kTableCenter.y + 64;
    float width = 0;
    for (const auto& l : lines) width = std::max(width, measureText(l, 17, true).x);
    const float h = static_cast<float>(lines.size()) * 24 + 16;
    const Rectangle box{kTableCenter.x - width / 2 - 24, y0, width + 48, h};
    DrawRectangleRounded(box, 0.3f, 10, Color{0, 0, 0, 190});
    DrawRectangleRoundedLinesEx(box, 0.3f, 10, 1.5f, withAlpha(theme::Gold, 0.7f));
    float y = y0 + 9;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const bool last = i + 1 == lines.size() && lines.size() > 1;
        drawTextCentered(lines[i], {kTableCenter.x, y + 11}, last ? 15.0f : 17.0f,
                         last ? theme::Text : theme::Gold, !last);
        y += 24;
    }
}

void PokerApp::drawGameOver() {
    DrawRectangle(0, 0, static_cast<int>(kWidth), static_cast<int>(kHeight), Color{0, 0, 0, 160});
    const Rectangle panel{390, 240, 500, 300};
    drawPanel(panel, Color{18, 24, 34, 250}, 0.06f);
    drawTextCentered(humanWon_ ? "ПЕРЕМОГА!" : "Ви вибули з гри", {kWidth / 2, panel.y + 60}, 34,
                     humanWon_ ? theme::Gold : theme::BadRed, true);
    const int chips = seats_.empty() ? 0 : seats_.front().chips;
    drawTextCentered("Зіграно роздач: " + std::to_string(handNumber_), {kWidth / 2, panel.y + 118}, 18, theme::Text);
    drawTextCentered("Результат: " + std::string(chips >= startingChips_ ? "+" : "") + formatChips(chips - startingChips_) + (showingBigBlinds() ? "" : " фішок"),
                     {kWidth / 2, panel.y + 148}, 18, chips >= startingChips_ ? theme::GoodGreen : theme::BadRed, true);
    if (button({panel.x + 40, panel.y + 210, 200, 50}, "Нова гра", ButtonStyle::Gold, true, false, 18)) startGame();
    if (button({panel.x + 260, panel.y + 210, 200, 50}, "Меню", ButtonStyle::Normal, true, false, 18)) {
        stopGame();
        screen_ = Screen::Menu;
    }
}

}
