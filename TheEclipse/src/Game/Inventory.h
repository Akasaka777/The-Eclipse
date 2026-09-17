//==============================================================================
// Inventory.h : 所持品・装備中スロット・強化処理
//==============================================================================
#pragma once

#include "Game/Equipment.h"
#include "Graphics/CharacterArt.h"

#include <vector>

namespace ecl {

//------------------------------------------------------------------------------
// 強化に必要なコスト
//------------------------------------------------------------------------------
struct UpgradeCost
{
    int   col = 0;
    int   material = 0;
    float successRate = 1.0f;
    bool  possible = false;
};

class Inventory
{
public:
    void AddItem(const EquipmentItem& item);
    void AddItems(const std::vector<EquipmentItem>& items);
    // セーブデータの読み込み用
    void Clear();
    void SetCurrency(int col, int material);

    std::vector<EquipmentItem>&       Items() { return items_; }
    const std::vector<EquipmentItem>& Items() const { return items_; }
    EquipmentItem*       FindByUid(int uid);
    const EquipmentItem* FindByUid(int uid) const;

    // 指定スロットの所持品（レアリティ・強化値順にソート済み）
    std::vector<const EquipmentItem*> ItemsForSlot(EquipSlot slot) const;

    // --- 装備 ---------------------------------------------------------------
    bool Equip(int uid);
    void Unequip(EquipSlot slot);
    int  EquippedUid(EquipSlot slot) const;
    const EquipmentItem* Equipped(EquipSlot slot) const;
    bool IsEquipped(int uid) const;
    // 装備中の合計ステータス
    Stats EquippedStats() const;
    WeaponType CurrentWeaponType() const;
    // 装備中のスキンからキャラクターの見た目を組み立てる
    ActorArt BuildAppearance() const;

    // --- 強化 ---------------------------------------------------------------
    UpgradeCost CalcUpgradeCost(int uid) const;
    // 戻り値 : 強化に成功したか（失敗時も素材は消費）
    bool TryUpgrade(int uid, bool& outSuccess);

    // --- 売却 ---------------------------------------------------------------
    int  SellValue(int uid) const;
    bool Sell(int uid);

    // --- 通貨・素材 ----------------------------------------------------------
    int  Col() const { return col_; }
    void AddCol(int amount);
    int  Material() const { return material_; }
    void AddMaterial(int amount);

private:
    std::vector<EquipmentItem> items_;
    int equippedUid_[static_cast<int>(EquipSlot::Count)] = {};
    int col_ = 3000;
    int material_ = 20;
};

} // namespace ecl
