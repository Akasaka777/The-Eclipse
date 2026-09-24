#include "Game/Ability.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"

namespace ecl {

int AbilityScores::Get(Ability ability) const
{
    const int index = static_cast<int>(ability);
    if (index < 0 || index >= kAbilityCount) return 0;
    return values[index];
}

void AbilityScores::Set(Ability ability, int value)
{
    const int index = static_cast<int>(ability);
    if (index < 0 || index >= kAbilityCount) return;
    values[index] = math::ClampInt(value, 0, kAbilityMaxValue);
}

void AbilityScores::Add(Ability ability, int amount)
{
    Set(ability, Get(ability) + amount);
}

int AbilityScores::Total() const
{
    int total = 0;
    for (int i = 0; i < kAbilityCount; ++i) total += values[i];
    return total;
}

const char* AbilityShortName(Ability ability)
{
    switch (ability) {
    case Ability::Str: return "STR";
    case Ability::Agi: return "AGI";
    case Ability::Vit: return "VIT";
    case Ability::Int: return "INT";
    case Ability::Luk: return "LUK";
    default: break;
    }
    return "---";
}

const char* AbilityName(Ability ability)
{
    switch (ability) {
    case Ability::Str: return "筋力";
    case Ability::Agi: return "敏捷";
    case Ability::Vit: return "体力";
    case Ability::Int: return "知力";
    case Ability::Luk: return "幸運";
    default: break;
    }
    return "---";
}

const char* AbilityShortEffect(Ability ability)
{
    switch (ability) {
    case Ability::Str: return "攻撃力";
    case Ability::Agi: return "移動速度・攻撃速度";
    case Ability::Vit: return "最大HP・防御力";
    case Ability::Int: return "クリティカル率・倍率";
    case Ability::Luk: return "ドロップ率";
    default: break;
    }
    return "";
}

std::string AbilityEffectText(Ability ability)
{
    switch (ability) {
    case Ability::Str:
        return str::Format("攻撃力 +%.1f%%", kStrAttackPerPoint * 100.0f);
    case Ability::Agi:
        return str::Format("移動速度 +%.1f%% ／ 攻撃速度 +%.1f%%",
                           kAgiMoveSpeedPerPoint * 100.0f, kAgiAttackSpeedPerPoint * 100.0f);
    case Ability::Vit:
        return str::Format("最大 HP +%.1f%% ／ 防御力 +%.1f%%",
                           kVitMaxHpPerPoint * 100.0f, kVitDefensePerPoint * 100.0f);
    case Ability::Int:
        return str::Format("クリティカル率 +%.1f%% ／ クリティカル倍率 +%.1f%%",
                           kIntCritRatePerPoint * 100.0f, kIntCritDamagePerPoint * 100.0f);
    case Ability::Luk:
        return str::Format("ドロップ率 +%.1f%%", kLukDropRatePerPoint * 100.0f);
    default:
        break;
    }
    return std::string();
}

Stats ApplyAbilities(const Stats& base, const AbilityScores& scores)
{
    // 初期値ぶんは「素の状態」とみなし、そこからの伸びだけを効果にする
    auto gain = [&scores](Ability ability) {
        return static_cast<float>(scores.Get(ability) - kAbilityInitialValue);
    };

    const float str = gain(Ability::Str);
    const float agi = gain(Ability::Agi);
    const float vit = gain(Ability::Vit);
    const float intel = gain(Ability::Int);

    Stats result = base;
    result.attack      = base.attack * (1.0f + kStrAttackPerPoint * str);
    result.moveSpeed   = base.moveSpeed * (1.0f + kAgiMoveSpeedPerPoint * agi);
    result.attackSpeed = base.attackSpeed + kAgiAttackSpeedPerPoint * agi;
    result.maxHp       = base.maxHp * (1.0f + kVitMaxHpPerPoint * vit);
    result.defense     = base.defense * (1.0f + kVitDefensePerPoint * vit);
    result.critRate    = base.critRate + kIntCritRatePerPoint * intel;
    result.critDamage  = base.critDamage + kIntCritDamagePerPoint * intel;
    return result;
}

float AbilityDropRate(const AbilityScores& scores)
{
    const float luk = static_cast<float>(scores.Get(Ability::Luk) - kAbilityInitialValue);
    return math::MaxF(0.0f, 1.0f + kLukDropRatePerPoint * luk);
}

} // namespace ecl
