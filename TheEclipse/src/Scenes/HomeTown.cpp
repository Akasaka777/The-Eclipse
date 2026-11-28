#include "Scenes/HomeTown.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/GameConfig.h"
#include "Graphics/DrawUtil.h"

#include <cmath>

namespace ecl {
namespace town {

namespace {

// 通りに並ぶ順（西から東へ）。x は入口の中心
const Building kBuildings[] = {
    { Facility::Home,      "自宅",           "ステータス・装備・アイテム・スキル", 620.0f,  440.0f, 290.0f },
    { Facility::Shop,      "ショップ",       "武器・防具・アイテム・アクセサリー", 1620.0f, 540.0f, 270.0f },
    { Facility::Smith,     "鍛冶屋",         "装備の強化・修理",                   2640.0f, 520.0f, 280.0f },
    { Facility::Labyrinth, "迷宮の門",       "ローグライクモード（毎回変わる迷宮）", 3660.0f, 420.0f, 360.0f },
    { Facility::Dungeon,   "ダンジョン入口", "クエストを選んで出撃する",           4680.0f, 440.0f, 400.0f },
};
constexpr int kBuildingCount = static_cast<int>(sizeof(kBuildings) / sizeof(kBuildings[0]));

// 扉の上の吊り看板の位置（土台からの高さ。扉の上端より少し上）
constexpr float kSignAboveBase = 194.0f;
// 入口の案内の位置（土台からの高さ。吊り看板に重ならない高さ）
constexpr float kPromptAboveBase = 268.0f;

const ColorRGB kInk(16, 14, 20);
const ColorRGB kWindowLight(255, 214, 140);
const ColorRGB kLampLight(255, 196, 120);

// 塗った多角形（扇形に三角形を並べる。凸形のみ）
void FillConvex(const std::vector<Vec2>& pts, const ColorRGB& color, int alpha = 255)
{
    for (size_t i = 1; i + 1 < pts.size(); ++i) draw::Triangle(pts[0], pts[i], pts[i + 1], color, true, alpha);
}

// 明かりの灯った窓（十字の桟）
void DrawWindow(const Rect& r, float flicker)
{
    draw::Glow(r.CenterX(), r.CenterY(), r.Width() * 0.9f, kWindowLight, static_cast<int>(60.0f * flicker), 3);
    draw::FillRect(r.Expanded(4.0f), kInk, 255);
    draw::GradientRectV(r, kWindowLight, kWindowLight.Scaled(0.75f), 255, 6);
    draw::Line(r.CenterX(), r.top, r.CenterX(), r.bottom, kInk, 4.0f, 255);
    draw::Line(r.left, r.CenterY(), r.right, r.CenterY(), kInk, 4.0f, 255);
}

// 吊り看板（名前）
void DrawHangingSign(float cx, float top, const char* name, const ColorRGB& board, const ColorRGB& text)
{
    const float w = static_cast<float>(draw::TextWidth(FontSize::Small, name)) + 40.0f;
    const Rect r = Rect::FromCenter(cx, top + 26.0f, w, 44.0f);
    draw::Line(r.left + 14.0f, top - 12.0f, r.left + 14.0f, r.top, kInk, 3.0f, 255);
    draw::Line(r.right - 14.0f, top - 12.0f, r.right - 14.0f, r.top, kInk, 3.0f, 255);
    draw::FillRect(r.Expanded(3.0f), kInk, 255);
    draw::FillRect(r, board, 255);
    draw::StrokeRect(r.Expanded(-4.0f), text.Scaled(0.6f), 1.0f, 200);
    draw::Text(FontSize::Small, r.CenterX(), r.top + 9.0f, text, name, draw::TextAlign::Center);
}

// 扉（入口）。open が立っていれば中の明かりが漏れる
void DrawDoor(const Rect& r, const ColorRGB& wood, bool lit)
{
    draw::FillRect(r.Expanded(5.0f), kInk, 255);
    if (lit) {
        draw::Glow(r.CenterX(), r.bottom - r.Height() * 0.4f, r.Width() * 1.3f, kLampLight, 70, 4);
    }
    draw::GradientRectV(r, wood, wood.Scaled(0.7f), 255, 8);
    draw::Line(r.CenterX(), r.top + 6.0f, r.CenterX(), r.bottom, wood.Scaled(0.55f), 3.0f, 255);
    draw::Circle(r.CenterX() + 10.0f, r.CenterY() + 8.0f, 4.0f, ColorRGB(230, 196, 110), true, 1.0f, 255);
    draw::Circle(r.CenterX() - 10.0f, r.CenterY() + 8.0f, 4.0f, ColorRGB(230, 196, 110), true, 1.0f, 255);
}

//------------------------------------------------------------------------------
// 自宅：木組みの壁と赤い三角屋根。煙突から薄い煙
//------------------------------------------------------------------------------
void DrawHome(const Building& b, float x, float base, float time, bool lit)
{
    const float w = b.width;
    const float h = b.height;
    const Rect wall(x - w * 0.5f, base - h, x + w * 0.5f, base);
    const ColorRGB plaster(176, 150, 118);
    const ColorRGB timber(86, 60, 42);

    // 煙突と煙（屋根の後ろ）
    const Rect chimney(x + w * 0.22f, base - h - 150.0f, x + w * 0.32f, base - h - 30.0f);
    draw::FillRect(chimney.Expanded(3.0f), kInk, 255);
    draw::FillRect(chimney, ColorRGB(120, 84, 72), 255);
    for (int i = 0; i < 4; ++i) {
        const float t = std::fmod(time * 0.35f + static_cast<float>(i) * 0.25f, 1.0f);
        draw::Circle(chimney.CenterX() + std::sin(t * 6.0f + i) * 10.0f + t * 30.0f, chimney.top - t * 120.0f,
                     10.0f + t * 18.0f, ColorRGB(200, 200, 210), true, 1.0f, static_cast<int>(90.0f * (1.0f - t)));
    }

    // 壁と木組み
    draw::FillRect(wall.Expanded(4.0f), kInk, 255);
    draw::GradientRectV(wall, plaster, plaster.Scaled(0.78f), 255, 10);
    draw::FillRect(Rect(wall.left, wall.top, wall.right, wall.top + 14.0f), timber, 255);
    draw::FillRect(Rect(wall.left, base - 26.0f, wall.right, base), ColorRGB(96, 88, 84), 255);
    for (int i = 0; i <= 4; ++i) {
        const float px = wall.left + w * static_cast<float>(i) / 4.0f;
        draw::Line(px, wall.top, px, base - 26.0f, timber, 10.0f, 255);
    }
    draw::Line(wall.left, wall.top + h * 0.45f, wall.right, wall.top + h * 0.45f, timber, 8.0f, 255);

    // 屋根
    const float eave = 34.0f;
    FillConvex({ Vec2(wall.left - eave - 4.0f, wall.top + 4.0f), Vec2(x, wall.top - 154.0f),
                 Vec2(wall.right + eave + 4.0f, wall.top + 4.0f) }, kInk);
    FillConvex({ Vec2(wall.left - eave, wall.top), Vec2(x, wall.top - 146.0f), Vec2(wall.right + eave, wall.top) },
               ColorRGB(156, 62, 54));
    for (int i = 1; i < 5; ++i) {
        const float t = static_cast<float>(i) / 5.0f;
        draw::Line(math::Lerp(wall.left - eave, x, t), math::Lerp(wall.top, wall.top - 146.0f, t),
                   math::Lerp(wall.right + eave, x, t), math::Lerp(wall.top, wall.top - 146.0f, t),
                   ColorRGB(120, 44, 40), 3.0f, 255);
    }
    // 屋根裏の丸窓
    draw::Circle(x, wall.top - 56.0f, 22.0f, kInk, true, 1.0f, 255);
    draw::Circle(x, wall.top - 56.0f, 17.0f, kWindowLight, true, 1.0f, 230);

    // 窓と扉
    const float flicker = 0.85f + 0.15f * std::sin(time * 3.0f);
    DrawWindow(Rect::FromCenter(x - w * 0.30f, wall.top + h * 0.25f, 64.0f, 56.0f), flicker);
    DrawWindow(Rect::FromCenter(x + w * 0.30f, wall.top + h * 0.25f, 64.0f, 56.0f), flicker);
    DrawWindow(Rect::FromCenter(x - w * 0.30f, wall.top + h * 0.66f, 64.0f, 56.0f), flicker);
    DrawWindow(Rect::FromCenter(x + w * 0.30f, wall.top + h * 0.66f, 64.0f, 56.0f), flicker);
    DrawDoor(Rect(x - 42.0f, base - 136.0f, x + 42.0f, base - 2.0f), ColorRGB(108, 72, 46), lit);
    // 植木鉢
    for (int side = -1; side <= 1; side += 2) {
        const float px = x + side * 78.0f;
        draw::FillRect(Rect(px - 16.0f, base - 26.0f, px + 16.0f, base), ColorRGB(150, 90, 60), 255);
        draw::Circle(px, base - 34.0f, 18.0f, ColorRGB(70, 130, 80), true, 1.0f, 255);
        draw::Circle(px - 6.0f, base - 40.0f, 5.0f, ColorRGB(255, 170, 190), true, 1.0f, 255);
    }
    DrawHangingSign(x, base - kSignAboveBase, b.name, ColorRGB(70, 50, 36), ColorRGB(255, 226, 170));
}

//------------------------------------------------------------------------------
// ショップ：青い壁、縞模様の日よけ、品物の並んだ飾り窓、屋上の大看板
//------------------------------------------------------------------------------
void DrawShop(const Building& b, float x, float base, float time, bool lit)
{
    const float w = b.width;
    const float h = b.height;
    const Rect wall(x - w * 0.5f, base - h, x + w * 0.5f, base);
    const ColorRGB plaster(92, 116, 150);

    draw::FillRect(wall.Expanded(4.0f), kInk, 255);
    draw::GradientRectV(wall, plaster, plaster.Scaled(0.72f), 255, 10);
    // 軒の飾り（平屋根の縁）
    draw::FillRect(Rect(wall.left - 18.0f, wall.top - 22.0f, wall.right + 18.0f, wall.top + 4.0f), kInk, 255);
    draw::FillRect(Rect(wall.left - 14.0f, wall.top - 18.0f, wall.right + 14.0f, wall.top), ColorRGB(214, 200, 170), 255);
    draw::FillRect(Rect(wall.left, base - 24.0f, wall.right, base), ColorRGB(84, 84, 96), 255);

    // 屋上の大看板（硬貨の印）
    const Rect board = Rect::FromCenter(x, wall.top - 66.0f, 300.0f, 72.0f);
    draw::Line(board.left + 40.0f, board.bottom, board.left + 40.0f, wall.top - 18.0f, kInk, 6.0f, 255);
    draw::Line(board.right - 40.0f, board.bottom, board.right - 40.0f, wall.top - 18.0f, kInk, 6.0f, 255);
    draw::FillRect(board.Expanded(4.0f), kInk, 255);
    draw::GradientRectV(board, ColorRGB(60, 86, 140), ColorRGB(40, 58, 98), 255, 8);
    draw::StrokeRect(board.Expanded(-5.0f), ColorRGB(255, 214, 120), 2.0f, 230);
    draw::Circle(board.left + 42.0f, board.CenterY(), 20.0f, ColorRGB(255, 206, 90), true, 1.0f, 255);
    draw::Circle(board.left + 42.0f, board.CenterY(), 13.0f, ColorRGB(220, 160, 60), false, 3.0f, 255);
    draw::Text(FontSize::Medium, board.CenterX() + 22.0f, board.top + 14.0f, ColorRGB(255, 236, 190), b.name,
               draw::TextAlign::Center);

    // 飾り窓（棚に瓶と剣）
    const Rect display(wall.left + 30.0f, base - h * 0.55f, x - 70.0f, base - 40.0f);
    draw::FillRect(display.Expanded(5.0f), kInk, 255);
    draw::GradientRectV(display, kWindowLight.Scaled(0.95f), kWindowLight.Scaled(0.62f), 255, 6);
    for (int shelf = 0; shelf < 2; ++shelf) {
        const float sy = display.top + display.Height() * (0.45f + 0.45f * static_cast<float>(shelf));
        draw::Line(display.left, sy, display.right, sy, ColorRGB(110, 76, 50), 5.0f, 255);
        for (int k = 0; k < 5; ++k) {
            const float bx = display.left + 22.0f + static_cast<float>(k) * (display.Width() - 44.0f) / 4.0f;
            const ColorRGB potion = (k % 3 == 0) ? ColorRGB(220, 70, 80)
                                  : (k % 3 == 1) ? ColorRGB(70, 140, 240) : ColorRGB(120, 220, 140);
            draw::Circle(bx, sy - 13.0f, 10.0f, kInk, true, 1.0f, 255);
            draw::Circle(bx, sy - 13.0f, 8.0f, potion, true, 1.0f, 255);
            draw::FillRect(Rect(bx - 3.0f, sy - 30.0f, bx + 3.0f, sy - 22.0f), ColorRGB(200, 190, 170), 255);
        }
    }
    // 右の窓
    DrawWindow(Rect::FromCenter(x + w * 0.34f, base - h * 0.62f, 70.0f, 62.0f), 0.9f);

    // 縞の日よけ（扉と飾り窓の上）
    const float awningTop = base - h * 0.70f;
    const float awningLeft = wall.left + 16.0f;
    const float awningRight = x + 78.0f;
    const int stripes = 8;
    for (int i = 0; i < stripes; ++i) {
        const float x0 = math::Lerp(awningLeft, awningRight, static_cast<float>(i) / stripes);
        const float x1 = math::Lerp(awningLeft, awningRight, static_cast<float>(i + 1) / stripes);
        const ColorRGB c = (i % 2 == 0) ? ColorRGB(54, 110, 200) : ColorRGB(232, 232, 240);
        FillConvex({ Vec2(x0 + 6.0f, awningTop), Vec2(x1 + 6.0f, awningTop), Vec2(x1, awningTop + 52.0f),
                     Vec2(x0, awningTop + 52.0f) }, c);
        draw::Circle((x0 + x1) * 0.5f, awningTop + 52.0f, (x1 - x0) * 0.5f, c, true, 1.0f, 255);
    }
    draw::Line(awningLeft + 6.0f, awningTop, awningRight + 6.0f, awningTop, kInk, 4.0f, 255);

    DrawDoor(Rect(x - 6.0f, base - 128.0f, x + 70.0f, base - 2.0f), ColorRGB(70, 58, 50), lit);
    // 店先の木箱
    draw::FillRect(Rect(x + w * 0.22f, base - 44.0f, x + w * 0.22f + 56.0f, base), kInk, 255);
    draw::FillRect(Rect(x + w * 0.22f + 3.0f, base - 41.0f, x + w * 0.22f + 53.0f, base - 3.0f), ColorRGB(150, 110, 70), 255);
    draw::Line(x + w * 0.22f + 3.0f, base - 22.0f, x + w * 0.22f + 53.0f, base - 22.0f, ColorRGB(110, 78, 50), 3.0f, 255);
    (void)time;
}

//------------------------------------------------------------------------------
// 鍛冶屋：石積みの壁、高い煙突と煙、炉の赤い明かり、店先の金床
//------------------------------------------------------------------------------
void DrawSmith(const Building& b, float x, float base, float time, bool lit)
{
    const float w = b.width;
    const float h = b.height;
    const Rect wall(x - w * 0.5f, base - h, x + w * 0.5f, base);
    const ColorRGB stone(104, 100, 106);

    // 煙突と煙
    const Rect chimney(wall.left + 40.0f, wall.top - 190.0f, wall.left + 100.0f, wall.top);
    for (int i = 0; i < 6; ++i) {
        const float t = std::fmod(time * 0.3f + static_cast<float>(i) / 6.0f, 1.0f);
        draw::Circle(chimney.CenterX() + t * 60.0f + std::sin(t * 5.0f + i) * 12.0f, chimney.top - 10.0f - t * 170.0f,
                     14.0f + t * 26.0f, ColorRGB(70, 70, 78), true, 1.0f, static_cast<int>(150.0f * (1.0f - t)));
    }
    draw::FillRect(chimney.Expanded(4.0f), kInk, 255);
    draw::FillRect(chimney, stone.Scaled(0.85f), 255);
    draw::FillRect(Rect(chimney.left - 8.0f, chimney.top, chimney.right + 8.0f, chimney.top + 16.0f), stone.Scaled(0.7f), 255);
    draw::Glow(chimney.CenterX(), chimney.top, 30.0f, ColorRGB(255, 120, 50), 70, 3);

    // 壁（石積み）
    draw::FillRect(wall.Expanded(4.0f), kInk, 255);
    draw::GradientRectV(wall, stone, stone.Scaled(0.72f), 255, 10);
    for (int row = 0; row < 7; ++row) {
        const float ry = wall.top + h * static_cast<float>(row) / 7.0f;
        draw::Line(wall.left, ry, wall.right, ry, stone.Scaled(0.6f), 2.0f, 255);
        const float shift = (row % 2 == 0) ? 0.0f : 40.0f;
        for (float bx = wall.left + shift; bx < wall.right; bx += 80.0f) {
            draw::Line(bx, ry, bx, ry + h / 7.0f, stone.Scaled(0.6f), 2.0f, 255);
        }
    }

    // 屋根（片流れのスレート）
    FillConvex({ Vec2(wall.left - 30.0f, wall.top + 4.0f), Vec2(wall.left - 30.0f, wall.top - 34.0f),
                 Vec2(wall.right + 34.0f, wall.top - 96.0f), Vec2(wall.right + 34.0f, wall.top + 4.0f) }, kInk);
    FillConvex({ Vec2(wall.left - 26.0f, wall.top), Vec2(wall.left - 26.0f, wall.top - 30.0f),
                 Vec2(wall.right + 30.0f, wall.top - 90.0f), Vec2(wall.right + 30.0f, wall.top) }, ColorRGB(62, 66, 80));
    for (int i = 1; i < 4; ++i) {
        const float t = static_cast<float>(i) / 4.0f;
        draw::Line(wall.left - 26.0f, wall.top - 30.0f * (1.0f - t), wall.right + 30.0f,
                   wall.top - 90.0f * (1.0f - t), ColorRGB(44, 46, 58), 3.0f, 255);
    }

    // 炉の口（アーチ）と赤い明かり
    const float flicker = 0.75f + 0.25f * std::sin(time * 9.0f) * std::sin(time * 3.7f + 1.0f);
    const Rect forge(x + 70.0f, base - 150.0f, x + 200.0f, base - 24.0f);
    draw::Glow(forge.CenterX(), forge.CenterY(), 150.0f * flicker, ColorRGB(255, 120, 40), 110, 5);
    draw::FillRect(forge.Expanded(6.0f), kInk, 255);
    draw::Circle(forge.CenterX(), forge.top, forge.Width() * 0.5f + 6.0f, kInk, true, 1.0f, 255);
    draw::GradientRectV(forge, ColorRGB(255, 170, 70), ColorRGB(200, 60, 30), 255, 8);
    draw::Circle(forge.CenterX(), forge.top, forge.Width() * 0.5f, ColorRGB(255, 186, 90), true, 1.0f, 255);
    draw::FillRect(Rect(forge.left, forge.bottom - 30.0f, forge.right, forge.bottom), ColorRGB(60, 30, 26), 255);
    for (int k = 0; k < 5; ++k) {
        draw::Circle(forge.left + 16.0f + k * 24.0f, forge.bottom - 30.0f, 9.0f, ColorRGB(255, 220, 120),
                     true, 1.0f, static_cast<int>(200.0f * flicker));
    }

    DrawDoor(Rect(x - 48.0f, base - 136.0f, x + 40.0f, base - 2.0f), ColorRGB(92, 66, 48), lit);
    DrawWindow(Rect::FromCenter(x - w * 0.30f, base - h * 0.58f, 66.0f, 58.0f), 0.8f);

    // 店先の金床と立てかけた剣（壁の前）
    const float ax = wall.left + 60.0f;
    FillConvex({ Vec2(ax - 46.0f, base - 62.0f), Vec2(ax + 56.0f, base - 62.0f), Vec2(ax + 34.0f, base - 44.0f),
                 Vec2(ax - 22.0f, base - 44.0f) }, ColorRGB(56, 58, 66));
    draw::FillRect(Rect(ax - 12.0f, base - 46.0f, ax + 18.0f, base - 14.0f), ColorRGB(46, 48, 56), 255);
    draw::FillRect(Rect(ax - 30.0f, base - 16.0f, ax + 36.0f, base), ColorRGB(56, 58, 66), 255);
    draw::Line(ax - 40.0f, base - 62.0f, ax + 50.0f, base - 62.0f, ColorRGB(150, 156, 170), 2.0f, 255);
    draw::Line(wall.right - 40.0f, base - 4.0f, wall.right - 70.0f, base - 140.0f, ColorRGB(200, 206, 220), 6.0f, 255);
    draw::Line(wall.right - 82.0f, base - 112.0f, wall.right - 52.0f, base - 118.0f, ColorRGB(150, 110, 60), 6.0f, 255);

    // 看板（鎚の印）
    DrawHangingSign(x - 4.0f, base - kSignAboveBase, b.name, ColorRGB(52, 46, 44), ColorRGB(255, 186, 110));
}

//------------------------------------------------------------------------------
// ダンジョン入口：石のアーチと、奥で渦を巻く青い光の門。両脇に松明
//------------------------------------------------------------------------------
void DrawDungeon(const Building& b, float x, float base, float time, bool lit)
{
    const float w = b.width;
    const float h = b.height;
    const ColorRGB stone(90, 96, 116);
    const ColorRGB portal(110, 190, 255);
    const float pillarW = 70.0f;
    const float innerL = x - w * 0.5f + pillarW;
    const float innerR = x + w * 0.5f - pillarW;
    const float archY = base - h + (innerR - innerL) * 0.5f;   // アーチの付け根

    // 奥の光の門
    const float pulse = 0.8f + 0.2f * std::sin(time * 2.2f);
    draw::Glow(x, base - h * 0.45f, w * 0.75f * pulse, portal, lit ? 160 : 120, 6);
    draw::FillRect(Rect(innerL, archY, innerR, base), ColorRGB(12, 20, 40), 255);
    draw::Circle(x, archY, (innerR - innerL) * 0.5f, ColorRGB(12, 20, 40), true, 1.0f, 255);
    for (int i = 0; i < 5; ++i) {
        const float r = (innerR - innerL) * (0.12f + 0.09f * static_cast<float>(i));
        const float a = time * (0.8f + 0.3f * i) * ((i % 2 == 0) ? 1.0f : -1.0f);
        const float cy = base - h * 0.42f;
        for (int k = 0; k < 3; ++k) {
            const float a0 = a + k * 2.094f;
            draw::Line(x + std::cos(a0) * r, cy + std::sin(a0) * r * 1.3f,
                       x + std::cos(a0 + 1.2f) * r, cy + std::sin(a0 + 1.2f) * r * 1.3f,
                       ColorRGB::Lerp(portal, palette::kWhite, 0.2f * i), 3.0f, 170 - i * 20);
        }
    }
    draw::Glow(x, base - h * 0.42f, 60.0f * pulse, palette::kWhite, 120, 4);

    // 柱とアーチ
    for (int side = -1; side <= 1; side += 2) {
        const Rect pillar = (side < 0) ? Rect(x - w * 0.5f, base - h, innerL, base)
                                       : Rect(innerR, base - h, x + w * 0.5f, base);
        draw::FillRect(pillar.Expanded(4.0f), kInk, 255);
        draw::GradientRectH(pillar, stone, stone.Scaled(0.7f), 255, 6);
        for (int row = 1; row < 6; ++row) {
            const float ry = pillar.top + pillar.Height() * static_cast<float>(row) / 6.0f;
            draw::Line(pillar.left, ry, pillar.right, ry, stone.Scaled(0.55f), 2.0f, 255);
        }
        draw::FillRect(Rect(pillar.left - 10.0f, base - 26.0f, pillar.right + 10.0f, base), stone.Scaled(0.8f), 255);
    }
    // アーチの石（半円に並べる）
    const float outerR = w * 0.5f;
    const float innerRad = (innerR - innerL) * 0.5f;
    const float archCenterY = archY;
    for (int i = 0; i < 9; ++i) {
        const float a0 = 3.14159f + 3.14159f * static_cast<float>(i) / 9.0f;
        const float a1 = 3.14159f + 3.14159f * static_cast<float>(i + 1) / 9.0f;
        const std::vector<Vec2> block = {
            Vec2(x + std::cos(a0) * innerRad, archCenterY + std::sin(a0) * innerRad),
            Vec2(x + std::cos(a0) * outerR, archCenterY + std::sin(a0) * outerR),
            Vec2(x + std::cos(a1) * outerR, archCenterY + std::sin(a1) * outerR),
            Vec2(x + std::cos(a1) * innerRad, archCenterY + std::sin(a1) * innerRad),
        };
        FillConvex(block, (i % 2 == 0) ? stone : stone.Scaled(0.85f));
        draw::Line(block[0].x, block[0].y, block[1].x, block[1].y, kInk, 3.0f, 255);
    }
    // 要石（青く光る）
    const Vec2 key(x, archCenterY - outerR + 10.0f);
    FillConvex({ Vec2(key.x - 26.0f, key.y - 10.0f), Vec2(key.x + 26.0f, key.y - 10.0f), Vec2(key.x + 18.0f, key.y + 40.0f),
                 Vec2(key.x - 18.0f, key.y + 40.0f) }, stone.Scaled(1.15f));
    draw::Glow(key.x, key.y + 14.0f, 26.0f, portal, 160, 3);
    draw::Circle(key.x, key.y + 14.0f, 8.0f, portal, true, 1.0f, 255);

    // 松明
    for (int side = -1; side <= 1; side += 2) {
        const float tx = x + side * (w * 0.5f + 34.0f);
        const float ty = base - 170.0f;
        draw::Line(tx, ty, tx, base, ColorRGB(70, 52, 40), 8.0f, 255);
        const float f = 0.8f + 0.2f * std::sin(time * 11.0f + side);
        draw::Glow(tx, ty - 14.0f, 64.0f * f, ColorRGB(255, 150, 60), 140, 4);
        draw::Triangle(Vec2(tx - 12.0f, ty), Vec2(tx + 12.0f, ty), Vec2(tx, ty - 38.0f * f), ColorRGB(255, 190, 80), true, 255);
        draw::Triangle(Vec2(tx - 6.0f, ty), Vec2(tx + 6.0f, ty), Vec2(tx, ty - 22.0f * f), ColorRGB(255, 244, 200), true, 255);
    }

    // 石段
    draw::FillRect(Rect(innerL - 20.0f, base - 12.0f, innerR + 20.0f, base + 4.0f), stone.Scaled(0.75f), 255);

    // 看板（アーチの上の銘板）
    const Rect plate = Rect::FromCenter(x, archCenterY - outerR - 34.0f, 260.0f, 50.0f);
    draw::FillRect(plate.Expanded(4.0f), kInk, 255);
    draw::GradientRectV(plate, stone.Scaled(1.1f), stone.Scaled(0.8f), 255, 6);
    draw::StrokeRect(plate.Expanded(-5.0f), portal, 1.0f, 200);
    draw::Text(FontSize::Small, plate.CenterX(), plate.top + 12.0f, ColorRGB(220, 240, 255), b.name,
               draw::TextAlign::Center);
}

//------------------------------------------------------------------------------
// 迷宮の門：紫の符が光る黒い石の尖塔門。閉じているときは鉄格子と鎖と錠前、
//   開いているときは奥で紫の光が渦を巻く
//------------------------------------------------------------------------------
void DrawLabyrinth(const Building& b, float x, float base, float time, bool lit, bool open)
{
    const float w = b.width;
    const float h = b.height;
    const ColorRGB stone(62, 58, 84);
    const ColorRGB violet(186, 140, 255);
    const float pillarW = 74.0f;
    const float innerL = x - w * 0.5f + pillarW;
    const float innerR = x + w * 0.5f - pillarW;
    const float archTop = base - h - 70.0f;

    // 奥（開いていれば紫の渦、閉じていれば暗闇）
    const float pulse = 0.8f + 0.2f * std::sin(time * 1.8f);
    if (open) draw::Glow(x, base - h * 0.45f, w * 0.7f * pulse, violet, lit ? 160 : 110, 6);
    FillConvex({ Vec2(innerL, base), Vec2(innerL, base - h + 40.0f), Vec2(x, archTop + 50.0f),
                 Vec2(innerR, base - h + 40.0f), Vec2(innerR, base) }, ColorRGB(10, 8, 20));
    if (open) {
        for (int i = 0; i < 6; ++i) {
            const float r = (innerR - innerL) * (0.10f + 0.07f * static_cast<float>(i));
            const float a = time * (0.7f + 0.25f * i) * ((i % 2 == 0) ? 1.0f : -1.0f);
            const float cy = base - h * 0.42f;
            for (int k = 0; k < 3; ++k) {
                const float a0 = a + k * 2.094f;
                draw::Line(x + std::cos(a0) * r, cy + std::sin(a0) * r * 1.4f, x + std::cos(a0 + 1.3f) * r,
                           cy + std::sin(a0 + 1.3f) * r * 1.4f, ColorRGB::Lerp(violet, palette::kWhite, 0.15f * i), 3.0f,
                           170 - i * 18);
            }
        }
    }

    // 柱（紫の符が光る）
    for (int side = -1; side <= 1; side += 2) {
        const Rect pillar = (side < 0) ? Rect(x - w * 0.5f, base - h, innerL, base) : Rect(innerR, base - h, x + w * 0.5f, base);
        draw::FillRect(pillar.Expanded(4.0f), kInk, 255);
        draw::GradientRectH(pillar, stone, stone.Scaled(0.65f), 255, 6);
        for (int k = 0; k < 4; ++k) {
            const float ry = pillar.top + 50.0f + 70.0f * static_cast<float>(k);
            const float flicker = 0.5f + 0.5f * std::sin(time * 2.0f + k + side);
            draw::Line(pillar.CenterX() - 12.0f, ry, pillar.CenterX() + 12.0f, ry + 18.0f, violet, 3.0f,
                       open ? static_cast<int>(120.0f + 120.0f * flicker) : 70);
            draw::Line(pillar.CenterX() + 12.0f, ry, pillar.CenterX() - 12.0f, ry + 18.0f, violet, 3.0f,
                       open ? static_cast<int>(120.0f + 120.0f * flicker) : 70);
        }
        // 尖った頂
        FillConvex({ Vec2(pillar.left - 8.0f, pillar.top + 4.0f), Vec2(pillar.CenterX(), pillar.top - 90.0f),
                     Vec2(pillar.right + 8.0f, pillar.top + 4.0f) }, kInk);
        FillConvex({ Vec2(pillar.left - 4.0f, pillar.top), Vec2(pillar.CenterX(), pillar.top - 82.0f),
                     Vec2(pillar.right + 4.0f, pillar.top) }, stone.Scaled(0.85f));
    }
    // 尖頭アーチの縁
    draw::Line(innerL, base - h + 40.0f, x, archTop + 50.0f, kInk, 12.0f, 255);
    draw::Line(innerR, base - h + 40.0f, x, archTop + 50.0f, kInk, 12.0f, 255);
    draw::Line(innerL, base - h + 40.0f, x, archTop + 50.0f, stone.Scaled(1.2f), 6.0f, 255);
    draw::Line(innerR, base - h + 40.0f, x, archTop + 50.0f, stone.Scaled(1.2f), 6.0f, 255);

    if (!open) {
        // 鉄格子・鎖・錠前
        for (float bx = innerL + 20.0f; bx < innerR - 10.0f; bx += 34.0f) {
            draw::Line(bx, base - h + 30.0f, bx, base, ColorRGB(30, 30, 36), 7.0f, 255);
            draw::Line(bx, base - h + 30.0f, bx, base, ColorRGB(96, 96, 108), 3.0f, 255);
        }
        for (int k = 0; k < 2; ++k) {
            const float cy = base - h * (0.35f + 0.25f * k);
            draw::Line(innerL, cy, innerR, cy + 20.0f, ColorRGB(120, 116, 128), 6.0f, 255);
        }
        const Rect lock = Rect::FromCenter(x, base - h * 0.42f, 44.0f, 40.0f);
        draw::Circle(x, lock.top, 14.0f, ColorRGB(160, 140, 90), false, 5.0f, 255);
        draw::FillRect(lock.Expanded(3.0f), kInk, 255);
        draw::FillRect(lock, ColorRGB(196, 164, 90), 255);
        draw::FillRect(Rect::FromCenter(x, lock.CenterY() + 2.0f, 6.0f, 14.0f), kInk, 255);
    }

    // 石段と銘板
    draw::FillRect(Rect(x - w * 0.5f - 16.0f, base - 14.0f, x + w * 0.5f + 16.0f, base + 4.0f), stone.Scaled(0.7f), 255);
    const Rect plate = Rect::FromCenter(x, archTop - 4.0f, 220.0f, 46.0f);
    draw::FillRect(plate.Expanded(4.0f), kInk, 255);
    draw::GradientRectV(plate, stone.Scaled(1.15f), stone.Scaled(0.8f), 255, 6);
    draw::StrokeRect(plate.Expanded(-5.0f), violet, 1.0f, open ? 220 : 90);
    draw::Text(FontSize::Small, plate.CenterX(), plate.top + 10.0f, open ? ColorRGB(230, 210, 255) : palette::kTextDim,
               b.name, draw::TextAlign::Center);
}

//------------------------------------------------------------------------------
// ダンジョンの中の施設
//------------------------------------------------------------------------------
// 行商人のテント：縞の屋根、台に並んだ瓶、吊るしたランタン
void DrawMerchantTent(float x, float base, float time, bool lit)
{
    const float w = 300.0f;
    const float roofY = base - 230.0f;
    // 支柱
    draw::Line(x - w * 0.42f, roofY + 20.0f, x - w * 0.42f, base, kInk, 10.0f, 255);
    draw::Line(x + w * 0.42f, roofY + 20.0f, x + w * 0.42f, base, kInk, 10.0f, 255);
    draw::Line(x - w * 0.42f, roofY + 20.0f, x - w * 0.42f, base, ColorRGB(120, 86, 56), 5.0f, 255);
    draw::Line(x + w * 0.42f, roofY + 20.0f, x + w * 0.42f, base, ColorRGB(120, 86, 56), 5.0f, 255);
    // 奥の布
    draw::FillRect(Rect(x - w * 0.42f, roofY + 30.0f, x + w * 0.42f, base - 70.0f), ColorRGB(70, 40, 48), 230);
    // 縞の屋根
    const int stripes = 7;
    for (int i = 0; i < stripes; ++i) {
        const float x0 = x - w * 0.5f + w * static_cast<float>(i) / stripes;
        const float x1 = x - w * 0.5f + w * static_cast<float>(i + 1) / stripes;
        const ColorRGB c = (i % 2 == 0) ? ColorRGB(196, 64, 70) : ColorRGB(236, 224, 200);
        FillConvex({ Vec2(x, roofY - 60.0f), Vec2(x1, roofY + 26.0f), Vec2(x0, roofY + 26.0f) }, c);
        draw::Circle((x0 + x1) * 0.5f, roofY + 26.0f, (x1 - x0) * 0.5f, c, true, 1.0f, 255);
    }
    // 台と品物
    const Rect counter(x - w * 0.40f, base - 74.0f, x + w * 0.40f, base - 40.0f);
    draw::FillRect(counter.Expanded(3.0f), kInk, 255);
    draw::FillRect(counter, ColorRGB(140, 100, 64), 255);
    draw::FillRect(Rect(counter.left + 8.0f, counter.bottom, counter.left + 20.0f, base), ColorRGB(110, 78, 50), 255);
    draw::FillRect(Rect(counter.right - 20.0f, counter.bottom, counter.right - 8.0f, base), ColorRGB(110, 78, 50), 255);
    for (int k = 0; k < 5; ++k) {
        const float bx = counter.left + 24.0f + static_cast<float>(k) * (counter.Width() - 48.0f) / 4.0f;
        const ColorRGB potion = (k % 3 == 0) ? ColorRGB(220, 70, 80) : (k % 3 == 1) ? ColorRGB(70, 140, 240)
                                                                                  : ColorRGB(120, 220, 140);
        draw::Circle(bx, counter.top - 11.0f, 10.0f, kInk, true, 1.0f, 255);
        draw::Circle(bx, counter.top - 11.0f, 8.0f, potion, true, 1.0f, 255);
    }
    // ランタン
    const float f = 0.85f + 0.15f * std::sin(time * 5.0f + x);
    draw::Glow(x + w * 0.30f, roofY + 60.0f, 50.0f * f, kLampLight, lit ? 150 : 110, 4);
    draw::FillRect(Rect::FromCenter(x + w * 0.30f, roofY + 60.0f, 16.0f, 22.0f), kLampLight, 240);
    // 看板
    DrawHangingSign(x, roofY + 34.0f, "ショップ", ColorRGB(70, 50, 36), ColorRGB(255, 226, 170));
}

// 旅の鍛冶屋：石の炉と煙、金床、立てかけた剣
void DrawTravelSmith(float x, float base, float time, bool lit)
{
    const ColorRGB stone(100, 96, 104);
    // 炉
    const Rect forge(x - 150.0f, base - 150.0f, x - 20.0f, base);
    for (int i = 0; i < 4; ++i) {
        const float t = std::fmod(time * 0.35f + static_cast<float>(i) / 4.0f, 1.0f);
        draw::Circle(forge.CenterX() + t * 40.0f, forge.top - 70.0f - t * 120.0f, 10.0f + t * 18.0f,
                     ColorRGB(70, 70, 78), true, 1.0f, static_cast<int>(140.0f * (1.0f - t)));
    }
    draw::FillRect(Rect(forge.CenterX() - 18.0f, forge.top - 70.0f, forge.CenterX() + 18.0f, forge.top), kInk, 255);
    draw::FillRect(Rect(forge.CenterX() - 14.0f, forge.top - 66.0f, forge.CenterX() + 14.0f, forge.top), stone.Scaled(0.8f), 255);
    draw::FillRect(forge.Expanded(4.0f), kInk, 255);
    draw::GradientRectV(forge, stone, stone.Scaled(0.7f), 255, 6);
    const float flicker = 0.75f + 0.25f * std::sin(time * 9.0f) * std::sin(time * 3.3f + 1.0f);
    const Rect mouth = Rect::FromCenter(forge.CenterX(), forge.CenterY() + 20.0f, 70.0f, 60.0f);
    draw::Glow(mouth.CenterX(), mouth.CenterY(), 90.0f * flicker, ColorRGB(255, 120, 40), lit ? 150 : 110, 5);
    draw::FillRect(mouth, ColorRGB(255, 150, 60), 255);
    draw::FillRect(Rect(mouth.left, mouth.bottom - 16.0f, mouth.right, mouth.bottom), ColorRGB(80, 30, 20), 255);
    // 金床
    const float ax = x + 70.0f;
    FillConvex({ Vec2(ax - 50.0f, base - 70.0f), Vec2(ax + 60.0f, base - 70.0f), Vec2(ax + 36.0f, base - 50.0f),
                 Vec2(ax - 24.0f, base - 50.0f) }, ColorRGB(56, 58, 66));
    draw::FillRect(Rect(ax - 14.0f, base - 52.0f, ax + 20.0f, base - 16.0f), ColorRGB(46, 48, 56), 255);
    draw::FillRect(Rect(ax - 34.0f, base - 18.0f, ax + 40.0f, base), ColorRGB(56, 58, 66), 255);
    draw::Line(ax - 44.0f, base - 70.0f, ax + 54.0f, base - 70.0f, ColorRGB(150, 156, 170), 2.0f, 255);
    // 立てかけた剣と槌
    draw::Line(x + 150.0f, base, x + 130.0f, base - 130.0f, ColorRGB(200, 206, 220), 6.0f, 255);
    draw::Line(x + 118.0f, base - 104.0f, x + 146.0f, base - 110.0f, ColorRGB(150, 110, 60), 6.0f, 255);
    // 看板
    DrawHangingSign(x + 70.0f, base - 194.0f, "鍛冶屋", ColorRGB(52, 46, 44), ColorRGB(255, 186, 110));
}

// 街灯（手前）
void DrawLamp(float x, float groundY, float time)
{
    const float top = groundY - 250.0f;
    draw::Line(x, top, x, groundY + 6.0f, kInk, 12.0f, 255);
    draw::Line(x, top, x, groundY + 6.0f, ColorRGB(52, 54, 66), 6.0f, 255);
    draw::Line(x, top + 6.0f, x + 34.0f, top + 6.0f, ColorRGB(52, 54, 66), 5.0f, 255);
    const float f = 0.9f + 0.1f * std::sin(time * 4.0f + x);
    draw::Glow(x + 34.0f, top + 26.0f, 70.0f * f, kLampLight, 110, 4);
    draw::FillRect(Rect(x + 22.0f, top + 10.0f, x + 46.0f, top + 40.0f), kInk, 255);
    draw::FillRect(Rect(x + 25.0f, top + 13.0f, x + 43.0f, top + 37.0f), kLampLight, 255);
    draw::FillRect(Rect(x - 12.0f, groundY - 8.0f, x + 12.0f, groundY + 6.0f), ColorRGB(52, 54, 66), 255);
}

} // namespace

int BuildingCount() { return kBuildingCount; }

const Building& BuildingAt(int index)
{
    return kBuildings[math::ClampInt(index, 0, kBuildingCount - 1)];
}

int BuildingNear(float x)
{
    for (int i = 0; i < kBuildingCount; ++i) {
        if (math::Abs(x - kBuildings[i].x) <= kDoorRange) return i;
    }
    return -1;
}

const Building& BuildingFor(Facility facility)
{
    for (const Building& b : kBuildings) {
        if (b.facility == facility) return b;
    }
    return kBuildings[0];
}

void DrawBuildings(const Camera& camera, float baseY, float time, int highlighted, bool labyrinthOpen)
{
    const Vec2 offset = camera.WorldToScreen(Vec2(0.0f, 0.0f));
    for (int i = 0; i < kBuildingCount; ++i) {
        const Building& b = kBuildings[i];
        const Rect bounds(b.x - b.width * 0.5f - 220.0f, baseY - b.height - 320.0f, b.x + b.width * 0.5f + 220.0f, baseY);
        if (!camera.IsVisible(bounds, 0.0f)) continue;
        const float x = b.x + offset.x;
        const float base = baseY + offset.y;
        const bool lit = (i == highlighted);
        switch (b.facility) {
        case Facility::Home:    DrawHome(b, x, base, time, lit); break;
        case Facility::Shop:    DrawShop(b, x, base, time, lit); break;
        case Facility::Smith:   DrawSmith(b, x, base, time, lit); break;
        case Facility::Labyrinth: DrawLabyrinth(b, x, base, time, lit, labyrinthOpen); break;
        case Facility::Dungeon: DrawDungeon(b, x, base, time, lit); break;
        }
    }
}

void DrawForeground(const Camera& camera, float groundY, float time)
{
    const Vec2 offset = camera.WorldToScreen(Vec2(0.0f, 0.0f));
    const float ground = groundY + offset.y;

    // 街灯：建物と建物の間に立てる
    const float lamps[] = { 160.0f, 1110.0f, 2130.0f, 3150.0f, 4170.0f, 5100.0f };
    for (float lx : lamps) {
        const float x = lx + offset.x;
        if (x < -120.0f || x > static_cast<float>(config::kScreenWidth) + 120.0f) continue;
        DrawLamp(x, ground, time);
    }

    // 道しるべ（自宅とショップの間）
    const float sx = 1240.0f + offset.x;
    if (sx > -200.0f && sx < static_cast<float>(config::kScreenWidth) + 200.0f) {
        draw::Line(sx, ground - 170.0f, sx, ground + 6.0f, kInk, 12.0f, 255);
        draw::Line(sx, ground - 170.0f, sx, ground + 6.0f, ColorRGB(110, 84, 60), 7.0f, 255);
        const char* rows[] = { config::kGameTitleUpper, "ショップ・鍛冶屋 →", "迷宮の門・ダンジョン入口 →" };
        for (int i = 0; i < 3; ++i) {
            const float y = ground - 168.0f + static_cast<float>(i) * 40.0f;
            const bool title = (i == 0);
            const float w = static_cast<float>(draw::TextWidth(title ? FontSize::Small : FontSize::Tiny, rows[i])) + 34.0f;
            FillConvex({ Vec2(sx - 16.0f, y), Vec2(sx - 16.0f + w, y), Vec2(sx - 2.0f + w, y + 17.0f),
                         Vec2(sx - 16.0f + w, y + 34.0f), Vec2(sx - 16.0f, y + 34.0f) }, kInk);
            FillConvex({ Vec2(sx - 13.0f, y + 3.0f), Vec2(sx - 18.0f + w, y + 3.0f), Vec2(sx - 6.0f + w, y + 17.0f),
                         Vec2(sx - 18.0f + w, y + 31.0f), Vec2(sx - 13.0f, y + 31.0f) },
                       title ? ColorRGB(120, 92, 64) : ColorRGB(96, 72, 52));
            draw::Text(title ? FontSize::Small : FontSize::Tiny, sx + 2.0f, y + (title ? 4.0f : 8.0f),
                       title ? ColorRGB(255, 230, 180) : palette::kText, rows[i]);
        }
    }
}

void DrawPromptAt(const Camera& camera, float worldX, float baseY, const std::string& keyName, const char* name,
                  const std::string& guide, float time)
{
    const Vec2 door = camera.WorldToScreen(Vec2(worldX, baseY));
    const float bob = std::sin(time * 4.0f) * 4.0f;

    const std::string head = str::Format("%s  入る", keyName.c_str());
    const float w = math::MaxF(static_cast<float>(draw::TextWidth(FontSize::Small, guide)),
                               static_cast<float>(draw::TextWidth(FontSize::Medium, head) + draw::TextWidth(FontSize::Small, name) + 28)) + 48.0f;
    const Rect panel = Rect::FromCenter(door.x, door.y - kPromptAboveBase + bob, w, 92.0f);
    draw::ChamferRect(panel.Expanded(2.0f), 10.0f, palette::kAccent, 230);
    draw::ChamferRect(panel, 10.0f, palette::kPanelDark, 240);

    // 「ENTER」は鍵帽の形にする
    const float keyW = static_cast<float>(draw::TextWidth(FontSize::Small, keyName)) + 22.0f;
    const Rect keyCap = Rect::FromXYWH(panel.left + 22.0f, panel.top + 12.0f, keyW, 34.0f);
    draw::FillRect(keyCap, palette::kAccent, 255);
    draw::Text(FontSize::Small, keyCap.CenterX(), keyCap.top + 6.0f, palette::kBlack, keyName, draw::TextAlign::Center);
    draw::Text(FontSize::Medium, keyCap.right + 12.0f, panel.top + 8.0f, palette::kText, "入る");
    draw::Text(FontSize::Small, panel.right - 22.0f, panel.top + 15.0f, palette::kAccent, name, draw::TextAlign::Right);
    draw::Text(FontSize::Small, panel.left + 22.0f, panel.top + 56.0f, palette::kTextDim, guide);

    // 下向きの矢印（入口を指す）
    draw::Triangle(Vec2(panel.CenterX() - 14.0f, panel.bottom + 2.0f), Vec2(panel.CenterX() + 14.0f, panel.bottom + 2.0f),
                   Vec2(panel.CenterX(), panel.bottom + 20.0f), palette::kAccent, true, 230);
}

void DrawEnterPrompt(const Camera& camera, int index, float baseY, const std::string& keyName, float time,
                     const char* guide)
{
    if (index < 0 || index >= kBuildingCount) return;
    const Building& b = kBuildings[index];
    DrawPromptAt(camera, b.x, baseY, keyName, b.name, guide ? std::string(guide) : std::string(b.guide), time);
}

void DrawStall(const Camera& camera, FacilityKind kind, float worldX, float baseY, float time, bool lit)
{
    const Vec2 at = camera.WorldToScreen(Vec2(worldX, baseY));
    if (at.x < -300.0f || at.x > static_cast<float>(config::kScreenWidth) + 300.0f) return;
    if (kind == FacilityKind::Shop) DrawMerchantTent(at.x, at.y, time, lit);
    else DrawTravelSmith(at.x, at.y, time, lit);
}

} // namespace town
} // namespace ecl
