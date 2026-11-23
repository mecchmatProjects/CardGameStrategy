#include "poker/analysis/Odds.hpp"

#include "poker/analysis/EquityCalculator.hpp"
#include "poker/eval/HandEvaluator.hpp"

#include <iomanip>
#include <sstream>

namespace poker {

double potOdds(int toCall, int pot) {
    if (toCall <= 0) return 0.0;
    return static_cast<double>(toCall) / static_cast<double>(pot + toCall);
}

double callEV(double equity, int pot, int toCall) {
    return equity * static_cast<double>(pot + toCall) - static_cast<double>(toCall);
}

double bluffBreakEven(int bet, int pot) {
    if (bet <= 0) return 0.0;
    return static_cast<double>(bet) / static_cast<double>(pot + bet);
}

OutsInfo countOuts(const HoleCards& hole, const std::vector<Card>& board) {
    OutsInfo info;
    if (board.size() != 3 && board.size() != 4) return info;

    CardMask used = hole[0].mask() | hole[1].mask() | toMask(board);
    const HandCategory current = evaluateHand(hole, board).category;

    std::vector<Card> next = board;
    next.push_back(Card{});
    for (int i = 0; i < kDeckSize; ++i) {
        const Card card = Card::fromIndex(i);
        if (used & card.mask()) continue;
        next.back() = card;
        const HandCategory improved = evaluateHand(hole, next).category;
        const HandCategory boardOnly = evaluateHand(next).category;
        if (improved > current && improved > boardOnly) info.outs.push_back(card);
    }

    const int outs = static_cast<int>(info.outs.size());
    info.unseen = kDeckSize - 2 - static_cast<int>(board.size());
    info.nextCard = static_cast<double>(outs) / info.unseen;
    if (board.size() == 3) {
        const double missBoth = static_cast<double>(combinations(info.unseen - outs, 2)) /
                                static_cast<double>(combinations(info.unseen, 2));
        info.byRiver = 1.0 - missBoth;
        info.ruleOf2And4 = outs * 4;
    } else {
        info.byRiver = info.nextCard;
        info.ruleOf2And4 = outs * 2;
    }
    return info;
}

std::string percentString(double fraction) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(1) << fraction * 100.0 << '%';
    return os.str();
}

}
