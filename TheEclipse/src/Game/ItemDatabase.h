//==============================================================================
// ItemDatabase.h : 装備定義のマスターデータ
//==============================================================================
#pragma once

#include "Game/Equipment.h"

#include <vector>

namespace ecl {

//------------------------------------------------------------------------------
// 装備の定義（レアリティ適用前の素の値）
//------------------------------------------------------------------------------
struct ItemTemplate
{
    int         id = 0;
    const char* name = "";
    const char* flavor = "";
    EquipSlot   slot = EquipSlot::Weapon;
    WeaponType  weaponType = WeaponType::OneHandSword;
    int         tier = 1; // 1〜3（出現フロアの目安）
    Stats       base;
    EquipSkin   skin;
};

class ItemDatabase
{
public:
    static const ItemDatabase& Instance();

    const std::vector<ItemTemplate>& Templates() const { return templates_; }
    const ItemTemplate* Find(int templateId) const;

    // 定義とレアリティから実アイテムを生成（±6% の個体差あり）
    EquipmentItem Create(int templateId, Rarity rarity) const;
    // スロット指定のランダム生成
    EquipmentItem CreateRandom(EquipSlot slot, Rarity rarity, int maxTier = 3) const;
    // 完全ランダム
    EquipmentItem CreateRandomAny(Rarity rarity, int maxTier = 3) const;

    // 初期装備一式
    std::vector<EquipmentItem> CreateStarterSet() const;

private:
    ItemDatabase();

    std::vector<ItemTemplate> templates_;
};

} // namespace ecl
