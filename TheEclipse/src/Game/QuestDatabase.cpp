#include "Game/QuestDatabase.h"

#include "Common/MathUtil.h"
#include "Game/ItemDatabase.h"

namespace ecl {

namespace {

// 地面の高さは全フロア共通
constexpr float kGroundY = 880.0f;

FloorDef MakeFloor(const std::string& name, float width, StageTheme theme)
{
    FloorDef floor;
    floor.name = name;
    floor.width = width;
    floor.groundY = kGroundY;
    floor.theme = theme;
    return floor;
}

} // namespace

QuestDatabase::QuestDatabase()
{
    //==========================================================================
    // クエスト 1 : はじまりの森
    //==========================================================================
    {
        QuestDef quest;
        quest.id = 1;
        quest.name = "はじまりの森";
        quest.subtitle = "FOREST OF DAWN";
        quest.description = "第1層の外周に広がる森。狼と小鬼が徘徊している。";
        quest.bossName = "深緑の狼王 ファングルフ";
        quest.difficulty = 1;
        quest.recommendedPower = 380;
        quest.enemyPowerScale = 1.0f;
        quest.colReward = 420;
        quest.expReward = 260;
        quest.firstClearCol = 1500;

        {
            FloorDef floor = MakeFloor("FLOOR 1 - 森の入口", 3800.0f, StageTheme::Forest);
            floor.spawns = {
                EnemySpawn(1100.0f, 1),
                EnemySpawn(1750.0f, 1),
                EnemySpawn(2400.0f, 2),
                EnemySpawn(3100.0f, 1),
            };
            quest.floors.push_back(floor);
        }
        {
            FloorDef floor = MakeFloor("FLOOR 2 - 深緑の道", 4200.0f, StageTheme::Forest);
            floor.spawns = {
                EnemySpawn(1000.0f, 2),
                EnemySpawn(1600.0f, 1),
                EnemySpawn(2200.0f, 5),
                EnemySpawn(2900.0f, 2),
                EnemySpawn(3400.0f, 1),
            };
            quest.floors.push_back(floor);
        }
        {
            FloorDef floor = MakeFloor("FINAL FLOOR - 狼王の縄張り", 3000.0f, StageTheme::Forest);
            floor.bossId = 1;
            quest.floors.push_back(floor);
        }

        quest.floorDrops = {
            { 100, 0.10f, Rarity::N,  Rarity::R  },
            { 200, 0.10f, Rarity::N,  Rarity::R  },
            { 210, 0.10f, Rarity::N,  Rarity::R  },
            { 250, 0.08f, Rarity::N,  Rarity::R  },
        };
        quest.bossDrops = {
            { 101, 0.55f, Rarity::R,  Rarity::SR },
            { 111, 0.30f, Rarity::R,  Rarity::SR },
            { 121, 0.30f, Rarity::R,  Rarity::SR },
            { 131, 0.30f, Rarity::R,  Rarity::SR },
            { 141, 0.30f, Rarity::R,  Rarity::SR },
            { 201, 0.45f, Rarity::R,  Rarity::SR },
            { 211, 0.45f, Rarity::R,  Rarity::SR },
            { 221, 0.35f, Rarity::R,  Rarity::SR },
        };
        quests_.push_back(quest);
    }

    //==========================================================================
    // クエスト 2 : 石牢の回廊
    //==========================================================================
    {
        QuestDef quest;
        quest.id = 2;
        quest.name = "石牢の回廊";
        quest.subtitle = "CORRIDOR OF STONE";
        quest.description = "遺跡の地下に続く回廊。石の番兵が侵入者を拒む。";
        quest.bossName = "石牢の守護者 ゴーレム・ガルド";
        quest.difficulty = 2;
        quest.recommendedPower = 900;
        quest.enemyPowerScale = 1.55f;
        quest.colReward = 880;
        quest.expReward = 620;
        quest.firstClearCol = 3200;

        {
            FloorDef floor = MakeFloor("FLOOR 1 - 崩れた前室", 4000.0f, StageTheme::Ruins);
            floor.spawns = {
                EnemySpawn(1000.0f, 2),
                EnemySpawn(1700.0f, 5),
                EnemySpawn(2300.0f, 3, 700.0f),
                EnemySpawn(2900.0f, 2),
                EnemySpawn(3400.0f, 4),
            };
            quest.floors.push_back(floor);
        }
        {
            FloorDef floor = MakeFloor("FLOOR 2 - 監視の回廊", 4400.0f, StageTheme::Ruins);
            floor.spawns = {
                EnemySpawn(900.0f, 4),
                EnemySpawn(1500.0f, 3, 680.0f),
                EnemySpawn(2100.0f, 2),
                EnemySpawn(2700.0f, 5),
                EnemySpawn(3300.0f, 3, 700.0f),
                EnemySpawn(3800.0f, 4),
            };
            quest.floors.push_back(floor);
        }
        {
            FloorDef floor = MakeFloor("FLOOR 3 - 番兵の間", 3600.0f, StageTheme::Ruins);
            floor.spawns = {
                EnemySpawn(1200.0f, 4),
                EnemySpawn(1900.0f, 6),
                EnemySpawn(2600.0f, 4),
                EnemySpawn(3000.0f, 5),
            };
            quest.floors.push_back(floor);
        }
        {
            FloorDef floor = MakeFloor("FINAL FLOOR - 守護者の広間", 3200.0f, StageTheme::Ruins);
            floor.bossId = 2;
            quest.floors.push_back(floor);
        }

        quest.floorDrops = {
            { 101, 0.10f, Rarity::N,  Rarity::SR },
            { 201, 0.10f, Rarity::N,  Rarity::SR },
            { 211, 0.10f, Rarity::N,  Rarity::SR },
            { 231, 0.10f, Rarity::N,  Rarity::R  },
        };
        quest.bossDrops = {
            { 102, 0.40f, Rarity::SR, Rarity::SSR },
            { 112, 0.30f, Rarity::SR, Rarity::SSR },
            { 122, 0.30f, Rarity::SR, Rarity::SSR },
            { 132, 0.30f, Rarity::SR, Rarity::SSR },
            { 142, 0.30f, Rarity::SR, Rarity::SSR },
            { 212, 0.45f, Rarity::R,  Rarity::SSR },
            { 221, 0.50f, Rarity::R,  Rarity::SR  },
            { 241, 0.40f, Rarity::R,  Rarity::SR  },
            { 251, 0.40f, Rarity::R,  Rarity::SR  },
        };
        quests_.push_back(quest);
    }

    //==========================================================================
    // クエスト 3 : 蝕の祭壇
    //==========================================================================
    {
        QuestDef quest;
        quest.id = 3;
        quest.name = "蝕の祭壇";
        quest.subtitle = "ALTAR OF THE ECLIPSE";
        quest.description = "月が欠けるとき、祭壇には蝕の騎士が現れる。";
        quest.bossName = "蝕の騎士 エクリプス・ナイト";
        quest.difficulty = 3;
        quest.recommendedPower = 1900;
        quest.enemyPowerScale = 2.35f;
        quest.colReward = 1650;
        quest.expReward = 1250;
        quest.firstClearCol = 6000;

        {
            FloorDef floor = MakeFloor("FLOOR 1 - 蝕の参道", 4200.0f, StageTheme::Altar);
            floor.spawns = {
                EnemySpawn(1000.0f, 6),
                EnemySpawn(1600.0f, 3, 680.0f),
                EnemySpawn(2200.0f, 5),
                EnemySpawn(2800.0f, 6),
                EnemySpawn(3500.0f, 4),
            };
            quest.floors.push_back(floor);
        }
        {
            FloorDef floor = MakeFloor("FLOOR 2 - 流星の回廊", 4600.0f, StageTheme::Altar);
            floor.spawns = {
                EnemySpawn(900.0f, 3, 660.0f),
                EnemySpawn(1400.0f, 6),
                EnemySpawn(2000.0f, 4),
                EnemySpawn(2600.0f, 6),
                EnemySpawn(3200.0f, 3, 680.0f),
                EnemySpawn(3900.0f, 6),
            };
            quest.floors.push_back(floor);
        }
        {
            FloorDef floor = MakeFloor("FLOOR 3 - 祭壇前", 3800.0f, StageTheme::Altar);
            floor.spawns = {
                EnemySpawn(1200.0f, 6),
                EnemySpawn(1800.0f, 4),
                EnemySpawn(2400.0f, 6),
                EnemySpawn(3000.0f, 4),
                EnemySpawn(3300.0f, 5),
            };
            quest.floors.push_back(floor);
        }
        {
            FloorDef floor = MakeFloor("FINAL FLOOR - 蝕の祭壇", 3400.0f, StageTheme::Altar);
            floor.bossId = 3;
            quest.floors.push_back(floor);
        }

        quest.floorDrops = {
            { 102, 0.10f, Rarity::R,  Rarity::SSR },
            { 203, 0.10f, Rarity::R,  Rarity::SSR },
            { 213, 0.10f, Rarity::R,  Rarity::SSR },
            { 222, 0.10f, Rarity::R,  Rarity::SR  },
        };
        quest.bossDrops = {
            { 102, 0.45f, Rarity::SSR, Rarity::UR },
            { 112, 0.35f, Rarity::SSR, Rarity::UR },
            { 122, 0.35f, Rarity::SSR, Rarity::UR },
            { 132, 0.35f, Rarity::SSR, Rarity::UR },
            { 142, 0.35f, Rarity::SSR, Rarity::UR },
            { 203, 0.50f, Rarity::SR,  Rarity::UR },
            { 213, 0.50f, Rarity::SR,  Rarity::UR },
            { 222, 0.45f, Rarity::SR,  Rarity::SSR },
        };
        quests_.push_back(quest);
    }
}

const QuestDatabase& QuestDatabase::Instance()
{
    static QuestDatabase instance;
    return instance;
}

const QuestDef* QuestDatabase::Find(int id) const
{
    for (const QuestDef& quest : quests_) {
        if (quest.id == id) return &quest;
    }
    return nullptr;
}

Rarity QuestDatabase::RollRarity(const DropEntry& entry, int difficulty) const
{
    const int minRarity = static_cast<int>(entry.minRarity);
    const int maxRarity = static_cast<int>(entry.maxRarity);
    if (maxRarity <= minRarity) return entry.minRarity;

    // 上位レアリティほど出にくい。難易度が高いほど上振れしやすい。
    const float upChance = 0.16f + 0.05f * static_cast<float>(difficulty);
    int rarity = minRarity;
    while (rarity < maxRarity && math::RandChance(upChance)) {
        ++rarity;
    }
    return static_cast<Rarity>(math::ClampInt(rarity, 0, static_cast<int>(Rarity::Count) - 1));
}

std::vector<EquipmentItem> QuestDatabase::RollDrops(const QuestDef& quest, bool bossDefeated,
                                                    int enemiesDefeated) const
{
    std::vector<EquipmentItem> drops;
    const ItemDatabase& items = ItemDatabase::Instance();

    // 道中ドロップ（倒した数だけ抽選）
    const int rolls = math::MinI(enemiesDefeated, 12);
    for (int i = 0; i < rolls; ++i) {
        for (const DropEntry& entry : quest.floorDrops) {
            if (!math::RandChance(entry.chance)) continue;
            EquipmentItem item = items.Create(entry.templateId, RollRarity(entry, quest.difficulty));
            if (item.IsValid()) drops.push_back(item);
            break; // 1 回の抽選につき最大 1 個
        }
    }

    // ボスドロップ
    if (bossDefeated) {
        bool gotAny = false;
        for (const DropEntry& entry : quest.bossDrops) {
            if (!math::RandChance(entry.chance)) continue;
            EquipmentItem item = items.Create(entry.templateId, RollRarity(entry, quest.difficulty));
            if (item.IsValid()) {
                drops.push_back(item);
                gotAny = true;
            }
        }
        // 最低 1 個は必ず落とす
        if (!gotAny && !quest.bossDrops.empty()) {
            const DropEntry& entry = quest.bossDrops[0];
            EquipmentItem item = items.Create(entry.templateId, entry.minRarity);
            if (item.IsValid()) drops.push_back(item);
        }
    }

    // 一度に持ち帰れる数は制限する
    if (drops.size() > 12) drops.resize(12);
    return drops;
}

} // namespace ecl
