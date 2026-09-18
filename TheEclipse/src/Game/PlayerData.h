//==============================================================================
// PlayerData.h : プレイヤーの永続情報（レベル / 所持品 / スキル構成）
//==============================================================================
#pragma once

#include "Game/Inventory.h"
#include "Game/SwordSkill.h"
#include "Game/UniqueSkill.h"

#include <string>
#include <vector>

namespace ecl {

class PlayerData
{
public:
    PlayerData();

    // 初期装備とスキルを設定する
    void SetupNewGame();

    const std::string& Name() const { return name_; }
    int Level() const { return level_; }
    int Exp() const { return exp_; }
    int ExpToNext() const;

    // 経験値加算（上がったレベル数を返す）
    int AddExp(int amount);

    // レベルのみから決まる基礎能力
    Stats BaseStats() const;
    // 基礎 + 装備
    Stats TotalStats() const;
    int   Power() const { return TotalStats().Power(); }

    Inventory&       GetInventory() { return inventory_; }
    const Inventory& GetInventory() const { return inventory_; }

    WeaponType CurrentWeaponType() const { return inventory_.CurrentWeaponType(); }

    // --- スキル構成 ----------------------------------------------------------
    // 武器種に合わせてロードアウトを組み直す（未解放スキルは外れる）
    void RefreshSkillLoadout();
    const SwordSkill* SkillAt(int slotIndex) const;
    // スロットへ装備（未解放・武器種違いは失敗）
    bool SetSkillAt(int slotIndex, int skillId);
    void ClearSkillSlot(int slotIndex);
    bool IsSkillEquipped(int skillId) const;
    // 装備中のスロット番号（未装備なら -1）
    int  SkillSlotOf(int skillId) const;
    // 今の装備でユニークスキル専用のスキルを使う状態か
    //   二刀流なら「両手に片手剣を持っている間」だけ true になり、
    //   その間は通常の片手剣スキルは使えない（独立した系統として扱う）。
    bool UsesUniqueSkillSet() const;
    // そのスキルを装備できるか（今の系統に合うスキルだけ装備できる）
    bool CanEquipSkill(int skillId) const;
    // そのスキルが今の系統のものか（未解放でも判定する）
    bool MatchesCurrentSkillSet(const SwordSkill& skill) const;
    const int* SkillLoadout() const { return skillLoadout_; }

    // --- スキルツリー ---------------------------------------------------------
    int  SkillPoints() const { return skillPoints_; }
    void AddSkillPoints(int amount);
    bool IsSkillUnlocked(int skillId) const;
    // 前提スキルを満たしているか（ポイント不足でも true）
    bool IsSkillReachable(int skillId) const;
    bool CanUnlockSkill(int skillId) const;
    bool UnlockSkill(int skillId);

    // --- ユニークスキル -------------------------------------------------------
    // 習得中のユニークスキル（未習得なら None）
    UniqueSkillType UniqueSkill() const { return uniqueSkill_; }
    bool HasUniqueSkill() const { return uniqueSkill_ != UniqueSkillType::None; }
    bool HasDualWield() const { return uniqueSkill_ == UniqueSkillType::DualWield; }
    // 解放条件を満たしたか（満たすとスキルメニューに現れる）
    bool IsUniqueSkillAvailable(UniqueSkillType type) const;
    void MakeUniqueSkillAvailable(UniqueSkillType type);
    // 習得する。既に別のユニークスキルを習得済みなら失敗する（force で上書き）
    bool AcquireUniqueSkill(UniqueSkillType type, bool force = false);
    // デバッグ用: 全ユニークスキルを解放可能にする
    void DebugUnlockAllUniqueSkills();
    // デバッグ用: 全武器のスキルツリーを解放する
    void DebugUnlockAllSkills();

    // 装備できるスキル数
    //   ユニークスキルの系統を使っている間だけ 3 つに減る。
    //   通常スキルの系統に戻せば 4 つに復活する。
    int SkillSlotLimit() const;

    // --- クエスト進行 --------------------------------------------------------
    bool IsQuestCleared(int questId) const;
    void MarkQuestCleared(int questId);

    // --- セーブデータの復元 ---------------------------------------------------
    void RestoreProgress(int level, int exp, int skillPoints,
                         const std::vector<int>& unlockedSkills,
                         const std::vector<int>& clearedQuests,
                         const int skillLoadout[4],
                         UniqueSkillType uniqueSkill,
                         const std::vector<int>& availableUniqueSkills);
    const std::vector<int>& AvailableUniqueSkills() const { return availableUniqueSkills_; }
    // 装備を戻した後にスキル構成だけを復元し直す（系統は装備で決まるため）
    void RestoreSkillLoadout(const int skillLoadout[4]);

private:
    std::string name_ = "プレイヤー";
    int level_ = 1;
    int exp_ = 0;
    Inventory inventory_;
    int skillLoadout_[4] = { 0, 0, 0, 0 };
    int skillPoints_ = 3;
    UniqueSkillType uniqueSkill_ = UniqueSkillType::None;
    std::vector<int> availableUniqueSkills_;
    std::vector<int> unlockedSkills_;
    std::vector<int> clearedQuests_;
};

} // namespace ecl
