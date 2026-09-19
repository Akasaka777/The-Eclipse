//==============================================================================
// Equipment.h : 装備アイテム / 個体値 / 装備スロット
//==============================================================================
#pragma once

#include "Common/Types.h"
#include "Game/Stats.h"

#include <string>

namespace ecl {

//==============================================================================
// 個体値
//   装備 1 つ 1 つが内部に持つ 0〜100 の値。能力値・強化上限・耐久力は
//   すべてこの値から決まるが、数値そのものは画面には出さない（内部の概念）。
//   プレイヤーは能力値・戦力・強化上限・耐久力の違いとして感じ取る。
//------------------------------------------------------------------------------
//   数値を変えたい場合はこのあたりを調整してください。
//==============================================================================
constexpr int kMinIv = 0;
constexpr int kMaxIv = 100;

// 基礎ステータスに掛かる倍率（個体値 0 → 0.60 倍 / 100 → 1.80 倍）
float IvStatScale(int iv);
// 強化上限（個体値 0 → +4 / 100 → +14）
int   IvMaxUpgrade(int iv);
// 耐久力の最大値に加算される分（個体値 0 → +0 / 100 → +80）
float IvDurabilityBonus(int iv);
// 0〜100 に収める
int  ClampIv(int iv);

//------------------------------------------------------------------------------
// 装備スロット
//------------------------------------------------------------------------------
enum class EquipSlot
{
    WeaponRight, // 武器（右手）＝ 武器カテゴリの代表でもある
    WeaponLeft,  // 武器（左手）。二刀流の解放後のみ使用できる
    Head,        // 頭装備
    Body,        // 体装備
    Shield,      // 盾
    Arm,         // 腕
    Hand,        // 手
    Foot,        // 足
    Count
};

const char* EquipSlotName(EquipSlot slot);
// 武器を装備するスロットか
bool IsWeaponSlot(EquipSlot slot);
// 反対の手のスロット（武器スロット以外を渡した場合はそのまま返す）
EquipSlot OppositeWeaponSlot(EquipSlot slot);

//------------------------------------------------------------------------------
// 装備品
//------------------------------------------------------------------------------
struct EquipmentItem
{
    int         uid = 0;          // 所持品を一意に識別する ID
    int         templateId = 0;   // 元になった定義 ID
    std::string name;
    std::string flavor;
    EquipSlot   slot = EquipSlot::WeaponRight;
    WeaponType  weaponType = WeaponType::OneHandSword;
    int         iv = 0;           // 個体値（0〜100）
    int         upgradeLevel = 0;
    Stats       baseStats;        // +0 時点の能力値
    float       durability = -1.0f; // 現在の耐久力（負値なら生成時に最大値で初期化）

    bool  IsValid() const { return uid != 0; }
    // 武器カテゴリのアイテムは slot に WeaponRight を持つ（左右どちらにも装備できる）
    bool  IsWeapon() const { return slot == EquipSlot::WeaponRight; }
    // 強化値を反映した最終ステータス
    Stats TotalStats() const;
    // "ロングソード +5"
    std::string DisplayName() const;
    int   Power() const { return TotalStats().Power(); }
    int   MaxUpgrade() const { return IvMaxUpgrade(iv); }

    // --- 耐久力 ---------------------------------------------------------------
    // 個体値が高く、強化するほど長持ちする
    float MaxDurability() const;
    float DurabilityRatio() const;
    // 表示用（切り上げ）
    int   DurabilityDisplay() const;
    int   MaxDurabilityDisplay() const { return static_cast<int>(MaxDurability()); }
    bool  IsBroken() const { return durability <= 0.0f; }
    // 残り 25% 以下
    bool  IsWorn() const { return DurabilityRatio() <= 0.25f; }
    // 耐久力を減らす（0 未満にはならない）
    void  Wear(float amount);
    void  RestoreDurability();
    // 耐久力の状態に応じた表示色
    ColorRGB DurabilityColor() const;
};

// 所持品用の一意 ID を発行
int IssueItemUid();
// ロード後に ID の重複を避けるため、次に発行する ID を予約する
void ReserveItemUid(int nextUid);

} // namespace ecl
