//==============================================================================
// DrawUtil.h : 図形 / テキスト描画のラッパ
//==============================================================================
#pragma once

#include "Common/Types.h"
#include "Core/FontManager.h"

#include <string>

namespace ecl {
namespace draw {

enum class TextAlign
{
    Left,
    Center,
    Right
};

unsigned int ToDx(const ColorRGB& color);

// --- 図形 -------------------------------------------------------------------
void FillRect(const Rect& rect, const ColorRGB& color, int alpha = 255);
void FillRectXYWH(float x, float y, float w, float h, const ColorRGB& color, int alpha = 255);
void StrokeRect(const Rect& rect, const ColorRGB& color, float thickness = 1.0f, int alpha = 255);
// 上下グラデーション（band 段階で塗り分け）
void GradientRectV(const Rect& rect, const ColorRGB& top, const ColorRGB& bottom, int alpha = 255, int bands = 24);
void GradientRectH(const Rect& rect, const ColorRGB& left, const ColorRGB& right, int alpha = 255, int bands = 24);
void Line(float x1, float y1, float x2, float y2, const ColorRGB& color, float thickness = 1.0f, int alpha = 255);
void Circle(float cx, float cy, float radius, const ColorRGB& color, bool fill = true,
            float thickness = 1.0f, int alpha = 255);
void Triangle(const Vec2& a, const Vec2& b, const Vec2& c, const ColorRGB& color, bool fill = true, int alpha = 255);
// 角を落とした矩形（UI パネル用）
void ChamferRect(const Rect& rect, float cut, const ColorRGB& color, int alpha = 255);
// 加算合成の光
void Glow(float cx, float cy, float radius, const ColorRGB& color, int alpha = 160, int layers = 5);

// --- UI パーツ ---------------------------------------------------------------
// 枠付きパネル（四隅にアクセント）
void Panel(const Rect& rect, const ColorRGB& fill, const ColorRGB& border, int fillAlpha = 235, float thickness = 2.0f);
// 汎用ゲージ（ratio 0..1、delayRatio は減少残像）
void Bar(const Rect& rect, float ratio, const ColorRGB& fg, const ColorRGB& bg,
         float delayRatio = -1.0f, const ColorRGB& delayColor = ColorRGB(226, 92, 92), int alpha = 255);
// 斜めに切ったゲージ（HUD 用）
void SkewBar(const Rect& rect, float ratio, const ColorRGB& fg, const ColorRGB& bg, float skew = 12.0f,
             float delayRatio = -1.0f, const ColorRGB& delayColor = ColorRGB(226, 92, 92));

// --- テキスト ---------------------------------------------------------------
int  TextWidth(FontSize size, const std::string& text);
int  TextHeight(FontSize size);
void Text(FontSize size, float x, float y, const ColorRGB& color, const std::string& text,
          TextAlign align = TextAlign::Left);
// 影付きテキスト
void TextShadow(FontSize size, float x, float y, const ColorRGB& color, const std::string& text,
                TextAlign align = TextAlign::Left, const ColorRGB& shadow = ColorRGB(0, 0, 0));
// 半透明テキスト
void TextAlpha(FontSize size, float x, float y, const ColorRGB& color, const std::string& text,
               int alpha, TextAlign align = TextAlign::Left);

} // namespace draw
} // namespace ecl
