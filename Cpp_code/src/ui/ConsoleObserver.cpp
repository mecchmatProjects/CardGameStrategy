#include "poker/ui/ConsoleObserver.hpp"

#include "poker/ui/Console.hpp"

#include <iostream>

namespace poker::ui {

namespace {

const char* actionColor(ActionType type) {
    switch (type) {
        case ActionType::Fold: return color::Gray;
        case ActionType::Check:
        case ActionType::Call: return color::Green;
        case ActionType::Bet:
        case ActionType::Raise: return color::Yellow;
        case ActionType::AllIn: return color::Magenta;
        case ActionType::PostBlind: return color::Blue;
    }
    return color::Reset;
}

}

void ConsoleObserver::onHandStart(int handNumber, const std::vector<Player>& players,
                                  int dealerSeat) {
    std::cout << '\n'
              << paint("━━━━━━━━━━━━━━━━ Роздача #" + std::to_string(handNumber) +
                           " ━━━━━━━━━━━━━━━━", color::Cyan)
              << '\n';
    for (const auto& p : players) {
        std::string line = "  " + padRight(p.name(), 18) + padLeft(std::to_string(p.chips()), 7);
        if (p.seat() == dealerSeat) line += paint("  [D]", color::Yellow);
        if (p.isSittingOut()) line = paint(line + "  (вибув)", color::Gray);
        std::cout << line << '\n';
    }
    std::cout << '\n';
}

void ConsoleObserver::onHoleCards(const Player& player) {
    if (player.isHuman()) {
        std::cout << "  " << paint("Ваші карти:", color::Bold) << ' ' << holeText(player.hole()) << '\n';
    } else if (revealAll_) {
        std::cout << "  " << padRight(player.name(), 18) << holeText(player.hole()) << '\n';
    }
}

void ConsoleObserver::onAction(const Player& player, const Action& action, int pot) {
    std::cout << "  " << padRight(paint(player.name(), color::Bold), 18) << ' '
              << padRight(paint(action.toString(), actionColor(action.type)), 18)
              << paint("банк " + std::to_string(pot), color::Gray) << '\n';
}

void ConsoleObserver::onStreet(Street street, const std::vector<Card>& board, int pot) {
    std::cout << '\n'
              << paint("── " + streetName(street) + " ──", color::Cyan) << "  " << cardsText(board)
              << "   " << paint("банк " + std::to_string(pot), color::Gray) << '\n';
}

void ConsoleObserver::onShowdown(const std::vector<ShowdownEntry>& entries,
                                 const std::vector<Player>& players) {
    std::cout << '\n' << paint("── Шоудаун ──", color::Cyan) << '\n';
    for (const auto& e : entries) {
        std::cout << "  " << padRight(players[static_cast<std::size_t>(e.seat)].name(), 18)
                  << holeText(e.hole) << "   " << e.value.describe() << '\n';
    }
}

void ConsoleObserver::onPotAwarded(const PotResult& result, const std::vector<Player>& players) {
    const std::string potName =
        result.index == 0 ? "банк" : "сайд-пот #" + std::to_string(result.index);
    for (const auto& w : result.winners) {
        std::cout << "  " << paint("★ " + players[static_cast<std::size_t>(w.seat)].name() +
                                       " виграє " + std::to_string(w.amount) + " (" + potName + ")",
                                   color::Yellow)
                  << " — " << result.description << '\n';
    }
}

}
