//==============================================================================
// EquipPanel.h : 装備タブ（スロット選択 → 所持品から装備）
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/GameContext.h"
#include "UI/UIWidgets.h"

#include <vector>

namespace ecl {
namespace ui {

class EquipPanel
{
public:
    EquipPanel();

    void Open();
    void Close() { open_ = false; }
    bool IsOpen() const { return open_; }

    void Update(float dt, const Input& input, GameContext& context);
    void Draw(const GameContext& context) const;

    bool CloseRequested() const { return closeRequested_; }
    // このフレームで装備が変化したか（戦闘中の反映に使う）
    bool EquipmentChanged() const { return equipmentChanged_; }
    // 売却ボタンを隠す（戦闘中は使わせない）
    void SetSellEnabled(bool enabled) { sellEnabled_ = enabled; }

private:
    void Layout();
    void DrawSlotColumn(const GameContext& context) const;
    void DrawItemList(const GameContext& context) const;
    void DrawComparison(const GameContext& context) const;


    Rect   window_;
    Button closeButton_;
    Button equipButton_;
    Button unequipButton_;
    Button sellButton_;
    std::vector<Button> slotButtons_;

    EquipSlot selectedSlot_ = EquipSlot::Weapon;
    int  selectedUid_ = 0;
    int  scroll_ = 0;
    bool open_ = false;
    bool closeRequested_ = false;
    bool equipmentChanged_ = false;
    bool sellEnabled_ = true;
    std::string message_;
    float messageTimer_ = 0.0f;
};

} // namespace ui
} // namespace ecl
