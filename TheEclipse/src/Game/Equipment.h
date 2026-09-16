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

    bool  IsValid() const { return uid != 0; }
    bool  IsWeapon() const { return slot == EquipSlot::Weapon; }
    // 強化値を反映した最終ステータス
    Stats TotalStats() const;
    // "ロングソード +5"
    std::string DisplayName() const;
    int   Power() const { return TotalStats().Power(); }
    int   MaxUpgrade() const { return RarityMaxUpgrade(rarity); }
};

// 所持品用の一意 ID を発行
int IssueItemUid();

} // namespace ecl
