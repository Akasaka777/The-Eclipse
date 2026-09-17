//==============================================================================
// EquipPanel.h : 装備タブ（スロット選択 → 所持品から装備）
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/GameContext.h"
#include "Game/SwordSkill.h"
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

private:
    void Layout();
    void DrawSlotColumn(const GameContext& context) const;
    void DrawItemList(const GameContext& context) const;
    void DrawSkillList(const GameContext& context) const;
    void DrawComparison(const GameContext& context) const;
    void DrawSkillDetail(const GameContext& context) const;

    // スキルスロットを選択中か
    bool SkillMode() const { return skillSlot_ >= 0; }
    // 現在の武器で使える解放済みスキル
    std::vector<const SwordSkill*> AvailableSkills(const GameContext& context) const;

    Rect   window_;
    Button closeButton_;
    Button equipButton_;
    Button unequipButton_;
    Button sellButton_;
    std::vector<Button> slotButtons_;
    std::vector<Button> skillSlotButtons_;

    EquipSlot selectedSlot_ = EquipSlot::Weapon;
    // 0 以上ならスキルスロットを選択中（装備スロットの選択より優先）
    int  skillSlot_ = -1;
    int  selectedUid_ = 0;
    int  selectedSkillId_ = 0;
    int  scroll_ = 0;
    bool open_ = false;
    bool closeRequested_ = false;
    std::string message_;
    float messageTimer_ = 0.0f;
};

} // namespace ui
} // namespace ecl
