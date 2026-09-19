//==============================================================================
// ItemDatabase.h : 装備定義のマスターデータ
//==============================================================================
#pragma once

#include "Game/Equipment.h"

#include <vector>

namespace ecl {

//------------------------------------------------------------------------------
// 装備の定義（個体値を掛ける前の素の値）
//------------------------------------------------------------------------------
struct ItemTemplate
{
    int         id = 0;
    const char* name = "";
    const char* flavor = "";
    EquipSlot   slot = EquipSlot::WeaponRight;
    WeaponType  weaponType = WeaponType::OneHandSword;
    int         tier = 1; // 1〜3（出現フロアの目安）
    Stats       base;
};

class ItemDatabase
{
public:
    static const ItemDatabase& Instance();

    const std::vector<ItemTemplate>& Templates() const { return templates_; }
    const ItemTemplate* Find(int templateId) const;

    // 定義と個体値（0〜100）から実アイテムを生成する
    EquipmentItem Create(int templateId, int iv) const;
    // スロット指定のランダム生成
    EquipmentItem CreateRandom(EquipSlot slot, int iv, int maxTier = 3) const;
    // 完全ランダム
    EquipmentItem CreateRandomAny(int iv, int maxTier = 3) const;

    // 初期装備一式
    std::vector<EquipmentItem> CreateStarterSet() const;

private:
    ItemDatabase();

    std::vector<ItemTemplate> templates_;
};

} // namespace ecl
