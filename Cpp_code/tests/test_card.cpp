#include "TestFramework.hpp"

#include "poker/core/Card.hpp"
#include "poker/core/Deck.hpp"

#include <set>

using namespace poker;

TEST_CASE(card_parses_from_string) {
    const Card card = Card::fromString("Ah");
    CHECK(card.rank() == Rank::Ace);
    CHECK(card.suit() == Suit::Hearts);
    CHECK_EQ(card.toString(), std::string("Ah"));
    CHECK(Card::fromString("10d") == Card(Rank::Ten, Suit::Diamonds));
    CHECK(Card::fromString("tc") == Card(Rank::Ten, Suit::Clubs));
}

TEST_CASE(card_rejects_invalid_text) {
    CHECK_THROWS(Card::fromString("Xh"));
    CHECK_THROWS(Card::fromString("A"));
    CHECK_THROWS(Card::fromString("Ahh"));
    CHECK_THROWS(Card::fromString("1h"));
}

TEST_CASE(card_index_round_trip) {
    std::set<int> indices;
    for (int i = 0; i < kDeckSize; ++i) {
        const Card card = Card::fromIndex(i);
        CHECK_EQ(card.index(), i);
        CHECK(Card::fromString(card.toString()) == card);
        indices.insert(card.index());
    }
    CHECK_EQ(indices.size(), std::size_t{52});
}

TEST_CASE(parse_cards_handles_separators_and_duplicates) {
    const auto cards = parseCards("AhKd 7c,10s");
    CHECK_EQ(cards.size(), std::size_t{4});
    CHECK(cards[3] == Card(Rank::Ten, Suit::Spades));
    CHECK_THROWS(parseCards("AhAh"));
    CHECK_THROWS(parseCards("AhK"));
    CHECK(parseCards("").empty());
}

TEST_CASE(deck_contains_52_unique_cards) {
    Deck deck;
    Rng rng(42);
    deck.shuffle(rng);
    CardMask seen = 0;
    while (!deck.empty()) seen |= deck.deal().mask();
    CHECK_EQ(seen, (CardMask{1} << 52) - 1);
    CHECK_THROWS(deck.deal());
}

TEST_CASE(deck_excludes_known_cards) {
    const CardMask excluded = toMask(parseCards("AhKh"));
    Deck deck(excluded);
    CHECK_EQ(deck.size(), std::size_t{50});
    while (!deck.empty()) CHECK((deck.deal().mask() & excluded) == 0);
}

TEST_CASE(same_seed_gives_same_shuffle) {
    Deck a, b;
    Rng rngA(7), rngB(7);
    a.shuffle(rngA);
    b.shuffle(rngB);
    for (int i = 0; i < 10; ++i) CHECK(a.deal() == b.deal());
}
