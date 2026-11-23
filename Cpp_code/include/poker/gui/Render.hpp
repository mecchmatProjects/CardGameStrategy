#pragma once

#include "poker/core/Card.hpp"

#include "raylib.h"

#include <string>
#include <vector>

namespace poker::gui {

inline constexpr float kWidth = 1280.0f;
inline constexpr float kHeight = 800.0f;

namespace theme {
inline constexpr Color Background{12, 28, 22, 255};
inline constexpr Color BackgroundTop{24, 52, 40, 255};
inline constexpr Color Felt{22, 110, 66, 255};
inline constexpr Color FeltDark{14, 82, 48, 255};
inline constexpr Color Rim{92, 52, 26, 255};
inline constexpr Color RimLight{140, 86, 44, 255};
inline constexpr Color Panel{16, 22, 30, 225};
inline constexpr Color PanelLight{34, 44, 58, 255};
inline constexpr Color Text{236, 240, 244, 255};
inline constexpr Color Muted{150, 162, 176, 255};
inline constexpr Color Gold{246, 196, 72, 255};
inline constexpr Color GoodGreen{88, 210, 120, 255};
inline constexpr Color BadRed{236, 92, 88, 255};
inline constexpr Color Accent{80, 160, 255, 255};
inline constexpr Color CardRed{200, 30, 40, 255};
inline constexpr Color CardBlack{24, 24, 30, 255};
}

void loadFonts(float dpiScale);
void unloadFonts();

Vector2 measureText(const std::string& text, float size, bool bold = false);
void drawText(const std::string& text, float x, float y, float size, Color color, bool bold = false);
void drawTextCentered(const std::string& text, Vector2 center, float size, Color color, bool bold = false);
void drawTextRight(const std::string& text, float right, float y, float size, Color color, bool bold = false);
float drawTextWrapped(const std::string& text, Rectangle box, float size, Color color, bool bold = false);

void setVirtualMouse(Vector2 position);
Vector2 mouse();
bool hovered(Rectangle r);
bool clicked(Rectangle r);

enum class ButtonStyle { Normal, Primary, Danger, Gold, Ghost };

bool button(Rectangle r, const std::string& label, ButtonStyle style = ButtonStyle::Normal,
            bool enabled = true, bool highlight = false, float fontSize = 18.0f);

void drawPanel(Rectangle r, Color color = theme::Panel, float roundness = 0.08f);
void drawShadow(Rectangle r, float roundness, float spread = 6.0f);

void drawCard(Card card, Rectangle r, float alpha = 1.0f, bool raised = false);
void drawCardBack(Rectangle r, float alpha = 1.0f);
void drawCardRotated(Card card, Vector2 center, Vector2 size, float angle, float alpha = 1.0f, bool raised = false);
void drawCardBackRotated(Vector2 center, Vector2 size, float angle, float alpha = 1.0f);
void drawMiniCard(Card card, Rectangle r);

void drawChipStack(Vector2 base, int amount, bool showLabel = true);

void drawMeter(Rectangle r, double value, Color fill, double marker = -1.0);

Color withAlpha(Color c, float alpha);
Color lerpColor(Color a, Color b, float t);
void setChipUnits(bool bigBlinds, int bigBlind);
bool showingBigBlinds();
std::string formatChips(int amount);
std::string formatChipsRaw(int amount);
std::string formatSigned(double chips);
std::string formatPercent(double fraction);
std::string firstLetter(const std::string& text);

}
