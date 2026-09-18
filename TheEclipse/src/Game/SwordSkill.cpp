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

    //--- 追加スキル（ツリー上位・下位の補完） -----------------------------------
    {
        SwordSkill s;
        s.id = 1004; s.name = "シャープネイル"; s.weapon = WeaponType::OneHandSword;
        s.description = "爪痕を刻む三連斬り。前進しながら畳み掛ける。";
        s.mpCost = 16.0f; s.cooldown = 6.5f; s.duration = 0.72f; s.effectStyle = 3; s.effectColor = cyan;
        s.strikes = {
            MakeStrike(0.14f, 1.4f, 1.6f, 1.0f, 70.0f, 300.0f, 0.04f),
            MakeStrike(0.30f, 1.4f, 1.6f, 1.0f, 70.0f, 280.0f, 0.04f),
            MakeStrike(0.48f, 2.6f, 1.8f, 1.1f, 320.0f, 300.0f, 0.11f),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1005; s.name = "サイクロンエッジ"; s.weapon = WeaponType::OneHandSword;
        s.description = "身体ごと旋回し、周囲を巻き上げる大技。";
        s.mpCost = 30.0f; s.cooldown = 12.5f; s.duration = 1.00f; s.effectStyle = 4; s.effectColor = gold;
        s.invincibleUntil = 0.58f;
        s.strikes = {
            MakeStrike(0.24f, 1.8f, 2.4f, 1.3f, 90.0f, 160.0f, 0.05f),
            MakeStrike(0.44f, 1.8f, 2.4f, 1.3f, 90.0f, 160.0f, 0.05f),
            MakeStrike(0.66f, 3.8f, 2.6f, 1.4f, 400.0f, 220.0f, 0.14f, true),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1104; s.name = "ダブルハンマー"; s.weapon = WeaponType::OneHandMace;
        s.description = "左右から挟み込む二連打。";
        s.mpCost = 17.0f; s.cooldown = 6.8f; s.duration = 0.80f; s.effectStyle = 1; s.effectColor = crimson;
        s.strikes = {
            MakeStrike(0.22f, 2.0f, 1.6f, 1.1f, 160.0f, 200.0f, 0.07f),
            MakeStrike(0.48f, 2.8f, 1.7f, 1.2f, 340.0f, 200.0f, 0.12f),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1105; s.name = "アースクエイク"; s.weapon = WeaponType::OneHandMace;
        s.description = "地面を殴りつけ、広範囲を揺らして打ち上げる。";
        s.mpCost = 31.0f; s.cooldown = 13.0f; s.duration = 1.05f; s.effectStyle = 4; s.effectColor = gold;
        s.invincibleUntil = 0.50f;
        s.strikes = {
            MakeStrike(0.40f, 3.0f, 1.8f, 1.2f, 200.0f, 120.0f, 0.10f),
            MakeStrike(0.50f, 2.2f, 4.2f, 0.7f, 300.0f, 0.0f, 0.08f, true),
            MakeStrike(0.72f, 2.2f, 4.6f, 0.7f, 340.0f, 0.0f, 0.10f, true),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1204; s.name = "ツインファング"; s.weapon = WeaponType::Dagger;
        s.description = "二本の牙で急所を抉る二連撃。";
        s.mpCost = 15.0f; s.cooldown = 6.0f; s.duration = 0.50f; s.effectStyle = 3; s.effectColor = violet;
        s.strikes = {
            MakeStrike(0.10f, 1.6f, 1.4f, 0.9f, 60.0f, 320.0f, 0.04f),
            MakeStrike(0.26f, 2.4f, 1.5f, 1.0f, 240.0f, 300.0f, 0.10f),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1205; s.name = "サウザンドエッジ"; s.weapon = WeaponType::Dagger;
        s.description = "無数の斬撃で相手を包み込む短剣の極致。";
        s.mpCost = 30.0f; s.cooldown = 12.0f; s.duration = 1.00f; s.effectStyle = 3; s.effectColor = crimson;
        s.invincibleUntil = 0.34f;
        s.strikes = {
            MakeStrike(0.10f, 1.1f, 1.7f, 1.0f, 30.0f, 200.0f, 0.02f),
            MakeStrike(0.20f, 1.1f, 1.7f, 1.0f, 30.0f, 200.0f, 0.02f),
            MakeStrike(0.30f, 1.1f, 1.7f, 1.0f, 30.0f, 200.0f, 0.02f),
            MakeStrike(0.40f, 1.1f, 1.7f, 1.0f, 30.0f, 200.0f, 0.02f),
            MakeStrike(0.52f, 1.3f, 1.8f, 1.1f, 50.0f, 220.0f, 0.03f),
            MakeStrike(0.70f, 3.4f, 2.0f, 1.2f, 360.0f, 280.0f, 0.14f),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1304; s.name = "パラレルスティング"; s.weapon = WeaponType::Rapier;
        s.description = "同じ軌道を二度なぞる高速の刺突。";
        s.mpCost = 15.0f; s.cooldown = 5.8f; s.duration = 0.56f; s.effectStyle = 2; s.effectColor = cyan;
        s.strikes = {
            MakeStrike(0.12f, 1.7f, 2.4f, 0.8f, 80.0f, 380.0f, 0.05f),
            MakeStrike(0.30f, 2.5f, 2.6f, 0.9f, 260.0f, 360.0f, 0.10f),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1305; s.name = "ミラージュエコー"; s.weapon = WeaponType::Rapier;
        s.description = "残像を置き去りにして前後から突き刺す。";
        s.mpCost = 29.0f; s.cooldown = 12.0f; s.duration = 0.96f; s.effectStyle = 3; s.effectColor = green;
        s.invincibleUntil = 0.62f;
        s.strikes = {
            MakeStrike(0.18f, 1.8f, 2.6f, 1.0f, 60.0f, 620.0f, 0.04f),
            MakeStrike(0.38f, 1.8f, 2.6f, 1.0f, 60.0f, -520.0f, 0.04f),
            MakeStrike(0.58f, 2.0f, 2.6f, 1.0f, 80.0f, 620.0f, 0.05f),
            MakeStrike(0.76f, 3.2f, 2.8f, 1.2f, 340.0f, 300.0f, 0.13f),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1404; s.name = "スカイスプリット"; s.weapon = WeaponType::Spear;
        s.description = "穂先で天を裂くように斬り上げる。";
        s.mpCost = 16.0f; s.cooldown = 6.4f; s.duration = 0.70f; s.effectStyle = 0; s.effectColor = green;
        s.strikes = {
            MakeStrike(0.24f, 3.2f, 2.6f, 1.5f, 200.0f, 220.0f, 0.10f, true),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 1405; s.name = "グングニル"; s.weapon = WeaponType::Spear;
        s.description = "投擲の如き一閃。射程の限界まで貫く。";
        s.mpCost = 32.0f; s.cooldown = 13.0f; s.duration = 0.98f; s.effectStyle = 2; s.effectColor = violet;
        s.invincibleUntil = 0.44f;
        s.strikes = {
            MakeStrike(0.30f, 3.4f, 4.0f, 1.0f, 160.0f, 420.0f, 0.10f),
            MakeStrike(0.58f, 4.4f, 4.4f, 1.2f, 420.0f, 260.0f, 0.15f),
        };
        skills_.push_back(s);
    }

    //==========================================================================
    // ユニークスキル「二刀流」専用スキル
    //   requiredUnique を設定したスキルは、そのユニークスキルを習得している間だけ
    //   ツリーに現れ、装備できる。
    //==========================================================================
    {
        SwordSkill s;
        s.id = 9000; s.name = "ダブル・サーキュラー"; s.weapon = WeaponType::OneHandSword;
        s.requiredUnique = UniqueSkillType::DualWield;
        s.description = "二本の刃で円を描くように薙ぎ払う三連撃。";
        s.mpCost = 18.0f; s.cooldown = 7.0f; s.duration = 0.78f; s.effectStyle = 4; s.effectColor = cyan;
        s.invincibleUntil = 0.34f;
        s.strikes = {
            MakeStrike(0.16f, 1.8f, 2.0f, 1.1f, 90.0f, 200.0f, 0.05f),
            MakeStrike(0.34f, 1.8f, 2.0f, 1.1f, 90.0f, 200.0f, 0.05f),
            MakeStrike(0.56f, 3.0f, 2.2f, 1.2f, 340.0f, 240.0f, 0.12f),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 9001; s.name = "スターバースト・ストリーム"; s.weapon = WeaponType::OneHandSword;
        s.requiredUnique = UniqueSkillType::DualWield;
        s.description = "十六連撃の後に渾身の一撃を叩き込む、二刀流の到達点。";
        s.mpCost = 38.0f; s.cooldown = 16.0f; s.duration = 1.45f; s.effectStyle = 3; s.effectColor = violet;
        s.invincibleUntil = 0.60f;
        s.strikes = {
            MakeStrike(0.10f, 1.0f, 1.8f, 1.0f, 20.0f, 220.0f, 0.02f),
            MakeStrike(0.20f, 1.0f, 1.8f, 1.0f, 20.0f, 220.0f, 0.02f),
            MakeStrike(0.30f, 1.0f, 1.8f, 1.0f, 20.0f, 220.0f, 0.02f),
            MakeStrike(0.40f, 1.0f, 1.8f, 1.0f, 20.0f, 220.0f, 0.02f),
            MakeStrike(0.50f, 1.1f, 1.9f, 1.0f, 20.0f, 220.0f, 0.02f),
            MakeStrike(0.60f, 1.1f, 1.9f, 1.0f, 20.0f, 220.0f, 0.02f),
            MakeStrike(0.70f, 1.2f, 1.9f, 1.1f, 20.0f, 220.0f, 0.03f),
            MakeStrike(0.80f, 1.2f, 1.9f, 1.1f, 20.0f, 220.0f, 0.03f),
            MakeStrike(0.90f, 1.3f, 2.0f, 1.1f, 30.0f, 220.0f, 0.03f),
            MakeStrike(1.00f, 1.3f, 2.0f, 1.1f, 30.0f, 220.0f, 0.03f),
            MakeStrike(1.12f, 2.0f, 2.1f, 1.2f, 60.0f, 260.0f, 0.05f),
            MakeStrike(1.26f, 4.2f, 2.4f, 1.3f, 420.0f, 300.0f, 0.16f, true),
        };
        skills_.push_back(s);
    }
    {
        SwordSkill s;
        s.id = 9002; s.name = "ジ・イクリプス"; s.weapon = WeaponType::OneHandSword;
        s.requiredUnique = UniqueSkillType::DualWield;
        s.description = "月を裂く二条の光。溜めは長いが一撃の威力は絶大。";
        s.mpCost = 46.0f; s.cooldown = 22.0f; s.duration = 1.30f; s.effectStyle = 0; s.effectColor = gold;
        s.invincibleUntil = 0.72f;
        s.strikes = {
            MakeStrike(0.40f, 2.4f, 2.6f, 1.4f, 120.0f, 320.0f, 0.08f),
            MakeStrike(0.62f, 2.4f, 2.6f, 1.4f, 120.0f, 300.0f, 0.08f),
            MakeStrike(0.88f, 7.0f, 3.0f, 1.8f, 520.0f, 360.0f, 0.22f, true),
        };
        skills_.push_back(s);
    }

    //--- スキルツリーの構成 ------------------------------------------------------
    //   column 0 : 火力特化系統 / column 1 : 範囲・機動系統
    //   tier 1 の左側は初期解放（コスト 0）
    struct TreeEntry
    {
        int id;
        int column;
        int tier;
        int cost;
        int require;
    };
    static const TreeEntry kTree[] = {
        // 片手剣
        { 1000, 0, 1, 0, 0    }, { 1004, 0, 2, 1, 1000 }, { 1003, 0, 3, 3, 1004 },
        { 1002, 1, 1, 1, 0    }, { 1001, 1, 2, 2, 1002 }, { 1005, 1, 3, 3, 1001 },
        // 片手棍
        { 1100, 0, 1, 0, 0    }, { 1104, 0, 2, 1, 1100 }, { 1103, 0, 3, 3, 1104 },
        { 1102, 1, 1, 1, 0    }, { 1101, 1, 2, 2, 1102 }, { 1105, 1, 3, 3, 1101 },
        // 短剣
        { 1200, 0, 1, 0, 0    }, { 1204, 0, 2, 1, 1200 }, { 1202, 0, 3, 3, 1204 },
        { 1201, 1, 1, 1, 0    }, { 1203, 1, 2, 2, 1201 }, { 1205, 1, 3, 3, 1203 },
        // 細剣
        { 1300, 0, 1, 0, 0    }, { 1304, 0, 2, 1, 1300 }, { 1303, 0, 3, 3, 1304 },
        { 1302, 1, 1, 1, 0    }, { 1301, 1, 2, 2, 1302 }, { 1305, 1, 3, 3, 1301 },
        // 槍
        { 1400, 0, 1, 0, 0    }, { 1404, 0, 2, 1, 1400 }, { 1403, 0, 3, 3, 1404 },
        { 1402, 1, 1, 1, 0    }, { 1401, 1, 2, 2, 1402 }, { 1405, 1, 3, 3, 1401 },
        // ユニークスキル「二刀流」専用（1 列 3 段）
        { 9000, 0, 1, 1, 0    }, { 9001, 0, 2, 2, 9000 }, { 9002, 0, 3, 3, 9001 },
    };

    for (const TreeEntry& entry : kTree) {
        for (SwordSkill& skill : skills_) {
            if (skill.id != entry.id) continue;
            skill.column = entry.column;
            skill.tier = entry.tier;
            skill.unlockCost = entry.cost;
            skill.requiredSkillId = entry.require;
            break;
        }
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
        if (skill.IsUnique()) continue;
        if (skill.weapon == weapon) result.push_back(&skill);
    }
    return result;
}

std::vector<const SwordSkill*> SkillDatabase::ForUnique(UniqueSkillType type) const
{
    std::vector<const SwordSkill*> result;
    if (type == UniqueSkillType::None) return result;

    for (int tier = 1; tier <= kSkillTreeTiers; ++tier) {
        for (const SwordSkill& skill : skills_) {
            if (skill.requiredUnique == type && skill.tier == tier) result.push_back(&skill);
        }
    }
    return result;
}

std::vector<const SwordSkill*> SkillDatabase::TreeForWeapon(WeaponType weapon) const
{
    std::vector<const SwordSkill*> nodes;
    for (int column = 0; column < kSkillTreeColumns; ++column) {
        for (int tier = 1; tier <= kSkillTreeTiers; ++tier) {
            nodes.push_back(NodeAt(weapon, column, tier));
        }
    }
    return nodes;
}

const SwordSkill* SkillDatabase::NodeAt(WeaponType weapon, int column, int tier) const
{
    for (const SwordSkill& skill : skills_) {
        if (skill.IsUnique()) continue;
        if (skill.weapon == weapon && skill.column == column && skill.tier == tier) return &skill;
    }
    return nullptr;
}

std::vector<int> SkillDatabase::StarterSkillIds() const
{
    std::vector<int> ids;
    for (const SwordSkill& skill : skills_) {
        if (skill.IsStarter()) ids.push_back(skill.id);
    }
    return ids;
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
