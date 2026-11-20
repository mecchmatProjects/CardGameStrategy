#include "poker/analysis/BasicAdvisor.hpp"

#include "poker/analysis/Odds.hpp"
#include "poker/analysis/PreflopChart.hpp"
#include "poker/eval/HandEvaluator.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace poker {

namespace {

constexpr double kStrongValue = 0.65;
constexpr double kMediumValue = 0.50;
constexpr double kRaiseValue = 0.75;
constexpr double kPreflopMargin = 0.05;
constexpr int kDrawOuts = 8;

std::string signedChips(double value) {
    std::ostringstream os;
    os << std::showpos << std::fixed << std::setprecision(1) << value;
    return os.str();
}

}

BasicAdvisor::BasicAdvisor(std::uint64_t seed, int iterations) : calculator_(seed), iterations_(iterations) {}

double BasicAdvisor::estimateEquity(const DecisionContext& ctx) {
    const int opponents = std::clamp(ctx.activeOpponents, 1, 9);
    return calculator_.vsRandom(ctx.hole, ctx.board, opponents, iterations_).equity;
}

Recommendation BasicAdvisor::advise(const DecisionContext& ctx) {
    Recommendation rec;
    rec.equity = estimateEquity(ctx);
    rec.requiredEquity = potOdds(ctx.toCall, ctx.pot);
    rec.callEv = ctx.toCall > 0 ? callEV(rec.equity, ctx.pot, ctx.toCall) : 0.0;

    if (ctx.board.size() >= 3) {
        rec.handSummary = evaluateHand(ctx.hole, ctx.board).describe();
        if (ctx.board.size() < 5) rec.outs = static_cast<int>(countOuts(ctx.hole, ctx.board).outs.size());
        postflop(ctx, rec);
    } else {
        rec.handSummary = handCode(ctx.hole) + " (" + tierName(preflopTier(ctx.hole)) + ")";
        preflop(ctx, rec);
    }
    return rec;
}

void BasicAdvisor::preflop(const DecisionContext& ctx, Recommendation& rec) const {
    const PreflopTier tier = preflopTier(ctx.hole);
    std::ostringstream why;
    why << handCode(ctx.hole) << ": " << tierName(tier) << " рука (формула Чена "
        << chenScore(ctx.hole) << "), позиція " << positionName(ctx.position) << ". ";

    const bool unopened = ctx.highestBet <= ctx.bigBlind;
    if (unopened) {
        if (shouldOpen(tier, ctx.position)) {
            rec.action = sizedBet(ctx, 3 * ctx.bigBlind);
            why << "Рука входить у діапазон відкриття для цієї позиції — рейз 3 ББ.";
        } else if (ctx.canCheck()) {
            rec.action = Action::check();
            why << "Рука слабка, але флоп можна побачити безкоштовно — чек.";
        } else if (tier <= PreflopTier::Speculative && ctx.toCall <= ctx.bigBlind / 2) {
            rec.action = Action::call();
            why << "Дешево доставити з блайнду спекулятивну руку — кол.";
        } else {
            rec.action = Action::fold();
            why << "Рука не входить у діапазон відкриття з цієї позиції — фолд.";
        }
    } else {
        why << "Проти рейзу: еквіті " << percentString(rec.equity) << ", шанси банку вимагають "
            << percentString(rec.requiredEquity) << ". ";
        if (tier == PreflopTier::Premium) {
            rec.action = sizedBet(ctx, 3 * ctx.highestBet);
            why << "Преміум-рука — 3-бет утричі.";
        } else if (tier != PreflopTier::Trash && rec.equity >= rec.requiredEquity + kPreflopMargin) {
            rec.action = Action::call();
            why << "Еквіті перевищує потрібне із запасом — кол.";
        } else {
            rec.action = Action::fold();
            why << "Еквіті недостатньо проти діапазону рейзера — фолд.";
        }
    }
    rec.reasoning = why.str();
}

void BasicAdvisor::postflop(const DecisionContext& ctx, Recommendation& rec) const {
    const double eq = rec.equity;
    const bool canImprove = ctx.board.size() < 5;
    const bool strongDraw = canImprove && rec.outs >= kDrawOuts;

    std::ostringstream why;
    why << "Еквіті " << percentString(eq) << " проти " << std::max(1, ctx.activeOpponents)
        << " суперн.";
    if (rec.outs >= 0) why << ", аутів: " << rec.outs;
    why << ". ";

    if (ctx.canCheck()) {
        if (eq >= kStrongValue) {
            rec.action = potFractionBet(ctx, 0.66);
            why << "Сильна рука — вельюбет 2/3 банку.";
        } else if (eq >= kMediumValue) {
            rec.action = potFractionBet(ctx, 0.5);
            why << "Ми попереду — бет пів банку на захист від дро.";
        } else if (strongDraw) {
            rec.action = potFractionBet(ctx, 0.5);
            why << "Сильне дро — напівблеф пів банку (можемо виграти одразу або докупитись).";
        } else {
            rec.action = Action::check();
            why << "Рука слабка — чек, беремо безкоштовну карту.";
        }
    } else {
        why << "Потрібно " << percentString(rec.requiredEquity) << " (шанси банку), EV колу "
            << signedChips(rec.callEv) << ". ";
        if (eq >= kRaiseValue) {
            rec.action = sizedBet(ctx, 3 * ctx.highestBet);
            why << "Домінуюча рука — рейз для максимізації виграшу.";
        } else if (eq >= rec.requiredEquity) {
            rec.action = Action::call();
            why << "EV колу додатне — кол.";
        } else if (strongDraw && ctx.toCall * 3 <= ctx.pot) {
            rec.action = Action::call();
            why << "Прямих шансів мало, але ставка невелика, а при влученні виграємо більше "
                   "(непрямі шанси) — кол.";
        } else {
            rec.action = Action::fold();
            why << "EV колу від'ємне — фолд.";
        }
    }
    rec.reasoning = why.str();
}

}
