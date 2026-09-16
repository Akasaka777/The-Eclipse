#include "Game/Equipment.h"

#include "Common/StringUtil.h"

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
    case EquipSlot::Weapon: return "武器";
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

int IssueItemUid()
{
    return g_nextUid++;
}

} // namespace ecl
