#include "poker/ui/App.hpp"

#include "poker/ai/Bots.hpp"
#include "poker/game/Game.hpp"
#include "poker/ui/AnalysisScreens.hpp"
#include "poker/ui/Console.hpp"
#include "poker/ui/ConsoleObserver.hpp"
#include "poker/ui/HumanStrategy.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace poker::ui {

App::App(AppOptions options) : options_(options), rng_(options.seed) {
    initConsole();
    setColorsEnabled(options_.colors);
}

int App::run() {
    std::cout << paint("\n  ♠ ♥  ТЕХАСЬКИЙ ХОЛДЕМ  ♦ ♣", color::Bold) << '\n'
              << paint("  Симулятор No-Limit Hold'em з аналізом оптимальної стратегії", color::Gray)
              << '\n';

    while (true) {
        std::cout << '\n'
                  << "  1. Грати проти ботів\n"
                  << "  2. Калькулятор еквіті (рука проти руки)\n"
                  << "  3. Аналіз ситуації — оптимальна дія\n"
                  << "  4. Префлоп-таблиця стартових рук\n"
                  << "  5. Турнір ботів (порівняння стратегій)\n"
                  << "  6. Правила і комбінації\n"
                  << "  0. Вихід\n";
        const auto choice = readLine("\nВаш вибір: ");
        if (!choice || *choice == "0") break;

        try {
            if (*choice == "1") playGame();
            else if (*choice == "2") runEquityCalculator(rng_);
            else if (*choice == "3") runSituationAnalysis(rng_);
            else if (*choice == "4") showPreflopChart();
            else if (*choice == "5") runBotSimulation(rng_);
            else if (*choice == "6") showRules();
            else std::cout << paint("Невідомий пункт меню", color::Red) << '\n';
        } catch (const std::exception& e) {
            std::cout << paint(std::string("Помилка: ") + e.what(), color::Red) << '\n';
        }
        if (std::cin.eof()) break;
    }
    std::cout << "\nДо зустрічі за столом!\n";
    return 0;
}

void App::playGame() {
    printTitle("Гра проти ботів");
    std::cout << "Рівень ботів: 1 — легкий, 2 — середній, 3 — складний (профі)\n";
    const int level = readInt("Рівень", 1, 3, 2);
    const int opponents = readInt("Кількість суперників (1–5)", 1, 5, 3);
    const int chips = readInt("Стартовий стек", 100, 1'000'000, 1000);
    const int bigBlind = readInt("Великий блайнд", 2, chips / 10, std::max(2, chips / 50));

    GameConfig config;
    config.bigBlind = bigBlind;
    config.smallBlind = std::max(1, bigBlind / 2);
    config.startingChips = chips;

    Game game(config, rng_.next());
    game.addPlayer("Ви", std::make_unique<HumanStrategy>(rng_.next()));

    const std::array<const char*, 5> names = {"Олег", "Марія", "Степан", "Ірина", "Тарас"};
    const auto types = lineupFor(static_cast<Difficulty>(level - 1));
    for (int i = 0; i < opponents; ++i) {
        const auto idx = static_cast<std::size_t>(i);
        game.addPlayer(std::string(names[idx]) + " (" + botTypeName(types[idx]) + ")",
                       makeBot(types[idx], rng_.next()));
    }

    std::cout << paint("\nПідказка: під час ходу введіть h — порадник покаже оптимальну дію.", color::Gray)
              << '\n';

    ConsoleObserver observer;
    game.setObserver(&observer);

    while (true) {
        const Player& human = game.players().front();
        if (human.chips() == 0) {
            std::cout << '\n' << paint("Ви вибули з гри. Спробуйте ще раз!", color::Red) << '\n';
            break;
        }
        if (game.playersWithChips() < 2) {
            std::cout << '\n' << paint("Вітаємо! Ви виграли всі фішки за столом!", color::Green) << '\n';
            break;
        }
        game.playHand();
        const auto next = readLine(paint("\nEnter — наступна роздача, q — вийти в меню: ", color::Gray));
        if (!next || *next == "q" || *next == "Q") break;
    }

    const int result = game.players().front().chips() - config.startingChips;
    std::cout << "\nЗіграно роздач: " << game.handNumber() << ".  Ваш результат: "
              << paint((result >= 0 ? "+" : "") + std::to_string(result),
                       result >= 0 ? color::Green : color::Red)
              << " фішок\n";
}

void App::showRules() {
    printTitle("Правила No-Limit Texas Hold'em");
    std::cout
        << "  • Кожен гравець отримує 2 закриті карти, на стіл викладається до 5 спільних.\n"
        << "  • Перед роздачею два гравці ліворуч від баттона ставлять малий і великий блайнди.\n"
        << "  • Торгівля йде 4 раунди: префлоп, флоп (3 карти), терн (4-а), рівер (5-а).\n"
        << "  • Дії: фолд, чек, кол, бет/рейз (мінімум — на розмір попереднього підвищення), ол-ін.\n"
        << "  • Якщо хтось пішов ол-ін, решта ставок іде в сайд-пот, який він виграти не може.\n"
        << "  • На шоудауні перемагає найкраща комбінація з 5 карт із 7 доступних.\n\n";

    struct Example {
        const char* name;
        const char* cards;
        const char* probability;
    };
    const std::array<Example, 10> examples = {{
        {"Роял-флеш", "AsKsQsJsTs", "0.003%"},
        {"Стріт-флеш", "9h8h7h6h5h", "0.03%"},
        {"Каре", "QcQdQhQs7d", "0.17%"},
        {"Фул-хаус", "KhKdKc8s8h", "2.6%"},
        {"Флеш", "Ad Jd 8d 6d 2d", "3.0%"},
        {"Стріт", "Ts9h8c7d6s", "4.6%"},
        {"Трійка (сет)", "7c7d7hKsQd", "4.8%"},
        {"Дві пари", "JhJsTcTd4h", "23.5%"},
        {"Пара", "AcAh9s6d3c", "43.8%"},
        {"Старша карта", "AdQc9h6s3d", "17.4%"},
    }};
    std::cout << paint("  Комбінації (від сильнішої) і їх імовірність з 7 карт:", color::Bold) << '\n';
    for (const auto& e : examples) {
        std::cout << "  " << padRight(e.name, 16) << padRight(cardsText(parseCards(e.cards)), 22)
                  << paint(e.probability, color::Gray) << '\n';
    }
}

}
