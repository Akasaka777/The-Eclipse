//==============================================================================
// BattleHud.h : 戦闘中の HUD
//   左上 : キャラアイコン＋HP/MP  上部中央 : ボス HP  右下 : スキルバー
//   右下（スキルバーの左）: アイテムスライダー（回復 ⇔ バフ）
//   上部右 : MENU ボタン
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/Boss.h"
#include "Game/Player.h"
#include "Game/PlayerData.h"
#include "Game/SwordSkill.h"
#include "UI/UIWidgets.h"

#include <string>

namespace ecl {
namespace ui {

//------------------------------------------------------------------------------
// HUD に表示する進行情報
//------------------------------------------------------------------------------
struct HudInfo
{
    std::string floorName;
    int   floorIndex = 1;
    int   floorCount = 1;
    int   enemiesRemaining = 0;
    float questTime = 0.0f;
    int   combo = 0;
    float comboTimer = 0.0f;
    bool  showFps = false;
};

class BattleHud
{
public:
    BattleHud();

    void Reset();
    void Update(float dt, const Player& player, const Boss* boss, const Input& input);
    void Draw(const Player& player, const PlayerData& data, const Boss* boss, const HudInfo& info) const;

    // クリック結果（Update 後に参照）
    int  ClickedSkillIndex() const { return clickedSkill_; }

    // --- アイテムスライダー ----------------------------------------------------
    //   Q（または上の見出しをクリック）で 回復 ⇔ バフ を切り替え、
    //   E（または本体をクリック）で選んでいる枠のアイテムを使う。
    QuickSlot ItemSlot() const { return itemSlot_; }
    void SetItemSlot(QuickSlot slot) { itemSlot_ = slot; }
    // このフレームでアイテムを使う操作があったか
    bool ItemUseRequested() const { return itemUseRequested_; }
    // スライダーをクリックしたフレームか（同じクリックで攻撃を出さないために使う）
    bool ItemClickConsumed() const { return itemClickConsumed_; }
    // 「HP は満タンです」などの短い通知
    void ShowItemMessage(const std::string& message);

    // --- レイアウト参照（検証用）---------------------------------------------
    const Rect& ItemPanelRect() const { return itemPanel_; }
    const Rect& ItemTabRect(QuickSlot slot) const;
    const Rect& ItemBodyRect() const { return itemBody_; }

private:
    void DrawPlayerStatus(const Player& player, const PlayerData& data) const;
    void DrawBossStatus(const Boss& boss) const;
    void DrawSkillBar(const Player& player, const PlayerData& data) const;
    void DrawFloorInfo(const HudInfo& info) const;
    void DrawCombo(const HudInfo& info) const;
    void DrawItemSlider(const Player& player, const PlayerData& data) const;

    Rect   skillRects_[kSkillSlotCount];
    float  hpDelay_ = 1.0f;
    float  bossHpDelay_ = 1.0f;
    int    clickedSkill_ = -1;
    float  time_ = 0.0f;

    // アイテムスライダー
    Rect      itemPanel_;
    Rect      itemTabs_[kQuickSlotCount];
    Rect      itemBody_;
    QuickSlot itemSlot_ = QuickSlot::Recovery;
    float     itemSlide_ = 0.0f;   // 0 = 回復 / 1 = バフ（見出しの光る帯を滑らせる）
    bool      itemUseRequested_ = false;
    bool      itemClickConsumed_ = false;
    std::string itemMessage_;
    float     itemMessageTimer_ = 0.0f;
};

} // namespace ui
} // namespace ecl
