#include "TestFramework.hpp"

#include "poker/cards/CardGame.hpp"
#include "poker/cards/Decks.hpp"
#include "poker/cards/PokerTasks.hpp"
#include "poker/cards/PrefCard.hpp"
#include "poker/cards/Preferans.hpp"

#include <numeric>
#include <set>

using namespace poker::cards;

namespace {

int code(const char* rank, const char* suit) { return Decks::toInternal({rank, suit}); }

std::uint8_t mask(std::initializer_list<int> ranks) {
    std::uint8_t m = 0;
    for (const int r : ranks) m = static_cast<std::uint8_t>(m | (1u << (r - 7)));
    return m;
}

}

TEST_CASE(pref_card_codes) {
    CHECK_EQ((Card{Rank::Nine, Suit::Clubs}.code()), 209);
    CHECK_EQ((Card{Rank::Ace, Suit::Hearts}.code()), 414);
    CHECK(Card::fromCode(312) == (Card{Rank::Queen, Suit::Diamonds}));
    CHECK_THROWS(Card::fromCode(515));
    CHECK_THROWS(Card::fromCode(115));
}

TEST_CASE(pref_card_beat) {
    const Card kingH{Rank::King, Suit::Hearts}, queenH{Rank::Queen, Suit::Hearts};
    const Card sevenS{Rank::Seven, Suit::Spades}, aceD{Rank::Ace, Suit::Diamonds};
    CHECK(beat(kingH, queenH, Suit::Spades));
    CHECK(!beat(queenH, kingH, Suit::Spades));
    CHECK(beat(sevenS, aceD, Suit::Spades));
    CHECK(!beat(aceD, sevenS, Suit::Spades));
    CHECK(!beat(aceD, queenH, Suit::Clubs));
    CHECK(beat(kingH, queenH, Suit::Hearts));
}

TEST_CASE(decks_conversion) {
    CHECK_EQ(code("9", "трефа"), 209);
    CHECK_EQ(code("Queen", "чирва"), 412);
    CHECK_EQ(code("10", "піка"), 110);
    CHECK_EQ(Decks::suitOf(209), 2);
    CHECK_EQ(Decks::rankOf(209), 9);
    const ExternalCard e = Decks::toExternal(314);
    CHECK_EQ(e.first, std::string("Ace"));
    CHECK_EQ(e.second, std::string("бубна"));
    for (int s = 1; s <= 4; ++s) {
        for (int r = 2; r <= 14; ++r) CHECK_EQ(Decks::toInternal(Decks::toExternal(s * 100 + r)), s * 100 + r);
    }
    CHECK_THROWS(Decks::toInternal({"1", "піка"}));
    CHECK_THROWS(Decks::toInternal({"Ace", "зірка"}));
}

TEST_CASE(decks_sizes) {
    CHECK_EQ(Decks(2, 0, 1, 1).size(), 52);
    CHECK_EQ(Decks(6, 0, 1, 1).size(), 36);
    CHECK_EQ(Decks(7, 0, 1, 1).size(), 32);
    CHECK_EQ(Decks(DeckType::Great, 1).size(), 54);
    CHECK_EQ(Decks(DeckType::Short, 1).size(), 32);
    CHECK_EQ(Decks(DeckType::Standard, 1, 6).size(), 312);
}

TEST_CASE(decks_deal_shapes_and_unique) {
    Decks decks(2, 0, 1, 42);
    const DealResult r = decks.deal(100, 4, 5);
    CHECK_EQ(r.hands.depth(), std::size_t{100});
    CHECK_EQ(r.hands.rows(), std::size_t{4});
    CHECK_EQ(r.hands.cols(), std::size_t{5});
    CHECK_EQ(r.rest.rows(), std::size_t{100});
    CHECK_EQ(r.rest.cols(), std::size_t{32});
    for (std::size_t d = 0; d < 100; ++d) {
        std::set<int> seen;
        for (std::size_t p = 0; p < 4; ++p)
            for (std::size_t c = 0; c < 5; ++c) seen.insert(r.hands(d, p, c));
        for (std::size_t c = 0; c < 32; ++c) seen.insert(r.rest(d, c));
        CHECK_EQ(seen.size(), std::size_t{52});
    }
}

