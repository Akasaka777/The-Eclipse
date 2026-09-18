#include "Game/UniqueSkill.h"

#include "Common/StringUtil.h"

namespace ecl {

UniqueSkillDatabase::UniqueSkillDatabase()
{
    {
        UniqueSkillDef def;
        def.type = UniqueSkillType::DualWield;
        def.name = "二刀流";
        def.description = "片手剣を両手に装備できる独立した系統。";
        def.unlockCondition = str::Format("「蝕の祭壇」を %d 秒以内でクリア",
                                          static_cast<int>(kDualWieldClearTimeLimit));
        // 1 行が長すぎるとツリー欄からはみ出すため、短く区切っている
        def.details = {
            "・右手と左手の両方に片手剣",
            "・左手の武器は能力値が控えめ",
            "・両手持ちの間は盾を装備不可",
            "・両手持ち中は専用スキルのみ",
            "・片手剣のスキルは使えない",
            "・片手持ちに戻せば通常スキル",
            "・装備できるスキルは 3 つ",
        };
        // 専用スキル（ツリーの上から順）
        def.skillIds = { 9000, 9001, 9002 };
        entries_.push_back(def);
    }
}

const UniqueSkillDatabase& UniqueSkillDatabase::Instance()
{
    static UniqueSkillDatabase instance;
    return instance;
}

const UniqueSkillDef* UniqueSkillDatabase::Find(UniqueSkillType type) const
{
    for (const UniqueSkillDef& def : entries_) {
        if (def.type == type) return &def;
    }
    return nullptr;
}

const char* UniqueSkillName(UniqueSkillType type)
{
    const UniqueSkillDef* def = UniqueSkillDatabase::Instance().Find(type);
    return def ? def->name.c_str() : "なし";
}

} // namespace ecl
