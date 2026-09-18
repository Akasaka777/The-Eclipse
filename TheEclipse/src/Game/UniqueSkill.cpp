#include "Game/UniqueSkill.h"

#include "Common/StringUtil.h"

namespace ecl {

UniqueSkillDatabase::UniqueSkillDatabase()
{
    {
        UniqueSkillDef def;
        def.type = UniqueSkillType::DualWield;
        def.name = "二刀流";
        def.description = "片手剣を両手に装備できるようになる。"
                          "その代わり盾は持てず、装備できるソードスキルは 3 つまでになる。";
        def.unlockCondition = str::Format("「蝕の祭壇」を %d 秒以内でクリア",
                                          static_cast<int>(kDualWieldClearTimeLimit));
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
