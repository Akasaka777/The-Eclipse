#include "Game/PlayerData.h"

#include "Common/MathUtil.h"
#include "Game/ItemDatabase.h"

#include <algorithm>
#include <cmath>

namespace ecl {

namespace {
// レベルごとの必要経験値
int ExpTableFor(int level)
{
    return static_cast<int>(90.0f + 52.0f * std::pow(static_cast<float>(level), 1.42f));
}
} // namespace

PlayerData::PlayerData() = default;

void PlayerData::SetupNewGame()
{
    const ItemDatabase& database = ItemDatabase::Instance();
    inventory_.AddItems(database.CreateStarterSet());

    // 先頭の武器・防具を初期装備にする
    for (const EquipmentItem& item : inventory_.Items()) {
        if (inventory_.EquippedUid(item.slot) == 0) {
            inventory_.Equip(item.uid);
        }
    }
    RefreshSkillLoadout();
}

int PlayerData::ExpToNext() const
{
    return ExpTableFor(level_);
}

int PlayerData::AddExp(int amount)
{
    if (amount <= 0) return 0;

    exp_ += amount;
    int gained = 0;
    while (exp_ >= ExpToNext() && level_ < 99) {
        exp_ -= ExpToNext();
        ++level_;
        ++gained;
    }
    if (level_ >= 99) exp_ = 0;
    return gained;
}

Stats PlayerData::BaseStats() const
{
    const float lv = static_cast<float>(level_ - 1);
    Stats stats;
    stats.maxHp = 420.0f + 46.0f * lv;
    stats.maxMp = 110.0f + 9.0f * lv;
    stats.attack = 26.0f + 5.4f * lv;
    stats.defense = 14.0f + 3.2f * lv;
    stats.critRate = 0.05f;
    stats.critDamage = 0.30f;
    stats.mpRegen = 4.5f + 0.15f * lv;
    stats.moveSpeed = 460.0f;
    stats.attackSpeed = 0.0f;
    return stats;
}

Stats PlayerData::TotalStats() const
{
    return BaseStats() + inventory_.EquippedStats();
}

void PlayerData::RefreshSkillLoadout()
{
    const SkillDatabase& database = SkillDatabase::Instance();
    const WeaponType weapon = CurrentWeaponType();

    // 現在の武器種に合わないスキルは外す
    for (int i = 0; i < 4; ++i) {
        const SwordSkill* skill = database.Find(skillLoadout_[i]);
        if (!skill || skill->weapon != weapon) skillLoadout_[i] = 0;
    }

    // 空きスロットを既定スキルで埋める
    const std::vector<int> defaults = database.DefaultLoadout(weapon);
    for (int i = 0; i < 4; ++i) {
        if (skillLoadout_[i] != 0) continue;
        for (int candidate : defaults) {
            if (candidate == 0) continue;
            bool used = false;
            for (int j = 0; j < 4; ++j) {
                if (skillLoadout_[j] == candidate) { used = true; break; }
            }
            if (!used) {
                skillLoadout_[i] = candidate;
                break;
            }
        }
    }
}

const SwordSkill* PlayerData::SkillAt(int slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= 4) return nullptr;
    return SkillDatabase::Instance().Find(skillLoadout_[slotIndex]);
}

void PlayerData::SetSkillAt(int slotIndex, int skillId)
{
    if (slotIndex < 0 || slotIndex >= 4) return;

    // 同じスキルが他スロットにあれば入れ替える
    for (int i = 0; i < 4; ++i) {
        if (i != slotIndex && skillLoadout_[i] == skillId) {
            skillLoadout_[i] = skillLoadout_[slotIndex];
        }
    }
    skillLoadout_[slotIndex] = skillId;
}

bool PlayerData::IsQuestCleared(int questId) const
{
    return std::find(clearedQuests_.begin(), clearedQuests_.end(), questId) != clearedQuests_.end();
}

void PlayerData::MarkQuestCleared(int questId)
{
    if (!IsQuestCleared(questId)) clearedQuests_.push_back(questId);
}

} // namespace ecl
