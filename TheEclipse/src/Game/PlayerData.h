//==============================================================================
// PlayerData.h : プレイヤーの永続情報（レベル / 所持品 / スキル構成）
//==============================================================================
#pragma once

#include "Game/Inventory.h"
#include "Game/SwordSkill.h"

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
    // 武器種に合わせてロードアウトを組み直す
    void RefreshSkillLoadout();
    const SwordSkill* SkillAt(int slotIndex) const;
    void SetSkillAt(int slotIndex, int skillId);
    const int* SkillLoadout() const { return skillLoadout_; }

    // --- クエスト進行 --------------------------------------------------------
    bool IsQuestCleared(int questId) const;
    void MarkQuestCleared(int questId);

private:
    std::string name_ = "プレイヤー";
    int level_ = 1;
    int exp_ = 0;
    Inventory inventory_;
    int skillLoadout_[4] = { 0, 0, 0, 0 };
    std::vector<int> clearedQuests_;
};

} // namespace ecl
