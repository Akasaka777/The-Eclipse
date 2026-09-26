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
//------------------------------------------------------------------------------
// 竜王の火山のボスドロップ（部位ごとに 1 つずつ）
//   ボスドロップ専用なので、通常のランダム抽選には出さない。
//------------------------------------------------------------------------------
constexpr int kDragonSwordId  = 160;  // 片手剣
constexpr int kDragonHelmId   = 204;  // 頭
constexpr int kDragonMailId   = 214;  // 体
constexpr int kDragonShieldId = 224;  // 盾
constexpr int kDragonArmId    = 232;  // 腕
constexpr int kDragonGloveId  = 242;  // 手
constexpr int kDragonBootsId  = 252;  // 足

// 竜王のセットか
bool IsDragonSetItem(int templateId);

struct ItemTemplate
{
    int         id = 0;
    const char* name = "";
    const char* flavor = "";
    EquipSlot   slot = EquipSlot::WeaponRight;
    WeaponType  weaponType = WeaponType::OneHandSword;
    int         tier = 1; // 1〜3（出現フロアの目安）
    Stats       base;
    // 特別枠（ユニークスキル用のセット武器など）。通常のランダム抽選には出ない
    bool        special = false;
    // 耐久力が 0 になっても消滅しない（代わりに性能が大きく落ちる）
    bool        indestructible = false;
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
