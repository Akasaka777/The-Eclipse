//==============================================================================
// Inventory.h : 所持品・装備中スロット・強化処理
//==============================================================================
#pragma once

#include "Game/Equipment.h"
#include "Graphics/CharacterArt.h"

#include <string>
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

    // 指定スロットの所持品（個体値・戦力順にソート済み）
    std::vector<const EquipmentItem*> ItemsForSlot(EquipSlot slot) const;

    // --- 装備 ---------------------------------------------------------------
    // アイテム本来のスロットへ装備する（武器は右手）
    bool Equip(int uid);
    // スロットを指定して装備する（武器を左手に持たせる場合など）
    bool EquipTo(int uid, EquipSlot slot);
    void Unequip(EquipSlot slot);

    // --- 二刀流 -------------------------------------------------------------
    // ユニークスキル「二刀流」を習得しているか（PlayerData から設定する）
    void SetDualWieldEnabled(bool enabled);
    bool DualWieldEnabled() const { return dualWieldEnabled_; }
    // 実際に両手へ武器を持っているか
    bool IsDualWielding() const;
    // 指定スロットにこのアイテムを装備できるか
    bool CanEquipTo(int uid, EquipSlot slot) const;
    int  EquippedUid(EquipSlot slot) const;
    const EquipmentItem* Equipped(EquipSlot slot) const;
    bool IsEquipped(int uid) const;
    // 装備中の合計ステータス（左手の武器は控えめに加算する）
    Stats EquippedStats() const;
    // 指定アイテムを指定スロットに装備した場合のステータス（比較表示用）
    Stats PreviewStats(int uid, EquipSlot slot) const;
    WeaponType CurrentWeaponType() const;
    // 装備中のスキンからキャラクターの見た目を組み立てる
    ActorArt BuildAppearance() const;

    // --- 強化 ---------------------------------------------------------------
    UpgradeCost CalcUpgradeCost(int uid) const;
    // 戻り値 : 強化に成功したか（失敗時も素材は消費）
    bool TryUpgrade(int uid, bool& outSuccess);

    // --- 耐久力 -------------------------------------------------------------
    // 装備中のスロットの装備を摩耗させる
    void ApplyWear(EquipSlot slot, float amount);
    // 装備中の防具すべてを摩耗させる
    void ApplyArmorWear(float amount);
    // 耐久力が尽きた装備を消滅させる（消えた装備名を返す）
    std::vector<std::string> DestroyBrokenItems();
    // 装備中に耐久力が残りわずかなものがあるか
    bool HasWornEquipment() const;

    // 修理費用（0 なら修理不要）
    int  RepairCost(int uid) const;
    bool Repair(int uid);
    // 所持品すべての修理費用と一括修理
    int  RepairAllCost() const;
    int  RepairAll();

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
    bool dualWieldEnabled_ = false;

    // 装備 uid の配列からステータスを合計する内部処理
    Stats StatsFromSlots(const int equipped[static_cast<int>(EquipSlot::Count)]) const;
};

} // namespace ecl
