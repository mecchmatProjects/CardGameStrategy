#include "poker/analysis/EquityCalculator.hpp"

#include "poker/analysis/HandRanking.hpp"
#include "poker/eval/HandEvaluator.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace poker {

namespace {

class Accumulator {
public:
    explicit Accumulator(std::size_t players) : win_(players), tie_(players), equity_(players) {}

    void add(const std::vector<std::uint32_t>& scores) {
        const std::uint32_t best = *std::max_element(scores.begin(), scores.end());
        const auto winners = std::count(scores.begin(), scores.end(), best);
        for (std::size_t i = 0; i < scores.size(); ++i) {
            if (scores[i] != best) continue;
            if (winners == 1) {
                win_[i] += 1.0;
            } else {
                tie_[i] += 1.0;
            }
            equity_[i] += 1.0 / static_cast<double>(winners);
        }
    }

    std::vector<EquityResult> result(long long trials) const {
        std::vector<EquityResult> out(win_.size());
        const double t = trials > 0 ? static_cast<double>(trials) : 1.0;
        for (std::size_t i = 0; i < out.size(); ++i) {
            out[i] = {win_[i] / t, tie_[i] / t, equity_[i] / t};
        }
        return out;
    }

private:
    std::vector<double> win_, tie_, equity_;
};

void pickRandom(std::vector<Card>& pool, int k, Rng& rng) {
    const int size = static_cast<int>(pool.size());
    for (int i = 0; i < k; ++i) {
        const int j = rng.uniformInt(i, size - 1);
        std::swap(pool[static_cast<std::size_t>(i)], pool[static_cast<std::size_t>(j)]);
    }
}

std::vector<Card> remainingCards(CardMask used) {
    std::vector<Card> pool;
    for (int i = 0; i < kDeckSize; ++i) {
        const Card card = Card::fromIndex(i);
        if (!(used & card.mask())) pool.push_back(card);
    }
    return pool;
}

std::uint32_t scoreOf(const HoleCards& hole, const std::array<Card, 5>& board) {
    const std::array<Card, 7> cards{hole[0], hole[1], board[0], board[1], board[2], board[3], board[4]};
    return evaluateHand(cards.data(), cards.size()).score;
}

bool hasDraw(const HoleCards& hole, const std::vector<Card>& board) {
    std::array<int, kNumSuits> suits{};
    std::array<int, kNumSuits> holeSuits{};
    std::uint16_t ranks = 0, holeRanks = 0;
    for (const Card c : board) {
        ++suits[static_cast<std::size_t>(c.suit())];
        ranks = static_cast<std::uint16_t>(ranks | (1u << c.rankValue()));
    }
    for (const Card c : hole) {
        ++suits[static_cast<std::size_t>(c.suit())];
        ++holeSuits[static_cast<std::size_t>(c.suit())];
        ranks = static_cast<std::uint16_t>(ranks | (1u << c.rankValue()));
        holeRanks = static_cast<std::uint16_t>(holeRanks | (1u << c.rankValue()));
    }
    for (std::size_t s = 0; s < suits.size(); ++s) {
        if (suits[s] == 4 && holeSuits[s] > 0) return true;
    }
    if (ranks & (1u << 14)) ranks = static_cast<std::uint16_t>(ranks | 2u);
    if (holeRanks & (1u << 14)) holeRanks = static_cast<std::uint16_t>(holeRanks | 2u);
    for (int low = 1; low <= 10; ++low) {
        const auto window = static_cast<std::uint16_t>(0x1Fu << low);
        int count = 0;
        for (int r = low; r < low + 5; ++r) count += (ranks >> r) & 1;
        if (count >= 4 && (holeRanks & window)) return true;
    }
    return false;
}

double strengthWeight(const HoleCards& hole, const std::vector<Card>& board, const RangeModel& range) {
    if (!range.showsStrength || board.size() < 3) return 1.0;
    const HandCategory withHole = evaluateHand(hole, board).category;
    const HandCategory boardOnly = evaluateHand(board).category;
    if (withHole > boardOnly) return 1.0;
    return hasDraw(hole, board) ? 0.7 : range.bluffShare;
}

enum class MadeClass { Strong, TopPair, WeakPair, Draw, Air };

MadeClass classify(const HoleCards& hole, const std::vector<Card>& board) {
    const HandValue withHole = evaluateHand(hole, board);
    const HandCategory boardOnly = evaluateHand(board).category;
    if (withHole.category > boardOnly) {
        if (withHole.category >= HandCategory::TwoPair) return MadeClass::Strong;
        int topBoard = 0;
        for (const Card c : board) topBoard = std::max(topBoard, c.rankValue());
        return withHole.ranks[0] >= topBoard ? MadeClass::TopPair : MadeClass::WeakPair;
    }
    return hasDraw(hole, board) ? MadeClass::Draw : MadeClass::Air;
}

double actionWeight(const HoleCards& hole, const std::vector<Card>& board, const OpponentRange& r) {
    if (board.size() < 3 || (r.bets == 0 && r.raises == 0 && r.calls == 0 && r.bigCalls == 0 && !r.checkedThisStreet)) {
        return 1.0;
    }
    const MadeClass c = classify(hole, board);
    double betF = 1, raiseF = 1, callF = 1, bigF = 1, checkF = 1;
    switch (c) {
        case MadeClass::Strong:   betF = 1.0;  raiseF = 1.0;  callF = 0.7;  bigF = 1.0;  checkF = 0.45; break;
        case MadeClass::TopPair:  betF = 0.75; raiseF = 0.35; callF = 1.0;  bigF = 0.45; checkF = 0.8;  break;
        case MadeClass::WeakPair: betF = 0.35; raiseF = 0.08; callF = 0.7;  bigF = 0.12; break;
        case MadeClass::Draw:     betF = 0.55; raiseF = 0.35; callF = 0.8;  bigF = 0.3;  break;
        case MadeClass::Air:      betF = r.bluffShare; raiseF = r.bluffShare * 0.4; callF = 0.12; bigF = 0.03; break;
    }
    raiseF += (betF - raiseF) * r.aggressive;
    bigF += (callF - bigF) * r.aggressive;
    double w = std::pow(betF, r.bets) * std::pow(raiseF, r.raises) * std::pow(callF, r.calls) * std::pow(bigF, r.bigCalls);
    if (r.checkedThisStreet) w *= checkF;
    return w;
}

}

