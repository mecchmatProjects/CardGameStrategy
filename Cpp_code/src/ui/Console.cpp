#include "poker/ui/Console.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

namespace poker::ui {

namespace {
bool g_colors = true;
}

void initConsole() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (handle != INVALID_HANDLE_VALUE && GetConsoleMode(handle, &mode)) {
        SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
}

void setColorsEnabled(bool enabled) { g_colors = enabled; }

bool colorsEnabled() { return g_colors; }

std::string paint(const std::string& text, const char* code) {
    if (!g_colors) return text;
    return std::string(code) + text + color::Reset;
}

std::string cardText(Card card) {
    return paint(card.pretty(), card.isRed() ? color::Red : color::Bold);
}

std::string cardsText(const std::vector<Card>& cards) {
    if (cards.empty()) return paint("—", color::Gray);
    std::string result;
    for (std::size_t i = 0; i < cards.size(); ++i) {
        if (i > 0) result += ' ';
        result += cardText(cards[i]);
    }
    return result;
}

std::string holeText(const HoleCards& hole) { return cardText(hole[0]) + " " + cardText(hole[1]); }

std::size_t displayWidth(const std::string& text) {
    std::size_t width = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const auto c = static_cast<unsigned char>(text[i]);
        if (c == 0x1B) {  // пропускаємо ANSI-послідовність \033[...m
            while (i < text.size() && text[i] != 'm') ++i;
            continue;
        }
        if ((c & 0xC0) != 0x80) ++width;  // рахуємо лише перші байти UTF-8 символів
    }
    return width;
}

std::string padRight(const std::string& text, std::size_t width) {
    const std::size_t w = displayWidth(text);
    return w >= width ? text : text + std::string(width - w, ' ');
}

std::string padLeft(const std::string& text, std::size_t width) {
    const std::size_t w = displayWidth(text);
    return w >= width ? text : std::string(width - w, ' ') + text;
}

std::string bar(double fraction, int width) {
    const int filled = static_cast<int>(std::lround(std::clamp(fraction, 0.0, 1.0) * width));
    std::string result;
    for (int i = 0; i < width; ++i) result += i < filled ? "█" : "░";
    return result;
}

void printTitle(const std::string& title) {
    std::cout << '\n' << paint("════ " + title + " ════", color::Cyan) << "\n\n";
}

std::string trim(const std::string& text) {
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return {};
    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::optional<std::string> readLine(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) return std::nullopt;
    return trim(line);
}

int readInt(const std::string& prompt, int min, int max, int defaultValue) {
    while (true) {
        const auto line = readLine(prompt + " [" + std::to_string(defaultValue) + "]: ");
        if (!line || line->empty()) return defaultValue;
        try {
            std::size_t used = 0;
            const int value = std::stoi(*line, &used);
            if (used == line->size() && value >= min && value <= max) return value;
        } catch (const std::exception&) {
            // некоректне число — запитуємо ще раз
        }
        std::cout << paint("  Введіть ціле число від " + std::to_string(min) + " до " +
                               std::to_string(max), color::Red)
                  << '\n';
    }
}

std::optional<std::vector<Card>> readCards(const std::string& prompt,
                                           const std::vector<std::size_t>& allowedCounts) {
    while (true) {
        const auto line = readLine(prompt);
        if (!line) return std::nullopt;
        try {
            auto cards = parseCards(*line);
            if (std::find(allowedCounts.begin(), allowedCounts.end(), cards.size()) !=
                allowedCounts.end()) {
                return cards;
            }
            std::string allowed;
            for (const auto n : allowedCounts) allowed += (allowed.empty() ? "" : ", ") + std::to_string(n);
            std::cout << paint("  Потрібна кількість карт: " + allowed, color::Red) << '\n';
        } catch (const std::exception& e) {
            std::cout << paint(std::string("  ") + e.what(), color::Red) << '\n';
        }
    }
}

}  // namespace poker::ui
