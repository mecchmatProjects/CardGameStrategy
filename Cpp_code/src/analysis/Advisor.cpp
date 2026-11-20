#include "poker/analysis/Advisor.hpp"

#include "poker/analysis/HandRanking.hpp"
#include "poker/analysis/Odds.hpp"
#include "poker/analysis/PreflopChart.hpp"
#include "poker/eval/HandEvaluator.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace poker {

namespace {

constexpr int kShortStackBB = 12;

std::string signedChips(double value) {
    std::ostringstream os;
    os << std::showpos << std::fixed << std::setprecision(1) << value;
    return os.str();
}

std::string percentOfHands(double fraction) {
    std::ostringstream os;
    os << "топ " << std::fixed << std::setprecision(0) << std::min(100.0, fraction * 100.0) << "%";
    return os.str();
}

bool jamAllowed(const DecisionContext& ctx) {
    if (ctx.board.empty()) return ctx.effectiveStack <= 30 * ctx.bigBlind || ctx.raisesThisStreet >= 3;
    return ctx.effectiveStack <= 3 * std::max(1, ctx.pot);
}

double futureValue(const DecisionContext& ctx, double equity, int invest, int potAfter) {
    if (ctx.board.size() == 5) return 0.0;
    const int remaining = ctx.effectiveStack - invest;
    if (remaining <= 0) return 0.0;
    const double streetsLeft = ctx.board.empty() ? 1.0 : (ctx.board.size() == 3 ? 0.8 : 0.5);
    return (2.0 * equity - 1.0) * std::min<double>(remaining, potAfter) * 0.25 * streetsLeft;
}

double loosenessOf(const DecisionContext& ctx) { return std::clamp(ctx.opponentsVpip / 0.28, 0.6, 2.4); }

}

Advisor::Advisor(std::uint64_t seed, int iterations, AdvisorStyle style)
    : calculator_(seed), rng_(seed ^ 0xA5A5A5A5ULL), iterations_(iterations), style_(style) {}

RangeModel Advisor::opponentRange(const DecisionContext& ctx) const {
    const double loose = loosenessOf(ctx);
    RangeModel range;
    const int raises = ctx.preflopRaises;

    if (ctx.board.empty()) {
        if (raises == 0) range.top = 1.0;
        else if (raises == 1) range.top = 0.20 * loose;
        else if (raises == 2) range.top = 0.07 * loose;
        else range.top = 0.035 * loose;
    } else {
        if (raises == 0) range.top = 0.75 * loose;
        else if (ctx.preflopAggressor) range.top = 0.30 * loose;
        else if (raises == 1) range.top = 0.22 * loose;
        else range.top = 0.08 * loose;

        range.showsStrength = ctx.toCall > 0;
        range.bluffShare = 0.15 + 0.5 * ctx.opponentsAggression;
        if (ctx.raisesThisStreet >= 2) {
            range.bluffShare *= 0.5;
            range.top *= 0.7;
        }
    }
    range.top = std::clamp(range.top, 0.02, 1.0);
    return range;
}

double Advisor::foldProbability(const DecisionContext& ctx, int  , int callAmount) const {
    const double potBefore = ctx.pot + ctx.toCall;
    const double alpha = callAmount / (potBefore + callAmount);
    const double tendency = std::clamp(1.15 - 2.2 * (ctx.opponentsVpip - 0.28), 0.25, 1.15);
    const bool facingAggression = ctx.raisesThisStreet >= 1;
    double ceiling = 0.0;
    if (ctx.board.empty()) ceiling = facingAggression ? 0.80 : 0.92;
    else ceiling = facingAggression ? 0.55 : 0.70;
    const double one = alpha * tendency * (facingAggression ? 0.8 : 1.0) * style_.bluffConfidence;
    return std::clamp(one, 0.02, ceiling);
}

double Advisor::realization(const DecisionContext& ctx, bool aggressor) const {
    if (ctx.board.size() == 5) return 1.0;
    double r = ctx.position == Position::Late ? 0.96 : 0.86;
    if (aggressor) r += 0.05;
    return std::min(1.0, r);
}