long long combinations(int n, int k) {
    if (k < 0 || k > n) return 0;
    k = std::min(k, n - k);
    long long result = 1;
    for (int i = 1; i <= k; ++i) result = result * (n - k + i) / i;
    return result;
}

EquityCalculator::EquityCalculator(std::uint64_t seed) : rng_(seed) {}

EquityReport EquityCalculator::calculate(const std::vector<HoleCards>& hands,
                                         const std::vector<Card>& board, int iterations,
                                         long long exactLimit) {
    if (hands.size() < 2) throw std::invalid_argument("Потрібно щонайменше дві руки");
    if (board.size() > 5) throw std::invalid_argument("На борді не може бути більше 5 карт");

    CardMask used = 0;
    auto use = [&used](Card card) {
        if (used & card.mask()) {
            throw std::invalid_argument("Карта використана двічі: " + card.toString());
        }
        used |= card.mask();
    };
    for (const auto& hand : hands) {
        use(hand[0]);
        use(hand[1]);
    }
    for (const Card card : board) use(card);

    std::vector<Card> pool = remainingCards(used);
    const int known = static_cast<int>(board.size());
    const int missing = 5 - known;

    std::array<Card, 5> fullBoard{};
    std::copy(board.begin(), board.end(), fullBoard.begin());

    Accumulator acc(hands.size());
    std::vector<std::uint32_t> scores(hands.size());
    auto evaluateRunout = [&]() {
        for (std::size_t h = 0; h < hands.size(); ++h) scores[h] = scoreOf(hands[h], fullBoard);
        acc.add(scores);
    };

    EquityReport report;
    const int poolSize = static_cast<int>(pool.size());
    const long long variants = combinations(poolSize, missing);

    if (variants <= exactLimit) {
        report.exact = true;
        std::vector<int> idx(static_cast<std::size_t>(missing));
        std::iota(idx.begin(), idx.end(), 0);
        while (true) {
            for (int j = 0; j < missing; ++j) {
                fullBoard[static_cast<std::size_t>(known + j)] = pool[static_cast<std::size_t>(idx[static_cast<std::size_t>(j)])];
            }
            evaluateRunout();
            ++report.trials;

            int j = missing - 1;
            while (j >= 0 && idx[static_cast<std::size_t>(j)] == poolSize - missing + j) --j;
            if (j < 0) break;
            ++idx[static_cast<std::size_t>(j)];
            for (int k = j + 1; k < missing; ++k) {
                idx[static_cast<std::size_t>(k)] = idx[static_cast<std::size_t>(k - 1)] + 1;
            }
        }
    } else {
        for (int it = 0; it < iterations; ++it) {
            pickRandom(pool, missing, rng_);
            for (int j = 0; j < missing; ++j) {
                fullBoard[static_cast<std::size_t>(known + j)] = pool[static_cast<std::size_t>(j)];
            }
            evaluateRunout();
        }
        report.trials = iterations;
    }

    report.players = acc.result(report.trials);
    return report;
}

