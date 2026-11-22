#include "poker/gui/Render.hpp"

#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

#ifndef HOLDEM_ASSETS_DIR
#define HOLDEM_ASSETS_DIR "assets"
#endif

namespace poker::gui {

namespace {

Font g_regular{};
Font g_bold{};
bool g_fontsLoaded = false;
Vector2 g_mouse{0, 0};
bool g_bigBlinds = false;
int g_bigBlind = 20;

const Font& fontFor(bool bold) {
    if (!g_fontsLoaded) {
        static const Font fallback = GetFontDefault();
        return fallback;
    }
    return bold ? g_bold : g_regular;
}

std::vector<int> fontCodepoints() {
    std::vector<int> cps;
    for (int c = 32; c < 127; ++c) cps.push_back(c);
    for (int c = 0x400; c <= 0x4FF; ++c) cps.push_back(c);
    for (int c = 0x2660; c <= 0x2667; ++c) cps.push_back(c);
    const int extra[] = {0x00A0, 0x00AB, 0x00BB, 0x00B1, 0x00B7, 0x00BD, 0x00BE, 0x00D7, 0x2212, 0x02BC, 0x2013, 0x2014,
                         0x2019, 0x2022, 0x2026, 0x2116, 0x2190, 0x2191, 0x2192, 0x2193, 0x2248,
                         0x2264, 0x2265, 0x25B2, 0x25B6, 0x25BC, 0x25C0, 0x25CF, 0x2605, 0x2713, 0x2715};
    cps.insert(cps.end(), std::begin(extra), std::end(extra));
    return cps;
}

Font loadFontFile(const std::string& fileName, int size) {
    const std::string candidates[] = {
        std::string(GetApplicationDirectory()) + "assets/fonts/" + fileName,
        std::string(HOLDEM_ASSETS_DIR) + "/fonts/" + fileName,
        "assets/fonts/" + fileName,
    };
    auto cps = fontCodepoints();
    for (const auto& path : candidates) {
        if (!FileExists(path.c_str())) continue;
        Font font = LoadFontEx(path.c_str(), size, cps.data(), static_cast<int>(cps.size()));
        if (font.texture.id != 0) {
            GenTextureMipmaps(&font.texture);
            SetTextureFilter(font.texture, TEXTURE_FILTER_TRILINEAR);
            return font;
        }
    }
    TraceLog(LOG_WARNING, "Шрифт %s не знайдено — кирилиця не відображатиметься", fileName.c_str());
    return Font{};
}

}

void loadFonts(float dpiScale) {
    const int size = static_cast<int>(std::clamp(48.0f * dpiScale, 48.0f, 96.0f));
    g_regular = loadFontFile("DejaVuSans.ttf", size);
    g_bold = loadFontFile("DejaVuSans-Bold.ttf", size);
    g_fontsLoaded = g_regular.texture.id != 0 && g_bold.texture.id != 0;
}

void unloadFonts() {
    if (g_regular.texture.id != 0) UnloadFont(g_regular);
    if (g_bold.texture.id != 0) UnloadFont(g_bold);
    g_fontsLoaded = false;
}

Vector2 measureText(const std::string& text, float size, bool bold) {
    return MeasureTextEx(fontFor(bold), text.c_str(), size, 0.0f);
}

void drawText(const std::string& text, float x, float y, float size, Color color, bool bold) {
    DrawTextEx(fontFor(bold), text.c_str(), {std::round(x), std::round(y)}, size, 0.0f, color);
}

void drawTextCentered(const std::string& text, Vector2 center, float size, Color color, bool bold) {
    const Vector2 m = measureText(text, size, bold);
    drawText(text, center.x - m.x / 2, center.y - m.y / 2, size, color, bold);
}

void drawTextRight(const std::string& text, float right, float y, float size, Color color, bool bold) {
    drawText(text, right - measureText(text, size, bold).x, y, size, color, bold);
}

float drawTextWrapped(const std::string& text, Rectangle box, float size, Color color, bool bold) {
    const float lineHeight = size * 1.3f;
    float y = box.y;
    std::istringstream paragraphs(text);
    std::string paragraph;
    while (std::getline(paragraphs, paragraph)) {
        std::istringstream words(paragraph);
        std::string word, line;
        while (words >> word) {
            const std::string candidate = line.empty() ? word : line + " " + word;
            if (!line.empty() && measureText(candidate, size, bold).x > box.width) {
                drawText(line, box.x, y, size, color, bold);
                y += lineHeight;
                line = word;
            } else {
                line = candidate;
            }
        }
        if (!line.empty()) {
            drawText(line, box.x, y, size, color, bold);
            y += lineHeight;
        }
    }
    return y - box.y;
}

void setVirtualMouse(Vector2 position) { g_mouse = position; }
Vector2 mouse() { return g_mouse; }
bool hovered(Rectangle r) { return CheckCollisionPointRec(g_mouse, r); }
bool clicked(Rectangle r) { return hovered(r) && IsMouseButtonReleased(MOUSE_BUTTON_LEFT); }

Color withAlpha(Color c, float alpha) {
    c.a = static_cast<unsigned char>(std::clamp(alpha, 0.0f, 1.0f) * static_cast<float>(c.a));
    return c;
}

Color lerpColor(Color a, Color b, float t) {
    auto mix = [t](unsigned char x, unsigned char y) {
        return static_cast<unsigned char>(static_cast<float>(x) + (static_cast<float>(y) - static_cast<float>(x)) * t);
    };
    return {mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a)};
}