Recommendation Advisor::advise(const DecisionContext& input) {
    DecisionContext ctx = input;
    if (ctx.effectiveStack <= 0) ctx.effectiveStack = ctx.stack;
    if (ctx.board.empty() && ctx.highestBet > ctx.bigBlind && ctx.preflopRaises == 0) {
        ctx.preflopRaises = 1;
        ctx.raisesThisStreet = std::max(ctx.raisesThisStreet, 1);
    }
    if (!ctx.board.empty() && ctx.toCall > 0 && ctx.raisesThisStreet == 0) ctx.raisesThisStreet = 1;

    Recommendation rec;
    const RangeModel range = opponentRange(ctx);
    const int opponents = std::clamp(ctx.activeOpponents, 1, 9);
    rec.rangeTop = range.top;
    rec.equity = calculator_.vsRange(ctx.hole, ctx.board, opponents, range, iterations_).equity;
    rec.requiredEquity = potOdds(ctx.toCall, ctx.pot);
    rec.callEv = ctx.toCall > 0 ? callEV(rec.equity, ctx.pot, ctx.toCall) : 0.0;

    if (ctx.board.size() >= 3) {
        rec.handSummary = evaluateHand(ctx.hole, ctx.board).describe();
        if (ctx.board.size() < 5) rec.outs = static_cast<int>(countOuts(ctx.hole, ctx.board).outs.size());
    } else {
        rec.handSummary = handCode(ctx.hole) + " (" + percentOfHands(handPercentile(ctx.hole)) + " рук)";
    }

    const bool unopenedPreflop = ctx.board.empty() && ctx.raisesThisStreet == 0;
    const bool shortStack = ctx.effectiveStack <= kShortStackBB * ctx.bigBlind;
    if (unopenedPreflop && !shortStack) {
        preflopOpen(ctx, rec);
        return rec;
    }

    evaluateOptions(ctx, rec, range);
    choose(ctx, rec);
    return rec;
}

void Advisor::preflopOpen(const DecisionContext& ctx, Recommendation& rec) {
    const double percentile = handPercentile(ctx.hole);
    const double range = openingRange(ctx.position, ctx.playersInHand) * style_.openLooseness;
    const int limpers = std::max(0, (ctx.pot - ctx.bigBlind - ctx.bigBlind / 2) / std::max(1, ctx.bigBlind));

    std::ostringstream why;
    why << handCode(ctx.hole) << " — " << percentOfHands(percentile) << " стартових рук. З позиції «"
        << positionName(ctx.position) << "» (" << ctx.playersInHand << " гравців) відкриваємо "
        << percentOfHands(range) << ". ";

    if (percentile <= range) {
        const int target = ctx.bigBlind * 5 / 2 + limpers * ctx.bigBlind;
        rec.action = sizedBet(ctx, target);
        why << "Рука в діапазоні — рейз " << (limpers > 0 ? "2.5 ББ + 1 ББ за кожного лімпера." : "2.5 ББ.");
    } else if (ctx.canCheck()) {
        rec.action = Action::check();
        why << "Поза діапазоном, але можна безкоштовно побачити флоп — чек.";
    } else if (ctx.position == Position::Blinds && ctx.toCall <= ctx.bigBlind / 2 && percentile <= range * 2.0) {
        rec.action = Action::call();
        why << "Дешево доставити з малого блайнду — кол.";
    } else {
        rec.action = Action::fold();
        why << "Рука поза діапазоном відкриття — фолд.";
    }
    rec.reasoning = why.str();
}

