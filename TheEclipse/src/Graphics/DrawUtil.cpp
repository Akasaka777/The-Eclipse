#include "Graphics/DrawUtil.h"

#include "Common/MathUtil.h"
#include "Core/DxInclude.h"
#include "Core/GameConfig.h"

namespace ecl {
namespace draw {

namespace {

// アルファ指定つき描画のスコープヘルパ
struct AlphaScope
{
    bool active = false;
    explicit AlphaScope(int alpha)
    {
        if (alpha < 255) {
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, math::ClampInt(alpha, 0, 255));
            active = true;
        }
    }
    ~AlphaScope()
    {
        if (active) SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
    }
};

} // namespace

unsigned int ToDx(const ColorRGB& color)
{
    return GetColor(math::ClampInt(color.r, 0, 255),
                    math::ClampInt(color.g, 0, 255),
                    math::ClampInt(color.b, 0, 255));
}

void FillRect(const Rect& rect, const ColorRGB& color, int alpha)
{
    AlphaScope scope(alpha);
    DrawBoxAA(rect.left, rect.top, rect.right, rect.bottom, ToDx(color), TRUE);
}

void FillRectXYWH(float x, float y, float w, float h, const ColorRGB& color, int alpha)
{
    FillRect(Rect::FromXYWH(x, y, w, h), color, alpha);
}

void StrokeRect(const Rect& rect, const ColorRGB& color, float thickness, int alpha)
{
    AlphaScope scope(alpha);
    DrawBoxAA(rect.left, rect.top, rect.right, rect.bottom, ToDx(color), FALSE, thickness);
}

void GradientRectV(const Rect& rect, const ColorRGB& top, const ColorRGB& bottom, int alpha, int bands)
{
    if (bands < 1) bands = 1;
    AlphaScope scope(alpha);
    const float step = rect.Height() / static_cast<float>(bands);
    for (int i = 0; i < bands; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(bands - 1 > 0 ? bands - 1 : 1);
        const ColorRGB c = ColorRGB::Lerp(top, bottom, t);
        const float y = rect.top + step * static_cast<float>(i);
        DrawBoxAA(rect.left, y, rect.right, y + step + 1.0f, ToDx(c), TRUE);
    }
}

void GradientRectH(const Rect& rect, const ColorRGB& left, const ColorRGB& right, int alpha, int bands)
{
    if (bands < 1) bands = 1;
    AlphaScope scope(alpha);
    const float step = rect.Width() / static_cast<float>(bands);
    for (int i = 0; i < bands; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(bands - 1 > 0 ? bands - 1 : 1);
        const ColorRGB c = ColorRGB::Lerp(left, right, t);
        const float x = rect.left + step * static_cast<float>(i);
        DrawBoxAA(x, rect.top, x + step + 1.0f, rect.bottom, ToDx(c), TRUE);
    }
}

void Line(float x1, float y1, float x2, float y2, const ColorRGB& color, float thickness, int alpha)
{
    AlphaScope scope(alpha);
    DrawLineAA(x1, y1, x2, y2, ToDx(color), thickness);
}

void Circle(float cx, float cy, float radius, const ColorRGB& color, bool fill, float thickness, int alpha)
{
    AlphaScope scope(alpha);
    DrawCircleAA(cx, cy, radius, 32, ToDx(color), fill ? TRUE : FALSE, thickness);
}

void Triangle(const Vec2& a, const Vec2& b, const Vec2& c, const ColorRGB& color, bool fill, int alpha)
{
    AlphaScope scope(alpha);
    DrawTriangleAA(a.x, a.y, b.x, b.y, c.x, c.y, ToDx(color), fill ? TRUE : FALSE);
}

void ChamferRect(const Rect& rect, float cut, const ColorRGB& color, int alpha)
{
    AlphaScope scope(alpha);
    const unsigned int c = ToDx(color);
    // 中央の矩形＋上下の切り欠き三角形で八角形風に見せる
    DrawBoxAA(rect.left, rect.top + cut, rect.right, rect.bottom - cut, c, TRUE);
    DrawBoxAA(rect.left + cut, rect.top, rect.right - cut, rect.bottom, c, TRUE);
    DrawTriangleAA(rect.left, rect.top + cut, rect.left + cut, rect.top, rect.left + cut, rect.top + cut, c, TRUE);
    DrawTriangleAA(rect.right, rect.top + cut, rect.right - cut, rect.top, rect.right - cut, rect.top + cut, c, TRUE);
    DrawTriangleAA(rect.left, rect.bottom - cut, rect.left + cut, rect.bottom, rect.left + cut, rect.bottom - cut, c, TRUE);
    DrawTriangleAA(rect.right, rect.bottom - cut, rect.right - cut, rect.bottom, rect.right - cut, rect.bottom - cut, c, TRUE);
}

void Glow(float cx, float cy, float radius, const ColorRGB& color, int alpha, int layers)
{
    if (layers < 1) layers = 1;
    SetDrawBlendMode(DX_BLENDMODE_ADD, math::ClampInt(alpha / layers, 1, 255));
    for (int i = layers; i >= 1; --i) {
        const float r = radius * (static_cast<float>(i) / static_cast<float>(layers));
        DrawCircleAA(cx, cy, r, 24, ToDx(color), TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void Panel(const Rect& rect, const ColorRGB& fill, const ColorRGB& border, int fillAlpha, float thickness)
{
    GradientRectV(rect, fill.Scaled(1.15f), fill.Scaled(0.75f), fillAlpha, 20);
    StrokeRect(rect, border, thickness, 255);

    // 四隅のアクセント
    const float len = math::MinF(28.0f, math::MinF(rect.Width(), rect.Height()) * 0.3f);
    const ColorRGB accent = palette::kAccent;
    Line(rect.left, rect.top, rect.left + len, rect.top, accent, thickness + 1.0f);
    Line(rect.left, rect.top, rect.left, rect.top + len, accent, thickness + 1.0f);
    Line(rect.right, rect.bottom, rect.right - len, rect.bottom, accent, thickness + 1.0f);
    Line(rect.right, rect.bottom, rect.right, rect.bottom - len, accent, thickness + 1.0f);
}

void Bar(const Rect& rect, float ratio, const ColorRGB& fg, const ColorRGB& bg,
         float delayRatio, const ColorRGB& delayColor, int alpha)
{
    ratio = math::Clamp(ratio, 0.0f, 1.0f);
    FillRect(rect, bg, alpha);

    if (delayRatio > ratio) {
        delayRatio = math::Clamp(delayRatio, 0.0f, 1.0f);
        Rect delayRect = rect;
        delayRect.right = rect.left + rect.Width() * delayRatio;
        FillRect(delayRect, delayColor, alpha);
    }

    Rect fill = rect;
    fill.right = rect.left + rect.Width() * ratio;
    if (fill.Width() > 0.5f) {
        GradientRectV(fill, fg.Scaled(1.35f), fg.Scaled(0.8f), alpha, 8);
        // 上部ハイライト
        Rect gloss = fill;
        gloss.bottom = fill.top + fill.Height() * 0.35f;
        FillRect(gloss, palette::kWhite, math::ClampInt(alpha / 5, 0, 255));
    }
}

void SkewBar(const Rect& rect, float ratio, const ColorRGB& fg, const ColorRGB& bg, float skew,
             float delayRatio, const ColorRGB& delayColor)
{
    ratio = math::Clamp(ratio, 0.0f, 1.0f);

    auto drawSkewed = [&](float r, const ColorRGB& color, int alpha) {
        if (r <= 0.0f) return;
        const float w = rect.Width() * r;
        AlphaScope scope(alpha);
        const unsigned int c = ToDx(color);
        // 平行四辺形（左下→右下→右上→左上）
        DrawTriangleAA(rect.left, rect.bottom, rect.left + w, rect.bottom,
                       rect.left + skew, rect.top, c, TRUE);
        DrawTriangleAA(rect.left + w, rect.bottom, rect.left + w + skew, rect.top,
                       rect.left + skew, rect.top, c, TRUE);
    };

    drawSkewed(1.0f, bg, 255);
    if (delayRatio > ratio) drawSkewed(math::Clamp(delayRatio, 0.0f, 1.0f), delayColor, 255);
    drawSkewed(ratio, fg, 255);

    // ハイライト
    if (ratio > 0.0f) {
        const float w = rect.Width() * ratio;
        AlphaScope scope(60);
        const unsigned int c = ToDx(palette::kWhite);
        const float mid = rect.top + rect.Height() * 0.32f;
        DrawTriangleAA(rect.left + skew * 0.6f, mid, rect.left + w + skew * 0.6f, mid,
                       rect.left + skew, rect.top, c, TRUE);
        DrawTriangleAA(rect.left + w + skew * 0.6f, mid, rect.left + w + skew, rect.top,
                       rect.left + skew, rect.top, c, TRUE);
    }
}

int TextWidth(FontSize size, const std::string& text)
{
    const int handle = FontManager::Instance().Handle(size);
    if (handle < 0) return 0;
    return GetDrawStringWidthToHandle(text.c_str(), static_cast<int>(text.size()), handle);
}

int TextHeight(FontSize size)
{
    const int handle = FontManager::Instance().Handle(size);
    if (handle < 0) return 0;
    return GetFontSizeToHandle(handle);
}

namespace {
float AlignedX(FontSize size, float x, const std::string& text, TextAlign align)
{
    if (align == TextAlign::Left) return x;
    const float w = static_cast<float>(TextWidth(size, text));
    if (align == TextAlign::Center) return x - w * 0.5f;
    return x - w;
}
} // namespace

void Text(FontSize size, float x, float y, const ColorRGB& color, const std::string& text, TextAlign align)
{
    const int handle = FontManager::Instance().Handle(size);
    if (handle < 0) return;
    const float drawX = AlignedX(size, x, text, align);
    DrawStringToHandle(static_cast<int>(drawX), static_cast<int>(y), text.c_str(),
                       ToDx(color), handle, ToDx(ColorRGB(0, 0, 0)));
}

void TextShadow(FontSize size, float x, float y, const ColorRGB& color, const std::string& text,
                TextAlign align, const ColorRGB& shadow)
{
    const int handle = FontManager::Instance().Handle(size);
    if (handle < 0) return;
    const float drawX = AlignedX(size, x, text, align);
    DrawStringToHandle(static_cast<int>(drawX) + 2, static_cast<int>(y) + 2, text.c_str(),
                       ToDx(shadow), handle, ToDx(shadow));
    DrawStringToHandle(static_cast<int>(drawX), static_cast<int>(y), text.c_str(),
                       ToDx(color), handle, ToDx(ColorRGB(0, 0, 0)));
}

void TextAlpha(FontSize size, float x, float y, const ColorRGB& color, const std::string& text,
               int alpha, TextAlign align)
{
    AlphaScope scope(alpha);
    Text(size, x, y, color, text, align);
}

} // namespace draw
} // namespace ecl
