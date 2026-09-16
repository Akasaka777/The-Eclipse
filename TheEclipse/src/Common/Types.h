//==============================================================================
// Types.h : 基本的な値型（ベクトル / 矩形 / 色）
//==============================================================================
#pragma once

namespace ecl {

//------------------------------------------------------------------------------
// 2次元ベクトル
//------------------------------------------------------------------------------
struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2& o) const { return Vec2(x + o.x, y + o.y); }
    Vec2 operator-(const Vec2& o) const { return Vec2(x - o.x, y - o.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }
    Vec2 operator-() const { return Vec2(-x, -y); }

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }

    float Length() const;
    float LengthSq() const;
    Vec2  Normalized() const;
};

float Distance(const Vec2& a, const Vec2& b);

//------------------------------------------------------------------------------
// 軸並行矩形（AABB）
//------------------------------------------------------------------------------
struct Rect
{
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    Rect() = default;
    Rect(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}

    // 左上と幅高さから生成
    static Rect FromXYWH(float x, float y, float w, float h);
    // 中心と幅高さから生成
    static Rect FromCenter(float cx, float cy, float w, float h);
    // 足元座標（キャラの基準点）から生成
    static Rect FromFoot(float footX, float footY, float halfWidth, float height);

    float Width() const { return right - left; }
    float Height() const { return bottom - top; }
    float CenterX() const { return (left + right) * 0.5f; }
    float CenterY() const { return (top + bottom) * 0.5f; }

    bool Intersects(const Rect& o) const;
    bool Contains(float px, float py) const;
    Rect Offset(float dx, float dy) const;
    Rect Expanded(float amount) const;
};

//------------------------------------------------------------------------------
// 色（DxLib へ渡す直前に変換する）
//------------------------------------------------------------------------------
struct ColorRGB
{
    int r = 255;
    int g = 255;
    int b = 255;

    ColorRGB() = default;
    ColorRGB(int r_, int g_, int b_) : r(r_), g(g_), b(b_) {}

    // 明度スケール（1.0 で等倍）
    ColorRGB Scaled(float f) const;
    static ColorRGB Lerp(const ColorRGB& a, const ColorRGB& b, float t);
};

} // namespace ecl
