#include "poker/gui/AnalysisPanel.hpp"

#include "poker/analysis/HandRanking.hpp"
#include "poker/analysis/PreflopChart.hpp"
#include "poker/eval/HandEvaluator.hpp"
#include "poker/gui/Render.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace poker::gui {

namespace {

Color tierColor(PreflopTier tier) {
    switch (tier) {
        case PreflopTier::Premium: return {196, 96, 236, 255};
        case PreflopTier::Strong: return {236, 92, 88, 255};
        case PreflopTier::Playable: return {246, 196, 72, 255};
        case PreflopTier::Speculative: return {88, 196, 120, 255};
        case PreflopTier::Trash: return {96, 106, 120, 255};
    }
    return GRAY;
}

Color equityColor(double equity) {
    if (equity >= 0.5) return theme::GoodGreen;
    if (equity >= 0.3) return theme::Gold;
    return theme::BadRed;
}

std::string actionLabel(const Action& action, const DecisionContext& ctx) {
    switch (action.type) {
        case ActionType::Fold: return "ФОЛД";
        case ActionType::Check: return "ЧЕК";
        case ActionType::Call: return "КОЛ " + formatChips(ctx.toCall);
        case ActionType::Bet: return "БЕТ " + formatChips(action.amount);
        case ActionType::Raise: return "РЕЙЗ ДО " + formatChips(action.amount);
        case ActionType::AllIn: return "ОЛ-ІН";
        case ActionType::PostBlind: return "БЛАЙНД";
    }
    return "?";
}

std::uint64_t hashKey(const HoleCards& hole, const std::vector<Card>& board, int opponents, bool folded) {
    std::uint64_t key = hole[0].mask() | hole[1].mask() | toMask(board);
    key ^= static_cast<std::uint64_t>(board.size()) << 56;
    key ^= static_cast<std::uint64_t>(opponents) << 60;
    key ^= folded ? 0x8000000000000000ULL : 0;
    return key;
}

void sectionTitle(const std::string& title, float x, float& y, float width) {
    drawText(title, x, y, 13, theme::Muted, true);
    DrawRectangleRec({x, y + 19, width, 1}, Color{255, 255, 255, 25});
    y += 26;
}

}

AnalysisPanel::AnalysisPanel(std::uint64_t seed) : advisor_(seed, 3000, 0.0), calculator_(seed ^ 0xABCDEFULL) {}

void AnalysisPanel::update(const std::vector<SeatState>& seats, const std::vector<Card>& board, int dealer) {
    if (seats.empty() || !seats.front().hasCards) {
        hasHand_ = false;
        position_.reset();
        lastKey_ = 0;
        return;
    }
    const SeatState& me = seats.front();
    hasHand_ = true;
    hole_ = me.hole;
    board_ = board;
    folded_ = me.folded;
    opponents_ = static_cast<int>(std::count_if(seats.begin() + 1, seats.end(),
                                                [](const SeatState& s) { return s.inHand(); }));
    playersDealt_ = std::max(2, static_cast<int>(std::count_if(seats.begin(), seats.end(),
                                                               [](const SeatState& s) { return s.hasCards; })));

    if (dealer >= 0) {
        std::vector<int> order;
        const int n = static_cast<int>(seats.size());
        for (int step = 0; step < n; ++step) {
            const int seat = (dealer + step) % n;
            if (seats[static_cast<std::size_t>(seat)].hasCards) order.push_back(seat);
        }
        const auto it = std::find(order.begin(), order.end(), 0);
        if (it != order.end()) {
            position_ = positionFromButton(static_cast<int>(it - order.begin()), static_cast<int>(order.size()));
            if (!decision_) chartPosition_ = *position_;
        }
    }

    const std::uint64_t key = hashKey(hole_, board_, opponents_, folded_);
    if (key == lastKey_) return;
    lastKey_ = key;

    if (!folded_ && opponents_ > 0) {
        equity_ = calculator_.vsRandom(hole_, board_, std::min(opponents_, 9), 4000).equity;
    }
    outs_ = (board_.size() == 3 || board_.size() == 4) ? countOuts(hole_, board_) : OutsInfo{};
}

void AnalysisPanel::onDecision(const DecisionContext& ctx) {
    decision_ = ctx;
    recommendation_ = advisor_.advise(ctx);
    equity_ = recommendation_->equity;
    chartPosition_ = ctx.position;
}

void AnalysisPanel::clearDecision() {
    decision_.reset();
    recommendation_.reset();
}

void AnalysisPanel::openChart() {
    if (position_) chartPosition_ = *position_;
}

