//==============================================================================
// UIWidgets.h : マウス操作に対応した共通 UI 部品
//==============================================================================
#pragma once

#include "Common/Types.h"
#include "Core/FontManager.h"
#include "Core/Input.h"
#include "Game/Equipment.h"

#include <string>

namespace ecl {
namespace ui {

//------------------------------------------------------------------------------
// ボタン
//------------------------------------------------------------------------------
class Button
{
public:
    Button() = default;
    Button(const Rect& rect, const std::string& label, FontSize fontSize = FontSize::Normal);

    void SetRect(const Rect& rect) { rect_ = rect; }
    void SetLabel(const std::string& label) { label_ = label; }
    void SetEnabled(bool enabled) { enabled_ = enabled; }
    void SetAccent(const ColorRGB& accent) { accent_ = accent; }
    void SetSelected(bool selected) { selected_ = selected; }
    void SetFontSize(FontSize size) { fontSize_ = size; }

    const Rect& GetRect() const { return rect_; }
    bool Enabled() const { return enabled_; }
    bool Hovered() const { return hovered_; }

    // クリックされたフレームで true
    bool Update(const Input& input, float dt);
    void Draw() const;

private:
    Rect        rect_;
    std::string label_;
    FontSize    fontSize_ = FontSize::Normal;
    ColorRGB    accent_ = ColorRGB(64, 206, 255);
    bool        enabled_ = true;
    bool        hovered_ = false;
    bool        selected_ = false;
    float       hoverAnim_ = 0.0f;
    float       pressAnim_ = 0.0f;
};

//------------------------------------------------------------------------------
// スライダー（0〜100）
//------------------------------------------------------------------------------
class Slider
{
public:
    Slider() = default;
    Slider(const Rect& rect, const std::string& label, int value);

    void SetRect(const Rect& rect) { rect_ = rect; }
    void SetValue(int value);
    int  Value() const { return value_; }
    void SetFontSize(FontSize size) { fontSize_ = size; }
    // ラベル欄の幅（つまみの開始位置）
    void SetLabelWidth(float width) { labelWidth_ = width; }

    // 値が変化したフレームで true
    bool Update(const Input& input);
    void Draw() const;

private:
    Rect        rect_;
    std::string label_;
    FontSize    fontSize_ = FontSize::Normal;
    float       labelWidth_ = 220.0f;
    int         value_ = 50;
    bool        dragging_ = false;
};

//------------------------------------------------------------------------------
// トグル
//------------------------------------------------------------------------------
class Toggle
{
public:
    Toggle() = default;
    Toggle(const Rect& rect, const std::string& label, bool value);

    void SetRect(const Rect& rect) { rect_ = rect; }
    void SetValue(bool value) { value_ = value; }
    bool Value() const { return value_; }
    void SetFontSize(FontSize size) { fontSize_ = size; }

    bool Update(const Input& input);
    void Draw() const;

private:
    Rect        rect_;
    std::string label_;
    FontSize    fontSize_ = FontSize::Normal;
    bool        value_ = false;
    bool        hovered_ = false;
};

//------------------------------------------------------------------------------
// 共通描画ヘルパ
//------------------------------------------------------------------------------
// 見出し付きのウィンドウ枠
void DrawWindow(const Rect& rect, const std::string& title);
// 一覧用のアイテム行
// showIv を立てると個体値を表示する（開発者モード専用。通常は隠す）
void DrawItemRow(const Rect& rect, const EquipmentItem& item, bool selected, bool equipped,
                 bool hovered, bool showIv = false);
// 武器種のアイコン
void DrawWeaponIcon(const Rect& rect, WeaponType type, const ColorRGB& color);
// スロットのアイコン
void DrawSlotIcon(const Rect& rect, EquipSlot slot, const ColorRGB& color);
// ステータス比較行（差分を色付きで表示）
void DrawStatDiffLine(float x, float y, const std::string& label, float current, float next, bool percent);
// 画面全体を覆う暗幕
void DrawDimOverlay(int alpha);

} // namespace ui
} // namespace ecl
