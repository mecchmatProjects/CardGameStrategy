#include "poker/ui/AnalysisScreens.hpp"

#include "poker/ai/Bots.hpp"
#include "poker/analysis/EquityCalculator.hpp"
#include "poker/analysis/ExpertAdvisor.hpp"
#include "poker/analysis/Odds.hpp"
#include "poker/analysis/PreflopChart.hpp"
#include "poker/game/Game.hpp"
#include "poker/ui/Console.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>

namespace poker::ui {

namespace {

std::string fixed(double value, int precision = 1, bool showSign = false) {
    std::ostringstream os;
    if (showSign) os << std::showpos;
    os << std::fixed << std::setprecision(precision) << value;
    return os.str();
}

std::string actionLabel(const Action& action) {
    switch (action.type) {
        case ActionType::Fold: return "ФОЛД";
        case ActionType::Check: return "ЧЕК";
        case ActionType::Call: return "КОЛ";
        case ActionType::Bet: return "БЕТ " + std::to_string(action.amount);
        case ActionType::Raise: return "РЕЙЗ ДО " + std::to_string(action.amount);
        case ActionType::AllIn: return "ОЛ-ІН";
        case ActionType::PostBlind: return "БЛАЙНД";
    }
    return "?";
}

const char* tierColor(PreflopTier tier) {
    switch (tier) {
        case PreflopTier::Premium: return color::Magenta;
        case PreflopTier::Strong: return color::Red;
        case PreflopTier::Playable: return color::Yellow;
        case PreflopTier::Speculative: return color::Green;
        case PreflopTier::Trash: return color::Gray;
    }
    return color::Reset;
}

Street streetFromBoard(std::size_t cards) {
    switch (cards) {
        case 3: return Street::Flop;
        case 4: return Street::Turn;
        case 5: return Street::River;
        default: return Street::Preflop;
    }
}

void infoLine(const std::string& label, const std::string& value) {
    std::cout << "  │ " << padRight(label + ":", 26) << value << '\n';
}

class SimulationTracker : public GameObserver {
public:
    struct Stats {
        int hands = 0;
        int vpip = 0;
        int pfr = 0;
        int potsWon = 0;
        int showdowns = 0;
        int showdownWins = 0;
    };

    explicit SimulationTracker(std::size_t players) : stats_(players), flags_(players) {}

    void onHandStart(int, const std::vector<Player>& players, int) override {
        street_ = Street::Preflop;
        std::fill(flags_.begin(), flags_.end(), Flags{});
        for (const auto& p : players) {
            if (!p.isSittingOut()) ++stats_[static_cast<std::size_t>(p.seat())].hands;
        }
    }

    void onStreet(Street street, const std::vector<Card>&, int) override { street_ = street; }

    void onAction(const Player& player, const Action& action, int) override {
        if (street_ != Street::Preflop) return;
        Flags& f = flags_[static_cast<std::size_t>(player.seat())];
        if (action.type == ActionType::Call || action.isAggressive()) f.vpip = true;
        if (action.isAggressive()) f.pfr = true;
    }

    void onShowdown(const std::vector<ShowdownEntry>& entries, const std::vector<Player>&) override {
        for (const auto& e : entries) flags_[static_cast<std::size_t>(e.seat)].showdown = true;
    }

    void onPotAwarded(const PotResult& result, const std::vector<Player>&) override {
        for (const auto& w : result.winners) flags_[static_cast<std::size_t>(w.seat)].won = true;
    }

    void onHandEnd(const std::vector<Player>&) override {
        for (std::size_t i = 0; i < flags_.size(); ++i) {
            const Flags& f = flags_[i];
            Stats& s = stats_[i];
            s.vpip += f.vpip;
            s.pfr += f.pfr;
            s.potsWon += f.won;
            s.showdowns += f.showdown;
            s.showdownWins += f.showdown && f.won;
        }
    }

