#include "poker/analysis/ExpertAdvisor.hpp"

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

std::string percent0(double fraction) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(0) << std::min(100.0, fraction * 100.0) << "%";
    return os.str();
}

DecisionContext normalize(const DecisionContext& input) {
    DecisionContext ctx = input;
    if (ctx.effectiveStack <= 0) ctx.effectiveStack = ctx.stack;
    if (ctx.opponents.empty()) {
        for (int i = 0; i < std::max(1, ctx.activeOpponents); ++i) {
            OpponentInfo o;
            o.seat = -2 - i;
            o.chips = ctx.effectiveStack;
            o.vpip = ctx.opponentsVpip;
            o.aggression = ctx.opponentsAggression;
            ctx.opponents.push_back(o);
        }
    }
    if (ctx.history.empty()) {
        const int raiser = ctx.opponents.front().seat;
        if (ctx.board.empty() && ctx.highestBet > ctx.bigBlind) {
            const int raises = std::max(1, ctx.preflopRaises);
            for (int k = 0; k < raises; ++k) {
                ctx.history.push_back({raiser, Street::Preflop, ActionType::Raise, ctx.highestBet, true, ctx.pot, ctx.bigBlind});
            }
            ctx.preflopRaises = raises;
            ctx.raisesThisStreet = std::max(ctx.raisesThisStreet, raises);
        } else if (!ctx.board.empty() && ctx.toCall > 0) {
            ctx.history.push_back({raiser, ctx.street, ActionType::Bet, ctx.highestBet, true, ctx.pot - ctx.toCall, 0});
            ctx.raisesThisStreet = std::max(ctx.raisesThisStreet, 1);
        }
    }
    return ctx;
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

bool raisedOn(const DecisionContext& ctx, int seat, Street street) {
    return std::any_of(ctx.history.begin(), ctx.history.end(),
                       [&](const ActionRecord& r) { return r.seat == seat && r.street == street && r.raised; });
}

}

ExpertAdvisor::ExpertAdvisor(std::uint64_t seed, int iterations, double temperature, double openLooseness,
                             double pressure)
    : calculator_(seed),
      rng_(seed ^ 0x51ED5EEDULL),
      iterations_(iterations),
      temperature_(temperature),
      openLooseness_(openLooseness),
      pressure_(pressure) {}

std::vector<OpponentRange> ExpertAdvisor::opponentRanges(const DecisionContext& ctx) const {
    std::vector<OpponentRange> ranges;
    for (const OpponentInfo& opp : ctx.opponents) {
        OpponentRange r;
        r.bluffShare = std::clamp(0.08 + 0.55 * opp.aggression, 0.05, 0.5);
        r.aggressive = std::clamp((opp.aggression - 0.35) / 0.45, 0.0, 1.0);

        const bool readsStrength = opp.aggression < 0.5;
        int raisesSoFar = 0;
        bool bet[5] = {}, checked[5] = {};
        for (const ActionRecord& a : ctx.history) {
            const auto st = static_cast<std::size_t>(a.street);
            if (a.street == Street::Preflop) {
                if (a.raised) ++raisesSoFar;
                if (a.seat != opp.seat) continue;
                if (a.raised) {
                    const bool bigShove = a.type == ActionType::AllIn && a.amount >= 15 * ctx.bigBlind;
                    const int k = raisesSoFar + (bigShove && raisesSoFar == 1 ? 1 : 0);
                    if (k == 1) r.top = std::min(r.top, std::clamp(opp.pfr * 1.1, 0.05, 0.6));
                    else if (k == 2) r.top = std::min(r.top, std::clamp(opp.pfr * 0.4, 0.025, 0.25));
                    else r.top = std::min(r.top, std::clamp(opp.pfr * 0.18, 0.015, 0.1));
                } else if (a.type == ActionType::Call || a.type == ActionType::AllIn) {
                    if (raisesSoFar == 0) r.top = std::min(r.top, std::clamp(opp.vpip, 0.25, 1.0));
                    else if (raisesSoFar == 1) r.top = std::min(r.top, std::clamp(opp.vpip * 0.8, 0.1, 0.7));
                    else r.top = std::min(r.top, std::clamp(opp.pfr * 0.6, 0.03, 0.2));
                }
            } else if (a.seat == opp.seat) {
                if (a.raised) {
                    bet[st] = true;
                    if (a.toCall > 0 && readsStrength) ++r.raises;
                    else ++r.bets;
                } else if (a.type == ActionType::Call || a.type == ActionType::AllIn) {
                    const double odds = static_cast<double>(a.toCall) / std::max(1, a.potBefore + a.toCall);
                    if (odds >= 0.36 && readsStrength) ++r.bigCalls;
                    else ++r.calls;
                } else if (a.type == ActionType::Check) {
                    checked[st] = true;
                }
            }
        }
        const auto cur = static_cast<std::size_t>(ctx.street);
        r.checkedThisStreet = checked[cur] && !bet[cur];
        ranges.push_back(r);
    }
    return ranges;
}