TEST_CASE(decks_deal_fixed) {
    Decks decks(7, 0, 1, 7);
    const std::vector<int> first = {code("Ace", "піка"), code("King", "піка"), code("Queen", "піка")};
    const std::vector<int> excluded = {code("7", "чирва")};
    const DealResult r = decks.dealFixed(200, 3, 3, first, excluded);
    for (std::size_t d = 0; d < 200; ++d) {
        for (std::size_t c = 0; c < 3; ++c) CHECK_EQ(r.hands(d, 0, c), first[c]);
        std::set<int> seen;
        for (std::size_t p = 0; p < 3; ++p)
            for (std::size_t c = 0; c < 3; ++c) seen.insert(r.hands(d, p, c));
        for (std::size_t c = 0; c < r.rest.cols(); ++c) seen.insert(r.rest(d, c));
        CHECK_EQ(seen.size(), std::size_t{31});
        CHECK(seen.count(excluded[0]) == 0);
    }
    CHECK_THROWS(decks.dealFixed(1, 3, 3, {code("2", "піка"), 0, 0}));
}

TEST_CASE(poker_task_predicates) {
    const int quads[] = {102, 202, 302, 402, 114};
    const int notQuads[] = {102, 202, 302, 403, 114};
    const int sf[] = {209, 205, 207, 206, 208};
    const int wheel[] = {114, 102, 103, 104, 105};
    const int flush[] = {102, 104, 106, 108, 110};
    CHECK(hasFourOfAKind(quads, 5));
    CHECK(!hasFourOfAKind(notQuads, 5));
    CHECK(isStraightFlush(sf, 5));
    CHECK(!isStraightFlush(wheel, 5));
    CHECK(!isStraightFlush(flush, 5));
}

TEST_CASE(poker_task_exact_values) {
    const std::vector<int> deck = Decks(2, 0, 1, 1).cards();
    CHECK_NEAR(exactOneHand(deck, 5, hasFourOfAKind), 624.0 / 2598960.0, 1e-12);
    CHECK_NEAR(exactOneHand(deck, 5, isStraightFlush), 36.0 / 2598960.0, 1e-12);
}

TEST_CASE(poker_task_monte_carlo_close_to_exact) {
    Decks decks(2, 0, 1, 2026);
    const PokerTaskResult r = pokerHandsRandom(decks, 400000);
    const double expected = r.approxAny(r.exactQuadsOneHand);
    CHECK(std::abs(r.quads.p() - expected) < 4 * r.quads.stdError());
}

TEST_CASE(card_game_presets) {
    const CardGame pref = CardGame::preferans();
    CHECK_EQ(pref.deckSize(), 32);
    CHECK_EQ(CardGame::durak().deckSize(), 36);
    CHECK_EQ(CardGame::thousand().deckSize(), 24);
    CHECK_EQ(CardGame::blackjack().deckSize(), 312);
    CHECK_EQ(CardGame::blackjack().points(code("King", "піка")), 10);
    CHECK_EQ(CardGame::blackjack().points(code("Ace", "піка")), 11);
    CHECK(CardGame::thousand().beats(code("10", "піка"), code("King", "піка")));
    CHECK(!pref.beats(code("10", "піка"), code("King", "піка")));
    CHECK(pref.beats(code("7", "трефа"), code("Ace", "піка"), 2));
    const DealResult r = pref.deal(3, 3, 1);
    CHECK_EQ(r.hands.cols(), std::size_t{10});
    CHECK_EQ(r.rest.cols(), std::size_t{2});
}

TEST_CASE(preferans_strong_trump) {
    CHECK(strongTrump(mask({14, 13, 12, 11})));
    CHECK(strongTrump(mask({14, 13, 12, 8, 7})));
    CHECK(!strongTrump(mask({14, 13, 12, 7})));
    CHECK(strongTrump(mask({14, 13, 10, 9, 8, 7})));
    CHECK(!strongTrump(mask({14, 13, 9, 8, 7})));
    CHECK(strongTrump(mask({14, 12, 11, 10, 9, 8, 7})));
    CHECK(strongTrump(0xFF));
    CHECK(!strongTrump(mask({13, 12, 11, 10, 9, 8, 7})));
    CHECK(topCards(mask({14})));
    CHECK(topCards(mask({14, 13, 12})));
    CHECK(!topCards(mask({14, 12})));
    CHECK(!topCards(mask({13})));
}