std::vector<double> AnalysisPanel::runoutEquity(const std::vector<SeatState>& seats,
                                                const std::vector<Card>& board) {
    std::vector<double> result(seats.size(), -1.0);
    std::vector<HoleCards> hands;
    std::vector<std::size_t> index;
    for (std::size_t i = 0; i < seats.size(); ++i) {
        if (!seats[i].inHand()) continue;
        hands.push_back(seats[i].hole);
        index.push_back(i);
    }
    if (hands.size() < 2) return result;
    const EquityReport report = calculator_.calculate(hands, board, 20000, 300000);
    for (std::size_t k = 0; k < index.size(); ++k) result[index[k]] = report.players[k].equity;
    return result;
}

bool AnalysisPanel::draw(Rectangle area, bool myTurn, bool handFinished) {
    drawPanel(area, theme::Panel, 0.04f);
    const float x = area.x + 18;
    const float width = area.width - 36;
    float y = area.y + 16;

    drawText("АНАЛІТИКА", x, y, 20, theme::Text, true);
    const Rectangle hintsButton{area.x + area.width - 128, y - 3, 110, 28};
    if (button(hintsButton, hints_ ? "Підказки ✓" : "Підказки ✕",
               hints_ ? ButtonStyle::Primary : ButtonStyle::Normal, true, false, 13)) {
        toggleHints();
    }
    y += 40;

    sectionTitle("ВАША РУКА", x, y, width);
    if (!hasHand_) {
        drawText("Очікування роздачі…", x, y, 15, theme::Muted);
        y += 30;
    } else {
        drawCard(hole_[0], {x, y, 42, 60});
        drawCard(hole_[1], {x + 46, y, 42, 60});
        const PreflopTier tier = preflopTier(hole_);
        const float tx = x + 104;
        drawText(handCode(hole_), tx, y - 2, 24, theme::Text, true);
        const std::string tierText = tierName(tier);
        const Vector2 tm = measureText(tierText, 12, true);
        const Rectangle badge{tx, y + 30, tm.x + 14, 20};
        DrawRectangleRounded(badge, 0.5f, 8, tierColor(tier));
        drawText(tierText, badge.x + 7, badge.y + 3, 12, Color{20, 20, 24, 255}, true);
        drawText("топ " + std::to_string(static_cast<int>(std::lround(handPercentile(hole_) * 100))) + "% рук",
                 badge.x + badge.width + 8, badge.y + 2, 13, theme::Muted);
        y += 66;
        if (board_.size() >= 3) {
            drawText(evaluateHand(hole_, board_).describe(), x, y, 15, theme::Gold, true);
            y += 22;
        }
        if (position_) {
            drawText("Позиція: " + positionName(*position_), x, y, 13, theme::Muted);
            y += 20;
        }
    }
    y += 6;

    if (!hasHand_) return button({x, area.y + area.height - 52, width, 38}, "Префлоп-таблиця  [T]",
                                 ButtonStyle::Normal, true, false, 15);

    if (handFinished) {
        drawTextWrapped("Роздачу завершено. Натисніть «Наступна роздача» або Пробіл.", {x, y, width, 60}, 14,
                        theme::Muted);
    } else if (folded_) {
        drawText("Ви скинули карти в цій роздачі.", x, y, 14, theme::Muted);
        y += 24;
    } else if (!hints_) {
        drawTextWrapped("Підказки вимкнено. Натисніть «Підказки» або H, щоб бачити еквіті, шанси банку "
                        "та оптимальну дію.",
                        {x, y, width, 80}, 14, theme::Muted);
    } else {
        const int opp = myTurn && decision_ ? std::max(1, decision_->activeOpponents) : opponents_;
        const std::string against = (myTurn && recommendation_)
                                        ? "діапазону (топ " + std::to_string(static_cast<int>(std::lround(recommendation_->rangeTop * 100))) + "%)"
                                        : std::to_string(opp) + " суперн., будь-які руки";
        sectionTitle("ЕКВІТІ  ·  проти " + against, x, y, width);
        const double required = (myTurn && recommendation_) ? recommendation_->requiredEquity : -1.0;
        drawText(formatPercent(equity_), x, y - 2, 34, equityColor(equity_), true);
        if (required > 0) {
            drawTextRight("потрібно " + formatPercent(required), x + width, y + 14, 14, theme::Text);
        }
        y += 44;
        drawMeter({x, y, width, 12}, equity_, equityColor(equity_), required > 0 ? required : -1.0);
        y += 26;

        if (myTurn && decision_ && recommendation_ && decision_->toCall > 0) {
            sectionTitle("ШАНСИ БАНКУ", x, y, width);
            drawText("Кол " + formatChips(decision_->toCall) + " у банк " + formatChips(decision_->pot), x, y, 15,
                     theme::Text);
            y += 22;
            const double ev = recommendation_->callEv;
            const bool good = ev >= 0;
            drawText("EV колу: " + formatSigned(ev), x, y, 15, good ? theme::GoodGreen : theme::BadRed, true);
            drawTextRight(good ? "вигідно" : "невигідно", x + width, y + 1, 14,
                          good ? theme::GoodGreen : theme::BadRed);
            y += 30;
        }

        if (board_.size() == 3 || board_.size() == 4) {
            sectionTitle("АУТИ: " + std::to_string(outs_.outs.size()), x, y, width);
            if (outs_.outs.empty()) {
                drawText("Немає карт, що покращують руку", x, y, 14, theme::Muted);
                y += 22;
            } else {
                const float cw = 33, ch = 22;
                const int perRow = static_cast<int>(width / (cw + 4));
                const std::size_t shown = std::min<std::size_t>(outs_.outs.size(), static_cast<std::size_t>(perRow * 2));
                for (std::size_t i = 0; i < shown; ++i) {
                    const auto col = static_cast<float>(static_cast<int>(i) % perRow);
                    const auto row = static_cast<float>(static_cast<int>(i) / perRow);
                    drawMiniCard(outs_.outs[i], {x + col * (cw + 4), y + row * (ch + 4), cw, ch});
                }
                y += (shown > static_cast<std::size_t>(perRow) ? 2.0f : 1.0f) * (ch + 4) + 4;
                const std::string odds = board_.size() == 3
                                             ? "Наступна карта " + formatPercent(outs_.nextCard) +
                                                   " · до рівера " + formatPercent(outs_.byRiver)
                                             : "На рівері: " + formatPercent(outs_.nextCard);
                drawText(odds, x, y, 13, theme::Muted);
                y += 24;
            }
        }

        if (myTurn && recommendation_ && decision_) {
            sectionTitle("ОПТИМАЛЬНА ДІЯ", x, y, width);
            const Rectangle box{x - 6, y - 4, width + 12, area.y + area.height - 64 - (y - 4)};
            DrawRectangleRounded(box, 0.05f, 8, Color{246, 196, 72, 26});
            DrawRectangleRoundedLinesEx(box, 0.05f, 8, 1.0f, withAlpha(theme::Gold, 0.5f));
            drawText("★ " + actionLabel(recommendation_->action, *decision_), x + 4, y + 4, 22, theme::Gold, true);
            y += 36;
            if (!recommendation_->options.empty()) {
                double bestEv = recommendation_->options.front().ev;
                for (const auto& o : recommendation_->options) bestEv = std::max(bestEv, o.ev);
                for (const auto& o : recommendation_->options) {
                    const bool best = o.ev >= bestEv - 1e-9;
                    std::string label = actionLabel(o.action, *decision_);
                    for (const auto& [from, to] : {std::pair<std::string, std::string>{"ФОЛД", "фолд"}, {"ЧЕК", "чек"},
                                                  {"КОЛ", "кол"}, {"БЕТ", "бет"}, {"РЕЙЗ ДО", "рейз до"}, {"ОЛ-ІН", "ол-ін"}}) {
                        if (label.rfind(from, 0) == 0) label = to + label.substr(from.size());
                    }
                    if (o.action.type == ActionType::AllIn) label += " " + formatChips(decision_->maxRaiseTo());
                    drawText(label, x + 6, y, 13, best ? theme::Gold : Color{210, 216, 224, 255}, best);
                    drawTextRight(formatSigned(o.ev), x + width - 4, y, 13, o.ev >= 0 ? theme::GoodGreen : theme::BadRed, best);
                    y += 18;
                }
                y += 6;
            }
            drawTextWrapped(recommendation_->reasoning, {x + 4, y, width - 8, box.y + box.height - y - 6}, 12,
                            Color{200, 208, 216, 255});
        } else if (!myTurn) {
            drawText("Порада з'явиться, коли буде ваш хід.", x, y, 13, theme::Muted);
        }
    }

    return button({x, area.y + area.height - 52, width, 38}, "Префлоп-таблиця  [T]", ButtonStyle::Normal,
                  true, false, 15);
}

