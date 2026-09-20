#include "UI/UIWidgets.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/GameConfig.h"
#include "Graphics/DrawUtil.h"

namespace ecl {
namespace ui {

//==============================================================================
// Button
//==============================================================================
Button::Button(const Rect& rect, const std::string& label, FontSize fontSize)
    : rect_(rect), label_(label), fontSize_(fontSize)
{
}

bool Button::Update(const Input& input, float dt)
{
    hovered_ = enabled_ && rect_.Contains(static_cast<float>(input.MouseX()),
                                          static_cast<float>(input.MouseY()));

    const float target = hovered_ ? 1.0f : 0.0f;
    hoverAnim_ = math::Approach(hoverAnim_, target, dt * 7.0f);
    pressAnim_ = math::MaxF(0.0f, pressAnim_ - dt * 5.0f);

    if (hovered_ && input.MouseClicked(MouseButton::Left)) {
        pressAnim_ = 1.0f;
        return true;
    }
    return false;
}

void Button::Draw() const
{
    const ColorRGB base = enabled_ ? palette::kPanelLight : palette::kPanelDark;
    const ColorRGB fill = ColorRGB::Lerp(base, accent_.Scaled(0.55f), hoverAnim_ * 0.75f);
    const ColorRGB border = enabled_ ? ColorRGB::Lerp(palette::kBorder, accent_, hoverAnim_)
                                     : palette::kTextDisabled;

    Rect drawRect = rect_;
    if (pressAnim_ > 0.0f) drawRect = rect_.Expanded(-2.0f * pressAnim_);

    draw::GradientRectV(drawRect, fill.Scaled(1.2f), fill.Scaled(0.7f), 240, 12);
    draw::StrokeRect(drawRect, border, selected_ ? 3.0f : 2.0f, 255);

    if (selected_) {
        draw::Line(drawRect.left + 6.0f, drawRect.bottom - 3.0f, drawRect.right - 6.0f,
                   drawRect.bottom - 3.0f, accent_, 3.0f, 255);
    }
    if (hoverAnim_ > 0.02f) {
        draw::StrokeRect(drawRect.Expanded(3.0f), accent_, 1.0f, static_cast<int>(160.0f * hoverAnim_));
    }

    const ColorRGB textColor = enabled_ ? palette::kText : palette::kTextDisabled;
    draw::Text(fontSize_, drawRect.CenterX(),
               drawRect.CenterY() - static_cast<float>(draw::TextHeight(fontSize_)) * 0.5f,
               textColor, label_, draw::TextAlign::Center);
}

//==============================================================================
// Slider
//==============================================================================
Slider::Slider(const Rect& rect, const std::string& label, int value)
    : rect_(rect), label_(label), value_(math::ClampInt(value, 0, 100))
{
}

void Slider::SetValue(int value)
{
    value_ = math::ClampInt(value, 0, 100);
}

bool Slider::Update(const Input& input)
{
    const float mouseX = static_cast<float>(input.MouseX());
    const float mouseY = static_cast<float>(input.MouseY());

    const Rect track = Rect(rect_.left + labelWidth_, rect_.CenterY() - 8.0f, rect_.right - 76.0f,
                            rect_.CenterY() + 8.0f);

    if (input.MouseClicked(MouseButton::Left) && track.Expanded(12.0f).Contains(mouseX, mouseY)) {
        dragging_ = true;
    }
    if (!input.MouseDown(MouseButton::Left)) dragging_ = false;

    if (dragging_) {
        const float ratio = math::Clamp((mouseX - track.left) / math::MaxF(track.Width(), 1.0f), 0.0f, 1.0f);
        const int newValue = static_cast<int>(ratio * 100.0f + 0.5f);
        if (newValue != value_) {
            value_ = newValue;
            return true;
        }
    }
    return false;
}

void Slider::Draw() const
{
    draw::Text(fontSize_, rect_.left,
               rect_.CenterY() - static_cast<float>(draw::TextHeight(fontSize_)) * 0.5f,
               palette::kText, label_);

    const Rect track = Rect(rect_.left + labelWidth_, rect_.CenterY() - 8.0f, rect_.right - 76.0f,
                            rect_.CenterY() + 8.0f);
    draw::FillRect(track, palette::kPanelDark, 255);
    draw::StrokeRect(track, palette::kBorder, 1.0f, 200);

    const float ratio = static_cast<float>(value_) / 100.0f;
    Rect fill = track;
    fill.right = track.left + track.Width() * ratio;
    draw::GradientRectV(fill, palette::kAccent.Scaled(1.2f), palette::kAccent.Scaled(0.6f), 255, 6);

    const float knobX = track.left + track.Width() * ratio;
    draw::Circle(knobX, track.CenterY(), 13.0f, palette::kText, true, 1.0f, 255);
    draw::Circle(knobX, track.CenterY(), 13.0f, palette::kAccent, false, 2.0f, 255);

    draw::Text(fontSize_, rect_.right,
               rect_.CenterY() - static_cast<float>(draw::TextHeight(fontSize_)) * 0.5f,
               palette::kTextDim, str::Format("%d", value_), draw::TextAlign::Right);
}

//==============================================================================
// Toggle
//==============================================================================
Toggle::Toggle(const Rect& rect, const std::string& label, bool value)
    : rect_(rect), label_(label), value_(value)
{
}

bool Toggle::Update(const Input& input)
{
    const Rect box(rect_.right - 84.0f, rect_.CenterY() - 16.0f, rect_.right - 12.0f,
                   rect_.CenterY() + 16.0f);
    hovered_ = box.Expanded(8.0f).Contains(static_cast<float>(input.MouseX()),
                                           static_cast<float>(input.MouseY()));
    if (hovered_ && input.MouseClicked(MouseButton::Left)) {
        value_ = !value_;
        return true;
    }
    return false;
}

void Toggle::Draw() const
{
    draw::Text(fontSize_, rect_.left,
               rect_.CenterY() - static_cast<float>(draw::TextHeight(fontSize_)) * 0.5f,
               palette::kText, label_);

    const Rect box(rect_.right - 84.0f, rect_.CenterY() - 16.0f, rect_.right - 12.0f,
                   rect_.CenterY() + 16.0f);
    const ColorRGB fill = value_ ? palette::kAccent.Scaled(0.55f) : palette::kPanelDark;
    draw::FillRect(box, fill, 255);
    draw::StrokeRect(box, hovered_ ? palette::kAccent : palette::kBorder, 2.0f, 255);

    const float knobX = value_ ? box.right - 18.0f : box.left + 18.0f;
    draw::Circle(knobX, box.CenterY(), 12.0f, value_ ? palette::kAccent : palette::kTextDim, true, 1.0f, 255);
    draw::Text(FontSize::Tiny, value_ ? box.left + 16.0f : box.right - 16.0f,
               box.CenterY() - 8.0f, value_ ? palette::kAccent : palette::kTextDim,
               value_ ? "ON" : "OFF", draw::TextAlign::Center);
}

//==============================================================================
// 共通描画
//==============================================================================
void DrawWindow(const Rect& rect, const std::string& title)
{
    draw::Panel(rect, palette::kPanel, palette::kBorder, 242, 2.0f);

    const Rect header(rect.left, rect.top, rect.right, rect.top + 54.0f);
    draw::GradientRectH(header, palette::kAccent.Scaled(0.42f), palette::kPanel, 235, 24);
    draw::Line(rect.left, header.bottom, rect.right, header.bottom, palette::kAccent, 2.0f, 220);
    draw::Text(FontSize::Medium, rect.left + 24.0f, rect.top + 10.0f, palette::kText, title);
}

void DrawWeaponIcon(const Rect& rect, WeaponType type, const ColorRGB& color)
{
    const float cx = rect.CenterX();
    const float cy = rect.CenterY();
    const float size = math::MinF(rect.Width(), rect.Height()) * 0.42f;

    switch (type) {
    case WeaponType::OneHandSword:
        draw::Line(cx - size * 0.5f, cy + size, cx + size * 0.4f, cy - size, color, 5.0f, 255);
        draw::Line(cx - size * 0.2f, cy + size * 0.3f, cx + size * 0.2f, cy + size * 0.6f, color, 4.0f, 255);
        break;
    case WeaponType::OneHandMace:
        draw::Line(cx - size * 0.5f, cy + size, cx + size * 0.2f, cy - size * 0.3f, color, 5.0f, 255);
        draw::Circle(cx + size * 0.35f, cy - size * 0.6f, size * 0.42f, color, true, 1.0f, 255);
        break;
    case WeaponType::Dagger:
        draw::Line(cx - size * 0.3f, cy + size * 0.7f, cx + size * 0.3f, cy - size * 0.5f, color, 5.0f, 255);
        draw::Line(cx - size * 0.1f, cy + size * 0.2f, cx + size * 0.25f, cy + size * 0.45f, color, 4.0f, 255);
        break;
    case WeaponType::Rapier:
        draw::Line(cx - size * 0.5f, cy + size, cx + size * 0.5f, cy - size, color, 3.0f, 255);
        draw::Circle(cx - size * 0.3f, cy + size * 0.6f, size * 0.22f, color, false, 3.0f, 255);
        break;
    case WeaponType::Spear:
        draw::Line(cx - size * 0.6f, cy + size, cx + size * 0.5f, cy - size * 0.7f, color, 4.0f, 255);
        draw::Triangle(Vec2(cx + size * 0.3f, cy - size * 0.5f), Vec2(cx + size * 0.7f, cy - size * 0.4f),
                       Vec2(cx + size * 0.6f, cy - size), color, true, 255);
        break;
    default:
        break;
    }
}

void DrawSlotIcon(const Rect& rect, EquipSlot slot, const ColorRGB& color)
{
    const float cx = rect.CenterX();
    const float cy = rect.CenterY();
    const float size = math::MinF(rect.Width(), rect.Height()) * 0.36f;

    switch (slot) {
    case EquipSlot::WeaponRight:
    case EquipSlot::WeaponLeft:
        DrawWeaponIcon(rect, WeaponType::OneHandSword, color);
        break;
    case EquipSlot::Head:
        draw::Circle(cx, cy + size * 0.2f, size * 0.7f, color, false, 3.0f, 255);
        draw::Line(cx - size * 0.7f, cy + size * 0.2f, cx + size * 0.7f, cy + size * 0.2f, color, 3.0f, 255);
        break;
    case EquipSlot::Body:
        draw::FillRect(Rect::FromCenter(cx, cy, size * 1.3f, size * 1.7f), color.Scaled(0.7f), 255);
        draw::StrokeRect(Rect::FromCenter(cx, cy, size * 1.3f, size * 1.7f), color, 2.0f, 255);
        break;
    case EquipSlot::Shield:
        draw::Triangle(Vec2(cx - size, cy - size), Vec2(cx + size, cy - size), Vec2(cx, cy + size * 1.2f),
                       color.Scaled(0.7f), true, 255);
        draw::Triangle(Vec2(cx - size, cy - size), Vec2(cx + size, cy - size), Vec2(cx, cy + size * 1.2f),
                       color, false, 255);
        break;
    case EquipSlot::Arm:
        draw::FillRect(Rect::FromCenter(cx, cy, size * 0.8f, size * 1.6f), color.Scaled(0.7f), 255);
        break;
    case EquipSlot::Hand:
        draw::Circle(cx, cy, size * 0.8f, color.Scaled(0.7f), true, 1.0f, 255);
        draw::Line(cx, cy - size, cx, cy + size, color, 2.0f, 255);
        break;
    case EquipSlot::Foot:
        draw::FillRect(Rect(cx - size, cy - size * 0.2f, cx + size * 0.4f, cy + size), color.Scaled(0.7f), 255);
        draw::FillRect(Rect(cx - size, cy + size * 0.5f, cx + size, cy + size), color, 255);
        break;
    default:
        break;
    }
}

void DrawItemRow(const Rect& rect, const EquipmentItem& item, bool selected, bool equipped, bool hovered)
{
    // 個体値は画面に出さない（能力値と耐久力の違いとして感じ取ってもらう）
    const ColorRGB accent = item.IsWeapon() ? palette::kAccent : palette::kAccentWarm;

    ColorRGB fill = palette::kPanelDark;
    if (selected) fill = ColorRGB::Lerp(palette::kPanelLight, accent.Scaled(0.5f), 0.55f);
    else if (hovered) fill = palette::kPanelLight;

    draw::GradientRectH(rect, fill, fill.Scaled(0.75f), 235, 12);
    draw::StrokeRect(rect, selected ? accent : palette::kBorder.Scaled(0.7f), selected ? 2.0f : 1.0f, 255);
    draw::FillRect(Rect(rect.left, rect.top, rect.left + 6.0f, rect.bottom), accent, 255);

    const Rect iconRect(rect.left + 12.0f, rect.top + 6.0f, rect.left + 62.0f, rect.bottom - 6.0f);
    if (item.IsWeapon()) DrawWeaponIcon(iconRect, item.weaponType, accent);
    else DrawSlotIcon(iconRect, item.slot, accent);

    draw::Text(FontSize::Normal, rect.left + 74.0f, rect.top + 8.0f, palette::kText, item.DisplayName());

    const std::string sub = item.IsWeapon()
        ? str::Format("%s  ATK %d", WeaponTypeName(item.weaponType),
                      static_cast<int>(item.TotalStats().attack))
        : str::Format("%s  DEF %d  HP %d", EquipSlotName(item.slot),
                      static_cast<int>(item.TotalStats().defense),
                      static_cast<int>(item.TotalStats().maxHp));
    draw::Text(FontSize::Small, rect.left + 74.0f, rect.top + 36.0f, palette::kTextDim, sub);

    draw::Text(FontSize::Small, rect.right - 12.0f, rect.top + 10.0f, palette::kText,
               str::Format("戦力 %d", item.Power()), draw::TextAlign::Right);

    // --- 耐久力 ---------------------------------------------------------------
    const Rect durabilityBar(rect.right - 200.0f, rect.bottom - 16.0f, rect.right - 12.0f,
                             rect.bottom - 8.0f);
    draw::FillRect(durabilityBar, palette::kPanelDark, 255);
    draw::Bar(durabilityBar, item.DurabilityRatio(), item.DurabilityColor(),
              ColorRGB(38, 40, 50));
    draw::Text(FontSize::Tiny, durabilityBar.left - 8.0f, durabilityBar.top - 7.0f,
               item.DurabilityColor(),
               str::Format("耐久 %d/%d", item.DurabilityDisplay(), item.MaxDurabilityDisplay()),
               draw::TextAlign::Right);

    if (item.IsWeakened()) {
        draw::Text(FontSize::Tiny, rect.right - 12.0f, rect.top + 36.0f, palette::kDanger,
                   "性能低下中（要修理）", draw::TextAlign::Right);
    }

    if (equipped) {
        draw::FillRect(Rect(rect.left + 6.0f, rect.top, rect.left + 68.0f, rect.top + 22.0f),
                       palette::kAccent, 220);
        draw::Text(FontSize::Tiny, rect.left + 37.0f, rect.top + 3.0f, palette::kBlack, "装備中",
                   draw::TextAlign::Center);
    }
}

void DrawStatDiffLine(float x, float y, const std::string& label, float current, float next, bool percent)
{
    draw::Text(FontSize::Small, x, y, palette::kTextDim, label);

    const std::string currentText = percent ? str::Format("%.1f%%", current * 100.0f)
                                            : str::Format("%d", static_cast<int>(current));
    draw::Text(FontSize::Small, x + 210.0f, y, palette::kText, currentText, draw::TextAlign::Right);

    const float diff = next - current;
    if (math::Abs(diff) < (percent ? 0.0005f : 0.5f)) return;

    const ColorRGB color = (diff > 0.0f) ? palette::kHp : palette::kDanger;
    const std::string diffText = percent
        ? str::Format("%s%.1f%%", diff > 0.0f ? "+" : "", diff * 100.0f)
        : str::Format("%s%d", diff > 0.0f ? "+" : "", static_cast<int>(diff));

    draw::Text(FontSize::Small, x + 226.0f, y, palette::kTextDim, "→");
    draw::Text(FontSize::Small, x + 340.0f, y, color, diffText, draw::TextAlign::Right);
}

void DrawDimOverlay(int alpha)
{
    draw::FillRect(Rect(0.0f, 0.0f, static_cast<float>(config::kScreenWidth),
                        static_cast<float>(config::kScreenHeight)),
                   palette::kBlack, alpha);
}

} // namespace ui
} // namespace ecl
