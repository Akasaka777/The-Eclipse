#include "Game/Equipment.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/GameConfig.h"

namespace ecl {

namespace {
int g_nextUid = 1;
} // namespace

int ClampIv(int iv)
{
    return math::ClampInt(iv, kMinIv, kMaxIv);
}

float IvStatScale(int iv)
{
    // 0 → 0.60 倍、100 → 1.80 倍
    return 0.60f + 0.012f * static_cast<float>(ClampIv(iv));
}

int IvMaxUpgrade(int iv)
{
    // 0 → +4、100 → +14
    return 4 + ClampIv(iv) / 10;
}

float IvDurabilityBonus(int iv)
{
    // 0 → +0、100 → +80
    return 0.8f * static_cast<float>(ClampIv(iv));
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
    return 100.0f + IvDurabilityBonus(iv) + 4.0f * static_cast<float>(upgradeLevel);
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
