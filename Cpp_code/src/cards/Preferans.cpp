#include "poker/cards/Preferans.hpp"

#include <algorithm>
#include <bitset>
#include <stdexcept>

namespace poker::cards {

namespace {

constexpr std::uint8_t kAce = 1u << 7, kKing = 1u << 6, kQueen = 1u << 5, kJack = 1u << 4;

int countBits(std::uint8_t mask) { return static_cast<int>(std::bitset<8>(mask).count()); }

int nthLowest(std::uint8_t mask, int n) {
    int seen = 0;
    for (int b = 0; b < 8; ++b) {
        if (mask & (1u << b)) {
            if (seen == n) return b;
            ++seen;
        }
    }
    return -1;
}

double binomial(int n, int k) {
    if (k < 0 || k > n) return 0.0;
    double r = 1.0;
    for (int i = 1; i <= k; ++i) r = r * (n - k + i) / i;
    return r;
}

}

SuitMasks toMasks(const int* cards, int count) {
    SuitMasks masks{};
    for (int i = 0; i < count; ++i) {
        const int suit = Decks::suitOf(cards[i]);
        const int rank = Decks::rankOf(cards[i]);
        if (suit < 1 || suit > 4 || rank < 7) throw std::invalid_argument("Преферанс: карта поза колодою з 32 карт");
        masks[static_cast<std::size_t>(suit - 1)] = static_cast<std::uint8_t>(masks[static_cast<std::size_t>(suit - 1)] | (1u << (rank - 7)));
    }
    return masks;
}

std::string describeHand(const int* cards, int count) {
    const SuitMasks m = toMasks(cards, count);
    static const char* names[] = {"7", "8", "9", "10", "J", "Q", "K", "A"};
    std::string out;
    for (std::size_t s = 0; s < 4; ++s) {
        if (!out.empty()) out += " | ";
        out += suitSymbol(static_cast<Suit>(s + 1));
        if (m[s] == 0) out += " —";
        for (int b = 7; b >= 0; --b) {
            if (m[s] & (1u << b)) out += std::string(" ") + names[b];
        }
    }
    return out;
}

bool strongTrump(std::uint8_t mask) {
    const int count = countBits(mask);
    const auto has = [mask](std::uint8_t bits) { return (mask & bits) == bits; };
    if (has(kAce | kKing | kQueen | kJack)) return true;
    if (has(kAce | kKing | kQueen) && count >= 5) return true;
    if (has(kAce | kKing) && count >= 6) return true;
    if (has(kAce) && count >= 7) return true;
    return count == 8;
}

bool topCards(std::uint8_t mask) {
    const int count = countBits(mask);
    const auto top = static_cast<std::uint8_t>((0xFFu << (8 - count)) & 0xFFu);
    return count > 0 && mask == top;
}

bool guaranteesAllTricks(const SuitMasks& hand) {
    for (std::size_t trump = 0; trump < 4; ++trump) {
        if (!strongTrump(hand[trump])) continue;
        const int trumpCount = countBits(hand[trump]);
        bool extraUsed = false;
        bool ok = true;
        for (std::size_t s = 0; s < 4 && ok; ++s) {
            if (s == trump || hand[s] == 0 || topCards(hand[s])) continue;
            const bool akq = (hand[s] & (kAce | kKing | kQueen)) == (kAce | kKing | kQueen) && countBits(hand[s]) >= 5;
            if (!extraUsed && akq && trumpCount <= 5) {
                extraUsed = true;
            } else {
                ok = false;
            }
        }
        if (ok) return true;
    }
    return false;
}

bool misereSafeSuit(std::uint8_t mask) {
    const int count = countBits(mask);
    if (count == 0) return true;
    if (nthLowest(mask, 0) != 0) return false;
    if (count >= 2 && nthLowest(mask, 1) > 2) return false;
    if (count >= 3 && nthLowest(mask, 2) > 4) return false;
    if (count >= 4 && nthLowest(mask, 3) > 6) return false;
    return true;
}

bool misereSafe(const SuitMasks& hand) {
    return std::all_of(hand.begin(), hand.end(), misereSafeSuit);
}

bool withTalon(const int* hand10, const int* talon2, bool (*check)(const SuitMasks&)) {
    int all[12];
    std::copy_n(hand10, 10, all);
    std::copy_n(talon2, 2, all + 10);
    const SuitMasks full = toMasks(all, 12);
    for (int i = 0; i < 12; ++i) {
        for (int j = i + 1; j < 12; ++j) {
            SuitMasks m = full;
            for (const int k : {i, j}) {
                const auto s = static_cast<std::size_t>(Decks::suitOf(all[k]) - 1);
                m[s] = static_cast<std::uint8_t>(m[s] & ~(1u << (Decks::rankOf(all[k]) - 7)));
            }
            if (check(m)) return true;
        }
    }
    return false;
}

PreferansResult preferansLayouts(Decks& decks, long long N, int batch) {
    if (decks.size() != 32) throw std::invalid_argument("Преферанс грають колодою з 32 карт (від сімки)");
    PreferansResult r;
    for (long long done = 0; done < N; done += batch) {
        const int size = static_cast<int>(std::min<long long>(batch, N - done));
        const DealResult deal = decks.deal(size, 3, 10);
        for (std::size_t d = 0; d < deal.hands.depth(); ++d) {
            const int* talon = deal.rest.row(d);
            bool ten = false, tenTalon = false, mis = false, misTalon = false;
            for (std::size_t p = 0; p < 3; ++p) {
                const int* hand = deal.hands.hand(d, p);
                const SuitMasks masks = toMasks(hand, 10);
                ten = ten || guaranteesAllTricks(masks);
                mis = mis || misereSafe(masks);
                tenTalon = tenTalon || withTalon(hand, talon, guaranteesAllTricks);
                misTalon = misTalon || withTalon(hand, talon, misereSafe);
            }
            r.tenTricksNoTalon.hits += ten;
            r.tenTricksWithTalon.hits += tenTalon;
            r.misereNoTalon.hits += mis;
            r.misereWithTalon.hits += misTalon;
        }
        for (Estimate* e : {&r.tenTricksNoTalon, &r.tenTricksWithTalon, &r.misereNoTalon, &r.misereWithTalon}) {
            e->trials += size;
        }
    }
    return r;
}

TrumpSplitResult trumpSplit(Decks& decks, long long N, const std::vector<int>& hand10, const std::vector<int>& discards,
                            Suit trump, int batch) {
    if (hand10.size() != 10 || discards.size() != 2) throw std::invalid_argument("trumpSplit: 10 карт руки і 2 знесені");
    const int trumpCode = static_cast<int>(trump);
    int known = 0;
    for (const auto* list : {&hand10, &discards}) {
        for (const int c : *list) known += Decks::suitOf(c) == trumpCode;
    }

    TrumpSplitResult r;
    r.remainingTrumps = 8 - known;
    r.counts.assign(static_cast<std::size_t>(r.remainingTrumps + 1), 0);
    for (long long done = 0; done < N; done += batch) {
        const int size = static_cast<int>(std::min<long long>(batch, N - done));
        const DealResult deal = decks.dealFixed(size, 3, 10, hand10, discards);
        for (std::size_t d = 0; d < deal.hands.depth(); ++d) {
            const int* second = deal.hands.hand(d, 1);
            const auto k = std::count_if(second, second + 10, [trumpCode](int c) { return Decks::suitOf(c) == trumpCode; });
            ++r.counts[static_cast<std::size_t>(k)];
        }
        r.trials += size;
    }

    const int rest = 20, t = r.remainingTrumps;
    for (int k = 0; k <= t; ++k) {
        r.exact.push_back(binomial(t, k) * binomial(rest - t, 10 - k) / binomial(rest, 10));
    }
    return r;
}

}
