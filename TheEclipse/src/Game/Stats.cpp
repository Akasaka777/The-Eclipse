#include "Game/Stats.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"

namespace ecl {

const char* WeaponTypeName(WeaponType type)
{
    switch (type) {
    case WeaponType::OneHandSword: return "片手剣";
    case WeaponType::OneHandMace:  return "片手棍";
    case WeaponType::Dagger:       return "短剣";
    case WeaponType::Rapier:       return "細剣";
    case WeaponType::Spear:        return "槍";
    default: return "武器";
    }
}

float WeaponReachScale(WeaponType type)
{
    switch (type) {
    case WeaponType::OneHandSword: return 1.00f;
    case WeaponType::OneHandMace:  return 0.95f;
    case WeaponType::Dagger:       return 0.70f;
    case WeaponType::Rapier:       return 1.15f;
    case WeaponType::Spear:        return 1.45f;
    default: return 1.0f;
    }
}

float WeaponSpeedScale(WeaponType type)
{
    switch (type) {
    case WeaponType::OneHandSword: return 1.00f;
    case WeaponType::OneHandMace:  return 0.80f;
    case WeaponType::Dagger:       return 1.40f;
    case WeaponType::Rapier:       return 1.25f;
    case WeaponType::Spear:        return 0.88f;
    default: return 1.0f;
    }
}

Stats& Stats::operator+=(const Stats& o)
{
    maxHp += o.maxHp;
    maxMp += o.maxMp;
    attack += o.attack;
    defense += o.defense;
    critRate += o.critRate;
    critDamage += o.critDamage;
    mpRegen += o.mpRegen;
    moveSpeed += o.moveSpeed;
    attackSpeed += o.attackSpeed;
    magicAttack += o.magicAttack;
    magicDefense += o.magicDefense;
    accuracy += o.accuracy;
    fireResist += o.fireResist;
    waterResist += o.waterResist;
    return *this;
}

Stats Stats::operator+(const Stats& o) const
{
    Stats result = *this;
    result += o;
    return result;
}

Stats Stats::Scaled(float factor) const
{
    Stats result;
    result.maxHp = maxHp * factor;
    result.maxMp = maxMp * factor;
    result.attack = attack * factor;
    result.defense = defense * factor;
    result.critRate = critRate * factor;
    result.critDamage = critDamage * factor;
    result.mpRegen = mpRegen * factor;
    result.moveSpeed = moveSpeed * factor;
    result.attackSpeed = attackSpeed * factor;
    // 魔法・属性・命中はアクセサリー専用の固定値。個体値では揺らがせない
    result.magicAttack = magicAttack;
    result.magicDefense = magicDefense;
    result.accuracy = accuracy;
    result.fireResist = fireResist;
    result.waterResist = waterResist;
    return result;
}

StatRates& StatRates::operator+=(const StatRates& o)
{
    maxHp += o.maxHp;
    maxMp += o.maxMp;
    attack += o.attack;
    defense += o.defense;
    moveSpeed += o.moveSpeed;
    return *this;
}

bool StatRates::Empty() const
{
    return maxHp == 0.0f && maxMp == 0.0f && attack == 0.0f
        && defense == 0.0f && moveSpeed == 0.0f;
}

Stats ApplyRates(const Stats& base, const StatRates& rates)
{
    Stats result = base;
    result.maxHp *= 1.0f + rates.maxHp;
    result.maxMp *= 1.0f + rates.maxMp;
    result.attack *= 1.0f + rates.attack;
    result.defense *= 1.0f + rates.defense;
    result.moveSpeed *= 1.0f + rates.moveSpeed;
    return result;
}

int Stats::Power() const
{
    const float score = attack * 2.4f
                      + defense * 1.8f
                      + maxHp * 0.22f
                      + maxMp * 0.12f
                      + critRate * 420.0f
                      + critDamage * 180.0f
                      + moveSpeed * 0.35f
                      + attackSpeed * 260.0f;
    return static_cast<int>(score);
}

DamageResult CalculateDamage(float attack, float defense, float multiplier,
                             float critRate, float critDamage, bool canCrit)
{
    DamageResult result;

    // 防御は減衰式（防御力が高いほど効果が逓減する）
    const float mitigation = 220.0f / (220.0f + math::MaxF(defense, 0.0f));
    float damage = math::MaxF(attack, 1.0f) * multiplier * mitigation;

    // ±8% のばらつき
    damage *= math::RandFloat(0.92f, 1.08f);

    if (canCrit && math::RandChance(math::Clamp(critRate, 0.0f, 0.95f))) {
        result.critical = true;
        damage *= (1.5f + critDamage);
    }

    result.value = math::MaxI(1, static_cast<int>(damage));
    return result;
}

std::string StatsSummaryLine(const Stats& stats)
{
    return str::Format("ATK %d  DEF %d  HP %d",
                       static_cast<int>(stats.attack),
                       static_cast<int>(stats.defense),
                       static_cast<int>(stats.maxHp));
}

} // namespace ecl
