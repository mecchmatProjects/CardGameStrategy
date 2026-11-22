#include "poker/ui/HumanStrategy.hpp"

#include "poker/ui/AnalysisScreens.hpp"
#include "poker/ui/Console.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>

namespace poker::ui {

HumanStrategy::HumanStrategy(std::uint64_t seed) : advisor_(seed, 4000, 0.0) {}

Action HumanStrategy::decide(const DecisionContext& ctx) {
    std::cout << '\n'
              << paint("▶ Ваш хід", color::Yellow) << "   карти: " << holeText(ctx.hole)
              << "   борд: " << cardsText(ctx.board) << '\n'
              << "  Банк: " << ctx.pot << "   До колу: " << ctx.toCall << "   Ваш стек: " << ctx.stack
              << "   Мін. рейз до: " << ctx.minRaiseTo << '\n';

    const std::string callLabel = ctx.canCheck() ? "[c] чек" : "[c] кол " + std::to_string(ctx.toCall);
    std::cout << paint("  [f] фолд   " + callLabel + "   [r N] рейз до N   [a] ол-ін   [h] підказка",
                       color::Gray)
              << '\n';

    while (true) {
        const auto line = readLine("  > ");
        if (!line) return ctx.canCheck() ? Action::check() : Action::fold();
        if (line->empty()) continue;

        const char command = static_cast<char>(std::tolower(static_cast<unsigned char>((*line)[0])));
        switch (command) {
            case 'f':
                if (ctx.canCheck()) {
                    std::cout << "  Ставок немає — замість фолду робимо чек.\n";
                    return Action::check();
                }
                return Action::fold();

            case 'c':
            case 'k':
                return ctx.canCheck() ? Action::check() : Action::call();

            case 'a':
                return Action::allIn();

            case 'h':
            case '?':
                printRecommendation(advisor_.advise(ctx));
                continue;

            case 'r':
            case 'b': {
                if (ctx.maxRaiseTo() <= ctx.highestBet) {
                    std::cout << paint("  Фішок не вистачає на рейз — лише кол/ол-ін.", color::Red) << '\n';
                    continue;
                }
                int amount = 0;
                const std::string rest = trim(line->substr(1));
                if (rest.empty()) {
                    amount = readInt("  Рейз до", ctx.minRaiseTo, ctx.maxRaiseTo(),
                                     std::min(ctx.minRaiseTo, ctx.maxRaiseTo()));
                } else {
                    try {
                        amount = std::stoi(rest);
                    } catch (const std::exception&) {
                        std::cout << paint("  Не вдалося прочитати суму.", color::Red) << '\n';
                        continue;
                    }
                }
                if (amount >= ctx.maxRaiseTo()) return Action::allIn();
                if (amount < ctx.minRaiseTo) {
                    std::cout << paint("  Мінімальний рейз — до " + std::to_string(ctx.minRaiseTo),
                                       color::Red)
                              << '\n';
                    continue;
                }
                return Action::raiseTo(amount);
            }

            default:
                std::cout << paint("  Невідома команда. Доступно: f, c, r N, a, h", color::Red) << '\n';
        }
    }
}

}
