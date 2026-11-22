#include "poker/cards/CardGame.hpp"
#include "poker/cards/Decks.hpp"
#include "poker/cards/PokerTasks.hpp"
#include "poker/cards/Preferans.hpp"
#include "poker/cards/PrefCard.hpp"
#include "poker/ui/Console.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace poker::cards;
namespace ui = poker::ui;

namespace {

struct Options {
    std::uint64_t seed = 2026;
    long long pokerN = 5'000'000;
    long long prefN = 1'000'000;
    long long splitN = 200'000;
};

std::string prob(double p) {
    char buffer[64];
    std::snprintf(buffer, sizeof buffer, "%.6f (%.4f%%)", p, p * 100.0);
    return buffer;
}

std::string estimateLine(const Estimate& e) {
    char buffer[160];
    std::snprintf(buffer, sizeof buffer, "%.6f (%.4f%%)  ±%.6f  [95%%: %.6f … %.6f]  (%lld з %lld)", e.p(),
                  e.p() * 100.0, e.stdError(), std::max(0.0, e.p() - 1.96 * e.stdError()), e.p() + 1.96 * e.stdError(),
                  e.hits, e.trials);
    return buffer;
}

double seconds(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

std::vector<int> codes(const std::vector<ExternalCard>& cards) {
    std::vector<int> result;
    for (const auto& c : cards) result.push_back(Decks::toInternal(c));
    return result;
}

std::string handText(const int* hand, int n) {
    std::string s;
    for (int i = 0; i < n; ++i) {
        const ExternalCard e = Decks::toExternal(hand[i]);
        s += (i ? ", " : "") + std::string("(") + e.first + ", " + e.second + ")";
    }
    return s;
}

void taskCard() {
    ui::printTitle("Задача 1. Карта для преферансу та функція beat()");
    const Card x{Rank::Queen, Suit::Hearts}, y{Rank::King, Suit::Hearts};
    const Card z{Rank::Seven, Suit::Spades}, w{Rank::Ace, Suit::Diamonds};
    struct Example {
        Card a, b;
        Suit trump;
    };
    const Example examples[] = {
        {y, x, Suit::Spades},
        {x, y, Suit::Spades},
        {z, w, Suit::Spades},
        {w, z, Suit::Spades},
        {w, x, Suit::Clubs},
    };
    for (const auto& e : examples) {
        std::cout << "  beat(" << e.a.toString() << ", " << e.b.toString() << ", козир " << suitName(e.trump)
                  << ") = " << (beat(e.a, e.b, e.trump) ? "true" : "false") << '\n';
    }
    std::cout << "\n  Внутрішній код: масть × 100 + гідність. Напр., " << Card{Rank::Nine, Suit::Clubs}.toString()
              << " → " << Card{Rank::Nine, Suit::Clubs}.code() << ", " << Card{Rank::Ace, Suit::Hearts}.toString()
              << " → " << Card{Rank::Ace, Suit::Hearts}.code() << '\n';
}

void taskDecks(const Options& o) {
    ui::printTitle("Задача 2. Клас Decks: представлення карт і роздавання");
    std::cout << "  toInternal((\"9\", \"трефа\")) = " << Decks::toInternal({"9", "трефа"}) << '\n'
              << "  toInternal((\"Queen\", \"чирва\")) = " << Decks::toInternal({"Queen", "чирва"}) << '\n';
    const ExternalCard e = Decks::toExternal(314);
    std::cout << "  toExternal(314) = (\"" << e.first << "\", \"" << e.second << "\")\n"
              << "  масть 209: 209 // 100 = " << Decks::suitOf(209) << ", гідність: 209 % 100 = " << Decks::rankOf(209)
              << "\n\n";

    for (const int minRank : {2, 6, 7}) {
        const int size = Decks(minRank, 0, 1, o.seed).size();
        std::cout << "  Колода з мінімальною гідністю " << minRank << ": " << size << " " << cardsWord(size) << "\n";
    }

    Decks decks(2, 0, 1, o.seed);
    const DealResult r = decks.deal(2, 4, 5);
    std::cout << "\n  deal(N=2, m=4, n=5): hands — масив " << r.hands.depth() << "×" << r.hands.rows() << "×"
              << r.hands.cols() << ", rest — масив " << r.rest.rows() << "×" << r.rest.cols() << '\n';
    for (std::size_t d = 0; d < r.hands.depth(); ++d) {
        std::cout << "  Роздача " << d + 1 << ":\n";
        for (std::size_t p = 0; p < r.hands.rows(); ++p) {
            std::cout << "    рука " << p + 1 << ": [";
            for (std::size_t c = 0; c < r.hands.cols(); ++c) std::cout << (c ? " " : "") << r.hands(d, p, c);
            std::cout << "]  " << handText(r.hands.hand(d, p), static_cast<int>(r.hands.cols())) << '\n';
        }
        std::cout << "    у колоді лишилось " << r.rest.cols() << " " << cardsWord(static_cast<int>(r.rest.cols())) << ": перші — " << r.rest(d, 0) << ", "
                  << r.rest(d, 1) << ", " << r.rest(d, 2) << " …\n";
    }

    const std::vector<int> fixed = codes({{"Ace", "піка"}, {"Ace", "трефа"}, {"Ace", "бубна"}, {"Ace", "чирва"}, {"King", "піка"}});
    const DealResult f = decks.dealFixed(1, 4, 5, fixed);
    std::cout << "\n  dealFixed: перша рука завжди " << handText(fixed.data(), 5) << "\n";
    for (std::size_t p = 1; p < 4; ++p) std::cout << "    рука " << p + 1 << ": " << handText(f.hands.hand(0, p), 5) << '\n';
}

void printPokerResult(const PokerTaskResult& r, double time) {
    std::cout << "  а) 4 карти однакової гідності:\n"
              << "     Монте-Карло:  " << estimateLine(r.quads) << '\n'
              << "     Аналітично:   одна рука " << prob(r.exactQuadsOneHand) << ", будь-яка з " << r.handsChecked
              << " ≈ " << prob(r.approxAny(r.exactQuadsOneHand)) << '\n'
              << "  б) 5 карт однієї масті підряд (стріт-флеш):\n"
              << "     Монте-Карло:  " << estimateLine(r.straightFlush) << '\n'
              << "     Аналітично:   одна рука " << prob(r.exactStraightFlushOneHand) << ", будь-яка з " << r.handsChecked
              << " ≈ " << prob(r.approxAny(r.exactStraightFlushOneHand)) << '\n'
              << "  Час: " << time << " с\n";
}

void taskPoker(const Options& o) {
    ui::printTitle("Задача 3. 4 гравцям по 5 карт (Монте-Карло, N = " + std::to_string(o.pokerN) + ")");
    Decks decks(2, 0, 1, o.seed);
    const auto start = std::chrono::steady_clock::now();
    const PokerTaskResult result = pokerHandsRandom(decks, o.pokerN);
    printPokerResult(result, seconds(start));
}

const std::vector<ExternalCard> kFixedPokerHand = {{"Ace", "піка"}, {"King", "чирва"}, {"10", "трефа"}, {"7", "бубна"}, {"2", "піка"}};

void taskPokerFixed(const Options& o) {
    ui::printTitle("Задача 4. Фіксоване роздавання: 5 карт на 1-й руці (N = " + std::to_string(o.pokerN) + ")");
    const std::vector<int> fixed = codes(kFixedPokerHand);
    std::cout << "  Перша рука: " << handText(fixed.data(), 5) << "\n  Перевіряються руки 2, 3, 4.\n\n";
    Decks decks(2, 0, 1, o.seed + 1);
    const auto start = std::chrono::steady_clock::now();
    const PokerTaskResult result = pokerHandsFixed(decks, o.pokerN, fixed);
    printPokerResult(result, seconds(start));
}

void taskGames(const Options& o) {
    ui::printTitle("Задача 5. Клас \"Гра\" (CardGame)");
    for (const CardGame& g : CardGame::all()) {
        std::cout << g.describe();
        const DealResult r = g.deal(1, g.minPlayers() == 1 ? 1 : std::min(g.maxPlayers(), 3), o.seed);
        std::cout << "  Приклад роздачі, рука 1: "
                  << handText(r.hands.hand(0, 0), static_cast<int>(std::min<std::size_t>(r.hands.cols(), 7)))
                  << (r.hands.cols() > 7 ? ", …" : "") << "\n\n";
    }
    const CardGame thousand = CardGame::thousand();
    const int ten = Decks::toInternal({"10", "піка"}), king = Decks::toInternal({"King", "піка"});
    std::cout << "  Вага карт залежить від гри: у Тисячі 10 піка б'є короля піка — "
              << (thousand.beats(ten, king) ? "так" : "ні") << ", у Преферансі — "
              << (CardGame::preferans().beats(ten, king) ? "так" : "ні") << ".\n";
}

void taskPreferans(const Options& o) {
    ui::printTitle("Задачі 6–7. Преферанс: 10 взяток і мізер (N = " + std::to_string(o.prefN) + ")");
    std::cout << "  Колода 32 карти, 3 гравці по 10 карт, 2 карти — прикуп.\n"
              << "  «З урахуванням прикупу» — рука бере прикуп і скидає будь-які 2 карти (перебір 66 варіантів).\n\n";
    Decks decks(7, 0, 1, o.seed + 2);
    const auto start = std::chrono::steady_clock::now();
    const PreferansResult r = preferansLayouts(decks, o.prefN);
    std::cout << "  Гарантовано взяти всі 10 взяток (власний перший хід):\n"
              << "    а) без прикупу:     " << estimateLine(r.tenTricksNoTalon) << '\n'
              << "    б) з прикупом:      " << estimateLine(r.tenTricksWithTalon) << '\n'
              << "  Гарантовано не взяти жодної взятки (мізер, хід не наш):\n"
              << "    а) без прикупу:     " << estimateLine(r.misereNoTalon) << '\n'
              << "    б) з прикупом:      " << estimateLine(r.misereWithTalon) << '\n'
              << "  Час: " << seconds(start) << " с\n";
}

const std::vector<ExternalCard> kPrefHand = {{"Ace", "піка"},   {"King", "піка"},  {"10", "піка"}, {"8", "піка"},
                                             {"Ace", "трефа"},  {"King", "трефа"}, {"9", "трефа"},
                                             {"Ace", "бубна"},  {"10", "бубна"},   {"King", "чирва"}};
const std::vector<ExternalCard> kPrefDiscards = {{"7", "чирва"}, {"8", "бубна"}};

void taskTrumpSplit(const Options& o) {
    ui::printTitle("Задача 8. Фіксоване роздавання у преферансі (N = " + std::to_string(o.splitN) + ")");
    const std::vector<int> hand = codes(kPrefHand), discards = codes(kPrefDiscards);
    std::cout << "  Перша рука:  " << describeHand(hand.data(), 10) << '\n'
              << "  Знесено:     " << handText(discards.data(), 2) << '\n'
              << "  Потенційний козир — піка (4 карти: A K 10 8). Решта 4 піки: Q J 9 7.\n\n";
    Decks decks(7, 0, 1, o.seed + 3);
    const TrumpSplitResult r = trumpSplit(decks, o.splitN, hand, discards, Suit::Spades);
    std::cout << "  Розподіл 2-а рука : 3-я рука     Монте-Карло          Точно (гіпергеометричний)\n";
    for (int k = r.remainingTrumps; k >= 0; --k) {
        const double mc = static_cast<double>(r.counts[static_cast<std::size_t>(k)]) / static_cast<double>(r.trials);
        char line[160];
        std::snprintf(line, sizeof line, "    %d:%d                       %.4f (%.2f%%)       %.4f (%.2f%%)\n", k,
                      r.remainingTrumps - k, mc, mc * 100, r.exact[static_cast<std::size_t>(k)],
                      r.exact[static_cast<std::size_t>(k)] * 100);
        std::cout << line;
    }
    std::cout << "  Формула: P(k) = C(4,k)·C(16,10−k) / C(20,10)\n";
}

void runAll(const Options& o) {
    taskCard();
    taskDecks(o);
    taskPoker(o);
    taskPokerFixed(o);
    taskGames(o);
    taskPreferans(o);
    taskTrumpSplit(o);
}

void usage(const char* program) {
    std::cout << "Використання: " << program << " [--all] [--seed N] [--poker-n N] [--pref-n N] [--split-n N] [--no-color]\n"
              << "  --all        усі задачі підряд (без меню)\n"
              << "  --seed N     зерно генератора (за замовчуванням 2026)\n"
              << "  --poker-n N  роздач у задачах 3–4 (5 000 000)\n"
              << "  --pref-n N   роздач у задачах 6–7 (1 000 000)\n"
              << "  --split-n N  роздач у задачі 8 (200 000)\n";
}

}

int main(int argc, char* argv[]) {
    ui::initConsole();
    Options o;
    bool all = false;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&]() { return i + 1 < argc ? std::stoll(argv[++i]) : 0LL; };
        if (a == "--all") all = true;
        else if (a == "--no-color") ui::setColorsEnabled(false);
        else if (a == "--seed") o.seed = static_cast<std::uint64_t>(next());
        else if (a == "--poker-n") o.pokerN = next();
        else if (a == "--pref-n") o.prefN = next();
        else if (a == "--split-n") o.splitN = next();
        else {
            usage(argv[0]);
            return a == "--help" || a == "-h" ? 0 : 1;
        }
    }
    if (all) {
        runAll(o);
        return 0;
    }

    while (true) {
        std::cout << "\n  ЛАБОРАТОРІЯ КАРТКОВИХ ІГОР\n"
                  << "  1. Карта для преферансу і beat()\n"
                  << "  2. Клас Decks: представлення карт, роздавання, фіксоване роздавання\n"
                  << "  3. 4 гравцям по 5 карт: каре і стріт-флеш (Монте-Карло)\n"
                  << "  4. Те саме з фіксованими 5 картами на 1-й руці\n"
                  << "  5. Клас \"Гра\": покер, преферанс, дурень, блекджек, тисяча, пасьянс\n"
                  << "  6. Преферанс: 10 взяток і мізер (без прикупу / з прикупом)\n"
                  << "  7. Преферанс: фіксоване роздавання, розклад козирів 4:0 … 0:4\n"
                  << "  8. Усі задачі підряд\n"
                  << "  0. Вихід\n";
        const auto choice = ui::readLine("\nВаш вибір: ");
        if (!choice || *choice == "0") break;
        try {
            if (*choice == "1") taskCard();
            else if (*choice == "2") taskDecks(o);
            else if (*choice == "3") taskPoker(o);
            else if (*choice == "4") taskPokerFixed(o);
            else if (*choice == "5") taskGames(o);
            else if (*choice == "6") taskPreferans(o);
            else if (*choice == "7") taskTrumpSplit(o);
            else if (*choice == "8") runAll(o);
        } catch (const std::exception& e) {
            std::cout << "Помилка: " << e.what() << '\n';
        }
    }
    return 0;
}
