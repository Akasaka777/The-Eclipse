#include "UI/KeyConfigPanel.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/DxInclude.h"
#include "Core/GameConfig.h"
#include "Graphics/DrawUtil.h"

#include <cmath>

namespace ecl {
namespace ui {

namespace {
constexpr float kRowHeight = 44.0f;
constexpr float kLabelX = 60.0f;      // 窓の左端から
constexpr float kSlotX = 400.0f;      // 1 つ目の枠の左端（窓の左端から）
constexpr float kSlotWidth = 300.0f;
constexpr float kSlotGap = 20.0f;
} // namespace

KeyConfigPanel::KeyConfigPanel()
{
    for (int i = 0; i < kGameActionCount; ++i) {
        const GameAction action = static_cast<GameAction>(i);
        if (KeyConfig::IsConfigurable(action)) actions_.push_back(action);
    }
    Layout();
}

void KeyConfigPanel::Layout()
{
    window_ = Rect::FromXYWH(400.0f, 40.0f, 1120.0f, 1000.0f);

    slotButtons_.clear();
    const float top = window_.top + 168.0f;
    for (size_t row = 0; row < actions_.size(); ++row) {
        const float y = top + kRowHeight * static_cast<float>(row);
        for (int slot = 0; slot < kKeySlotsPerAction; ++slot) {
            const float x = window_.left + kSlotX + (kSlotWidth + kSlotGap) * static_cast<float>(slot);
            slotButtons_.push_back(Button(Rect::FromXYWH(x, y, kSlotWidth, kRowHeight - 6.0f), "",
                                          FontSize::Small));
        }
    }

    resetButton_ = Button(Rect::FromXYWH(window_.left + 40.0f, window_.bottom - 84.0f, 280.0f, 56.0f),
                          "初期設定に戻す", FontSize::Small);
    resetButton_.SetAccent(palette::kAccentWarm);
    closeButton_ = Button(Rect::FromXYWH(window_.right - 220.0f, window_.bottom - 84.0f, 180.0f, 56.0f),
                          "閉じる");
}

const Rect& KeyConfigPanel::SlotRect(int row, int slot) const
{
    const int index = math::ClampInt(row, 0, static_cast<int>(actions_.size()) - 1) * kKeySlotsPerAction
                    + math::ClampInt(slot, 0, kKeySlotsPerAction - 1);
    return slotButtons_[static_cast<size_t>(index)].GetRect();
}

void KeyConfigPanel::Open()
{
    open_ = true;
    waitingAction_ = -1;
    waitingSlot_ = -1;
    message_.clear();
    messageTimer_ = 0.0f;
}

void KeyConfigPanel::ShowMessage(const std::string& text, bool warning)
{
    message_ = text;
    messageWarning_ = warning;
    messageTimer_ = 4.0f;
}

void KeyConfigPanel::Update(float dt, const Input& input, GameContext& context)
{
    if (!open_) return;
    time_ += dt;
    messageTimer_ = math::MaxF(0.0f, messageTimer_ - dt);

    // 入力待ちの間は、押されたキー（マウスのボタン）をその枠に割り当てる
    if (waitingAction_ >= 0) {
        UpdateWaiting(input, context);
        return;
    }

    for (size_t row = 0; row < actions_.size(); ++row) {
        for (int slot = 0; slot < kKeySlotsPerAction; ++slot) {
            Button& button = slotButtons_[row * kKeySlotsPerAction + static_cast<size_t>(slot)];
            if (!button.Update(input, dt)) continue;
            waitingAction_ = static_cast<int>(row);
            waitingSlot_ = slot;
            ShowMessage(str::Format("「%s」のキー %d に割り当てるキーかマウスのボタンを押してください",
                                    KeyConfig::ActionLabel(actions_[row]), slot + 1),
                        false);
            return;
        }
    }

    if (resetButton_.Update(input, dt)) {
        context.settings.keys.ResetToDefaults();
        Input::Instance().ApplyKeyConfig(context.settings.keys);
        ShowMessage("キー設定を初期設定に戻しました", false);
    }

    if (closeButton_.Update(input, dt) || input.Pressed(GameAction::Cancel)) {
        Close();
    }
}

bool KeyConfigPanel::UpdateWaiting(const Input& input, GameContext& context)
{
    const GameAction action = actions_[static_cast<size_t>(waitingAction_)];
    const int key = input.FirstPressedKey();

    // ESC : 取り消し
    if (key == KEY_INPUT_ESCAPE) {
        waitingAction_ = -1;
        waitingSlot_ = -1;
        ShowMessage("変更を取り消しました", false);
        return true;
    }
    // BackSpace : 枠を空にする（最後の 1 つは外せない）
    if (key == KEY_INPUT_BACK) {
        if (context.settings.keys.Clear(action, waitingSlot_)) {
            Input::Instance().ApplyKeyConfig(context.settings.keys);
            ShowMessage(str::Format("「%s」のキー %d を空にしました", KeyConfig::ActionLabel(action),
                                    waitingSlot_ + 1),
                        false);
        } else {
            ShowMessage(str::Format("「%s」のキーをすべて外すことはできません", KeyConfig::ActionLabel(action)),
                        true);
        }
        waitingAction_ = -1;
        waitingSlot_ = -1;
        return true;
    }
    if (key >= 0) {
        if (!KeyConfig::IsAssignable(key)) {
            // 待ったまま、ほかのキーを押してもらう
            ShowMessage("そのキーは割り当てられません（ESC で取り消し）", true);
            return false;
        }
        AssignTo(key, context);
        return true;
    }

    // マウスのボタン
    const MouseButton buttons[] = { MouseButton::Left, MouseButton::Right, MouseButton::Middle };
    for (MouseButton button : buttons) {
        if (input.MouseClicked(button)) {
            AssignTo(MouseInputCode(button), context);
            return true;
        }
    }
    return false;
}

void KeyConfigPanel::AssignTo(int code, GameContext& context)
{
    const GameAction action = actions_[static_cast<size_t>(waitingAction_)];
    KeyConfig& keys = context.settings.keys;
    const GameAction swapped = keys.Assign(action, waitingSlot_, code);
    Input::Instance().ApplyKeyConfig(keys);

    const std::string name = KeyConfig::InputName(code);
    if (swapped == GameAction::Count || swapped == action) {
        ShowMessage(str::Format("「%s」に %s を割り当てました", KeyConfig::ActionLabel(action), name.c_str()),
                    false);
    } else if (!keys.HasAny(swapped)) {
        // 入れ替えた相手にキーが残らなかった
        ShowMessage(str::Format("%s は「%s」から移しました。「%s」のキーが未設定です",
                                name.c_str(), KeyConfig::ActionLabel(swapped), KeyConfig::ActionLabel(swapped)),
                    true);
    } else {
        ShowMessage(str::Format("%s を「%s」と入れ替えました", name.c_str(), KeyConfig::ActionLabel(swapped)),
                    false);
    }
    waitingAction_ = -1;
    waitingSlot_ = -1;
}

void KeyConfigPanel::Draw(const GameContext& context) const
{
    if (!open_) return;

    DrawDimOverlay(120);
    DrawWindow(window_, "キー設定");

    const KeyConfig& keys = context.settings.keys;
    const float left = window_.left + 40.0f;
    const float right = window_.right - 40.0f;

    draw::Text(FontSize::Small, left, window_.top + 70.0f, palette::kText,
               "変えたい枠をクリックして、割り当てるキーかマウスのボタンを押してください。");
    draw::Text(FontSize::Tiny, left, window_.top + 100.0f, palette::kTextDim,
               "ESC：取り消し ／ BackSpace：枠を空にする ／ ほかで使っているキーは、その操作と入れ替わります");

    // 見出し
    const float headY = window_.top + 136.0f;
    draw::Text(FontSize::Small, window_.left + kLabelX, headY, palette::kAccent, "操作");
    for (int slot = 0; slot < kKeySlotsPerAction; ++slot) {
        const float x = window_.left + kSlotX + (kSlotWidth + kSlotGap) * static_cast<float>(slot);
        draw::Text(FontSize::Small, x + kSlotWidth * 0.5f, headY, palette::kAccent,
                   str::Format("キー %d", slot + 1), draw::TextAlign::Center);
    }
    draw::Line(left, headY + 26.0f, right, headY + 26.0f, palette::kBorder, 1.0f, 130);

    // 行
    for (size_t row = 0; row < actions_.size(); ++row) {
        const GameAction action = actions_[row];
        const Rect first = SlotRect(static_cast<int>(row), 0);
        const Rect band(left, first.top - 3.0f, right, first.bottom + 3.0f);
        if (row % 2 == 0) draw::FillRect(band, palette::kPanelLight, 60);

        const bool unbound = !keys.HasAny(action);
        draw::Text(FontSize::Small, window_.left + kLabelX, first.top + 8.0f,
                   unbound ? palette::kDanger : palette::kText,
                   unbound ? str::Format("%s（未設定）", KeyConfig::ActionLabel(action))
                           : std::string(KeyConfig::ActionLabel(action)));

        for (int slot = 0; slot < kKeySlotsPerAction; ++slot) {
            const Button& button = slotButtons_[row * kKeySlotsPerAction + static_cast<size_t>(slot)];
            button.Draw();
            const Rect& rect = button.GetRect();
            const bool waiting = (waitingAction_ == static_cast<int>(row) && waitingSlot_ == slot);
            if (waiting) {
                const int alpha = static_cast<int>(150.0f + 100.0f * std::sin(time_ * 8.0f));
                draw::StrokeRect(rect.Expanded(2.0f), palette::kAccentWarm, 3.0f, math::ClampInt(alpha, 0, 255));
                draw::Text(FontSize::Small, rect.CenterX(), rect.top + 8.0f, palette::kAccentWarm,
                           "キーを押してください", draw::TextAlign::Center);
                continue;
            }
            const int code = keys.Get(action, slot);
            draw::Text(FontSize::Small, rect.CenterX(), rect.top + 8.0f,
                       (code == kNoInput) ? palette::kTextDisabled : palette::kText,
                       (code == kNoInput) ? std::string("―") : KeyConfig::InputName(code),
                       draw::TextAlign::Center);
        }
    }

    // 固定のキー
    const float noteY = SlotRect(static_cast<int>(actions_.size()) - 1, 0).bottom + 18.0f;
    draw::Text(FontSize::Tiny, left, noteY, palette::kTextDim,
               "メニュー（ESC）・決定（ENTER）・取り消し（ESC / BackSpace）と開発者用の F1〜F12 は変更できません");

    if (messageTimer_ > 0.0f) {
        draw::Text(FontSize::Small, left, noteY + 32.0f, messageWarning_ ? palette::kDanger : palette::kAccent,
                   message_);
    }

    resetButton_.Draw();
    closeButton_.Draw();
}

} // namespace ui
} // namespace ecl