ExpertAdvisor::Response ExpertAdvisor::respond(const DecisionContext& ctx, const OpponentInfo& opp,
                                               const OpponentRange& range, int callAmount) const {
    Response resp;
    resp.calling = range;
    const double potBefore = ctx.pot + ctx.toCall;
    const double alpha = std::max(0.05, callAmount / (potBefore + callAmount));

    if (ctx.board.empty()) {
        const bool raisedBefore = raisedOn(ctx, opp.seat, Street::Preflop);
        double continueShare = 0.0;
        if (raisedBefore) {
            const double floor = ctx.raisesThisStreet >= 3 ? 0.6 : (ctx.raisesThisStreet == 2 ? 0.45 : 0.35);
            continueShare = std::clamp(0.5 * std::sqrt(0.4 / alpha), floor, 0.85);
        } else {
            const double absolute = std::clamp(opp.vpip * 0.75 * std::sqrt(0.4 / alpha), 0.02, 0.9);
            continueShare = std::min(1.0, absolute / range.top);
        }
        resp.fold = std::clamp((1.0 - continueShare) * pressure_, 0.02, 0.95);
        resp.calling.top = std::max(0.015, range.top * continueShare);
    } else {
        const double base = opp.foldToBet;
        double fold = alpha <= 0.4 ? base * std::pow(alpha / 0.4, 0.7) : base + (1.0 - base) * 0.5 * (alpha - 0.4);
        if (raisedOn(ctx, opp.seat, ctx.street)) fold *= 0.5;
        fold *= std::pow(0.85, range.calls + range.bigCalls);
        resp.fold = std::clamp(fold * pressure_, 0.02, 0.8);
        ++resp.calling.calls;
        resp.calling.bluffShare = std::min(range.bluffShare, 0.2);
    }
    return resp;
}

double ExpertAdvisor::realization(const DecisionContext& ctx, bool aggressor) const {
    if (ctx.board.size() == 5) return 1.0;
    const double spr = static_cast<double>(ctx.effectiveStack) / std::max(1, ctx.pot);
    if (spr <= 1.5) return 1.0;
    double r = ctx.position == Position::Late ? 0.97 : 0.87;
    if (aggressor) r += 0.04;
    return std::min(1.0, r);
}

Recommendation ExpertAdvisor::advise(const DecisionContext& input) {
    const DecisionContext ctx = normalize(input);
    const std::vector<OpponentRange> ranges = opponentRanges(ctx);

    Recommendation rec;
    rec.equity = calculator_.vsOpponents(ctx.hole, ctx.board, ranges, iterations_).equity;
    rec.requiredEquity = potOdds(ctx.toCall, ctx.pot);
    rec.callEv = ctx.toCall > 0 ? callEV(rec.equity, ctx.pot, ctx.toCall) : 0.0;
    double topSum = 0.0;
    for (const auto& r : ranges) topSum += r.top;
    rec.rangeTop = topSum / static_cast<double>(ranges.size());

    if (ctx.board.size() >= 3) {
        rec.handSummary = evaluateHand(ctx.hole, ctx.board).describe();
        if (ctx.board.size() < 5) rec.outs = static_cast<int>(countOuts(ctx.hole, ctx.board).outs.size());
    } else {
        rec.handSummary = handCode(ctx.hole) + " (топ " + percent0(handPercentile(ctx.hole)) + " рук)";
    }

    const bool unopened = ctx.board.empty() && ctx.raisesThisStreet == 0;
    const bool shortStack = ctx.effectiveStack <= kShortStackBB * ctx.bigBlind;
    if (unopened && !shortStack) {
        preflopOpen(ctx, rec);
        return rec;
    }
    evaluate(ctx, rec, ranges);
    return rec;
}

