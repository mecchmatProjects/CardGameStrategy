#include "poker/eval/HandEvaluator.hpp"

#include <algorithm>
#include <stdexcept>

namespace poker {

namespace {

int straightHigh(std::uint16_t mask) {
    if (mask & (1u << 14)) mask |= (1u << 1);
    for (int high = 14; high >= 5; --high) {
        const auto need = static_cast<std::uint16_t>(0x1Fu << (high - 4));
        if ((mask & need) == need) return high;
    }
    return 0;
}

HandValue makeValue(HandCategory category, const std::array<int, 5>& ranks) {
    HandValue value;
    value.category = category;
    value.ranks = ranks;
    auto score = static_cast<std::uint32_t>(category);
    for (const int r : ranks) score = (score << 4) | static_cast<std::uint32_t>(r);
    value.score = score;
    return value;
}

std::string rankStr(int rank) {
    return rank >= 2 ? std::string(1, rankToChar(static_cast<Rank>(rank))) : std::string();
}

std::string kickers(const std::array<int, 5>& ranks, std::size_t from, std::size_t to) {
    std::string result;
    for (std::size_t i = from; i < to; ++i) {
        if (ranks[i] < 2) continue;
        if (!result.empty()) result += ' ';
        result += rankStr(ranks[i]);
    }
    return result;
}

std::string withKickers(const std::string& base, const std::string& list, bool plural) {
    if (list.empty()) return base;
    return base + (plural ? ", кікери " : ", кікер ") + list;
}

}

std::string categoryName(HandCategory category) {
    switch (category) {
        case HandCategory::HighCard: return "Старша карта";
        case HandCategory::OnePair: return "Пара";
        case HandCategory::TwoPair: return "Дві пари";
        case HandCategory::ThreeOfAKind: return "Трійка (сет)";
        case HandCategory::Straight: return "Стріт";
        case HandCategory::Flush: return "Флеш";
        case HandCategory::FullHouse: return "Фул-хаус";
        case HandCategory::FourOfAKind: return "Каре";
        case HandCategory::StraightFlush: return "Стріт-флеш";
    }
    return "?";
}

std::string HandValue::describe() const {
    const std::string r0 = rankStr(ranks[0]);
    const std::string r1 = rankStr(ranks[1]);
    switch (category) {
        case HandCategory::HighCard:
            return withKickers("Старша карта " + r0, kickers(ranks, 1, 5), true);
        case HandCategory::OnePair:
            return withKickers("Пара " + r0 + r0, kickers(ranks, 1, 4), true);
        case HandCategory::TwoPair:
            return withKickers("Дві пари " + r0 + r0 + " і " + r1 + r1, kickers(ranks, 2, 3), false);
        case HandCategory::ThreeOfAKind:
            return withKickers("Трійка " + r0 + r0 + r0, kickers(ranks, 1, 3), true);
        case HandCategory::Straight:
            return "Стріт до " + r0;
        case HandCategory::Flush:
            return "Флеш: " + kickers(ranks, 0, 5);
        case HandCategory::FullHouse:
            return "Фул-хаус: " + r0 + r0 + r0 + " + " + r1 + r1;
        case HandCategory::FourOfAKind:
            return withKickers("Каре " + r0 + r0 + r0 + r0, kickers(ranks, 1, 2), false);
        case HandCategory::StraightFlush:
            return ranks[0] == 14 ? "Роял-флеш" : "Стріт-флеш до " + r0;
    }
    return "?";
}

HandValue evaluateHand(const Card* cards, std::size_t count) {
    if (count == 0 || count > 7) {
        throw std::invalid_argument("Оцінювати можна від 1 до 7 карт");
    }

    std::array<int, 15> counts{};
    std::array<std::uint16_t, kNumSuits> suitMask{};
    std::array<int, kNumSuits> suitCount{};
    std::uint16_t rankMask = 0;

    for (std::size_t i = 0; i < count; ++i) {
        const int r = cards[i].rankValue();
        const int s = static_cast<int>(cards[i].suit());
        ++counts[r];
        ++suitCount[s];
        suitMask[s] = static_cast<std::uint16_t>(suitMask[s] | (1u << r));
        rankMask = static_cast<std::uint16_t>(rankMask | (1u << r));
    }

    int flushSuit = -1;
    for (int s = 0; s < kNumSuits; ++s) {
        if (suitCount[s] >= 5) flushSuit = s;
    }
    if (flushSuit >= 0) {
        if (const int high = straightHigh(suitMask[flushSuit])) {
            return makeValue(HandCategory::StraightFlush, {high, 0, 0, 0, 0});
        }
    }

    int quad = 0;
    std::array<int, 3> trips{};
    std::array<int, 4> pairs{};
    std::array<int, 8> singles{};
    int nTrips = 0, nPairs = 0, nSingles = 0;
    for (int r = 14; r >= 2; --r) {
        switch (counts[r]) {
            case 4: quad = r; break;
            case 3: trips[nTrips++] = r; break;
            case 2: pairs[nPairs++] = r; break;
            case 1: singles[nSingles++] = r; break;
            default: break;
        }
    }
    auto highestExcept = [&counts](int a, int b) {
        for (int r = 14; r >= 2; --r) {
            if (counts[r] > 0 && r != a && r != b) return r;
        }
        return 0;
    };

    if (quad) {
        return makeValue(HandCategory::FourOfAKind, {quad, highestExcept(quad, 0), 0, 0, 0});
    }
    if (nTrips >= 1 && (nTrips >= 2 || nPairs >= 1)) {
        const int pairRank = std::max(nTrips >= 2 ? trips[1] : 0, nPairs >= 1 ? pairs[0] : 0);
        return makeValue(HandCategory::FullHouse, {trips[0], pairRank, 0, 0, 0});
    }
    if (flushSuit >= 0) {
        std::array<int, 5> top{};
        int k = 0;
        for (int r = 14; r >= 2 && k < 5; --r) {
            if (suitMask[flushSuit] & (1u << r)) top[k++] = r;
        }
        return makeValue(HandCategory::Flush, top);
    }
    if (const int high = straightHigh(rankMask)) {
        return makeValue(HandCategory::Straight, {high, 0, 0, 0, 0});
    }
    if (nTrips == 1) {
        return makeValue(HandCategory::ThreeOfAKind, {trips[0], singles[0], singles[1], 0, 0});
    }
    if (nPairs >= 2) {
        return makeValue(HandCategory::TwoPair,
                         {pairs[0], pairs[1], highestExcept(pairs[0], pairs[1]), 0, 0});
    }
    if (nPairs == 1) {
        return makeValue(HandCategory::OnePair,
                         {pairs[0], singles[0], singles[1], singles[2], 0});
    }
    return makeValue(HandCategory::HighCard,
                     {singles[0], singles[1], singles[2], singles[3], singles[4]});
}

HandValue evaluateHand(const std::vector<Card>& cards) {
    return evaluateHand(cards.data(), cards.size());
}

HandValue evaluateHand(const HoleCards& hole, const std::vector<Card>& board) {
    if (board.size() > 5) throw std::invalid_argument("На борді не може бути більше 5 карт");
    std::array<Card, 7> all{};
    all[0] = hole[0];
    all[1] = hole[1];
    std::copy(board.begin(), board.end(), all.begin() + 2);
    return evaluateHand(all.data(), board.size() + 2);
}

std::vector<Card> bestFiveCards(const HoleCards& hole, const std::vector<Card>& board) {
    if (board.size() < 3 || board.size() > 5) {
        throw std::invalid_argument("bestFiveCards: на борді має бути 3..5 карт");
    }
    std::vector<Card> all{hole[0], hole[1]};
    all.insert(all.end(), board.begin(), board.end());
    const int n = static_cast<int>(all.size());

    std::array<Card, 5> current{};
    std::vector<Card> best;
    std::uint32_t bestScore = 0;
    for (int a = 0; a < n; ++a)
        for (int b = a + 1; b < n; ++b)
            for (int c = b + 1; c < n; ++c)
                for (int d = c + 1; d < n; ++d)
                    for (int e = d + 1; e < n; ++e) {
                        current = {all[static_cast<std::size_t>(a)], all[static_cast<std::size_t>(b)],
                                   all[static_cast<std::size_t>(c)], all[static_cast<std::size_t>(d)],
                                   all[static_cast<std::size_t>(e)]};
                        const std::uint32_t score = evaluateHand(current.data(), 5).score;
                        if (best.empty() || score > bestScore) {
                            bestScore = score;
                            best.assign(current.begin(), current.end());
                        }
                    }
    return best;
}

}
