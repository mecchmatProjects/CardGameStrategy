#pragma once

#include "poker/analysis/ExpertAdvisor.hpp"
#include "poker/analysis/EquityCalculator.hpp"
#include "poker/analysis/Odds.hpp"
#include "poker/gui/GameBridge.hpp"

#include "raylib.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace poker::gui {

class AnalysisPanel {
public:
    explicit AnalysisPanel(std::uint64_t seed);

    void update(const std::vector<SeatState>& seats, const std::vector<Card>& board, int dealer);

    void onDecision(const DecisionContext& ctx);
    void clearDecision();

    bool draw(Rectangle area, bool myTurn, bool handFinished = false);

    bool drawPreflopChart();
    void openChart();

    std::vector<double> runoutEquity(const std::vector<SeatState>& seats, const std::vector<Card>& board);

    bool hintsEnabled() const { return hints_; }
    void toggleHints() { hints_ = !hints_; }
    const std::optional<Recommendation>& recommendation() const { return recommendation_; }
    std::optional<Position> humanPosition() const { return position_; }

private:
    ExpertAdvisor advisor_;
    EquityCalculator calculator_;

    bool hints_ = true;
    bool hasHand_ = false;
    bool folded_ = false;
    HoleCards hole_{};
    std::vector<Card> board_;
    int opponents_ = 0;
    int playersDealt_ = 6;
    std::optional<Position> position_;

    double equity_ = 0.0;
    OutsInfo outs_;
    std::optional<Recommendation> recommendation_;
    std::optional<DecisionContext> decision_;

    std::uint64_t lastKey_ = 0;

    Position chartPosition_ = Position::Middle;
};

}
