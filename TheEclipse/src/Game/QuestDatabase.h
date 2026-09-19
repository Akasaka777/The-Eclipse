//==============================================================================
// QuestDatabase.h : クエスト定義（フロア構成とドロップ）
//==============================================================================
#pragma once

#include "Game/Equipment.h"
#include "Game/Stage.h"

#include <string>
#include <vector>

namespace ecl {

//------------------------------------------------------------------------------
// ドロップ候補
//------------------------------------------------------------------------------
struct DropEntry
{
    int   templateId = 0;
    float chance = 0.5f;      // 抽選確率
    int   minIv = 0;          // 落ちる装備の個体値の下限
    int   maxIv = 60;         // 同・上限（難易度が高いほど上振れしやすい）
};

//------------------------------------------------------------------------------
// クエスト定義
//------------------------------------------------------------------------------
struct QuestDef
{
    int         id = 0;
    std::string name;
    std::string subtitle;
    std::string description;
    std::string bossName;
    int         difficulty = 1;         // ★の数
    int         recommendedPower = 400;
    float       enemyPowerScale = 1.0f;
    int         colReward = 400;
    int         expReward = 300;
    int         firstClearCol = 1500;
    std::vector<FloorDef>  floors;
    std::vector<DropEntry> bossDrops;   // ボス撃破時の抽選
    std::vector<DropEntry> floorDrops;  // 道中の抽選
    // 特別クエスト。クエスト選択タブには出さず、スキルツリーから挑む
    bool special = false;

    int FloorCount() const { return static_cast<int>(floors.size()); }
};

class QuestDatabase
{
public:
    static const QuestDatabase& Instance();

    const std::vector<QuestDef>& Quests() const { return quests_; }
    const QuestDef* Find(int id) const;

    // ドロップ抽選（クリア時はボスドロップを含む）
    std::vector<EquipmentItem> RollDrops(const QuestDef& quest, bool bossDefeated,
                                         int enemiesDefeated) const;

private:
    QuestDatabase();

    // 難易度に応じた個体値抽選
    // ドロップ 1 個分の個体値を抽選する
    int RollIv(const DropEntry& entry, int difficulty) const;

    std::vector<QuestDef> quests_;
};

} // namespace ecl
