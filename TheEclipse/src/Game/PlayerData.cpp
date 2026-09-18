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
    // 各武器の起点スキルは最初から使える
    for (int id : SkillDatabase::Instance().StarterSkillIds()) {
        if (!IsSkillUnlocked(id)) unlockedSkills_.push_back(id);
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

    // レベルアップ 1 回につきスキルポイント 1
    if (gained > 0) AddSkillPoints(gained);
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
    const int limit = SkillSlotLimit();

    // 使えなくなったスキルを外す
    for (int i = 0; i < kSkillSlotCount; ++i) {
        if (i >= limit) {
            // ユニークスキル習得で減った枠は空にする
            skillLoadout_[i] = 0;
            continue;
        }
        const SwordSkill* skill = database.Find(skillLoadout_[i]);
        if (!skill || !IsSkillUnlocked(skill->id) || skill->weapon != weapon) {
            skillLoadout_[i] = 0;
            continue;
        }
        // 専用スキルは対応するユニークスキルを習得している場合のみ
        if (skill->IsUnique() && skill->requiredUnique != uniqueSkill_) {
            skillLoadout_[i] = 0;
        }
    }

    // 空きスロットを解放済みスキルで埋める
    for (int i = 0; i < limit; ++i) {
        if (skillLoadout_[i] != 0) continue;
        for (const SwordSkill* candidate : database.ForWeapon(weapon)) {
            if (!IsSkillUnlocked(candidate->id)) continue;
            if (IsSkillEquipped(candidate->id)) continue;
            skillLoadout_[i] = candidate->id;
            break;
        }
    }
}

void PlayerData::AddSkillPoints(int amount)
{
    skillPoints_ = math::MaxI(0, skillPoints_ + amount);
}

bool PlayerData::IsSkillUnlocked(int skillId) const
{
    if (skillId == 0) return false;
    return std::find(unlockedSkills_.begin(), unlockedSkills_.end(), skillId) != unlockedSkills_.end();
}

bool PlayerData::IsSkillReachable(int skillId) const
{
    const SwordSkill* skill = SkillDatabase::Instance().Find(skillId);
    if (!skill) return false;
    if (skill->requiredSkillId == 0) return true;
    return IsSkillUnlocked(skill->requiredSkillId);
}

bool PlayerData::CanUnlockSkill(int skillId) const
{
    const SwordSkill* skill = SkillDatabase::Instance().Find(skillId);
    if (!skill) return false;
    if (IsSkillUnlocked(skillId)) return false;
    if (!IsSkillReachable(skillId)) return false;
    // 専用スキルは対応するユニークスキルを習得している必要がある
    if (skill->IsUnique() && skill->requiredUnique != uniqueSkill_) return false;
    return skillPoints_ >= skill->unlockCost;
}

bool PlayerData::UnlockSkill(int skillId)
{
    if (!CanUnlockSkill(skillId)) return false;

    const SwordSkill* skill = SkillDatabase::Instance().Find(skillId);
    skillPoints_ -= skill->unlockCost;
    unlockedSkills_.push_back(skillId);

    // 同じ武器種で空きスロットがあれば自動で装備する
    if (skill->weapon == CurrentWeaponType()) {
        for (int i = 0; i < SkillSlotLimit(); ++i) {
            if (skillLoadout_[i] == 0) {
                skillLoadout_[i] = skillId;
                break;
            }
        }
    }
    return true;
}

//------------------------------------------------------------------------------
// ユニークスキル
//------------------------------------------------------------------------------
int PlayerData::SkillSlotLimit() const
{
    // ユニークスキルを習得すると装備枠が 1 つ減る
    return HasUniqueSkill() ? 3 : kSkillSlotCount;
}

bool PlayerData::IsUniqueSkillAvailable(UniqueSkillType type) const
{
    if (type == UniqueSkillType::None) return false;
    const int value = static_cast<int>(type);
    return std::find(availableUniqueSkills_.begin(), availableUniqueSkills_.end(), value)
        != availableUniqueSkills_.end();
}

void PlayerData::MakeUniqueSkillAvailable(UniqueSkillType type)
{
    if (type == UniqueSkillType::None) return;
    if (IsUniqueSkillAvailable(type)) return;
    availableUniqueSkills_.push_back(static_cast<int>(type));
}