    const std::vector<Stats>& stats() const { return stats_; }

private:
    struct Flags {
        bool vpip = false, pfr = false, showdown = false, won = false;
    };
    std::vector<Stats> stats_;
    std::vector<Flags> flags_;
    Street street_ = Street::Preflop;
};

double ratio(int part, int whole) { return whole > 0 ? static_cast<double>(part) / whole : 0.0; }

}

void printRecommendation(const Recommendation& rec) {
    std::cout << paint("  ┌─ Аналіз порадника ───────────────────────────", color::Cyan) << '\n';
    infoLine("Рука", rec.handSummary);
    infoLine("Еквіті", percentString(rec.equity) + "  " + bar(rec.equity));
    if (rec.requiredEquity > 0.0) {
        infoLine("Потрібно (шанси банку)", percentString(rec.requiredEquity));
        infoLine("EV колу", fixed(rec.callEv, 1, true) + " фішок");
    }
    if (rec.outs >= 0) infoLine("Аутів", std::to_string(rec.outs));
    for (const auto& o : rec.options) infoLine("  EV: " + o.label, fixed(o.ev, 1, true));
    infoLine("Оптимальна дія", paint(actionLabel(rec.action), color::Yellow));
    std::cout << "  │ " << rec.reasoning << '\n'
              << paint("  └──────────────────────────────────────────────", color::Cyan) << '\n';
}

void runEquityCalculator(Rng& rng) {
    printTitle("Калькулятор еквіті");
    std::cout << "Введіть руки гравців підряд, по 2 карти на кожного (2–9 гравців).\n"
              << "Приклад: " << paint("AhKh QsQd", color::Bold)
              << "   ранги: 2-9 T J Q K A, масті: c ♣  d ♦  h ♥  s ♠\n\n";

    const auto cards = readCards("Руки: ", {4, 6, 8, 10, 12, 14, 16, 18});
    if (!cards) return;
    const auto board = readCards("Борд (Enter — порожній, або 3/4/5 карт): ", {0, 3, 4, 5});
    if (!board) return;

    std::vector<HoleCards> hands;
    for (std::size_t i = 0; i + 1 < cards->size(); i += 2) hands.push_back({(*cards)[i], (*cards)[i + 1]});

    EquityReport report;
    try {
        EquityCalculator calculator(rng.next());
        report = calculator.calculate(hands, *board);
    } catch (const std::exception& e) {
        std::cout << paint(std::string("Помилка: ") + e.what(), color::Red) << '\n';
        return;
    }

    std::cout << "\nБорд: " << cardsText(*board) << "\n\n"
              << paint("  " + padRight("Рука", 10) + padLeft("Виграш", 9) + padLeft("Нічия", 9) +
                           padLeft("Еквіті", 9),
                       color::Bold)
              << '\n';
    std::size_t favourite = 0;
    for (std::size_t i = 0; i < hands.size(); ++i) {
        const auto& r = report.players[i];
        if (r.equity > report.players[favourite].equity) favourite = i;
        std::cout << "  " << padRight(holeText(hands[i]), 10) << padLeft(percentString(r.win), 9)
                  << padLeft(percentString(r.tie), 9) << padLeft(percentString(r.equity), 9) << "  "
                  << paint(bar(r.equity), color::Green) << '\n';
    }
    std::cout << '\n'
              << paint(report.exact ? "Метод: точний перебір " : "Метод: Монте-Карло, ", color::Gray)
              << paint(std::to_string(report.trials) + " варіантів борду", color::Gray) << '\n'
              << "Фаворит: " << holeText(hands[favourite]) << " (" << handCode(hands[favourite]) << ")\n";
}

void runSituationAnalysis(Rng& rng) {
    printTitle("Аналіз ситуації — оптимальна дія");

    const auto hole = readCards("Ваші карти (напр. AhKh): ", {2});
    if (!hole) return;
    const auto board = readCards("Борд (Enter — префлоп, або 3/4/5 карт): ", {0, 3, 4, 5});
    if (!board) return;
    if (toMask(*hole) & toMask(*board)) {
        std::cout << paint("Карти в руці і на борді не можуть повторюватись.", color::Red) << '\n';
        return;
    }

    const int bigBlind = readInt("Великий блайнд", 2, 100000, 20);
    const int pot = readInt("Банк зараз (з усіма ставками)", 0, 10'000'000,
                            board->empty() ? bigBlind * 3 / 2 : 10 * bigBlind);
    const int toCall = readInt("Скільки треба доставити (0 — ставок не було)", 0, 10'000'000,
                               board->empty() ? bigBlind : 0);
    const int stack = readInt("Ваш стек", 1, 10'000'000, 50 * bigBlind);
    const int opponents = readInt("Активних суперників", 1, 9, 1);
    std::cout << "Позиція: 1 — рання, 2 — середня, 3 — пізня, 4 — блайнди\n";
    const int position = readInt("Позиція", 1, 4, 3);

    DecisionContext ctx;
    ctx.hole = {(*hole)[0], (*hole)[1]};
    ctx.board = *board;
    ctx.street = streetFromBoard(board->size());
    ctx.position = static_cast<Position>(position - 1);
    ctx.pot = pot;
    ctx.toCall = std::min(toCall, stack);
    ctx.highestBet = board->empty() ? std::max(toCall, bigBlind) : toCall;
    ctx.currentBet = ctx.highestBet - toCall;
    ctx.minRaiseTo = ctx.highestBet + std::max(bigBlind, toCall);
    ctx.stack = stack;
    ctx.bigBlind = bigBlind;
    ctx.activeOpponents = opponents;
    ctx.playersInHand = opponents + 1;

    std::cout << "\nРахую...\n";
    ExpertAdvisor advisor(rng.next(), 8000, 0.0);
    printRecommendation(advisor.advise(ctx));

    if (board->empty()) {
        const PreflopTier tier = preflopTier(ctx.hole);
        std::cout << "\nСтартова рука " << paint(handCode(ctx.hole), tierColor(tier)) << ": "
                  << tierName(tier) << ", формула Чена = " << chenScore(ctx.hole) << '\n'
                  << "Відкривати рейзом з позиції:";
        for (const Position p : {Position::Early, Position::Middle, Position::Late, Position::Blinds}) {
            std::cout << "  " << positionName(p) << " — "
                      << (shouldOpen(tier, p) ? paint("так", color::Green) : paint("ні", color::Gray));
        }
        std::cout << '\n';
    } else if (board->size() < 5) {
        const OutsInfo info = countOuts(ctx.hole, ctx.board);
        std::cout << "\nАути (" << info.outs.size() << "): " << cardsText(info.outs) << '\n'
                  << "  Покращення на наступній карті: " << percentString(info.nextCard) << '\n';
        if (board->size() == 3) {
            std::cout << "  Покращення до рівера (2 карти): " << percentString(info.byRiver) << '\n';
        }
        std::cout << "  Правило 2 і 4 (швидка оцінка): ≈" << info.ruleOf2And4 << "%\n";
    }

    if (pot > 0) {
        std::cout << "\nБлеф окупається, якщо суперник скидає частіше ніж: "
                  << "ставка 1/2 банку — " << percentString(bluffBreakEven(pot / 2, pot))
                  << ", ставка в банк — " << percentString(bluffBreakEven(pot, pot)) << '\n';
    }
}

void showPreflopChart() {
    printTitle("Префлоп-таблиця 169 стартових рук (формула Чена)");
    const PreflopChart chart = buildPreflopChart();

    std::array<int, 5> combosByTier{};
    for (std::size_t row = 0; row < chart.size(); ++row) {
        std::cout << "  ";
        for (std::size_t col = 0; col < chart[row].size(); ++col) {
            const ChartCell& cell = chart[row][col];
            std::cout << paint(padRight(cell.code, 4), tierColor(cell.tier));
            combosByTier[static_cast<std::size_t>(cell.tier)] += row == col ? 6 : (row < col ? 4 : 12);
        }
        std::cout << '\n';
    }

    std::cout << "\n  Вище діагоналі — одномастні (s), нижче — різномастні (o), на діагоналі — пари.\n\n";
    const char* thresholds[] = {"Чен ≥ 12", "Чен 10–11", "Чен 8–9", "Чен 5–7", "Чен < 5"};
    for (std::size_t t = 0; t < combosByTier.size(); ++t) {
        const auto tier = static_cast<PreflopTier>(t);
        std::cout << "  " << paint("■ " + padRight(tierName(tier), 14), tierColor(tier))
                  << padRight(thresholds[t], 11)
                  << percentString(combosByTier[t] / 1326.0) << " усіх рук\n";
    }

    std::cout << "\n  Рекомендований діапазон відкриття рейзом:\n";
    for (const Position p : {Position::Early, Position::Middle, Position::Late, Position::Blinds}) {
        int combos = 0;
        std::string tiers;
        for (std::size_t t = 0; t < combosByTier.size(); ++t) {
            const auto tier = static_cast<PreflopTier>(t);
            if (!shouldOpen(tier, p)) continue;
            combos += combosByTier[t];
            tiers += (tiers.empty() ? "" : ", ") + tierName(tier);
        }
        std::cout << "    " << padRight(positionName(p), 10) << padLeft(percentString(combos / 1326.0), 7)
                  << "   " << tiers << '\n';
    }
}

void runBotSimulation(Rng& rng) {
    printTitle("Турнір ботів — порівняння стратегій");
    std::cout << "За столом по одному боту кожного типу. Бот без фішок докуповується,\n"
                 "тож результат показує чистий прибуток стратегії на дистанції.\n\n";
    for (const BotType type : allBotTypes()) {
        std::cout << "  " << padRight(paint(botTypeName(type), color::Bold), 8) << " — "
                  << botTypeDescription(type) << '\n';
    }
    std::cout << '\n';

    const int hands = readInt("Кількість роздач", 50, 100000, 1000);

    GameConfig config;
    Game game(config, rng.next());
    const auto types = allBotTypes();
    for (const BotType type : types) game.addPlayer(botTypeName(type), makeBot(type, rng.next(), 800));

    SimulationTracker tracker(types.size());
    game.setObserver(&tracker);
    std::vector<int> rebuys(types.size(), 0);

    const auto start = std::chrono::steady_clock::now();
    const int step = std::max(1, hands / 50);
    for (int h = 0; h < hands; ++h) {
        for (const auto& p : game.players()) {
            if (p.chips() == 0) {
                game.rebuy(p.seat(), config.startingChips);
                ++rebuys[static_cast<std::size_t>(p.seat())];
            }
        }
        game.playHand();
        if ((h + 1) % step == 0) {
            std::cout << "\r  Прогрес: " << (h + 1) * 100 / hands << "% " << std::flush;
        }
    }
    const double seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "\r  Зіграно " << hands << " роздач за " << fixed(seconds) << " с\n\n";

    struct Row {
        std::size_t seat;
        int profit;
        double bbPer100;
    };
    std::vector<Row> rows;
    for (std::size_t i = 0; i < types.size(); ++i) {
        const int invested = config.startingChips * (1 + rebuys[i]);
        const int profit = game.players()[i].chips() - invested;
        rows.push_back({i, profit, 100.0 * profit / config.bigBlind / hands});
    }
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.profit > b.profit; });

    std::cout << paint("  " + padRight("#", 3) + padRight("Бот", 8) + padLeft("Профіт", 9) +
                           padLeft("bb/100", 9) + padLeft("VPIP", 8) + padLeft("PFR", 8) +
                           padLeft("Банків", 9) + padLeft("SD win", 9) + padLeft("Ребаї", 7),
                       color::Bold)
              << '\n';
    for (std::size_t place = 0; place < rows.size(); ++place) {
        const Row& r = rows[place];
        const auto& s = tracker.stats()[r.seat];
        const std::string profit = fixed(r.profit, 0, true);
        std::cout << "  " << padRight(std::to_string(place + 1), 3)
                  << padRight(game.players()[r.seat].name(), 8)
                  << padLeft(paint(profit, r.profit >= 0 ? color::Green : color::Red), 9)
                  << padLeft(fixed(r.bbPer100, 1, true), 9)
                  << padLeft(percentString(ratio(s.vpip, s.hands)), 8)
                  << padLeft(percentString(ratio(s.pfr, s.hands)), 8)
                  << padLeft(percentString(ratio(s.potsWon, s.hands)), 9)
                  << padLeft(percentString(ratio(s.showdownWins, s.showdowns)), 9)
                  << padLeft(std::to_string(rebuys[r.seat]), 7) << '\n';
    }

    const Row& best = rows.front();
    std::cout << "\n  " << paint("Висновок:", color::Bold) << " найкращий результат показала стратегія «"
              << game.players()[best.seat].name() << "» (" << fixed(best.bbPer100, 1, true)
              << " bb/100).\n"
              << paint("  VPIP — як часто бот добровільно грає префлоп, PFR — як часто рейзить,\n"
                       "  SD win — частка виграних шоудаунів.\n"
                       "  Боти на основі порадника (TAG, LAG: еквіті + шанси банку + позиція) не\n"
                       "  платять без математичного обґрунтування і забирають фішки у пасивного\n"
                       "  (Колер) та випадкового (Рандом) гравців. Порядок TAG і LAG залежить від\n"
                       "  дисперсії — запустіть кілька разів або збільште кількість роздач.",
                       color::Gray)
              << '\n';
}

}
