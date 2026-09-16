//==============================================================================
// EnemyDatabase.h : 雑魚敵の定義
//==============================================================================
#pragma once

#include "Common/Types.h"
#include "Graphics/CharacterArt.h"

#include <string>
#include <vector>

namespace ecl {

struct EnemyDef
{
    int         id = 0;
    std::string name;
    ArtStyle    style = ArtStyle::Beast;
    ColorRGB    main = ColorRGB(90, 90, 110);
    ColorRGB    accent = ColorRGB(210, 210, 230);
    ColorRGB    trim = ColorRGB(255, 120, 120);

    float maxHp = 120.0f;
    float attack = 30.0f;
    float defense = 8.0f;
    float moveSpeed = 180.0f;
    float halfWidth = 34.0f;
    float height = 120.0f;

    float aggroRange = 620.0f;
    float attackRange = 90.0f;
    float attackWindup = 0.45f;   // 予備動作
    float attackRecover = 0.55f;  // 硬直
    float attackCooldown = 1.4f;
    float damageMultiplier = 1.0f;
    float knockback = 260.0f;
    float knockbackResist = 0.0f; // 1.0 で完全耐性

    bool  ranged = false;
    float projectileSpeed = 520.0f;
    bool  floating = false;       // 空中に浮遊する

    int   expReward = 18;
    int   colReward = 24;

    // assets/enemies/<assetFolder> から素材を読み込む（空なら代替表示）
    std::string assetFolder;
};

class EnemyDatabase
{
public:
    static const EnemyDatabase& Instance();

    const EnemyDef* Find(int id) const;
    const std::vector<EnemyDef>& All() const { return enemies_; }

private:
    EnemyDatabase();
    std::vector<EnemyDef> enemies_;
};

} // namespace ecl
