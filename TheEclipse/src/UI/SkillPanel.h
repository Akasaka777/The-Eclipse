//==============================================================================
// SkillPanel.h : スキルツリータブ
//   スキルポイントでスキルを解放し、スロットへ装備する。
//   ユニークスキルを習得すると、そのユニークスキル名のタブが増える。
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/GameContext.h"
#include "Game/SwordSkill.h"
#include "Game/UniqueSkill.h"
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
    // このフレームでスキル構成が変わったか（戦闘中の反映に使う）
    bool LoadoutChanged() const { return loadoutChanged_; }
    // ユニークスキルの特別クエストへ出撃したいか（0 なら要求なし）
    int  SpecialQuestRequested() const { return specialQuestId_; }

private:
    void Layout();
    Rect NodeRect(int column, int tier) const;
    void DrawWeaponTabs(const GameContext& context) const;
    void DrawTree(const GameContext& context) const;
    void DrawUniqueTree(const GameContext& context) const;
    void DrawNode(const Rect& rect, const SwordSkill& skill, const PlayerData& player,
                  const std::string& status, const ColorRGB& statusColor) const;
    // 表示できるユニークスキル（習得済み、または解放条件を満たしたもの）
    UniqueSkillType VisibleUniqueType(const GameContext& context) const;
    void DrawDetail(const GameContext& context) const;
    void DrawSlots(const GameContext& context) const;
    // タブの右上に付ける小さなバッジ
    void DrawTabBadge(const Rect& tabRect, const char* label, const ColorRGB& color) const;

    // 選択中のスキルを今すぐ装備できるか（できない場合は理由を返す）
    bool CanEquipSelected(const GameContext& context, std::string& outReason) const;

    Rect window_;
    Rect treeArea_;
    Rect detailArea_;
    Rect slotRects_[kSkillSlotCount];
    Rect spTag_;

    std::vector<Button> weaponButtons_;
    Button unlockButton_;
    Button uniqueTabButton_;
    // 専用スキルを持たないユニークスキル用の習得ボタン
    Button acquireButton_;
    // 特別クエストへの出撃ボタン
    Button specialQuestButton_;
    Button equipButton_;
    Button unequipButton_;
    Button closeButton_;

    WeaponType viewWeapon_ = WeaponType::OneHandSword;
    bool  uniqueTab_ = false;
    int   selectedSkillId_ = 0;
    int   targetSlot_ = 0;
    bool  open_ = false;
    bool  closeRequested_ = false;
    bool  loadoutChanged_ = false;
    int   specialQuestId_ = 0;

    std::string message_;
    float messageTimer_ = 0.0f;
    float time_ = 0.0f;
};

} // namespace ui
} // namespace ecl
