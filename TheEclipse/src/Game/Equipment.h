//==============================================================================
// Equipment.h : 装備アイテム / レアリティ / 装備スロット
//==============================================================================
#pragma once

#include "Common/Types.h"
#include "Game/Stats.h"

#include <string>

namespace ecl {

//------------------------------------------------------------------------------
// レアリティ
//------------------------------------------------------------------------------
enum class Rarity
{
    N,
    R,
    SR,
    SSR,
    UR,
    Count
};

const char* RarityName(Rarity rarity);
ColorRGB    RarityColor(Rarity rarity);
// 基礎ステータスに掛かる倍率
float       RarityMultiplier(Rarity rarity);
// 強化上限
int         RarityMaxUpgrade(Rarity rarity);

//------------------------------------------------------------------------------
// 装備スロット
//------------------------------------------------------------------------------
enum class EquipSlot
{
    Weapon, // 武器
    Head,   // 頭装備
    Body,   // 体装備
    Shield, // 盾
    Arm,    // 腕
    Hand,   // 手
    Foot,   // 足
    Count
};

const char* EquipSlotName(EquipSlot slot);

//------------------------------------------------------------------------------
// 装備スキン（見た目）
//   素材が用意されていれば assets/skins/<spriteFolder> を優先して使用し、
//   無ければ色と形状の指定で代替描画に反映する。
//------------------------------------------------------------------------------
enum class SkinShape
{
    None,      // 見た目を変えない
    Light,     // 軽装（布・革）
    Heavy,     // 重装（金属）
    Mystic,    // 魔導・ローブ
    Eclipse    // 蝕（漆黒＋発光）
};

struct EquipSkin
{
    SkinShape   shape = SkinShape::None;
    ColorRGB    primary = ColorRGB(70, 90, 130);   // 主色（体装備など）
    ColorRGB    secondary = ColorRGB(220, 230, 245); // 差し色
    ColorRGB    glow = ColorRGB(64, 206, 255);     // 発光色
    const char* spriteFolder = "";                 // assets/skins/<folder>
    bool        hasCape = false;                   // マント
    bool        hasHelmet = false;                 // 兜（頭部形状の変化）
};

//------------------------------------------------------------------------------
// 装備品
//------------------------------------------------------------------------------
struct EquipmentItem
{
    int         uid = 0;          // 所持品を一意に識別する ID
    int         templateId = 0;   // 元になった定義 ID
    std::string name;
    std::string flavor;
    EquipSlot   slot = EquipSlot::Weapon;
    WeaponType  weaponType = WeaponType::OneHandSword;
    Rarity      rarity = Rarity::N;
    int         upgradeLevel = 0;
    Stats       baseStats;        // +0 時点の能力値
    EquipSkin   skin;             // 見た目
    float       durability = -1.0f; // 現在の耐久力（負値なら生成時に最大値で初期化）

    bool  IsValid() const { return uid != 0; }
    bool  IsWeapon() const { return slot == EquipSlot::Weapon; }
    // 強化値を反映した最終ステータス
    Stats TotalStats() const;
    // "ロングソード +5"
    std::string DisplayName() const;
    int   Power() const { return TotalStats().Power(); }
    int   MaxUpgrade() const { return RarityMaxUpgrade(rarity); }

    // --- 耐久力 ---------------------------------------------------------------
    // レアリティが高く、強化するほど長持ちする
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
