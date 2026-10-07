//==============================================================================
// QuestDatabase.h : クエスト定義（フロア構成とドロップ）
//==============================================================================
#pragma once

#include "Game/Consumable.h"
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
// 素材のドロップ候補（個数つき）
//------------------------------------------------------------------------------
struct MaterialDrop
{
    int   itemId = 0;
    float chance = 0.2f;   // 抽選確率
    int   minCount = 1;
    int   maxCount = 1;
};

//------------------------------------------------------------------------------
// ボスドロップ 1 件の実際の確率（開発者モードの表示用）
//   定義の chance は「その部位が選ばれたあとの抽選確率」なので、
//   部位抽選や同じ部位の先客を含めた「1 回の撃破で落ちる確率」を別に求める。
//------------------------------------------------------------------------------
struct BossDropOdds
{
    int   templateId = 0;
    float chance = 0.0f;      // 定義上の抽選確率（LUK 補正込み、最大 100%）
    float normal = 0.0f;      // 2 回目以降のクリアで落ちる確率
    float firstClear = 0.0f;  // 初回クリアで落ちる確率
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
    // 素材（武器・防具の作成用）。道中は倒した数だけ、ボスは撃破時に抽選する
    std::vector<MaterialDrop> floorMaterials;
    std::vector<MaterialDrop> bossMaterials;
    // 特別クエスト。クエスト選択タブには出さず、スキルツリーから挑む
    bool special = false;
    // クエスト詳細にボスドロップの中身を載せず「不明」とだけ出す
    bool hideDrops = false;

    int FloorCount() const { return static_cast<int>(floors.size()); }
};

class QuestDatabase
{
public:
    static const QuestDatabase& Instance();

    const std::vector<QuestDef>& Quests() const { return quests_; }
    const QuestDef* Find(int id) const;

    // ドロップ抽選（クリア時はボスドロップを含む）
    //   dropRate  : LUK などによる倍率。1.0 で定義どおりの確率になる。
    //   firstClear: 初回クリアなら、ボスドロップの候補から必ず 1 つ落とす。
    std::vector<EquipmentItem> RollDrops(const QuestDef& quest, bool bossDefeated,
                                         int enemiesDefeated, float dropRate = 1.0f,
                                         bool firstClear = false) const;

    // 素材の抽選（同じ素材はまとめて個数にする）
    std::vector<ItemStack> RollMaterials(const QuestDef& quest, bool bossDefeated,
                                         int enemiesDefeated, float dropRate = 1.0f) const;

    // ボスドロップの実際の確率（RollDrops と同じ手順で計算する）
    std::vector<BossDropOdds> CalcBossDropOdds(const QuestDef& quest,
                                               float dropRate = 1.0f) const;

private:
    QuestDatabase();

    // 難易度に応じた個体値抽選
    // ドロップ 1 個分の個体値を抽選する
    int RollIv(const DropEntry& entry, int difficulty) const;

    std::vector<QuestDef> quests_;
};

} // namespace ecl