bool AnalysisPanel::drawPreflopChart() {
    DrawRectangle(0, 0, static_cast<int>(kWidth), static_cast<int>(kHeight), Color{0, 0, 0, 170});
    const Rectangle panel{150, 60, 980, 690};
    drawPanel(panel, Color{18, 24, 34, 250}, 0.03f);

    drawText("Префлоп-таблиця стартових рук", panel.x + 28, panel.y + 22, 26, theme::Text, true);
    drawText("Колір — група за формулою Чена · яскраві клітинки — руки, які порадник відкриває рейзом з обраної позиції (" +
                 std::to_string(playersDealt_) + " гравців)",
             panel.x + 28, panel.y + 58, 14, theme::Muted);

    const PreflopChart chart = buildPreflopChart();
    const float cell = 44.0f, cellH = 40.0f;
    const float gx = panel.x + 28, gy = panel.y + 92;

    int myRow = -1, myCol = -1;
    if (hasHand_) {
        const int a = 14 - hole_[0].rankValue();
        const int b = 14 - hole_[1].rankValue();
        const int hi = std::min(a, b), lo = std::max(a, b);
        const bool suited = hole_[0].suit() == hole_[1].suit();
        if (hi == lo) {
            myRow = myCol = hi;
        } else if (suited) {
            myRow = hi, myCol = lo;
        } else {
            myRow = lo, myCol = hi;
        }
    }

    for (int row = 0; row < kNumRanks; ++row) {
        for (int col = 0; col < kNumRanks; ++col) {
            const ChartCell& c = chart[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
            const Rectangle r{gx + static_cast<float>(col) * cell, gy + static_cast<float>(row) * cellH, cell - 3, cellH - 3};
            const int highRank = 14 - std::min(row, col);
            const int lowRank = 14 - std::max(row, col);
            const bool cellSuited = row < col;
            const HoleCards rep{Card(static_cast<Rank>(highRank), Suit::Spades),
                                Card(static_cast<Rank>(lowRank), cellSuited ? Suit::Spades : Suit::Hearts)};
            const bool open = shouldOpenHand(rep, chartPosition_, playersDealt_);
            Color fill = tierColor(c.tier);
            if (!open) fill = lerpColor(fill, Color{20, 24, 30, 255}, 0.72f);
            DrawRectangleRounded(r, 0.2f, 6, fill);
            drawTextCentered(c.code, {r.x + r.width / 2, r.y + r.height / 2}, 13,
                             open ? Color{18, 18, 22, 255} : Color{150, 156, 166, 255}, true);
            if (row == myRow && col == myCol) {
                const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 4.0f);
                DrawRectangleRoundedLinesEx({r.x - 3, r.y - 3, r.width + 6, r.height + 6}, 0.25f, 6,
                                            3.0f + pulse * 1.5f, WHITE);
            }
        }
    }

    const float rx = gx + 13 * cell + 26;
    float y = gy;
    drawText("ПОЗИЦІЯ", rx, y, 13, theme::Muted, true);
    y += 22;
    const Position positions[] = {Position::Early, Position::Middle, Position::Late, Position::Blinds};
    const char* labels[] = {"Рання", "Середня", "Пізня", "Блайнди"};
    for (const Position p : positions) {
        const std::string label = labels[static_cast<int>(p)];
        const bool mine = position_ && *position_ == p;
        if (button({rx, y, 180, 32}, label + (mine ? "  (ви)" : ""),
                   chartPosition_ == p ? ButtonStyle::Primary : ButtonStyle::Normal, true, false, 14)) {
            chartPosition_ = p;
        }
        y += 38;
    }

    y += 10;
    drawText("ГРУПИ РУК", rx, y, 13, theme::Muted, true);
    y += 22;
    const PreflopTier tiers[] = {PreflopTier::Premium, PreflopTier::Strong, PreflopTier::Playable,
                                 PreflopTier::Speculative, PreflopTier::Trash};
    for (const PreflopTier t : tiers) {
        DrawRectangleRounded({rx, y + 2, 16, 16}, 0.3f, 6, tierColor(t));
        drawText(tierName(t), rx + 24, y, 15, theme::Text);
        y += 24;
    }

    if (hasHand_) {
        y += 14;
        drawText("ВАША РУКА", rx, y, 13, theme::Muted, true);
        y += 22;
        drawCard(hole_[0], {rx, y, 44, 62});
        drawCard(hole_[1], {rx + 48, y, 44, 62});
        const PreflopTier tier = preflopTier(hole_);
        drawText(handCode(hole_), rx + 104, y + 4, 22, theme::Text, true);
        drawText(tierName(tier), rx + 104, y + 34, 14, tierColor(tier), true);
        y += 74;
        const bool open = shouldOpenHand(hole_, chartPosition_, playersDealt_);
        const int pct = static_cast<int>(std::lround(handPercentile(hole_) * 100));
        const int range = static_cast<int>(std::lround(openingRange(chartPosition_, playersDealt_) * 100));
        drawTextWrapped("Ваша рука — топ " + std::to_string(pct) + "%, діапазон відкриття — топ " +
                            std::to_string(range) + "%. " + (open ? "Рейз." : "Фолд (або чек)."),
                        {rx, y, 190, 80}, 14, open ? theme::GoodGreen : theme::BadRed, true);
    }

    const Rectangle close{panel.x + panel.width - 150, panel.y + panel.height - 56, 124, 38};
    const bool closePressed = button(close, "Закрити", ButtonStyle::Primary, true, false, 16);
    const bool clickedOutside = IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && !hovered(panel);
    return !(closePressed || clickedOutside || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_T));
}

}
