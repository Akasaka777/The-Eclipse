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
    Count
};

//==============================================================================
// ユニークスキルの解放条件
//   ここの数値を変更するだけで条件を調整できます。
//------------------------------------------------------------------------------
// ▼「二刀流」の解放条件
//     クエスト「蝕の祭壇」を、下記の秒数以内でクリアすること。
//     ・制限時間を変えたい場合  : kDualWieldClearTimeLimit の値を変更
//     ・対象クエストを変えたい場合: kDualWieldQuestId の値を変更（QuestDatabase の id）
//==============================================================================
constexpr float kDualWieldClearTimeLimit = 50.0f; // 秒
constexpr int   kDualWieldQuestId = 3;            // 蝕の祭壇

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
    std::vector<int> skillIds;        // 専用スキル（ツリーの上から順）
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
