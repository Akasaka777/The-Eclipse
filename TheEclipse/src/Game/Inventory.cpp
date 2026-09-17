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
        if (item.slot == slot) result.push_back(&item);
    }

    std::sort(result.begin(), result.end(), [](const EquipmentItem* a, const EquipmentItem* b) {
        if (a->rarity != b->rarity) return static_cast<int>(a->rarity) > static_cast<int>(b->rarity);
        if (a->Power() != b->Power()) return a->Power() > b->Power();
        return a->uid < b->uid;
    });
    return result;
}

bool Inventory::Equip(int uid)
{
    const EquipmentItem* item = FindByUid(uid);
    if (!item) return false;
    equippedUid_[static_cast<int>(item->slot)] = uid;
    return true;
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

Stats Inventory::EquippedStats() const
{
    Stats total;
    for (int i = 0; i < static_cast<int>(EquipSlot::Count); ++i) {
        const EquipmentItem* item = Equipped(static_cast<EquipSlot>(i));
        if (item) total += item->TotalStats();
    }
    return total;
}

WeaponType Inventory::CurrentWeaponType() const
{
    const EquipmentItem* weapon = Equipped(EquipSlot::Weapon);
    if (!weapon) return WeaponType::OneHandSword;
    return weapon->weaponType;
}

ActorArt Inventory::BuildAppearance() const
{
    ActorArt art;
    art.style = ArtStyle::Humanoid;
    art.weapon = CurrentWeaponType();

    // 既定（素の状態）
    art.main = ColorRGB(48, 58, 84);
    art.accent = ColorRGB(226, 234, 248);
    art.trim = ColorRGB(64, 206, 255);
    art.helmetColor = ColorRGB(226, 234, 248);
    art.shieldColor = ColorRGB(120, 130, 150);
    art.weaponColor = ColorRGB(226, 234, 248);

    const EquipmentItem* weapon = Equipped(EquipSlot::Weapon);
    const EquipmentItem* head = Equipped(EquipSlot::Head);
    const EquipmentItem* body = Equipped(EquipSlot::Body);
    const EquipmentItem* shield = Equipped(EquipSlot::Shield);

    art.hasWeapon = (weapon != nullptr);
    art.hasShield = (shield != nullptr);

    // 体装備が全体の色を決める
    if (body && body->skin.shape != SkinShape::None) {
        art.main = body->skin.primary;
        art.accent = body->skin.secondary;
        art.trim = body->skin.glow;
        art.hasCape = body->skin.hasCape;
        if (body->skin.shape == SkinShape::Eclipse) art.glowing = true;
    }
    // 頭装備
    if (head && head->skin.shape != SkinShape::None) {
        art.helmetColor = head->skin.primary;
        art.hasHelmet = head->skin.hasHelmet;
        if (!body) art.trim = head->skin.glow;
    }
    // 盾
    if (shield && shield->skin.shape != SkinShape::None) {
        art.shieldColor = shield->skin.primary;
    }
    // 武器（刀身の色と発光）
    if (weapon && weapon->skin.shape != SkinShape::None) {
        art.weaponColor = weapon->skin.secondary;
        if (weapon->skin.shape == SkinShape::Eclipse) {
            art.glowing = true;
            art.trim = weapon->skin.glow;
        }
    }
    return art;
}

UpgradeCost Inventory::CalcUpgradeCost(int uid) const
{
    UpgradeCost cost;
    const EquipmentItem* item = FindByUid(uid);
    if (!item) return cost;
    if (item->upgradeLevel >= item->MaxUpgrade()) return cost;

    const int level = item->upgradeLevel;
    const float rarityFactor = 1.0f + static_cast<float>(item->rarity) * 0.55f;

    cost.col = static_cast<int>((160.0f + 130.0f * static_cast<float>(level) * (1.0f + level * 0.25f)) * rarityFactor);
    cost.material = 1 + level / 2 + static_cast<int>(item->rarity) / 2;

    // +3 までは確実に成功、以降は段階的に低下
    if (level < 3) {
        cost.successRate = 1.0f;
    } else {
        cost.successRate = math::Clamp(1.0f - 0.10f * static_cast<float>(level - 2), 0.35f, 1.0f);
    }
    cost.possible = true;
    return cost;
}

bool Inventory::TryUpgrade(int uid, bool& outSuccess)
{
    outSuccess = false;

    EquipmentItem* item = FindByUid(uid);
    if (!item) return false;

    const UpgradeCost cost = CalcUpgradeCost(uid);
    if (!cost.possible) return false;
    if (col_ < cost.col || material_ < cost.material) return false;

    col_ -= cost.col;
    material_ -= cost.material;

    if (math::RandChance(cost.successRate)) {
        item->upgradeLevel += 1;
        outSuccess = true;
    }
    return true;
}

int Inventory::SellValue(int uid) const
{
    const EquipmentItem* item = FindByUid(uid);
    if (!item) return 0;
    const float rarityFactor = 1.0f + static_cast<float>(item->rarity) * 1.2f;
    return static_cast<int>((60.0f + item->Power() * 0.9f) * rarityFactor);
}

bool Inventory::Sell(int uid)
{
    if (IsEquipped(uid)) return false;

    for (size_t i = 0; i < items_.size(); ++i) {
        if (items_[i].uid != uid) continue;
        col_ += SellValue(uid);
        material_ += 1 + static_cast<int>(items_[i].rarity);
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