bool PlayerData::AcquireUniqueSkill(UniqueSkillType type, bool force)
{
    if (type == UniqueSkillType::None) return false;
    if (!IsUniqueSkillAvailable(type)) return false;
    // 習得できるユニークスキルは 1 つだけ
    if (HasUniqueSkill() && !force) return false;

    uniqueSkill_ = type;
    inventory_.SetDualWieldEnabled(HasDualWield());
    RefreshSkillLoadout();
    return true;
}

void PlayerData::DebugUnlockAllUniqueSkills()
{
    for (const UniqueSkillDef& def : UniqueSkillDatabase::Instance().All()) {
        MakeUniqueSkillAvailable(def.type);
    }
    if (!HasUniqueSkill() && !UniqueSkillDatabase::Instance().All().empty()) {
        AcquireUniqueSkill(UniqueSkillDatabase::Instance().All().front().type, true);
    }
}

bool PlayerData::IsSkillEquipped(int skillId) const
{
    return SkillSlotOf(skillId) >= 0;
}

int PlayerData::SkillSlotOf(int skillId) const
{
    if (skillId == 0) return -1;
    for (int i = 0; i < kSkillSlotCount; ++i) {
        if (skillLoadout_[i] == skillId) return i;
    }
    return -1;
}

const SwordSkill* PlayerData::SkillAt(int slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= kSkillSlotCount) return nullptr;
    return SkillDatabase::Instance().Find(skillLoadout_[slotIndex]);
}

bool PlayerData::SetSkillAt(int slotIndex, int skillId)
{
    if (slotIndex < 0 || slotIndex >= SkillSlotLimit()) return false;
    if (skillId == 0) {
        skillLoadout_[slotIndex] = 0;
        return true;
    }

    const SwordSkill* skill = SkillDatabase::Instance().Find(skillId);
    if (!skill) return false;
    if (!IsSkillUnlocked(skillId)) return false;
    if (skill->weapon != CurrentWeaponType()) return false;
    // 専用スキルは対応するユニークスキルが必要
    if (skill->IsUnique() && skill->requiredUnique != uniqueSkill_) return false;

    // 既に他スロットにある場合は入れ替える
    const int existing = SkillSlotOf(skillId);
    if (existing >= 0 && existing != slotIndex) {
        skillLoadout_[existing] = skillLoadout_[slotIndex];
    }
    skillLoadout_[slotIndex] = skillId;
    return true;
}

void PlayerData::ClearSkillSlot(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= kSkillSlotCount) return;
    skillLoadout_[slotIndex] = 0;
}

bool PlayerData::IsQuestCleared(int questId) const
{
    return std::find(clearedQuests_.begin(), clearedQuests_.end(), questId) != clearedQuests_.end();
}

void PlayerData::MarkQuestCleared(int questId)
{
    if (!IsQuestCleared(questId)) clearedQuests_.push_back(questId);
}

void PlayerData::RestoreProgress(int level, int exp, int skillPoints,
                                 const std::vector<int>& unlockedSkills,
                                 const std::vector<int>& clearedQuests,
                                 const int skillLoadout[4],
                                 UniqueSkillType uniqueSkill,
                                 const std::vector<int>& availableUniqueSkills)
{
    uniqueSkill_ = uniqueSkill;
    availableUniqueSkills_ = availableUniqueSkills;
    inventory_.SetDualWieldEnabled(HasDualWield());

    level_ = math::ClampInt(level, 1, 99);
    exp_ = math::MaxI(0, exp);
    skillPoints_ = math::MaxI(0, skillPoints);
    unlockedSkills_ = unlockedSkills;
    clearedQuests_ = clearedQuests;

    for (int i = 0; i < kSkillSlotCount; ++i) skillLoadout_[i] = skillLoadout[i];

    // 起点スキルは必ず解放済みにしておく（データが壊れていても詰まないように）
    for (int id : SkillDatabase::Instance().StarterSkillIds()) {
        if (!IsSkillUnlocked(id)) unlockedSkills_.push_back(id);
    }
    RefreshSkillLoadout();
}

} // namespace ecl