void ExpertAdvisor::preflopOpen(const DecisionContext& ctx, Recommendation& rec) {
    double vpipSum = 0.0;
    for (const auto& o : ctx.opponents) vpipSum += o.vpip;
    const double avgVpip = vpipSum / static_cast<double>(ctx.opponents.size());
    const bool stealSpot = ctx.position == Position::Late || ctx.position == Position::Blinds;
    const double factor = stealSpot ? std::clamp(0.30 / avgVpip, 0.75, 1.5) : std::clamp(std::sqrt(0.30 / avgVpip), 0.85, 1.2);
    const int limpers = std::max(0, (ctx.pot - ctx.bigBlind - ctx.bigBlind / 2) / std::max(1, ctx.bigBlind));
    const double range =
        std::min(0.9, openingRange(ctx.position, ctx.playersInHand) * factor * openLooseness_ * (limpers > 0 ? 0.8 : 1.0));
    const double pct = handPercentile(ctx.hole);

    std::ostringstream why;
    why << handCode(ctx.hole) << " — топ " << percent0(pct) << " рук. Діапазон відкриття з позиції «"
        << positionName(ctx.position) << "» — топ " << percent0(range);
    if (stealSpot && factor > 1.05) why << " (суперники тайтові — крадемо блайнди ширше)";
    why << ". ";

    if (pct <= range) {
        const int base = ctx.position == Position::Late ? ctx.bigBlind * 11 / 5 : ctx.bigBlind * 5 / 2;
        rec.action = sizedBet(ctx, base + limpers * ctx.bigBlind);
        why << (limpers > 0 ? "Ізолюємо лімперів рейзом." : "Відкриваємось рейзом.");
    } else if (ctx.canCheck()) {
        rec.action = Action::check();
        why << "Поза діапазоном — безкоштовний чек.";
    } else if (ctx.position == Position::Blinds && ctx.toCall <= ctx.bigBlind / 2 && pct <= range * 1.8) {
        rec.action = Action::call();
        why << "Дешево доставити з малого блайнду.";
    } else {
        rec.action = Action::fold();
        why << "Поза діапазоном — фолд.";
    }
    rec.reasoning = why.str();
}

