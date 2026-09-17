//==============================================================================
// SkillPanel.h : スキルツリータブ
//   スキルポイントでスキルを解放し、4 つのスロットへ装備する。
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/GameContext.h"
#include "Game/SwordSkill.h"
#include "UI/UIWidgets.h"

#include <string>
#include <vector>

namespace ecl {
namespace ui {

class SkillPanel
{
public:
    SkillPanel();

    void Open(const GameContext& context);
    void Close() { open_ = false; }
    bool IsOpen() const { return open_; }

    void Update(float dt, const Input& input, GameContext& context);
    void Draw(const GameContext& context) const;

    bool CloseRequested() const { return closeRequested_; }

private:
    void Layout();
    Rect NodeRect(int column, int tier) const;
    void DrawWeaponTabs(const GameContext& context) const;
    void DrawTree(const GameContext& context) const;
    void DrawDetail(const GameContext& context) const;
    void DrawSlots(const GameContext& context) const;

    // 表示中の武器種がプレイヤーの装備武器と一致しているか
    bool IsCurrentWeapon(const GameContext& context) const;

    Rect window_;
    Rect treeArea_;
    Rect detailArea_;
    Rect slotRects_[kSkillSlotCount];

    std::vector<Button> weaponButtons_;
    Button unlockButton_;
    Button equipButton_;
    Button unequipButton_;
    Button closeButton_;

    WeaponType viewWeapon_ = WeaponType::OneHandSword;
    int   selectedSkillId_ = 0;
    int   targetSlot_ = 0;
    bool  open_ = false;
    bool  closeRequested_ = false;

    std::string message_;
    float messageTimer_ = 0.0f;
    float time_ = 0.0f;
};

} // namespace ui
} // namespace ecl
