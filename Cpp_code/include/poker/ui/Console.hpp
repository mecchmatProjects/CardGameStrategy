#pragma once

#include "poker/core/Card.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace poker::ui {

/// ANSI-коди кольорів.
namespace color {
inline constexpr const char* Reset = "\033[0m";
inline constexpr const char* Bold = "\033[1m";
inline constexpr const char* Red = "\033[31m";
inline constexpr const char* Green = "\033[32m";
inline constexpr const char* Yellow = "\033[33m";
inline constexpr const char* Blue = "\033[34m";
inline constexpr const char* Magenta = "\033[35m";
inline constexpr const char* Cyan = "\033[36m";
inline constexpr const char* Gray = "\033[90m";
}  // namespace color

/// Налаштовує консоль (UTF-8 і ANSI-кольори у Windows).
void initConsole();

void setColorsEnabled(bool enabled);
bool colorsEnabled();

/// Обгортає текст у колір (якщо кольори ввімкнені).
std::string paint(const std::string& text, const char* code);

std::string cardText(Card card);
std::string cardsText(const std::vector<Card>& cards);
std::string holeText(const HoleCards& hole);

/// Кількість символів на екрані (UTF-8 і ANSI-коди враховуються).
std::size_t displayWidth(const std::string& text);
std::string padRight(const std::string& text, std::size_t width);
std::string padLeft(const std::string& text, std::size_t width);

/// Смуга з блоків для візуалізації частки 0..1.
std::string bar(double fraction, int width = 20);

void printTitle(const std::string& title);

std::string trim(const std::string& text);

/// Читає рядок. std::nullopt — кінець вводу (Ctrl+D / Ctrl+Z).
std::optional<std::string> readLine(const std::string& prompt);

/// Читає ціле число в межах [min, max]; Enter — значення за замовчуванням.
int readInt(const std::string& prompt, int min, int max, int defaultValue);

/// Читає карти; повторює запит, поки ввід некоректний. allowedCounts — дозволена кількість карт.
std::optional<std::vector<Card>> readCards(const std::string& prompt,
                                           const std::vector<std::size_t>& allowedCounts);

}  // namespace poker::ui