void setChipUnits(bool bigBlinds, int bigBlind) {
    g_bigBlinds = bigBlinds;
    g_bigBlind = std::max(1, bigBlind);
}

bool showingBigBlinds() { return g_bigBlinds; }

std::string formatChips(int amount) {
    if (!g_bigBlinds) return formatChipsRaw(amount);
    const double bb = static_cast<double>(amount) / g_bigBlind;
    char buffer[32];
    if (std::fabs(bb - std::round(bb)) < 0.05) std::snprintf(buffer, sizeof buffer, "%.0f ББ", bb);
    else std::snprintf(buffer, sizeof buffer, "%.1f ББ", bb);
    return buffer;
}

std::string formatSigned(double chips) {
    char buffer[32];
    if (g_bigBlinds) std::snprintf(buffer, sizeof buffer, "%+.2f ББ", chips / g_bigBlind);
    else std::snprintf(buffer, sizeof buffer, "%+.1f", chips);
    return buffer;
}

std::string formatChipsRaw(int amount) {
    std::string digits = std::to_string(std::abs(amount));
    std::string result;
    for (std::size_t i = 0; i < digits.size(); ++i) {
        if (i > 0 && (digits.size() - i) % 3 == 0) result += ' ';
        result += digits[i];
    }
    return amount < 0 ? "-" + result : result;
}

std::string formatPercent(double fraction) {
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%.1f%%", fraction * 100.0);
    return buffer;
}

std::string firstLetter(const std::string& text) {
    if (text.empty()) return "?";
    int bytes = 0;
    GetCodepoint(text.c_str(), &bytes);
    return text.substr(0, static_cast<std::size_t>(std::max(1, bytes)));
}

void drawShadow(Rectangle r, float roundness, float spread) {
    for (int i = 3; i >= 1; --i) {
        const float s = spread * static_cast<float>(i) / 3.0f;
        DrawRectangleRounded({r.x - s + 2, r.y - s + 4, r.width + 2 * s, r.height + 2 * s}, roundness,
                             8, Color{0, 0, 0, static_cast<unsigned char>(28)});
    }
}

void drawPanel(Rectangle r, Color color, float roundness) {
    drawShadow(r, roundness, 5.0f);
    DrawRectangleRounded(r, roundness, 10, color);
    DrawRectangleRoundedLinesEx(r, roundness, 10, 1.0f, Color{255, 255, 255, 22});
}