void Advisor::evaluateOptions(const DecisionContext& ctx, Recommendation& rec, const RangeModel& range) {
    const double eq = rec.equity;
    const int pot = ctx.pot;
    const int toCall = ctx.toCall;
    auto add = [&](const Action& a, double ev) {
        for (const auto& o : rec.options) {
            if (o.action.type == a.type && o.action.amount == a.amount) return;
        }
        rec.options.push_back({a, ev, optionLabel(a, ctx)});
    };

    if (toCall > 0) {
        add(Action::fold(), 0.0);
        const bool callAllIn = toCall >= ctx.stack;
        const double r = callAllIn ? 1.0 : realization(ctx, false);
        add(Action::call(), eq * r * (pot + toCall) - toCall + (callAllIn ? 0.0 : futureValue(ctx, eq, toCall, pot + 2 * toCall)));
    } else {
        add(Action::check(), eq * pot * realization(ctx, false) + futureValue(ctx, eq, 0, pot));
    }

    const bool canRaise = ctx.maxRaiseTo() > ctx.highestBet && ctx.effectiveStack > toCall;
    if (!canRaise) return;

    std::vector<int> targets;
    const bool shortStack = ctx.effectiveStack <= kShortStackBB * ctx.bigBlind;
    if (shortStack && ctx.board.empty()) {
        targets.push_back(ctx.maxRaiseTo());
    } else if (ctx.highestBet == 0 || ctx.highestBet == ctx.currentBet) {
        for (const double f : {0.33, 0.5, 0.75, 1.0}) {
            targets.push_back(ctx.highestBet + std::max(ctx.bigBlind, static_cast<int>(std::lround(pot * f))));
        }
        if (jamAllowed(ctx)) targets.push_back(ctx.maxRaiseTo());
    } else if (ctx.board.empty()) {
        if (ctx.raisesThisStreet <= 1) targets.push_back(ctx.highestBet * 3);
        else if (ctx.raisesThisStreet == 2) targets.push_back(ctx.highestBet * 23 / 10);
        if (jamAllowed(ctx)) targets.push_back(ctx.maxRaiseTo());
    } else {
        targets.push_back(ctx.highestBet * 5 / 2);
        targets.push_back(ctx.highestBet + pot + toCall);
        if (jamAllowed(ctx)) targets.push_back(ctx.maxRaiseTo());
    }

    for (const int target : targets) {
        const Action action = sizedBet(ctx, target);
        const int total = action.type == ActionType::AllIn ? ctx.maxRaiseTo() : action.amount;
        const int invest = std::min(total - ctx.currentBet, ctx.effectiveStack);
        const int call = std::max(0, invest - toCall);
        const double foldOne = foldProbability(ctx, invest, call);
        const double foldAll = std::pow(foldOne, std::max(1, ctx.activeOpponents));

        RangeModel calling = range;
        calling.top = std::max(0.015, range.top * std::max(0.12, 1.0 - foldOne));
        if (!ctx.board.empty()) {
            calling.showsStrength = true;
            calling.bluffShare = std::min(range.bluffShare, 0.2);
        }
        const double equityWhenCalled =
            calculator_.vsRange(ctx.hole, ctx.board, std::clamp(ctx.activeOpponents, 1, 9), calling,
                                std::max(300, iterations_ / 2)).equity;

        const bool isFinal = action.type == ActionType::AllIn || ctx.board.size() == 5;
        const double r = isFinal ? 1.0 : realization(ctx, true);
        double whenCalled = equityWhenCalled * r * (pot + invest + call) - invest;
        if (!isFinal) whenCalled += futureValue(ctx, equityWhenCalled, invest, pot + invest + call);
        add(action, foldAll * pot + (1.0 - foldAll) * whenCalled);
    }
}

void Advisor::choose(const DecisionContext& ctx, Recommendation& rec) {
    std::size_t best = 0;
    for (std::size_t i = 1; i < rec.options.size(); ++i) {
        if (rec.options[i].ev > rec.options[best].ev) best = i;
    }

    std::size_t chosen = best;
    if (style_.temperature > 0.0) {
        const double scale = style_.temperature * ctx.bigBlind;
        std::vector<double> weights;
        double sum = 0.0;
        for (const auto& o : rec.options) {
            weights.push_back(std::exp((o.ev - rec.options[best].ev) / scale));
            sum += weights.back();
        }
        double pick = rng_.uniform01() * sum;
        for (std::size_t i = 0; i < weights.size(); ++i) {
            pick -= weights[i];
            if (pick <= 0) {
                chosen = i;
                break;
            }
        }
    }
    rec.action = rec.options[chosen].action;

    std::ostringstream why;
    why << "Діапазон суперників ≈ " << percentOfHands(rec.rangeTop) << " рук";
    if (ctx.preflopRaises > 0) why << " (рейзів префлоп: " << ctx.preflopRaises << ")";
    why << ", вони грають " << std::fixed << std::setprecision(0) << ctx.opponentsVpip * 100 << "% роздач. ";
    why << "Еквіті проти нього " << percentString(rec.equity) << ". ";
    if (ctx.effectiveStack <= kShortStackBB * ctx.bigBlind && ctx.board.empty()) {
        why << "Короткий стек (" << ctx.effectiveStack / std::max(1, ctx.bigBlind) << " ББ) — граємо ол-ін або фолд. ";
    }
    why << "Обираємо дію з найбільшим EV: " << rec.options[best].label << " ("
        << signedChips(rec.options[best].ev) << ").";
    rec.reasoning = why.str();
}

}
