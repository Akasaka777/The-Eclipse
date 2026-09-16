#include "Game/SwordSkill.h"

namespace ecl {

namespace {

SkillStrike MakeStrike(float time, float multiplier, float reach, float height,
                       float knockback, float forwardImpulse, float hitStop = 0.05f,
                       bool launch = false, float offsetY = 0.0f)
{
    SkillStrike s;
    s.time = time;
    s.damageMultiplier = multiplier;
    s.reach = reach;
    s.height = height;
    s.knockback = knockback;
    s.forwardImpulse = forwardImpulse;
    s.hitStop = hitStop;
    s.launch = launch;
    s.offsetY = offsetY;
    return s;
}

} // namespace

float SwordSkill::TotalMultiplier() const
{
    float total = 0.0f;
    for (const SkillStrike& strike : strikes) total += strike.damageMultiplier;
    return total;
}

SkillDatabase::SkillDatabase()
{
    const ColorRGB cyan(120, 220, 255);
    const ColorRGB gold(255, 200, 96);
    const ColorRGB violet(200, 130, 255);
    const ColorRGB crimson(255, 110, 110);
    const ColorRGB green(150, 255, 170);

    //--- 片手剣 ---------------------------------------------------------------
    {
        SwordSkill s;
        s.id = 1000; s.name = "バーティカル"; s.weapon = WeaponType::OneHandSword;
        s.description = "上段からの一撃。素直で隙が少ない。";
        s.mpCost = 8.0f; s.cooldown = 3.5f; s.duration = 0.50f; s.effectStyle = 0; s.effectColor = cyan;
        s.strikes = { MakeStrike(0.18f, 2.2f, 1.5f, 1.1f, 220.0f, 260.0f, 0.06f) };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1001; s.name = "ホリゾンタル・スクエア"; s.weapon = WeaponType::OneHandSword;
        s.description = "四連の横薙ぎ。広い範囲を巻き込む。";
        s.mpCost = 22.0f; s.cooldown = 9.0f; s.duration = 0.95f; s.effectStyle = 1; s.effectColor = cyan;
        s.invincibleUntil = 0.20f;
        s.strikes = {
            MakeStrike(0.14f, 1.3f, 1.8f, 0.9f, 90.0f, 220.0f, 0.04f),
            MakeStrike(0.32f, 1.3f, 1.8f, 0.9f, 90.0f, 200.0f, 0.04f),
            MakeStrike(0.50f, 1.4f, 1.9f, 1.0f, 110.0f, 200.0f, 0.05f),
            MakeStrike(0.68f, 2.4f, 2.1f, 1.2f, 320.0f, 240.0f, 0.10f),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1002; s.name = "ソニックリープ"; s.weapon = WeaponType::OneHandSword;
        s.description = "跳躍して距離を詰める突進斬り。";
        s.mpCost = 16.0f; s.cooldown = 7.0f; s.duration = 0.62f; s.effectStyle = 2; s.effectColor = gold;
        s.invincibleUntil = 0.34f;
        s.strikes = { MakeStrike(0.22f, 3.0f, 2.6f, 1.0f, 300.0f, 900.0f, 0.09f) };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1003; s.name = "ノヴァ・アセンダント"; s.weapon = WeaponType::OneHandSword;
        s.description = "光を纏った渾身の斬り上げ。敵を打ち上げる。";
        s.mpCost = 34.0f; s.cooldown = 14.0f; s.duration = 1.05f; s.effectStyle = 4; s.effectColor = gold;
        s.invincibleUntil = 0.42f;
        s.strikes = {
            MakeStrike(0.30f, 2.0f, 2.0f, 1.4f, 160.0f, 160.0f, 0.06f, true),
            MakeStrike(0.55f, 4.6f, 2.2f, 1.6f, 420.0f, 220.0f, 0.14f, true),
        };
        skills_.push_back(s);
    }

    //--- 片手棍 ---------------------------------------------------------------
    {
        SwordSkill s;
        s.id = 1100; s.name = "クラッシュブロウ"; s.weapon = WeaponType::OneHandMace;
        s.description = "真上から叩き潰す一撃。";
        s.mpCost = 10.0f; s.cooldown = 4.5f; s.duration = 0.62f; s.effectStyle = 0; s.effectColor = crimson;
        s.strikes = { MakeStrike(0.26f, 3.0f, 1.5f, 1.2f, 260.0f, 180.0f, 0.10f) };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1101; s.name = "グランドインパクト"; s.weapon = WeaponType::OneHandMace;
        s.description = "地面を砕き、衝撃で周囲を巻き込む。";
        s.mpCost = 26.0f; s.cooldown = 10.0f; s.duration = 0.90f; s.effectStyle = 4; s.effectColor = crimson;
        s.strikes = {
            MakeStrike(0.34f, 2.4f, 1.6f, 1.2f, 180.0f, 120.0f, 0.08f),
            MakeStrike(0.42f, 3.2f, 3.0f, 0.7f, 380.0f, 0.0f, 0.14f, true),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1102; s.name = "ヘヴィスイング"; s.weapon = WeaponType::OneHandMace;
        s.description = "体ごと回転する横薙ぎ。ノックバックが大きい。";
        s.mpCost = 18.0f; s.cooldown = 7.5f; s.duration = 0.78f; s.effectStyle = 1; s.effectColor = crimson;
        s.invincibleUntil = 0.30f;
        s.strikes = {
            MakeStrike(0.24f, 2.0f, 2.0f, 1.0f, 200.0f, 260.0f, 0.07f),
            MakeStrike(0.46f, 2.8f, 2.1f, 1.0f, 420.0f, 200.0f, 0.12f),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1103; s.name = "メテオフォール"; s.weapon = WeaponType::OneHandMace;
        s.description = "跳び上がって全体重を乗せた落下攻撃。";
        s.mpCost = 36.0f; s.cooldown = 15.0f; s.duration = 1.15f; s.effectStyle = 4; s.effectColor = gold;
        s.invincibleUntil = 0.60f;
        s.strikes = {
            MakeStrike(0.66f, 5.4f, 2.2f, 1.6f, 320.0f, 140.0f, 0.16f),
            MakeStrike(0.72f, 2.2f, 3.6f, 0.6f, 260.0f, 0.0f, 0.06f, true),
        };
        skills_.push_back(s);
    }

    //--- 短剣 -----------------------------------------------------------------
    {
        SwordSkill s;
        s.id = 1200; s.name = "ラピッドバイト"; s.weapon = WeaponType::Dagger;
        s.description = "三連の刺突。手数で押す。";
        s.mpCost = 9.0f; s.cooldown = 3.2f; s.duration = 0.44f; s.effectStyle = 3; s.effectColor = violet;
        s.strikes = {
            MakeStrike(0.08f, 0.9f, 1.2f, 0.8f, 40.0f, 200.0f, 0.03f),
            MakeStrike(0.17f, 0.9f, 1.2f, 0.8f, 40.0f, 200.0f, 0.03f),
            MakeStrike(0.26f, 1.4f, 1.3f, 0.9f, 160.0f, 240.0f, 0.06f),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1201; s.name = "シャドウステップ"; s.weapon = WeaponType::Dagger;
        s.description = "影に紛れて背後へ回り込み斬り裂く。";
        s.mpCost = 18.0f; s.cooldown = 8.0f; s.duration = 0.58f; s.effectStyle = 2; s.effectColor = violet;
        s.invincibleUntil = 0.36f;
        s.strikes = {
            MakeStrike(0.26f, 2.0f, 2.2f, 1.0f, 60.0f, 760.0f, 0.05f),
            MakeStrike(0.36f, 2.6f, 1.6f, 1.0f, 180.0f, 200.0f, 0.10f),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1202; s.name = "ファタルスラッシュ"; s.weapon = WeaponType::Dagger;
        s.description = "急所を狙う一閃。クリティカル時の伸びが大きい。";
        s.mpCost = 20.0f; s.cooldown = 9.5f; s.duration = 0.52f; s.effectStyle = 0; s.effectColor = crimson;
        s.strikes = { MakeStrike(0.20f, 4.2f, 1.4f, 1.1f, 200.0f, 300.0f, 0.12f) };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1203; s.name = "ブラッディストーム"; s.weapon = WeaponType::Dagger;
        s.description = "嵐のような連続攻撃で敵を切り刻む。";
        s.mpCost = 32.0f; s.cooldown = 13.0f; s.duration = 1.10f; s.effectStyle = 3; s.effectColor = violet;
        s.invincibleUntil = 0.25f;
        s.strikes = {
            MakeStrike(0.12f, 1.0f, 1.6f, 1.0f, 30.0f, 180.0f, 0.02f),
            MakeStrike(0.24f, 1.0f, 1.6f, 1.0f, 30.0f, 180.0f, 0.02f),
            MakeStrike(0.36f, 1.0f, 1.6f, 1.0f, 30.0f, 180.0f, 0.02f),
            MakeStrike(0.48f, 1.0f, 1.6f, 1.0f, 30.0f, 180.0f, 0.02f),
            MakeStrike(0.60f, 1.2f, 1.7f, 1.0f, 40.0f, 180.0f, 0.03f),
            MakeStrike(0.78f, 3.0f, 1.9f, 1.2f, 340.0f, 260.0f, 0.13f),
        };
        skills_.push_back(s);
    }

    //--- 細剣 -----------------------------------------------------------------
    {
        SwordSkill s;
        s.id = 1300; s.name = "リニアシュート"; s.weapon = WeaponType::Rapier;
        s.description = "一直線に伸びる鋭い刺突。";
        s.mpCost = 8.0f; s.cooldown = 3.4f; s.duration = 0.42f; s.effectStyle = 2; s.effectColor = cyan;
        s.strikes = { MakeStrike(0.14f, 2.3f, 2.4f, 0.7f, 180.0f, 340.0f, 0.06f) };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1301; s.name = "スターバースト"; s.weapon = WeaponType::Rapier;
        s.description = "星を描く多段突き。";
        s.mpCost = 24.0f; s.cooldown = 9.5f; s.duration = 0.86f; s.effectStyle = 3; s.effectColor = cyan;
        s.strikes = {
            MakeStrike(0.12f, 1.1f, 2.2f, 0.8f, 30.0f, 220.0f, 0.03f),
            MakeStrike(0.24f, 1.1f, 2.2f, 0.9f, 30.0f, 220.0f, 0.03f),
            MakeStrike(0.36f, 1.1f, 2.2f, 1.0f, 30.0f, 220.0f, 0.03f),
            MakeStrike(0.48f, 1.3f, 2.3f, 1.1f, 60.0f, 240.0f, 0.04f),
            MakeStrike(0.64f, 2.8f, 2.6f, 1.2f, 300.0f, 320.0f, 0.12f),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1302; s.name = "フルーレ・ダンス"; s.weapon = WeaponType::Rapier;
        s.description = "舞うように前後へ動きながら刺す。";
        s.mpCost = 19.0f; s.cooldown = 8.0f; s.duration = 0.74f; s.effectStyle = 3; s.effectColor = green;
        s.invincibleUntil = 0.40f;
        s.strikes = {
            MakeStrike(0.16f, 1.6f, 2.2f, 0.9f, 60.0f, 420.0f, 0.04f),
            MakeStrike(0.34f, 1.6f, 2.2f, 0.9f, 60.0f, -300.0f, 0.04f),
            MakeStrike(0.54f, 2.6f, 2.4f, 1.0f, 240.0f, 480.0f, 0.10f),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1303; s.name = "イクリプス・ソーン"; s.weapon = WeaponType::Rapier;
        s.description = "蝕の光を纏う貫通の一撃。";
        s.mpCost = 33.0f; s.cooldown = 13.5f; s.duration = 0.92f; s.effectStyle = 2; s.effectColor = violet;
        s.invincibleUntil = 0.46f;
        s.strikes = {
            MakeStrike(0.30f, 3.0f, 3.2f, 1.0f, 120.0f, 520.0f, 0.10f),
            MakeStrike(0.52f, 4.0f, 3.4f, 1.2f, 360.0f, 260.0f, 0.14f),
        };
        skills_.push_back(s);
    }

    //--- 槍 -------------------------------------------------------------------
    {
        SwordSkill s;
        s.id = 1400; s.name = "パイクスラスト"; s.weapon = WeaponType::Spear;
        s.description = "間合いの外から繰り出す長い突き。";
        s.mpCost = 9.0f; s.cooldown = 3.8f; s.duration = 0.54f; s.effectStyle = 2; s.effectColor = green;
        s.strikes = { MakeStrike(0.20f, 2.5f, 3.0f, 0.8f, 240.0f, 260.0f, 0.07f) };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1401; s.name = "ワールウィンド"; s.weapon = WeaponType::Spear;
        s.description = "自身を軸に回転し、周囲すべてを薙ぎ払う。";
        s.mpCost = 25.0f; s.cooldown = 10.0f; s.duration = 0.96f; s.effectStyle = 4; s.effectColor = green;
        s.invincibleUntil = 0.70f;
        s.strikes = {
            MakeStrike(0.22f, 1.5f, 2.6f, 1.1f, 80.0f, 100.0f, 0.04f),
            MakeStrike(0.44f, 1.5f, 2.6f, 1.1f, 80.0f, 100.0f, 0.04f),
            MakeStrike(0.66f, 2.8f, 2.8f, 1.2f, 340.0f, 140.0f, 0.12f),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1402; s.name = "チャージランス"; s.weapon = WeaponType::Spear;
        s.description = "穂先を構えたまま突撃する。";
        s.mpCost = 20.0f; s.cooldown = 8.5f; s.duration = 0.80f; s.effectStyle = 2; s.effectColor = gold;
        s.invincibleUntil = 0.52f;
        s.strikes = {
            MakeStrike(0.24f, 1.8f, 3.0f, 1.0f, 120.0f, 880.0f, 0.05f),
            MakeStrike(0.40f, 1.8f, 3.0f, 1.0f, 120.0f, 700.0f, 0.05f),
            MakeStrike(0.56f, 3.0f, 3.2f, 1.1f, 380.0f, 420.0f, 0.13f),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1403; s.name = "ドラグーンダイブ"; s.weapon = WeaponType::Spear;
        s.description = "天へ跳び、真下へ落下貫通する大技。";
        s.mpCost = 35.0f; s.cooldown = 14.5f; s.duration = 1.20f; s.effectStyle = 4; s.effectColor = violet;
        s.invincibleUntil = 0.72f;
        s.strikes = {
            MakeStrike(0.78f, 5.0f, 2.4f, 1.8f, 300.0f, 200.0f, 0.15f),
            MakeStrike(0.86f, 2.4f, 4.0f, 0.6f, 280.0f, 0.0f, 0.06f, true),
        };
        skills_.push_back(s);
    }
}

const SkillDatabase& SkillDatabase::Instance()
{
    static SkillDatabase instance;
    return instance;
}

const SwordSkill* SkillDatabase::Find(int id) const
{
    for (const SwordSkill& skill : skills_) {
        if (skill.id == id) return &skill;
    }
    return nullptr;
}

std::vector<const SwordSkill*> SkillDatabase::ForWeapon(WeaponType weapon) const
{
    std::vector<const SwordSkill*> result;
    for (const SwordSkill& skill : skills_) {
        if (skill.weapon == weapon) result.push_back(&skill);
    }
    return result;
}

std::vector<int> SkillDatabase::DefaultLoadout(WeaponType weapon) const
{
    std::vector<int> loadout;
    for (const SwordSkill* skill : ForWeapon(weapon)) {
        loadout.push_back(skill->id);
        if (loadout.size() >= 4) break;
    }
    while (loadout.size() < 4) loadout.push_back(0);
    return loadout;
}

} // namespace ecl
