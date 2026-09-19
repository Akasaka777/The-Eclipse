//==============================================================================
// UniqueSkill.h : ユニークスキル
//   条件を満たすと「習得可能」になり、複数ある場合でも習得できるのは 1 つだけ。
//   習得していないユニークスキルはスキルメニューに表示されない。
//==============================================================================
#pragma once

#include <string>
#include <vector>

namespace ecl {

enum class UniqueSkillType
{
    None,
    DualWield, // 二刀流
    HolySword, // 神聖剣
    Count
};

//==============================================================================
// ユニークスキルの解放条件
//   ここの数値を変更するだけで条件を調整できます。
//   ※ どれか 1 つでも条件を満たすと、以降は他のユニークスキルを解放できません。
//------------------------------------------------------------------------------
// ▼「二刀流」の解放条件
//     クエスト「蝕の祭壇」を、下記の秒数以内でクリアすること。
//     ・制限時間を変えたい場合  : kDualWieldClearTimeLimit の値を変更
//     ・対象クエストを変えたい場合: kDualWieldQuestId の値を変更（QuestDatabase の id）
//------------------------------------------------------------------------------
// ▼「神聖剣」の解放条件
//     パリィを、下記の回数だけ成功させること（クエストをまたいで合計する）。
//     ・必要回数を変えたい場合  : kHolySwordParryCount の値を変更
//==============================================================================
constexpr float kDualWieldClearTimeLimit = 50.0f; // 秒
constexpr int   kDualWieldQuestId = 3;            // 蝕の祭壇

constexpr int   kHolySwordParryCount = 10;        // パリィ成功回数

//------------------------------------------------------------------------------
// 「神聖剣」専用の特別クエスト
//   解放後に一度だけ挑める、ボス戦のみのクエスト。
//   専用のセット武器（片手剣・盾）が 100% ドロップする。
//------------------------------------------------------------------------------
constexpr int kHolySwordQuestId = 4;   // QuestDatabase の id
constexpr int kHolySwordSwordId = 150; // ItemDatabase の id（片手剣）
constexpr int kHolySwordShieldId = 223; // ItemDatabase の id（盾）

//------------------------------------------------------------------------------
// ユニークスキルの定義
//------------------------------------------------------------------------------
struct UniqueSkillDef
{
    UniqueSkillType  type = UniqueSkillType::None;
    std::string      name;
    std::string      description;
    std::string      unlockCondition; // 解放条件の説明文
    // 効果の詳細（スキルツリーで 1 行ずつ表示する）
    std::vector<std::string> details;
    std::vector<int> skillIds;        // 専用スキル（ツリーの上から順。無い場合もある）
    // 専用スキルの代わりに挑める特別クエスト（0 なら無し）
    int specialQuestId = 0;

    // 専用スキルを持たない（＝ツリーではなく効果そのものが本体）
    bool HasDedicatedSkills() const { return !skillIds.empty(); }
};

class UniqueSkillDatabase
{
public:
    static const UniqueSkillDatabase& Instance();

    const std::vector<UniqueSkillDef>& All() const { return entries_; }
    const UniqueSkillDef* Find(UniqueSkillType type) const;

private:
    UniqueSkillDatabase();
    std::vector<UniqueSkillDef> entries_;
};

const char* UniqueSkillName(UniqueSkillType type);

} // namespace ecl
