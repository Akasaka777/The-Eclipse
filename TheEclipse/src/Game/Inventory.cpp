#include "Game/Inventory.h"

#include "Common/MathUtil.h"

#include <algorithm>

namespace ecl {

void Inventory::AddItem(const EquipmentItem& item)
{
    if (!item.IsValid()) return;
    items_.push_back(item);
}

void Inventory::AddItems(const std::vector<EquipmentItem>& items)
{
    for (const EquipmentItem& item : items) AddItem(item);
}

void Inventory::Clear()
{
    items_.clear();
    for (int i = 0; i < static_cast<int>(EquipSlot::Count); ++i) equippedUid_[i] = 0;
}

void Inventory::SetCurrency(int col, int material)
{
    col_ = math::MaxI(0, col);
    material_ = math::MaxI(0, material);
}

EquipmentItem* Inventory::FindByUid(int uid)
{
    for (EquipmentItem& item : items_) {
        if (item.uid == uid) return &item;
    }
    return nullptr;
}

const EquipmentItem* Inventory::FindByUid(int uid) const
{
    for (const EquipmentItem& item : items_) {
        if (item.uid == uid) return &item;
    }
    return nullptr;
}

std::vector<const EquipmentItem*> Inventory::ItemsForSlot(EquipSlot slot) const
{
    std::vector<const EquipmentItem*> result;
    for (const EquipmentItem& item : items_) {
        // 武器は左右どちらのスロットにも候補として出す
        const bool match = IsWeaponSlot(slot) ? item.IsWeapon() : (item.slot == slot);
        if (match) result.push_back(&item);
    }

    std::sort(result.begin(), result.end(), [](const EquipmentItem* a, const EquipmentItem* b) {
        if (a->iv != b->iv) return a->iv > b->iv;
        if (a->Power() != b->Power()) return a->Power() > b->Power();
        return a->uid < b->uid;
    });
    return result;
}

bool Inventory::Equip(int uid)
{
    const EquipmentItem* item = FindByUid(uid);
    if (!item) return false;
    return EquipTo(uid, item->slot);
}

bool Inventory::CanEquipTo(int uid, EquipSlot slot) const
{
    const EquipmentItem* item = FindByUid(uid);
    if (!item) return false;

    if (IsWeaponSlot(slot)) {
        // 武器は左右どちらのスロットにも装備できる。
        // 二刀流を習得していない場合は、装備した時点で反対の手が空く（EquipTo 参照）。
        return item->IsWeapon();
    }

    if (item->IsWeapon()) return false;

    // 両手に武器を持っている間は盾を装備できない
    if (slot == EquipSlot::Shield && IsDualWielding()) return false;

    return item->slot == slot;
}

bool Inventory::EquipTo(int uid, EquipSlot slot)
{
    if (!CanEquipTo(uid, slot)) return false;

    const EquipmentItem* item = FindByUid(uid);
    const int slotIndex = static_cast<int>(slot);

    if (IsWeaponSlot(slot)) {
        const EquipSlot other = OppositeWeaponSlot(slot);
        const EquipmentItem* otherItem = Equipped(other);

        // 同じ装備を両手に持つことはできない
        if (otherItem && otherItem->uid == uid) {
            equippedUid_[static_cast<int>(other)] = 0;
        }

        // 二刀流でない、または片手剣以外なら片手持ちにする
        const bool canPairUp = dualWieldEnabled_
                            && item->weaponType == WeaponType::OneHandSword
                            && (!otherItem || otherItem->weaponType == WeaponType::OneHandSword);
        if (!canPairUp) {
            equippedUid_[static_cast<int>(other)] = 0;
        }
    }

    equippedUid_[slotIndex] = uid;

    // 両手に武器を持ったら盾は外す
    if (IsDualWielding()) {
        equippedUid_[static_cast<int>(EquipSlot::Shield)] = 0;
    }
    return true;
}

void Inventory::SetDualWieldEnabled(bool enabled)
{
    dualWieldEnabled_ = enabled;
    // 解除された時に両手持ちだった場合のみ、左手の武器を外して片手持ちに戻す
    if (!dualWieldEnabled_ && IsDualWielding()) {
        equippedUid_[static_cast<int>(EquipSlot::WeaponLeft)] = 0;
    }
}

bool Inventory::IsDualWielding() const
{
    return EquippedUid(EquipSlot::WeaponRight) != 0 && EquippedUid(EquipSlot::WeaponLeft) != 0;
}

void Inventory::Unequip(EquipSlot slot)
{
    equippedUid_[static_cast<int>(slot)] = 0;
}

int Inventory::EquippedUid(EquipSlot slot) const
{
    const int index = static_cast<int>(slot);
    if (index < 0 || index >= static_cast<int>(EquipSlot::Count)) return 0;
    return equippedUid_[index];
}

const EquipmentItem* Inventory::Equipped(EquipSlot slot) const
{
    const int uid = EquippedUid(slot);
    if (uid == 0) return nullptr;
    return FindByUid(uid);
}

bool Inventory::IsEquipped(int uid) const
{
    if (uid == 0) return false;
    for (int i = 0; i < static_cast<int>(EquipSlot::Count); ++i) {
        if (equippedUid_[i] == uid) return true;
    }
    return false;
}

// 二刀流の左手武器が能力値に寄与する割合
//   1.0 にすると単純に 2 本分の火力になるため、控えめにしている
constexpr float kOffHandStatRate = 0.60f;

Stats Inventory::StatsFromSlots(const int equipped[static_cast<int>(EquipSlot::Count)]) const
{
    Stats total;
    for (int i = 0; i < static_cast<int>(EquipSlot::Count); ++i) {
        if (equipped[i] == 0) continue;
        const EquipmentItem* item = FindByUid(equipped[i]);
        if (!item) continue;

        // 左手の武器は「両手に持っている時だけ」控えめに加算する
        const bool offHand = (static_cast<EquipSlot>(i) == EquipSlot::WeaponLeft)
                          && equipped[static_cast<int>(EquipSlot::WeaponRight)] != 0;
        total += offHand ? item->TotalStats().Scaled(kOffHandStatRate) : item->TotalStats();
    }
    return total;
}

Stats Inventory::EquippedStats() const
{
    return StatsFromSlots(equippedUid_);
}

Stats Inventory::PreviewStats(int uid, EquipSlot slot) const
{
    int preview[static_cast<int>(EquipSlot::Count)];
    for (int i = 0; i < static_cast<int>(EquipSlot::Count); ++i) preview[i] = equippedUid_[i];

    const EquipmentItem* item = FindByUid(uid);
    if (!item || !CanEquipTo(uid, slot)) return StatsFromSlots(preview);

    if (IsWeaponSlot(slot)) {
        const EquipSlot other = OppositeWeaponSlot(slot);
        const EquipmentItem* otherItem = Equipped(other);
        const bool canPairUp = dualWieldEnabled_
                            && item->weaponType == WeaponType::OneHandSword
                            && (!otherItem || otherItem->weaponType == WeaponType::OneHandSword);
        if (!canPairUp || (otherItem && otherItem->uid == uid)) {
            preview[static_cast<int>(other)] = 0;
        }
    }
    preview[static_cast<int>(slot)] = uid;

    // 両手持ちになるなら盾は外れる
    if (preview[static_cast<int>(EquipSlot::WeaponRight)] != 0
        && preview[static_cast<int>(EquipSlot::WeaponLeft)] != 0) {
        preview[static_cast<int>(EquipSlot::Shield)] = 0;
    }
    return StatsFromSlots(preview);
}

WeaponType Inventory::CurrentWeaponType() const
{
    const EquipmentItem* weapon = Equipped(EquipSlot::WeaponRight);
    if (!weapon) weapon = Equipped(EquipSlot::WeaponLeft);
    if (!weapon) return WeaponType::OneHandSword;
    return weapon->weaponType;
}

ActorArt Inventory::BuildAppearance() const
{
    ActorArt art;
    art.style = ArtStyle::Humanoid;
    art.weapon = CurrentWeaponType();
    art.accent = ColorRGB(226, 234, 248);
    art.trim = ColorRGB(64, 206, 255);

    // 武器種で体の色みを少し変える
    switch (art.weapon) {
    case WeaponType::Dagger:      art.main = ColorRGB(58, 46, 76); break;
    case WeaponType::Rapier:      art.main = ColorRGB(44, 62, 88); break;
    case WeaponType::Spear:       art.main = ColorRGB(52, 66, 62); break;
    case WeaponType::OneHandMace: art.main = ColorRGB(70, 58, 48); break;
    default:                      art.main = ColorRGB(48, 58, 84); break;
    }

    art.hasWeapon = Equipped(EquipSlot::WeaponRight) != nullptr
                 || Equipped(EquipSlot::WeaponLeft) != nullptr;
    art.hasOffHandWeapon = IsDualWielding();
    art.hasShield = Equipped(EquipSlot::Shield) != nullptr;
    return art;
}

UpgradeCost Inventory::CalcUpgradeCost(int uid, int crystals) const
{
    UpgradeCost cost;
    const EquipmentItem* item = FindByUid(uid);
    if (!item) return cost;
    if (item->upgradeLevel >= item->MaxUpgrade()) return cost;

    const int use = math::ClampInt(crystals, kMinUpgradeCrystals, kMaxUpgradeCrystals);
    const int level = item->upgradeLevel;
    // 個体値が高い装備ほど強化費用も上がる（0 → 1.00 倍 / 100 → 3.20 倍）
    const float ivFactor = 1.0f + 0.022f * static_cast<float>(item->iv);
    // 結晶を多く使うほど col も増える（1 個 → 1.0 倍 / 10 個 → 4.6 倍）
    const float crystalFactor = 0.6f + 0.4f * static_cast<float>(use);

    const float base = 160.0f + 130.0f * static_cast<float>(level) * (1.0f + level * 0.25f);
    cost.col = static_cast<int>(base * ivFactor * crystalFactor);
    cost.material = use;

    // --- 伸び率の幅（結晶が多いほど下限も上限も大きくなる）----------------------
    //   1 個  :  1% 〜 10%
    //   10 個 : 5.5% 〜 50.5%
    const float steps = static_cast<float>(use - 1);
    cost.minGrowth = 0.010f + 0.005f * steps;
    cost.maxGrowth = 0.100f + 0.045f * steps;

    // +3 までは確実に成功、以降は段階的に低下。結晶を多く使うほど成功しやすい。
    if (level < 3) {
        cost.successRate = 1.0f;
    } else {
        const float bonus = 0.02f * steps;
        cost.successRate =
            math::Clamp(1.0f - 0.10f * static_cast<float>(level - 2) + bonus, 0.35f, 1.0f);
    }
    cost.possible = true;
    return cost;
}

bool Inventory::TryUpgrade(int uid, int crystals, UpgradeResult& outResult)
{
    outResult = UpgradeResult();

    EquipmentItem* item = FindByUid(uid);
    if (!item) return false;

    const UpgradeCost cost = CalcUpgradeCost(uid, crystals);
    if (!cost.possible) return false;
    if (col_ < cost.col || material_ < cost.material) return false;

    outResult.before = item->TotalStats();

    col_ -= cost.col;
    material_ -= cost.material;

    if (math::RandChance(cost.successRate)) {
        // 強化で最大耐久力が伸びるため、消耗の度合い（比率）を保つ
        const float ratio = item->DurabilityRatio();
        item->upgradeLevel += 1;
        item->durability = item->MaxDurability() * ratio;

        // 攻撃力・クリティカル率・クリティカル倍率は 1 つずつ別に抽選する。
        // 同じ装備でも伸び方が毎回変わり、上振れ・下振れが生まれる。
        item->growthAttack += math::RandFloat(cost.minGrowth, cost.maxGrowth);
        item->growthCritRate += math::RandFloat(cost.minGrowth, cost.maxGrowth);
        item->growthCritDamage += math::RandFloat(cost.minGrowth, cost.maxGrowth);

        outResult.success = true;
    }
    outResult.after = item->TotalStats();
    return true;
}

//------------------------------------------------------------------------------
// 耐久力
//------------------------------------------------------------------------------
void Inventory::ApplyWear(EquipSlot slot, float amount)
{
    const int uid = EquippedUid(slot);
    if (uid == 0) return;

    EquipmentItem* item = FindByUid(uid);
    if (item) item->Wear(amount);
}

void Inventory::ApplyArmorWear(float amount)
{
    for (int i = 0; i < static_cast<int>(EquipSlot::Count); ++i) {
        const EquipSlot slot = static_cast<EquipSlot>(i);
        if (IsWeaponSlot(slot)) continue;   // 武器は攻撃時に摩耗する
        ApplyWear(slot, amount);
    }
}

std::vector<std::string> Inventory::DestroyBrokenItems()
{
    std::vector<std::string> destroyed;

    for (size_t i = 0; i < items_.size();) {
        // 壊れない装備は耐久力 0 でも残る（性能が落ちるだけ）
        if (!items_[i].IsBroken() || items_[i].indestructible) {
            ++i;
            continue;
        }

        const EquipmentItem broken = items_[i];
        // 装備中なら外してから消す
        for (int slot = 0; slot < static_cast<int>(EquipSlot::Count); ++slot) {
            if (equippedUid_[slot] == broken.uid) equippedUid_[slot] = 0;
        }
        items_.erase(items_.begin() + static_cast<long>(i));
        destroyed.push_back(broken.DisplayName());
    }
    return destroyed;
}

bool Inventory::HasWornEquipment() const
{
    for (int i = 0; i < static_cast<int>(EquipSlot::Count); ++i) {
        const EquipmentItem* item = Equipped(static_cast<EquipSlot>(i));
        if (item && item->IsWorn()) return true;
    }
    return false;
}

int Inventory::RepairCost(int uid) const
{
    const EquipmentItem* item = FindByUid(uid);
    if (!item) return 0;

    const float missing = item->MaxDurability() - item->durability;
    if (missing <= 0.0f) return 0;

    // 個体値が高いほど修理費も高い
    // 修理費用は耐久力 1 点あたり（個体値 0 → 8 col / 100 → 24 col）
    const float unit = 8.0f + 0.16f * static_cast<float>(item->iv);
    return math::MaxI(1, static_cast<int>(missing * unit));
}

bool Inventory::Repair(int uid)
{
    EquipmentItem* item = FindByUid(uid);
    if (!item) return false;

    const int cost = RepairCost(uid);
    if (cost <= 0) return false;
    if (col_ < cost) return false;

    col_ -= cost;
    item->RestoreDurability();
    return true;
}

int Inventory::RepairAllCost() const
{
    int total = 0;
    for (const EquipmentItem& item : items_) {
        total += RepairCost(item.uid);
    }
    return total;
}

int Inventory::RepairAll()
{
    int repaired = 0;
    for (EquipmentItem& item : items_) {
        const int cost = RepairCost(item.uid);
        if (cost <= 0) continue;
        if (col_ < cost) continue;

        col_ -= cost;
        item.RestoreDurability();
        ++repaired;
    }
    return repaired;
}

int Inventory::SellValue(int uid) const
{
    const EquipmentItem* item = FindByUid(uid);
    if (!item) return 0;
    const float ivFactor = 1.0f + 0.048f * static_cast<float>(item->iv);
    // 傷んだ装備は買い叩かれる
    const float condition = 0.4f + 0.6f * item->DurabilityRatio();
    return math::MaxI(1, static_cast<int>((60.0f + item->Power() * 0.9f) * ivFactor * condition));
}

bool Inventory::Sell(int uid)
{
    if (IsEquipped(uid)) return false;

    for (size_t i = 0; i < items_.size(); ++i) {
        if (items_[i].uid != uid) continue;
        col_ += SellValue(uid);
        material_ += 1 + items_[i].iv / 25;
        items_.erase(items_.begin() + static_cast<long>(i));
        return true;
    }
    return false;
}

void Inventory::AddCol(int amount)
{
    col_ = math::MaxI(0, col_ + amount);
}

void Inventory::AddMaterial(int amount)
{
    material_ = math::MaxI(0, material_ + amount);
}

} // namespace ecl