bool button(Rectangle r, const std::string& label, ButtonStyle style, bool enabled, bool highlight,
            float fontSize) {
    Color base{52, 62, 78, 255};
    Color textColor = theme::Text;
    switch (style) {
        case ButtonStyle::Normal: base = {52, 62, 78, 255}; break;
        case ButtonStyle::Primary: base = {38, 120, 220, 255}; break;
        case ButtonStyle::Danger: base = {170, 52, 52, 255}; break;
        case ButtonStyle::Gold: base = {214, 160, 40, 255}; textColor = {30, 22, 6, 255}; break;
        case ButtonStyle::Ghost: base = {255, 255, 255, 18}; break;
    }
    const bool hover = enabled && hovered(r);
    const bool pressed = hover && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    if (!enabled) {
        base = {48, 52, 60, 200};
        textColor = {120, 126, 136, 255};
    } else if (pressed) {
        base = lerpColor(base, BLACK, 0.2f);
    } else if (hover) {
        base = lerpColor(base, WHITE, 0.14f);
    }

    const float roundness = 0.35f;
    if (highlight && enabled) {
        const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(GetTime()) * 5.0f);
        const Rectangle glow{r.x - 4, r.y - 4, r.width + 8, r.height + 8};
        DrawRectangleRounded(glow, roundness, 10, withAlpha(theme::Gold, 0.25f + 0.35f * pulse));
    }
    if (style != ButtonStyle::Ghost) drawShadow(r, roundness, 3.0f);
    DrawRectangleRounded(r, roundness, 10, base);
    DrawRectangleRounded({r.x + 2, r.y + 2, r.width - 4, r.height * 0.45f}, roundness, 10,
                         Color{255, 255, 255, static_cast<unsigned char>(enabled ? 16 : 6)});
    if (highlight && enabled) DrawRectangleRoundedLinesEx(r, roundness, 10, 2.5f, theme::Gold);

    drawTextCentered(label, {r.x + r.width / 2, r.y + r.height / 2}, fontSize, textColor, true);
    if (hover) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
    return enabled && clicked(r);
}

void drawCard(Card card, Rectangle r, float alpha, bool raised) {
    if (raised) r.y -= r.height * 0.12f;
    const float roundness = 0.16f;
    drawShadow(r, roundness, 4.0f);
    DrawRectangleRounded(r, roundness, 8, withAlpha(Color{252, 252, 250, 255}, alpha));
    DrawRectangleRoundedLinesEx(r, roundness, 8, 1.0f, withAlpha(Color{180, 184, 190, 255}, alpha));
    if (raised) DrawRectangleRoundedLinesEx(r, roundness, 8, 3.0f, withAlpha(theme::Gold, alpha));

    const Color ink = withAlpha(card.isRed() ? theme::CardRed : theme::CardBlack, alpha);
    const std::string rank = card.rank() == Rank::Ten ? "10" : std::string(1, rankToChar(card.rank()));
    const std::string suit = suitSymbol(card.suit());

    const float rankSize = r.height * 0.27f;
    const float pad = r.width * 0.09f;
    drawText(rank, r.x + pad, r.y + r.height * 0.04f, rankSize, ink, true);
    drawText(suit, r.x + pad, r.y + r.height * 0.04f + rankSize * 0.95f, rankSize * 0.8f, ink);

    drawTextCentered(suit, {r.x + r.width * 0.56f, r.y + r.height * 0.62f}, r.height * 0.48f, ink);

    if (r.height >= 80) {
        DrawTextPro(fontFor(true), rank.c_str(), {r.x + r.width - pad, r.y + r.height - r.height * 0.04f},
                    {0, 0}, 180.0f, rankSize * 0.8f, 0.0f, withAlpha(ink, 0.85f));
    }
}

void drawCardBack(Rectangle r, float alpha) {
    const float roundness = 0.16f;
    drawShadow(r, roundness, 3.0f);
    DrawRectangleRounded(r, roundness, 8, withAlpha(Color{250, 250, 248, 255}, alpha));
    const Rectangle inner{r.x + 3, r.y + 3, r.width - 6, r.height - 6};
    DrawRectangleRounded(inner, roundness, 8, withAlpha(Color{150, 28, 44, 255}, alpha));

    const float step = std::max(6.0f, r.width / 5.0f);
    for (float y = inner.y + step / 2; y < inner.y + inner.height - step / 3; y += step / 2) {
        const bool odd = static_cast<int>((y - inner.y) / (step / 2)) % 2 == 1;
        for (float x = inner.x + (odd ? step : step / 2); x < inner.x + inner.width - step / 3; x += step) {
            DrawPoly({x, y}, 4, step * 0.28f, 0.0f, withAlpha(Color{196, 60, 76, 255}, alpha));
        }
    }
    DrawRectangleRoundedLinesEx({inner.x + 2, inner.y + 2, inner.width - 4, inner.height - 4}, roundness,
                                8, 1.0f, withAlpha(Color{255, 220, 220, 120}, alpha));
}