EquityResult EquityCalculator::vsRandom(const HoleCards& hero, const std::vector<Card>& board,
                                        int opponents, int iterations) {
    if (opponents < 1 || opponents > 9) throw std::invalid_argument("Суперників має бути 1..9");
    if (board.size() > 5) throw std::invalid_argument("На борді не може бути більше 5 карт");
    if (hero[0] == hero[1]) throw std::invalid_argument("Однакові карти в руці");

    CardMask used = hero[0].mask() | hero[1].mask();
    for (const Card card : board) {
        if (used & card.mask()) throw std::invalid_argument("Карта використана двічі: " + card.toString());
        used |= card.mask();
    }

    std::vector<Card> pool = remainingCards(used);
    const int known = static_cast<int>(board.size());
    const int missing = 5 - known;
    const int needed = missing + 2 * opponents;

    std::array<Card, 5> fullBoard{};
    std::copy(board.begin(), board.end(), fullBoard.begin());

    double win = 0.0, tie = 0.0, equity = 0.0;
    for (int it = 0; it < iterations; ++it) {
        pickRandom(pool, needed, rng_);
        for (int j = 0; j < missing; ++j) {
            fullBoard[static_cast<std::size_t>(known + j)] = pool[static_cast<std::size_t>(j)];
        }
        const std::uint32_t heroScore = scoreOf(hero, fullBoard);

        std::uint32_t bestOpponent = 0;
        int tiedOpponents = 0;
        for (int o = 0; o < opponents; ++o) {
            const auto base = static_cast<std::size_t>(missing + 2 * o);
            const std::uint32_t s = scoreOf({pool[base], pool[base + 1]}, fullBoard);
            if (s > bestOpponent) {
                bestOpponent = s;
                tiedOpponents = 1;
            } else if (s == bestOpponent) {
                ++tiedOpponents;
            }
        }

        if (heroScore > bestOpponent) {
            win += 1.0;
            equity += 1.0;
        } else if (heroScore == bestOpponent) {
            tie += 1.0;
            equity += 1.0 / (tiedOpponents + 1);
        }
    }

    const double n = iterations > 0 ? static_cast<double>(iterations) : 1.0;
    return {win / n, tie / n, equity / n};
}

}

