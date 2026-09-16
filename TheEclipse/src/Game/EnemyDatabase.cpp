#include "Game/EnemyDatabase.h"

namespace ecl {

EnemyDatabase::EnemyDatabase()
{
    {
        // 素早いが打たれ弱い
        EnemyDef e;
        e.id = 1; e.name = "フォレストウルフ";
        e.style = ArtStyle::Beast;
        e.main = ColorRGB(72, 84, 76); e.accent = ColorRGB(196, 206, 200); e.trim = ColorRGB(255, 196, 96);
        e.maxHp = 150.0f; e.attack = 34.0f; e.defense = 6.0f; e.moveSpeed = 260.0f;
        e.halfWidth = 46.0f; e.height = 104.0f;
        e.attackRange = 110.0f; e.attackWindup = 0.35f; e.attackRecover = 0.45f; e.attackCooldown = 1.1f;
        e.damageMultiplier = 1.0f; e.knockback = 240.0f;
        e.assetFolder = "wolf";
        e.expReward = 16; e.colReward = 22;
        enemies_.push_back(e);
    }
    {
        // 標準的な近接型
        EnemyDef e;
        e.id = 2; e.name = "コボルド・ソルジャー";
        e.style = ArtStyle::Humanoid;
        e.main = ColorRGB(96, 76, 60); e.accent = ColorRGB(210, 196, 174); e.trim = ColorRGB(255, 140, 90);
        e.maxHp = 240.0f; e.attack = 42.0f; e.defense = 14.0f; e.moveSpeed = 180.0f;
        e.halfWidth = 32.0f; e.height = 140.0f;
        e.attackRange = 120.0f; e.attackWindup = 0.45f; e.attackRecover = 0.55f; e.attackCooldown = 1.5f;
        e.damageMultiplier = 1.1f; e.knockback = 280.0f;
        e.assetFolder = "kobold";
        e.expReward = 24; e.colReward = 32;
        enemies_.push_back(e);
    }
    {
        // 遠距離攻撃
        EnemyDef e;
        e.id = 3; e.name = "ダーク・ウィスプ";
        e.style = ArtStyle::Wisp;
        e.main = ColorRGB(72, 52, 112); e.accent = ColorRGB(196, 168, 255); e.trim = ColorRGB(186, 120, 255);
        e.maxHp = 130.0f; e.attack = 38.0f; e.defense = 4.0f; e.moveSpeed = 130.0f;
        e.halfWidth = 36.0f; e.height = 86.0f;
        e.aggroRange = 780.0f;
        e.attackRange = 620.0f; e.attackWindup = 0.65f; e.attackRecover = 0.7f; e.attackCooldown = 2.2f;
        e.damageMultiplier = 1.0f; e.knockback = 180.0f;
        e.ranged = true; e.projectileSpeed = 460.0f; e.floating = true;
        e.assetFolder = "wisp";
        e.expReward = 22; e.colReward = 30;
        enemies_.push_back(e);
    }
    {
        // 硬く鈍い
        EnemyDef e;
        e.id = 4; e.name = "ストーン・センチネル";
        e.style = ArtStyle::Golem;
        e.main = ColorRGB(94, 92, 104); e.accent = ColorRGB(186, 186, 200); e.trim = ColorRGB(120, 200, 255);
        e.maxHp = 520.0f; e.attack = 52.0f; e.defense = 32.0f; e.moveSpeed = 110.0f;
        e.halfWidth = 48.0f; e.height = 168.0f;
        e.attackRange = 140.0f; e.attackWindup = 0.7f; e.attackRecover = 0.8f; e.attackCooldown = 2.0f;
        e.damageMultiplier = 1.35f; e.knockback = 380.0f; e.knockbackResist = 0.75f;
        e.assetFolder = "sentinel";
        e.expReward = 48; e.colReward = 60;
        enemies_.push_back(e);
    }
    {
        // 手数の多い小型
        EnemyDef e;
        e.id = 5; e.name = "シャドウ・インプ";
        e.style = ArtStyle::Imp;
        e.main = ColorRGB(58, 44, 72); e.accent = ColorRGB(192, 174, 216); e.trim = ColorRGB(255, 96, 140);
        e.maxHp = 180.0f; e.attack = 40.0f; e.defense = 10.0f; e.moveSpeed = 300.0f;
        e.halfWidth = 26.0f; e.height = 108.0f;
        e.attackRange = 96.0f; e.attackWindup = 0.28f; e.attackRecover = 0.34f; e.attackCooldown = 0.9f;
        e.damageMultiplier = 0.85f; e.knockback = 200.0f;
        e.assetFolder = "imp";
        e.expReward = 26; e.colReward = 34;
        enemies_.push_back(e);
    }
    {
        // 蝕のフロア用の上位種
        EnemyDef e;
        e.id = 6; e.name = "イクリプス・レイス";
        e.style = ArtStyle::Knight;
        e.main = ColorRGB(40, 34, 62); e.accent = ColorRGB(206, 196, 255); e.trim = ColorRGB(196, 110, 255);
        e.maxHp = 420.0f; e.attack = 62.0f; e.defense = 26.0f; e.moveSpeed = 220.0f;
        e.halfWidth = 34.0f; e.height = 158.0f;
        e.attackRange = 150.0f; e.attackWindup = 0.42f; e.attackRecover = 0.5f; e.attackCooldown = 1.3f;
        e.damageMultiplier = 1.25f; e.knockback = 300.0f; e.knockbackResist = 0.35f;
        e.assetFolder = "wraith";
        e.expReward = 58; e.colReward = 72;
        enemies_.push_back(e);
    }
}

const EnemyDatabase& EnemyDatabase::Instance()
{
    static EnemyDatabase instance;
    return instance;
}

const EnemyDef* EnemyDatabase::Find(int id) const
{
    for (const EnemyDef& e : enemies_) {
        if (e.id == id) return &e;
    }
    return nullptr;
}

} // namespace ecl
