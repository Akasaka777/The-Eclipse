//==============================================================================
// BattleHud.h : 戦闘中の HUD
//   左上 : キャラアイコン＋HP/MP  右下 : スキルバー
//   ボスの頭上の右上 : ボス HP（下へ複数段に連なるゲージ）
//   左下 : アイテムスロット（回復 / バフの 2 枚を前後に重ねたカード）
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
    // ボスの頭の位置（画面座標）。HP ゲージをこの右上に出す
    Vec2  bossHead;
    bool  hasBossHead = false;
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

    // --- アイテムスロット ------------------------------------------------------
    //   回復とバフの 2 枚のカードを前後に重ねて置く。手前のカードが選択中。
    //   Q（または奥のカードをクリック）で前後が入れ替わり、
    //   E（または手前のカードをクリック）で手前のアイテムを使う。
    QuickSlot ItemSlot() const { return itemSlot_; }
    void SetItemSlot(QuickSlot slot) { itemSlot_ = slot; }
    // このフレームでアイテムを使う操作があったか
    bool ItemUseRequested() const { return itemUseRequested_; }
    // アイテムスロットをクリックしたフレームか（同じクリックで攻撃を出さないために使う）
    bool ItemClickConsumed() const { return itemClickConsumed_; }
    // 「HP は満タンです」などの短い通知
    void ShowItemMessage(const std::string& message);

    // --- レイアウト参照（検証用）---------------------------------------------
    // 手前 / 奥のカードの位置（入れ替えの動きが終わったあとの位置）
    const Rect& ItemFrontRect() const { return itemFront_; }
    const Rect& ItemBackRect() const { return itemBack_; }
    // 2 枚を合わせた範囲
    Rect ItemAreaRect() const;
    // ボス HP ゲージの枠（Draw で最後に描いた位置）
    const Rect& BossGaugeRect() const { return bossGauge_; }
    // ボスの HP ゲージを何本に分けるか（定義が無ければ 3）
    static int BossBarCount(const Boss& boss);
    // HP の割合 ratio のとき、上から index 本目（0 始まり）のゲージがどれだけ残っているか
    static float BossBarFill(float ratio, int index, int bars);

private:
    void DrawPlayerStatus(const Player& player, const PlayerData& data) const;
    void DrawBossStatus(const Boss& boss, const HudInfo& info) const;
    void DrawSkillBar(const Player& player, const PlayerData& data) const;
    void DrawFloorInfo(const HudInfo& info) const;
    void DrawCombo(const HudInfo& info) const;
    void DrawItemSlots(const Player& player, const PlayerData& data) const;
    void DrawItemCard(const Rect& rect, QuickSlot slot, bool front, const Player& player,
                      const PlayerData& data) const;

    Rect   skillRects_[kSkillSlotCount];
    float  hpDelay_ = 1.0f;
    float  bossHpDelay_ = 1.0f;
    int    clickedSkill_ = -1;
    float  time_ = 0.0f;

    // アイテムスロット
    Rect      itemFront_;
    Rect      itemBack_;
    QuickSlot itemSlot_ = QuickSlot::Recovery;
    float     itemSlide_ = 0.0f;   // 0 = 回復が手前 / 1 = バフが手前（入れ替えの動き）
    bool      itemUseRequested_ = false;
    bool      itemClickConsumed_ = false;
    std::string itemMessage_;
    float     itemMessageTimer_ = 0.0f;

    // ボス HP ゲージ（Draw の中で位置が決まる）
    mutable Rect bossGauge_;
};

} // namespace ui
} // namespace ecl