void ExpertAdvisor::evaluate(const DecisionContext& ctx, Recommendation& rec, const std::vector<OpponentRange>& ranges) {
    const double eq = rec.equity;
    const int pot = ctx.pot;
    const int toCall = ctx.toCall;
    const int halfIters = std::max(300, iterations_ / 2);

    auto add = [&](const Action& a, double ev) {
        for (const auto& o : rec.options) {
            if (o.action.type == a.type && o.action.amount == a.amount) return;
        }
        rec.options.push_back({a, ev, optionLabel(a, ctx)});
    };

    double vpipSum = 0.0, aggrMax = 0.0;
    for (const auto& o : ctx.opponents) {
        vpipSum += o.vpip;
        aggrMax = std::max(aggrMax, o.aggression);
    }
    const double looseness = std::clamp(vpipSum / ctx.opponents.size() / 0.28, 0.5, 1.6);

    if (toCall > 0) {
        add(Action::fold(), 0.0);
        const bool callAllIn = toCall >= ctx.stack;
        double ev = eq * (callAllIn ? 1.0 : realization(ctx, false)) * (pot + toCall) - toCall;
        if (!callAllIn) ev += futureValue(ctx, eq, toCall, pot + 2 * toCall);
        if (!callAllIn && (ctx.board.size() == 3 || ctx.board.size() == 4)) {
            const OutsInfo outs = countOuts(ctx.hole, ctx.board);
            if (outs.outs.size() >= 6) {
                const double future = std::min<double>(ctx.effectiveStack - toCall, pot + toCall);
                ev += outs.nextCard * future * 0.3 * looseness;
            }
        }
        add(Action::call(), ev);
    } else {
        double ev = eq * pot * realization(ctx, false) + futureValue(ctx, eq, 0, pot);
        if (eq >= 0.85 && aggrMax >= 0.45 && ctx.board.size() >= 3 && ctx.board.size() < 5) {
            ev += aggrMax * 0.5 * pot * (2.0 * eq - 1.0) * 0.6;
        }
        add(Action::check(), ev);
    }

    const bool canRaise = ctx.maxRaiseTo() > ctx.highestBet && ctx.effectiveStack > toCall;
    std::vector<int> targets;
    const bool shortStack = ctx.effectiveStack <= kShortStackBB * ctx.bigBlind;
    if (!canRaise) {
    } else if (shortStack && ctx.board.empty()) {
        targets.push_back(ctx.maxRaiseTo());
    } else if (ctx.highestBet == 0 || ctx.highestBet == ctx.currentBet) {
        std::vector<double> fractions = {0.33, 0.66, 1.0};
        if (!ctx.board.empty() && (eq >= 0.75 || eq <= 0.25)) fractions.push_back(1.5);
        for (const double f : fractions) {
            targets.push_back(ctx.highestBet + std::max(ctx.bigBlind, static_cast<int>(std::lround(pot * f))));
        }
        if (jamAllowed(ctx)) targets.push_back(ctx.maxRaiseTo());
    } else if (ctx.board.empty()) {
        if (ctx.raisesThisStreet <= 1) {
            targets.push_back(ctx.position == Position::Late ? ctx.highestBet * 3 : ctx.highestBet * 7 / 2);
        } else if (ctx.raisesThisStreet == 2) {
            targets.push_back(ctx.highestBet * 23 / 10);
        }
        if (jamAllowed(ctx)) targets.push_back(ctx.maxRaiseTo());
    } else {
        targets.push_back(ctx.highestBet * 5 / 2);
        targets.push_back(ctx.highestBet * 7 / 2);
        targets.push_back(ctx.highestBet + pot + toCall);
        if (jamAllowed(ctx)) targets.push_back(ctx.maxRaiseTo());
    }

    for (const int target : targets) {
        const Action action = sizedBet(ctx, target);
        const int total = action.type == ActionType::AllIn ? ctx.maxRaiseTo() : action.amount;
        const int invest = std::min(total - ctx.currentBet, ctx.effectiveStack);
        const int call = std::max(0, invest - toCall);

        double foldAll = 1.0, expectedCallers = 0.0;
        std::vector<OpponentRange> callingRanges;
        for (std::size_t i = 0; i < ctx.opponents.size(); ++i) {
            const Response r = respond(ctx, ctx.opponents[i], ranges[i], call);
            foldAll *= r.fold;
            expectedCallers += 1.0 - r.fold;
            callingRanges.push_back(r.calling);
        }
        const double callers = std::max(1.0, expectedCallers / std::max(1e-6, 1.0 - foldAll));
        const double eqCalled = calculator_.vsOpponents(ctx.hole, ctx.board, callingRanges, halfIters).equity;
        const bool isFinal = action.type == ActionType::AllIn || ctx.board.size() == 5;
        const double r = isFinal ? 1.0 : realization(ctx, true);
        const double potCalled = pot + invest + callers * call;
        double whenCalled = eqCalled * r * potCalled - invest;
        if (!isFinal) whenCalled += futureValue(ctx, eqCalled, invest, static_cast<int>(potCalled));
        add(action, foldAll * pot + (1.0 - foldAll) * whenCalled);
    }

    std::size_t best = 0;
    for (std::size_t i = 1; i < rec.options.size(); ++i) {
        if (rec.options[i].ev > rec.options[best].ev) best = i;
    }
    std::size_t chosen = best;
    if (temperature_ > 0.0) {
        const double scale = temperature_ * ctx.bigBlind;
        std::vector<double> w;
        double sum = 0.0;
        for (const auto& o : rec.options) {
            w.push_back(std::exp((o.ev - rec.options[best].ev) / scale));
            sum += w.back();
        }
        double pick = rng_.uniform01() * sum;
        for (std::size_t i = 0; i < w.size(); ++i) {
            pick -= w[i];
            if (pick <= 0) {
                chosen = i;
                break;
            }
        }
    }
    rec.action = rec.options[chosen].action;

    std::ostringstream why;
    for (std::size_t i = 0; i < ctx.opponents.size() && i < 3; ++i) {
        const auto& o = ctx.opponents[i];
        const auto& rg = ranges[i];
        why << "Суперник " << i + 1 << ": топ " << percent0(rg.top) << " рук";
        if (rg.bets + rg.raises > 0) why << ", ставки/рейзи: " << rg.bets + rg.raises;
        if (rg.calls + rg.bigCalls > 0) why << ", коли: " << rg.calls + rg.bigCalls;
        if (o.hands >= 10) why << " (VPIP " << percent0(o.vpip) << ", скидає на ставку " << percent0(o.foldToBet) << ")";
        why << ". ";
    }
    why << "Еквіті проти їхніх діапазонів " << percentString(eq) << ". Найбільше EV: " << rec.options[best].label
        << " (" << signedChips(rec.options[best].ev) << ").";
    rec.reasoning = why.str();
}

}