TEST_CASE(preferans_all_tricks) {
    CHECK(guaranteesAllTricks({mask({14, 13, 12, 11, 9, 8, 7}), mask({14, 13, 12}), 0, 0}));
    CHECK(!guaranteesAllTricks({mask({14, 13, 12, 11, 9, 8, 7}), mask({14, 13}), mask({13}), 0}));
    CHECK(guaranteesAllTricks({mask({14, 13, 12, 8, 7}), mask({14, 13, 12, 9, 8}), 0, 0}));
    CHECK(!guaranteesAllTricks({mask({14, 13, 12, 11, 8, 7}), mask({14, 13, 12, 9}), 0, 0}));
}

TEST_CASE(preferans_misere) {
    CHECK(misereSafeSuit(0));
    CHECK(misereSafeSuit(mask({7})));
    CHECK(misereSafeSuit(mask({7, 8})));
    CHECK(misereSafeSuit(mask({7, 9})));
    CHECK(!misereSafeSuit(mask({7, 10})));
    CHECK(!misereSafeSuit(mask({8})));
    CHECK(misereSafeSuit(mask({7, 8, 11})));
    CHECK(misereSafeSuit(mask({7, 9, 10})));
    CHECK(!misereSafeSuit(mask({7, 9, 12})));
    CHECK(misereSafeSuit(mask({7, 9, 11, 13})));
    CHECK(!misereSafeSuit(mask({7, 9, 11, 14})));
    CHECK(misereSafeSuit(mask({7, 8, 10, 12, 14})));
}

TEST_CASE(preferans_talon_helps) {
    const int hand[] = {code("Ace", "піка"), code("King", "піка"), code("Queen", "піка"), code("Jack", "піка"),
                        code("9", "піка"),   code("8", "піка"),    code("7", "піка"),     code("Ace", "трефа"),
                        code("King", "трефа"), code("7", "бубна")};
    const int talon[] = {code("10", "піка"), code("Queen", "трефа")};
    CHECK(!guaranteesAllTricks(toMasks(hand, 10)));
    CHECK(withTalon(hand, talon, guaranteesAllTricks));
}

TEST_CASE(preferans_trump_split) {
    const std::vector<int> hand = {code("Ace", "піка"), code("King", "піка"), code("10", "піка"), code("8", "піка"),
                                   code("Ace", "трефа"), code("King", "трефа"), code("9", "трефа"),
                                   code("Ace", "бубна"), code("10", "бубна"), code("King", "чирва")};
    const std::vector<int> discards = {code("7", "чирва"), code("8", "бубна")};
    Decks decks(7, 0, 1, 11);
    const TrumpSplitResult r = trumpSplit(decks, 50000, hand, discards, Suit::Spades);
    CHECK_EQ(r.remainingTrumps, 4);
    CHECK_EQ(std::accumulate(r.counts.begin(), r.counts.end(), 0LL), 50000LL);
    CHECK_NEAR(std::accumulate(r.exact.begin(), r.exact.end(), 0.0), 1.0, 1e-12);
    CHECK_NEAR(r.exact[2], 0.4180, 1e-4);
    CHECK_NEAR(r.exact[4], 0.0433, 1e-4);
    for (int k = 0; k <= 4; ++k) {
        CHECK_NEAR(static_cast<double>(r.counts[static_cast<std::size_t>(k)]) / 50000.0, r.exact[static_cast<std::size_t>(k)], 0.01);
    }
}

TEST_CASE(preferans_layouts_runs) {
    Decks decks(7, 0, 1, 5);
    const PreferansResult r = preferansLayouts(decks, 20000, 5000);
    CHECK_EQ(r.tenTricksNoTalon.trials, 20000LL);
    CHECK(r.tenTricksWithTalon.hits >= r.tenTricksNoTalon.hits);
    CHECK(r.misereWithTalon.hits >= r.misereNoTalon.hits);
    CHECK(r.misereWithTalon.p() > 0.03 && r.misereWithTalon.p() < 0.08);
}