void drawCardRotated(Card card, Vector2 center, Vector2 size, float angle, float alpha, bool raised) {
    rlPushMatrix();
    rlTranslatef(center.x, center.y, 0.0f);
    rlRotatef(angle, 0.0f, 0.0f, 1.0f);
    drawCard(card, {-size.x / 2, -size.y / 2, size.x, size.y}, alpha, raised);
    rlPopMatrix();
}

void drawCardBackRotated(Vector2 center, Vector2 size, float angle, float alpha) {
    rlPushMatrix();
    rlTranslatef(center.x, center.y, 0.0f);
    rlRotatef(angle, 0.0f, 0.0f, 1.0f);
    drawCardBack({-size.x / 2, -size.y / 2, size.x, size.y}, alpha);
    rlPopMatrix();
}

void drawMiniCard(Card card, Rectangle r) {
    DrawRectangleRounded(r, 0.25f, 6, Color{248, 248, 246, 255});
    const std::string rank = card.rank() == Rank::Ten ? "10" : std::string(1, rankToChar(card.rank()));
    drawTextCentered(rank + suitSymbol(card.suit()), {r.x + r.width / 2, r.y + r.height / 2},
                     r.height * 0.6f, card.isRed() ? theme::CardRed : theme::CardBlack, true);
}

void drawChipStack(Vector2 base, int amount, bool showLabel) {
    if (amount <= 0) return;
    struct Denomination {
        int value;
        Color color;
    };
    const Denomination denominations[] = {{1000, {230, 180, 40, 255}}, {500, {120, 60, 170, 255}},
                                          {100, {30, 30, 36, 255}},    {25, {40, 150, 80, 255}},
                                          {5, {200, 44, 50, 255}},     {1, {220, 220, 225, 255}}};
    std::vector<Color> chips;
    int rest = amount;
    for (const auto& d : denominations) {
        while (rest >= d.value && chips.size() < 14) {
            chips.push_back(d.color);
            rest -= d.value;
        }
    }

    const int perColumn = 7;
    const int columns = static_cast<int>((chips.size() + perColumn - 1) / perColumn);
    const float colWidth = 24.0f;
    const float startX = base.x - (static_cast<float>(columns) - 1) * colWidth / 2;
    for (std::size_t i = 0; i < chips.size(); ++i) {
        const int col = static_cast<int>(i) / perColumn;
        const int row = static_cast<int>(i) % perColumn;
        const float x = startX + static_cast<float>(col) * colWidth;
        const float y = base.y - static_cast<float>(row) * 4.0f;
        DrawEllipse(static_cast<int>(x), static_cast<int>(y + 2), 11.0f, 5.5f, Color{0, 0, 0, 90});
        DrawEllipse(static_cast<int>(x), static_cast<int>(y), 11.0f, 5.5f, lerpColor(chips[i], BLACK, 0.35f));
        DrawEllipse(static_cast<int>(x), static_cast<int>(y - 1.5f), 11.0f, 5.0f, chips[i]);
        DrawEllipseLines(static_cast<int>(x), static_cast<int>(y - 1.5f), 7.0f, 3.0f, Color{255, 255, 255, 110});
    }

    if (showLabel) {
        const std::string label = formatChips(amount);
        const Vector2 m = measureText(label, 15, true);
        const float lx = startX + static_cast<float>(columns) * colWidth - colWidth / 2 + 6;
        const Rectangle pill{lx, base.y - 10, m.x + 12, 20};
        DrawRectangleRounded(pill, 0.5f, 8, Color{0, 0, 0, 150});
        drawText(label, pill.x + 6, pill.y + 2, 15, theme::Text, true);
    }
}

void drawMeter(Rectangle r, double value, Color fill, double marker) {
    DrawRectangleRounded(r, 0.5f, 8, Color{255, 255, 255, 28});
    const float w = r.width * static_cast<float>(std::clamp(value, 0.0, 1.0));
    if (w > 2) DrawRectangleRounded({r.x, r.y, w, r.height}, 0.5f, 8, fill);
    if (marker >= 0.0) {
        const float mx = r.x + r.width * static_cast<float>(std::clamp(marker, 0.0, 1.0));
        DrawRectangleRec({mx - 1.5f, r.y - 5, 3, r.height + 10}, theme::Text);
        DrawTriangle({mx - 5, r.y - 9}, {mx, r.y - 3}, {mx + 5, r.y - 9}, theme::Text);
    }
}

}
