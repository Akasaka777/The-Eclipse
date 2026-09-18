#include "Game/Equipment.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/GameConfig.h"

namespace ecl {

namespace {
int g_nextUid = 1;
} // namespace

const char* RarityName(Rarity rarity)
{
    switch (rarity) {
    case Rarity::N:   return "N";
    case Rarity::R:   return "R";
    case Rarity::SR:  return "SR";
    case Rarity::SSR: return "SSR";
    case Rarity::UR:  return "UR";
    default: return "?";
    }
}

ColorRGB RarityColor(Rarity rarity)
{
    switch (rarity) {
    case Rarity::N:   return ColorRGB(172, 180, 192);
    case Rarity::R:   return ColorRGB(88, 164, 255);
    case Rarity::SR:  return ColorRGB(190, 116, 255);
    case Rarity::SSR: return ColorRGB(255, 196, 64);
    case Rarity::UR:  return ColorRGB(255, 96, 128);
    default: return ColorRGB(255, 255, 255);
    }
}

float RarityMultiplier(Rarity rarity)
{
    switch (rarity) {
    case Rarity::N:   return 1.00f;
    case Rarity::R:   return 1.35f;
    case Rarity::SR:  return 1.85f;
    case Rarity::SSR: return 2.55f;
    case Rarity::UR:  return 3.50f;
    default: return 1.0f;
    }
}

int RarityMaxUpgrade(Rarity rarity)
{
    switch (rarity) {
    case Rarity::N:   return 5;
    case Rarity::R:   return 7;
    case Rarity::SR:  return 9;
    case Rarity::SSR: return 11;
    case Rarity::UR:  return 13;
    default: return 5;
    }
}

const char* EquipSlotName(EquipSlot slot)
{
    switch (slot) {
    case EquipSlot::WeaponRight: return "武器（右手）";
    case EquipSlot::WeaponLeft:  return "武器（左手）";
    case EquipSlot::Head:   return "頭装備";
    case EquipSlot::Body:   return "体装備";
    case EquipSlot::Shield: return "盾";
    case EquipSlot::Arm:    return "腕装備";
    case EquipSlot::Hand:   return "手装備";
    case EquipSlot::Foot:   return "足装備";
    default: return "装備";
    }
}

Stats EquipmentItem::TotalStats() const
{
    // 強化1段階ごとに +9%
    return baseStats.Scaled(1.0f + 0.09f * static_cast<float>(upgradeLevel));
}

std::string EquipmentItem::DisplayName() const
{
    if (upgradeLevel <= 0) return name;
    return str::Format("%s +%d", name.c_str(), upgradeLevel);
}

bool IsWeaponSlot(EquipSlot slot)
{
    return slot == EquipSlot::WeaponRight || slot == EquipSlot::WeaponLeft;
}

EquipSlot OppositeWeaponSlot(EquipSlot slot)
{
    if (slot == EquipSlot::WeaponRight) return EquipSlot::WeaponLeft;
    if (slot == EquipSlot::WeaponLeft) return EquipSlot::WeaponRight;
    return slot;
}

float EquipmentItem::MaxDurability() const
{
    return 100.0f + 20.0f * static_cast<float>(static_cast<int>(rarity))
         + 4.0f * static_cast<float>(upgradeLevel);
}

float EquipmentItem::DurabilityRatio() const
{
    const float max = MaxDurability();
    if (max <= 0.0f) return 0.0f;
    return math::Clamp(durability / max, 0.0f, 1.0f);
}

int EquipmentItem::DurabilityDisplay() const
{
    if (durability <= 0.0f) return 0;
    // 1 未満でも「残っている」ことが分かるように切り上げる
    return math::MaxI(1, static_cast<int>(durability + 0.999f));
}

void EquipmentItem::Wear(float amount)
{
    if (amount <= 0.0f) return;
    durability = math::MaxF(0.0f, durability - amount);
}

void EquipmentItem::RestoreDurability()
{
    durability = MaxDurability();
}

ColorRGB EquipmentItem::DurabilityColor() const
{
    const float ratio = DurabilityRatio();
    if (ratio <= 0.0f) return palette::kDanger;
    if (ratio <= 0.25f) return palette::kDanger;
    if (ratio <= 0.55f) return palette::kAccentWarm;
    return palette::kHp;
}

int IssueItemUid()
{
    return g_nextUid++;
}

void ReserveItemUid(int nextUid)
{
    if (nextUid > g_nextUid) g_nextUid = nextUid;
}

} // namespace ecl