namespace poker {

EquityResult EquityCalculator::vsRange(const HoleCards& hero, const std::vector<Card>& board, int opponents,
                                       const RangeModel& range, int iterations) {
    if (opponents < 1 || opponents > 9) throw std::invalid_argument("Суперників має бути 1..9");
    if (board.size() > 5) throw std::invalid_argument("На борді не може бути більше 5 карт");
    if (hero[0] == hero[1]) throw std::invalid_argument("Однакові карти в руці");

    CardMask used = hero[0].mask() | hero[1].mask();
    for (const Card card : board) {
        if (used & card.mask()) throw std::invalid_argument("Карта використана двічі: " + card.toString());
        used |= card.mask();
    }

    const std::vector<Card> pool = remainingCards(used);
    const int n = static_cast<int>(pool.size());
    const int known = static_cast<int>(board.size());
    const int missing = 5 - known;

    std::array<Card, 5> fullBoard{};
    std::copy(board.begin(), board.end(), fullBoard.begin());
    std::vector<HoleCards> hands(static_cast<std::size_t>(opponents));

    auto randomHand = [&](CardMask dead) {
        while (true) {
            const Card a = pool[static_cast<std::size_t>(rng_.uniformInt(0, n - 1))];
            const Card b = pool[static_cast<std::size_t>(rng_.uniformInt(0, n - 1))];
            if (a != b && !(dead & a.mask()) && !(dead & b.mask())) return HoleCards{a, b};
        }
    };

    double win = 0.0, tie = 0.0, equity = 0.0;
    for (int it = 0; it < iterations; ++it) {
        CardMask dead = used;
        for (auto& hand : hands) {
            HoleCards chosen = randomHand(dead);
            for (int attempt = 0; attempt < 25; ++attempt) {
                const auto candidate = range.top >= 0.999 ? std::optional<HoleCards>(randomHand(dead))
                                                          : sampleFromTop(range.top, dead, rng_);
                if (!candidate) break;
                chosen = *candidate;
                if (rng_.uniform01() < strengthWeight(chosen, board, range)) break;
            }
            hand = chosen;
            dead |= chosen[0].mask() | chosen[1].mask();
        }
        for (int j = 0; j < missing; ++j) {
            Card c;
            do {
                c = pool[static_cast<std::size_t>(rng_.uniformInt(0, n - 1))];
            } while (dead & c.mask());
            dead |= c.mask();
            fullBoard[static_cast<std::size_t>(known + j)] = c;
        }

        const std::uint32_t heroScore = scoreOf(hero, fullBoard);
        std::uint32_t bestOpponent = 0;
        int tiedOpponents = 0;
        for (const auto& hand : hands) {
            const std::uint32_t s = scoreOf(hand, fullBoard);
            if (s > bestOpponent) {
                bestOpponent = s;
                tiedOpponents = 1;
            } else if (s == bestOpponent) {
                ++tiedOpponents;
            }
        }
        if (heroScore > bestOpponent) {
            win += 1.0;
            equity += 1.0;
        } else if (heroScore == bestOpponent) {
            tie += 1.0;
            equity += 1.0 / (tiedOpponents + 1);
        }
    }

    const double total = iterations > 0 ? static_cast<double>(iterations) : 1.0;
    return {win / total, tie / total, equity / total};
}

EquityResult EquityCalculator::vsOpponents(const HoleCards& hero, const std::vector<Card>& board,
                                           const std::vector<OpponentRange>& opponents, int iterations) {
    if (opponents.empty() || opponents.size() > 9) throw std::invalid_argument("Суперників має бути 1..9");
    if (board.size() > 5) throw std::invalid_argument("На борді не може бути більше 5 карт");

    CardMask used = hero[0].mask() | hero[1].mask();
    for (const Card card : board) {
        if (used & card.mask()) throw std::invalid_argument("Карта використана двічі: " + card.toString());
        used |= card.mask();
    }
    const std::vector<Card> pool = remainingCards(used);
    const int n = static_cast<int>(pool.size());
    const int known = static_cast<int>(board.size());
    const int missing = 5 - known;

    std::array<Card, 5> fullBoard{};
    std::copy(board.begin(), board.end(), fullBoard.begin());
    std::vector<HoleCards> hands(opponents.size());

    auto randomHand = [&](CardMask dead) {
        while (true) {
            const Card a = pool[static_cast<std::size_t>(rng_.uniformInt(0, n - 1))];
            const Card b = pool[static_cast<std::size_t>(rng_.uniformInt(0, n - 1))];
            if (a != b && !(dead & a.mask()) && !(dead & b.mask())) return HoleCards{a, b};
        }
    };

    double win = 0.0, tie = 0.0, equity = 0.0;
    for (int it = 0; it < iterations; ++it) {
        CardMask dead = used;
        for (std::size_t o = 0; o < opponents.size(); ++o) {
            const OpponentRange& r = opponents[o];
            HoleCards chosen = randomHand(dead);
            for (int attempt = 0; attempt < 40; ++attempt) {
                const auto candidate = r.top >= 0.999 ? std::optional<HoleCards>(randomHand(dead))
                                                      : sampleFromTop(r.top, dead, rng_);
                if (!candidate) break;
                chosen = *candidate;
                if (rng_.uniform01() < actionWeight(chosen, board, r)) break;
            }
            hands[o] = chosen;
            dead |= chosen[0].mask() | chosen[1].mask();
        }
        for (int j = 0; j < missing; ++j) {
            Card c;
            do {
                c = pool[static_cast<std::size_t>(rng_.uniformInt(0, n - 1))];
            } while (dead & c.mask());
            dead |= c.mask();
            fullBoard[static_cast<std::size_t>(known + j)] = c;
        }

        const std::uint32_t heroScore = scoreOf(hero, fullBoard);
        std::uint32_t best = 0;
        int tied = 0;
        for (const auto& hand : hands) {
            const std::uint32_t sc = scoreOf(hand, fullBoard);
            if (sc > best) {
                best = sc;
                tied = 1;
            } else if (sc == best) {
                ++tied;
            }
        }
        if (heroScore > best) {
            win += 1.0;
            equity += 1.0;
        } else if (heroScore == best) {
            tie += 1.0;
            equity += 1.0 / (tied + 1);
        }
    }
    const double total = iterations > 0 ? static_cast<double>(iterations) : 1.0;
    return {win / total, tie / total, equity / total};
}

}
