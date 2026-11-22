#include "poker/cards/PokerTasks.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace poker::cards {

bool hasFourOfAKind(const int* hand, int n) {
    std::array<int, 17> counts{};
    for (int i = 0; i < n; ++i) {
        if (++counts[static_cast<std::size_t>(Decks::rankOf(hand[i]))] == 4) return true;
    }
    return false;
}

bool isStraightFlush(const int* hand, int n) {
    std::array<int, 16> ranks{};
    const int suit = Decks::suitOf(hand[0]);
    for (int i = 0; i < n; ++i) {
        if (Decks::suitOf(hand[i]) != suit) return false;
        ranks[static_cast<std::size_t>(i)] = Decks::rankOf(hand[i]);
    }
    std::sort(ranks.begin(), ranks.begin() + n);
    for (int i = 1; i < n; ++i) {
        if (ranks[static_cast<std::size_t>(i)] != ranks[static_cast<std::size_t>(i - 1)] + 1) return false;
    }
    return true;
}

namespace {

void countBatch(const DealResult& deal, int firstHand, PokerTaskResult& result) {
    const std::size_t m = deal.hands.rows();
    const int n = static_cast<int>(deal.hands.cols());
    for (std::size_t d = 0; d < deal.hands.depth(); ++d) {
        bool quads = false, flush = false;
        for (std::size_t p = static_cast<std::size_t>(firstHand); p < m; ++p) {
            const int* hand = deal.hands.hand(d, p);
            quads = quads || hasFourOfAKind(hand, n);
            flush = flush || isStraightFlush(hand, n);
        }
        result.quads.hits += quads;
        result.straightFlush.hits += flush;
    }
    result.quads.trials += static_cast<long long>(deal.hands.depth());
    result.straightFlush.trials += static_cast<long long>(deal.hands.depth());
}

}

double exactOneHand(const std::vector<int>& cards, int n, const std::function<bool(const int*, int)>& predicate) {
    const int total = static_cast<int>(cards.size());
    if (n < 1 || n > total) throw std::invalid_argument("exactOneHand: некоректне n");
    std::vector<int> idx(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) idx[static_cast<std::size_t>(i)] = i;
    std::vector<int> hand(static_cast<std::size_t>(n));
    long long hits = 0, all = 0;
    while (true) {
        for (int i = 0; i < n; ++i) hand[static_cast<std::size_t>(i)] = cards[static_cast<std::size_t>(idx[static_cast<std::size_t>(i)])];
        hits += predicate(hand.data(), n);
        ++all;
        int j = n - 1;
        while (j >= 0 && idx[static_cast<std::size_t>(j)] == total - n + j) --j;
        if (j < 0) break;
        ++idx[static_cast<std::size_t>(j)];
        for (int k = j + 1; k < n; ++k) idx[static_cast<std::size_t>(k)] = idx[static_cast<std::size_t>(k - 1)] + 1;
    }
    return static_cast<double>(hits) / static_cast<double>(all);
}

PokerTaskResult pokerHandsRandom(Decks& decks, long long N, int m, int n, int batch) {
    PokerTaskResult result;
    result.handsChecked = m;
    for (long long done = 0; done < N; done += batch) {
        const int size = static_cast<int>(std::min<long long>(batch, N - done));
        countBatch(decks.deal(size, m, n), 0, result);
    }
    result.exactQuadsOneHand = exactOneHand(decks.cards(), n, hasFourOfAKind);
    result.exactStraightFlushOneHand = exactOneHand(decks.cards(), n, isStraightFlush);
    return result;
}

PokerTaskResult pokerHandsFixed(Decks& decks, long long N, const std::vector<int>& firstHand, int m, int n, int batch) {
    PokerTaskResult result;
    result.handsChecked = m - 1;
    for (long long done = 0; done < N; done += batch) {
        const int size = static_cast<int>(std::min<long long>(batch, N - done));
        countBatch(decks.dealFixed(size, m, n, firstHand), 1, result);
    }
    std::vector<int> remaining = decks.cards();
    for (const int code : firstHand) remaining.erase(std::find(remaining.begin(), remaining.end(), code));
    result.exactQuadsOneHand = exactOneHand(remaining, n, hasFourOfAKind);
    result.exactStraightFlushOneHand = exactOneHand(remaining, n, isStraightFlush);
    return result;
}

}
