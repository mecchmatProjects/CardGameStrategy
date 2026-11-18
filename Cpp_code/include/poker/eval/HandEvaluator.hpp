#pragma once

#include "poker/core/Card.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace poker {

/// Категорія покерної комбінації (від найслабшої до найсильнішої).
enum class HandCategory : std::uint8_t {
    HighCard = 0,
    OnePair,
    TwoPair,
    ThreeOfAKind,
    Straight,
    Flush,
    FullHouse,
    FourOfAKind,
    StraightFlush
};

std::string categoryName(HandCategory category);

/// Результат оцінки руки. Дві руки порівнюються просто за полем score.
struct HandValue {
    HandCategory category = HandCategory::HighCard;
    std::array<int, 5> ranks{};  ///< Значущі ранги у порядку важливості (0 — немає)
    std::uint32_t score = 0;     ///< Чим більше — тим сильніша рука

    /// Людський опис, напр. "Дві пари KK і 77, кікер A".
    std::string describe() const;

    friend bool operator<(const HandValue& a, const HandValue& b) { return a.score < b.score; }
    friend bool operator>(const HandValue& a, const HandValue& b) { return a.score > b.score; }
    friend bool operator==(const HandValue& a, const HandValue& b) { return a.score == b.score; }
    friend bool operator!=(const HandValue& a, const HandValue& b) { return a.score != b.score; }
    friend bool operator<=(const HandValue& a, const HandValue& b) { return a.score <= b.score; }
    friend bool operator>=(const HandValue& a, const HandValue& b) { return a.score >= b.score; }
};

/// Оцінює найкращу п'ятикарткову комбінацію з 1..7 карт.
/// Для менше ніж 5 карт стріт і флеш неможливі (корисно для аналізу борду).
/// @throws std::invalid_argument якщо карт 0 або більше 7
HandValue evaluateHand(const Card* cards, std::size_t count);
HandValue evaluateHand(const std::vector<Card>& cards);
HandValue evaluateHand(const HoleCards& hole, const std::vector<Card>& board);

/// П'ять карт, що утворюють найкращу комбінацію (для підсвічування на шоудауні).
/// Потрібно щонайменше 3 карти на борді.
std::vector<Card> bestFiveCards(const HoleCards& hole, const std::vector<Card>& board);

}  // namespace poker
