//==============================================================================
// BattleHud.h : 戦闘中の HUD
//   左上 : プレイヤーステータス（顔の丸いメダル＋レベル・名前・HP/MP）  右下 : スキルバー
//   ボスの HP バーは雑魚敵と同じく Boss::Draw で頭上に描く（HUD には出さない）
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
    bool  hiddenFloor = false;   // 隠しフロアにいる（「FLOOR 1 / 5」の代わりに「HIDDEN FLOOR」と出す）
};

//------------------------------------------------------------------------------
// 左上のプレイヤーステータスの配置（画面座標）
//   顔の丸いメダルの右に、右へ薄れていく板を敷いて名前・HP・MP を並べる
//------------------------------------------------------------------------------
struct PlayerStatusLayout
{
    Rect  bounds;              // メダル・レベルの札まで含めた全体
    Rect  plate;               // 背景の板（右端は斜めに切る：上辺の右端が plate.right）
    float plateSlant = 0.0f;   // 板の右端を下辺でどれだけ左へ寄せるか
    Vec2  portraitCenter;      // 顔のメダルの中心
    float portraitRadius = 0.0f;
    Rect  levelBadge;          // メダルの下に重ねるレベルの札
    Vec2  namePos;             // 名前の左上
    Rect  hpBar;
    Rect  mpBar;
    float numbersRight = 0.0f; // HP / MP の数値の右端
    Rect  guardBadge;          // ガード中だけ出す札（名前のすぐ右。名前が長くてもこの位置より右へは出さない）
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
    // 左上のプレイヤーステータスの配置
    static PlayerStatusLayout StatusLayout();

private:
    void DrawPlayerStatus(const Player& player, const PlayerData& data) const;
    void DrawPortrait(const PlayerStatusLayout& layout, const Player& player, const PlayerData& data) const;
    void DrawSkillBar(const Player& player, const PlayerData& data) const;
    void DrawFloorInfo(const HudInfo& info) const;
    void DrawCombo(const HudInfo& info) const;
    void DrawItemSlots(const Player& player, const PlayerData& data) const;
    void DrawItemCard(const Rect& rect, QuickSlot slot, bool front, const Player& player,
                      const PlayerData& data) const;

    Rect   skillRects_[kSkillSlotCount];
    float  hpDelay_ = 1.0f;

    // プレイヤーステータスの演出
    bool   statusPrimed_ = false;   // 最初の Update で今の HP を覚えるまで false（開始時の全回復を光らせない）
    float  prevHp_ = 0.0f;
    float  prevHpRatio_ = 1.0f;
    float  hitFlash_ = 0.0f;        // 被弾したときの光（1 → 0）
    float  healFlash_ = 0.0f;       // 回復したときの光（1 → 0）
    float  healFrom_ = -1.0f;       // 回復が始まった割合（ここから今の HP までを光らせる）
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
};

} // namespace ui
} // namespace ecl
