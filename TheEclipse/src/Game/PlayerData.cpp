#include "Game/PlayerData.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Game/ItemDatabase.h"

#include <algorithm>
#include <cmath>

namespace ecl {

namespace {

//------------------------------------------------------------------------------
// レベルごとの必要経験値
//   両端を固定した冪乗カーブ。
//     レベル  1 →  2 : kExpFirst      EXP
//     レベル 98 → 99 : kExpLast       EXP
//   この 2 点を通るように指数を求めているので、数値を変えたいときは
//   kExpFirst / kExpLast だけを触れば中間も自動で付いてくる。
//------------------------------------------------------------------------------
constexpr double kExpFirst = 150.0;        // レベル 1 → 2
constexpr double kExpLast = 2000000.0;     // レベル 98 → 99
constexpr int    kExpLastLevel = 98;       // 最後のレベルアップ

int ExpTableFor(int level)
{
    const int clamped = math::ClampInt(level, 1, kExpLastLevel);
    // kExpFirst * level^exponent が kExpLastLevel で kExpLast になる指数
    static const double exponent =
        std::log(kExpLast / kExpFirst) / std::log(static_cast<double>(kExpLastLevel));
    const double need = kExpFirst * std::pow(static_cast<double>(clamped), exponent);
    return static_cast<int>(need + 0.5);
}

} // namespace

PlayerData::PlayerData() = default;

void PlayerData::SetupNewGame(WeaponType weapon)
{
    const ItemDatabase& database = ItemDatabase::Instance();
    inventory_.AddItems(database.CreateStarterSet(weapon));

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
    // 最初は選んだ武器の初期スキルだけを装備した状態にする
    ResetSkillLoadoutToStarter();
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

    // レベルアップ 1 回につきスキルポイント 1 とステータスポイント
    if (gained > 0) {
        AddSkillPoints(gained);
        AddAbilityPoints(gained * kAbilityPointsPerLevel);
    }
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
    // MP は 10 秒に 1 回復する（0.1 / 秒）。装備で少しだけ上乗せできる。
    stats.mpRegen = 0.1f;
    stats.moveSpeed = 460.0f;
    stats.attackSpeed = 0.0f;
    return stats;
}

Stats PlayerData::EquippedStats() const
{
    // アクセサリーの倍率バフ（攻撃力 +5% など）は、素の値と装備値の合計に掛ける
    return ApplyRates(BaseStats() + inventory_.EquippedStats(), inventory_.EquippedRates());
}

Stats PlayerData::TotalStats() const
{
    // 振り分けステータス（STR / AGI / VIT / INT）を最後に掛ける
    return ApplyAbilities(EquippedStats(), abilities_);
}

//------------------------------------------------------------------------------
// 振り分けステータス
//------------------------------------------------------------------------------
void PlayerData::AddAbilityPoints(int amount)
{
    if (amount <= 0) return;
    abilityPoints_ += amount;
}

bool PlayerData::SpendAbilityPoint(ecl::Ability ability, int amount)
{
    if (amount <= 0 || amount > abilityPoints_) return false;
    const int index = static_cast<int>(ability);
    if (index < 0 || index >= kAbilityCount) return false;
    if (abilities_.Get(ability) >= kAbilityMaxValue) return false;

    abilities_.Add(ability, amount);
    abilityPoints_ -= amount;
    return true;
}

void PlayerData::RestoreAbilities(const AbilityScores& scores, int abilityPoints)
{
    abilities_ = scores;
    abilityPoints_ = math::MaxI(0, abilityPoints);
}

//------------------------------------------------------------------------------
// プレイヤー名
//------------------------------------------------------------------------------
namespace {
constexpr int kMaxNameLength = 12; // 表示できる文字数（UTF-8 の文字単位）
} // namespace

int PlayerData::MaxNameLength()
{
    return kMaxNameLength;
}

void PlayerData::SetName(const std::string& name)
{
    // 前後の空白を落とす
    size_t begin = 0;
    size_t end = name.size();
    while (begin < end && (name[begin] == ' ' || name[begin] == '\t')) ++begin;
    while (end > begin && (name[end - 1] == ' ' || name[end - 1] == '\t')) --end;
    const std::string trimmed = name.substr(begin, end - begin);

    if (trimmed.empty()) {
        name_ = "プレイヤー";
        return;
    }

    // 最大文字数で切り詰める（UTF-8 の途中で切らない）
    name_ = str::Truncate(trimmed, kMaxNameLength);
}

int PlayerData::StarterSkillForCurrentSet() const
{
    const SkillDatabase& database = SkillDatabase::Instance();
    // 武器を外している間はどの系統のスキルも使えない
    if (!UsesUniqueSkillSet() && !HasWeaponEquipped()) return 0;

    const std::vector<const SwordSkill*> pool =
        UsesUniqueSkillSet() ? database.ForUnique(uniqueSkill_)
                             : database.ForWeapon(CurrentWeaponType());

    // 通常の系統は「起点スキル（解放コスト 0）」が初期スキル。
    // ユニークスキルの系統には起点が無いので、解放済みの最上段を初期スキルとする。
    for (const SwordSkill* skill : pool) {
        if (skill->IsStarter() && IsSkillUnlocked(skill->id)) return skill->id;
    }
    for (const SwordSkill* skill : pool) {
        if (IsSkillUnlocked(skill->id)) return skill->id;
    }
    return 0;
}

void PlayerData::ResetSkillLoadoutToStarter()
{
    for (int i = 0; i < kSkillSlotCount; ++i) skillLoadout_[i] = 0;
    const int starter = StarterSkillForCurrentSet();
    if (starter != 0 && SkillSlotLimit() > 0) skillLoadout_[0] = starter;

    lastLoadoutUnique_ = UsesUniqueSkillSet();
    lastLoadoutWeapon_ = CurrentWeaponType();
}

void PlayerData::RefreshSkillLoadoutForEquipment()
{
    // 系統（武器種 / ユニーク）が変わったら、スロットは初期スキルだけに戻す。
    // 前の武器のスキルを引き継がず、勝手に埋め直しもしない。
    if (UsesUniqueSkillSet() != lastLoadoutUnique_ || CurrentWeaponType() != lastLoadoutWeapon_) {
        ResetSkillLoadoutToStarter();
        return;
    }
    RefreshSkillLoadout();
}

void PlayerData::RefreshSkillLoadout()
{
    const bool uniqueSet = UsesUniqueSkillSet();
    const int  limit = SkillSlotLimit();

    // 次回の装備変更で系統の変化を検知できるように控えておく
    lastLoadoutUnique_ = uniqueSet;
    lastLoadoutWeapon_ = CurrentWeaponType();

    // --- 1. 今の系統で使えないスキルを外す --------------------------------------
    for (int i = 0; i < kSkillSlotCount; ++i) {
        if (!CanEquipSkill(skillLoadout_[i])) skillLoadout_[i] = 0;
    }

    // --- 2. 枠数に収める（余った分は前へ詰める）----------------------------------
    {
        int packed[kSkillSlotCount] = { 0, 0, 0, 0 };
        int write = 0;
        for (int i = 0; i < kSkillSlotCount; ++i) {
            if (skillLoadout_[i] != 0 && write < limit) packed[write++] = skillLoadout_[i];
        }
        bool changed = false;
        for (int i = 0; i < kSkillSlotCount; ++i) {
            if (packed[i] != skillLoadout_[i]) changed = true;
        }
        // 並び替えが要る時だけ書き戻す（プレイヤーが空けた枠を勝手に詰めない）
        if (changed) {
            for (int i = 0; i < kSkillSlotCount; ++i) skillLoadout_[i] = packed[i];
        }
    }

    // 空いた枠を勝手に埋めることはしない（どれを装備するかはプレイヤーが決める）。
    // 武器を変えたときだけ ResetSkillLoadoutToStarter() が初期スキルを入れる。
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
    // 専用スキルは、習得中のユニークスキルのものか、
    // まだ何も習得しておらず解放条件を満たしているものだけ解放できる
    if (skill->IsUnique() && skill->requiredUnique != uniqueSkill_) {
        if (HasUniqueSkill()) return false;
        if (!IsUniqueSkillAvailable(skill->requiredUnique)) return false;
    }
    return skillPoints_ >= skill->unlockCost;
}

bool PlayerData::UnlockSkill(int skillId)
{
    if (!CanUnlockSkill(skillId)) return false;

    const SwordSkill* skill = SkillDatabase::Instance().Find(skillId);

    // ユニークスキル専用の最初のスキルを解放した時点でユニークスキルを習得する
    if (skill->IsUnique() && skill->requiredUnique != uniqueSkill_) {
        AcquireUniqueSkill(skill->requiredUnique);
    }

    skillPoints_ -= skill->unlockCost;
    unlockedSkills_.push_back(skillId);

    // 解放しただけではスロットに入れない（どれを装備するかはプレイヤーが決める）
    return true;
}

//------------------------------------------------------------------------------
// ユニークスキル
//------------------------------------------------------------------------------
int PlayerData::SkillSlotLimit() const
{
    // ユニークスキルの系統を使っている間だけ枠が 1 つ減る。
    // 通常スキルの系統に戻せば 4 枠に復活する。
    return UsesUniqueSkillSet() ? kUniqueSkillSlotCount : kSkillSlotCount;
}

//------------------------------------------------------------------------------
// 今どの系統のスキルを使うか
//   ユニークスキルは武器種（片手剣・片手棍…）と並ぶ独立した系統として扱う。
//   ユニークスキルを追加したら、その発動条件をここに足す。
//------------------------------------------------------------------------------
bool PlayerData::UsesUniqueSkillSet() const
{
    switch (uniqueSkill_) {
    // 二刀流 : 両手に片手剣を装備している間だけ。
    //          片手持ちに戻すと通常の片手剣スキルへ戻る。
    case UniqueSkillType::DualWield: return inventory_.IsDualWielding();
    // 神聖剣 : 専用スキルを持たないので、スキルの系統は通常どおり。
    default: return false;
    }
}

bool PlayerData::HasPerfectGuard() const
{
    // 神聖剣の効果は「盾での防御」が前提。盾を外していると働かない。
    return HasHolySword() && inventory_.Equipped(EquipSlot::Shield) != nullptr;
}

void PlayerData::AddParrySuccess(int count)
{
    if (count <= 0) return;
    parrySuccessCount_ += count;
}

bool PlayerData::TryUnlockByParry()
{
    if (parrySuccessCount_ < kHolySwordParryCount) return false;
    return MakeUniqueSkillAvailable(UniqueSkillType::HolySword);
}

bool PlayerData::MatchesCurrentSkillSet(const SwordSkill& skill) const
{
    if (UsesUniqueSkillSet()) {
        // ユニークスキルの系統中は専用スキルしか使えない
        return skill.IsUnique() && skill.requiredUnique == uniqueSkill_;
    }
    // 通常の系統中はユニークスキル専用のスキルを使えない
    return !skill.IsUnique() && skill.weapon == CurrentWeaponType();
}

bool PlayerData::CanEquipSkill(int skillId) const
{
    const SwordSkill* skill = SkillDatabase::Instance().Find(skillId);
    if (!skill) return false;
    if (!IsSkillUnlocked(skillId)) return false;
    return MatchesCurrentSkillSet(*skill);
}

bool PlayerData::IsUniqueSkillAvailable(UniqueSkillType type) const
{
    if (type == UniqueSkillType::None) return false;
    const int value = static_cast<int>(type);
    return std::find(availableUniqueSkills_.begin(), availableUniqueSkills_.end(), value)
        != availableUniqueSkills_.end();
}

bool PlayerData::MakeUniqueSkillAvailable(UniqueSkillType type, bool force)
{
    if (type == UniqueSkillType::None) return false;
    if (IsUniqueSkillAvailable(type)) return false;
    // 先に別のユニークスキルの条件を満たしていたら、以降は手に入らない
    if (IsUniqueSkillLocked() && !force) return false;

    availableUniqueSkills_.push_back(static_cast<int>(type));
    return true;
}

bool PlayerData::AcquireUniqueSkill(UniqueSkillType type, bool force)
{
    if (type == UniqueSkillType::None) return false;
    if (!IsUniqueSkillAvailable(type)) return false;
    // 習得できるユニークスキルは 1 つだけ
    if (HasUniqueSkill() && !force) return false;

    uniqueSkill_ = type;
    inventory_.SetDualWieldEnabled(HasDualWield());
    // 別のユニークスキルへ切り替えた場合、前の専用スキルを外す
    RefreshSkillLoadout();
    return true;
}

void PlayerData::DebugUnlockAllSkills()
{
    // 通常スキルをすべて解放する（専用スキルは習得中のユニークスキルのものだけ）
    for (const SwordSkill& skill : SkillDatabase::Instance().AllSkills()) {
        if (skill.IsUnique() && skill.requiredUnique != uniqueSkill_) continue;
        if (!IsSkillUnlocked(skill.id)) unlockedSkills_.push_back(skill.id);
    }
    RefreshSkillLoadout();
}

bool PlayerData::DebugAcquireUniqueSkill(UniqueSkillType type)
{
    if (type == UniqueSkillType::None) return false;
    MakeUniqueSkillAvailable(type, true);
    return AcquireUniqueSkill(type, true);
}

void PlayerData::DebugAddLevel(int levels)
{
    if (levels <= 0) return;
    const int before = level_;
    level_ = math::ClampInt(level_ + levels, 1, 99);
    exp_ = 0;
    AddSkillPoints(level_ - before);
    AddAbilityPoints((level_ - before) * kAbilityPointsPerLevel);
}

void PlayerData::DebugUnlockAllUniqueSkills()
{
    // デバッグモードでは締め切りを無視して全て解放できるようにする
    for (const UniqueSkillDef& def : UniqueSkillDatabase::Instance().All()) {
        MakeUniqueSkillAvailable(def.type, true);
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
    if (slotIndex < 0 || slotIndex >= kSkillSlotCount) return false;
    if (skillId == 0) {
        skillLoadout_[slotIndex] = 0;
        return true;
    }

    // 未解放・系統違い（武器種／ユニーク）はここで弾く
    if (!CanEquipSkill(skillId)) return false;
    // ユニークスキルの系統中は 3 枠までなので、4 番目の枠には入れられない
    if (slotIndex >= SkillSlotLimit()) return false;

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
                                 const std::vector<int>& availableUniqueSkills,
                                 int parrySuccessCount)
{
    uniqueSkill_ = uniqueSkill;
    availableUniqueSkills_ = availableUniqueSkills;
    parrySuccessCount_ = math::MaxI(0, parrySuccessCount);
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

void PlayerData::RestoreSkillLoadout(const int skillLoadout[4])
{
    for (int i = 0; i < kSkillSlotCount; ++i) skillLoadout_[i] = skillLoadout[i];
    RefreshSkillLoadout();
}

} // namespace ecl
