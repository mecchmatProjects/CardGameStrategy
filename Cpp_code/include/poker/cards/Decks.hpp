#pragma once

#include "poker/cards/NdArray.hpp"
#include "poker/cards/PrefCard.hpp"
#include "poker/core/Random.hpp"

#include <cstdint>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace poker::cards {

    enum class DeckType {
        Standard,
        Great,
        Middle,
        Short
    };

    std::string deckTypeName(DeckType type);
    std::string cardsWord(int count);
    int deckMinRank(DeckType type);
    int deckJokers(DeckType type);

    using ExternalCard = std::pair<std::string, std::string>;

    inline constexpr int kRedJoker = 515;
    inline constexpr int kBlackJoker = 516;

    struct DealResult {
        Array3D hands;
        Array2D rest;
    };

    class Decks {
    public:
        explicit Decks(int minRank = 2, int jokers = 0, int decks = 1,
                       std::uint64_t seed = std::random_device{}());
        explicit Decks(DeckType type, std::uint64_t seed = std::random_device{}(), int decks = 1);

        static int toInternal(const ExternalCard& card);
        static ExternalCard toExternal(int code);
        static int suitOf(int code) { return code / 100; }
        static int rankOf(int code) { return code % 100; }
        static std::string toString(int code);

        int size() const { return static_cast<int>(cards_.size()); }
        int minRank() const { return minRank_; }
        const std::vector<int>& cards() const { return cards_; }
        bool contains(int code) const;

        DealResult deal(int N, int m, int n);

        DealResult dealFixed(int N, int m, int n, const std::vector<int>& firstHand,
                             const std::vector<int>& excluded = {});

        std::vector<int> shuffled();

    private:
        std::vector<int> pool(const std::vector<int>& removeA, const std::vector<int>& removeB) const;

        std::vector<int> cards_;
        int minRank_;
        Rng rng_;
    };

}
