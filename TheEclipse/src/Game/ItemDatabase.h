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

//------------------------------------------------------------------------------
// 初期装備
//------------------------------------------------------------------------------
// 初期装備の個体値（低めに固定して、ドロップで更新していく想定）
constexpr int kStarterIv = 20;
// アクセサリーの個体値（効果は固定なので表示用の固定値）
constexpr int kAccessoryIv = 50;
// 武器種ごとの初期武器のテンプレート ID
int StarterWeaponId(WeaponType type);

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

    // --- ショップ -------------------------------------------------------------
    // 0 より大きいとショップに並ぶ（購入価格）
    int         price = 0;

    // --- アクセサリー ---------------------------------------------------------
    AccessoryKind accessory = AccessoryKind::None;
    // 倍率で効くバフ（0.05 = +5%）。個体値では変わらない固定値
    StatRates   rates = StatRates();
    // 「攻撃力 +5%」のような効果の説明（ショップと装備メニューに出す）
    const char* effect = "";
    // 戦闘開始時に攻撃力が上がる（旅人の護符）
    float       openingAttackRate = 0.0f;
    float       openingDuration = 0.0f;

    bool        InShop() const { return price > 0; }
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

    // 初期装備一式（選んだ武器種の武器 1 本と防具。他の武器種は配らない）
    std::vector<EquipmentItem> CreateStarterSet(WeaponType weapon) const;

    // --- ショップ -------------------------------------------------------------
    // ショップに並ぶ品（price 順ではなく定義順）
    std::vector<const ItemTemplate*> ShopItems() const;
    // 指定スロットのショップの品（武器は WeaponRight でまとめて取れる）
    std::vector<const ItemTemplate*> ShopItemsForSlot(EquipSlot slot) const;
    // 防具（頭・体・盾・腕・手・足）のショップの品
    std::vector<const ItemTemplate*> ShopArmors() const;

private:
    ItemDatabase();

    std::vector<ItemTemplate> templates_;
};

} // namespace ecl
