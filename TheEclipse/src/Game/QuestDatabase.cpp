#include "Game/QuestDatabase.h"

#include "Common/MathUtil.h"
#include "Game/ItemDatabase.h"
#include "Game/UniqueSkill.h"

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
            { 100, 0.02f,   0,  40  },
            { 200, 0.02f,   0,  40  },
            { 210, 0.02f,   0,  40  },
            { 250, 0.016f,   0,  40  },
        };
        quest.bossDrops = {
            { 101, 0.11f,  20,  60 },
            { 111, 0.06f,  20,  60 },
            { 121, 0.06f,  20,  60 },
            { 131, 0.06f,  20,  60 },
            { 141, 0.06f,  20,  60 },
            { 201, 0.09f,  20,  60 },
            { 211, 0.09f,  20,  60 },
            { 221, 0.07f,  20,  60 },
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
            { 101, 0.02f,   0,  60 },
            { 201, 0.02f,   0,  60 },
            { 211, 0.02f,   0,  60 },
            { 231, 0.02f,   0,  40  },
        };
        quest.bossDrops = {
            { 102, 0.08f,  40,  80 },
            { 112, 0.06f,  40,  80 },
            { 122, 0.06f,  40,  80 },
            { 132, 0.06f,  40,  80 },
            { 142, 0.06f,  40,  80 },
            { 212, 0.09f,  20,  80 },
            { 221, 0.1f,  20,  60  },
            { 241, 0.08f,  20,  60  },
            { 251, 0.08f,  20,  60  },
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
            { 102, 0.02f,  20,  80 },
            { 203, 0.02f,  20,  80 },
            { 213, 0.02f,  20,  80 },
            { 222, 0.02f,  20,  60  },
        };
        quest.bossDrops = {
            { 102, 0.09f,  60, 100 },
            { 112, 0.07f,  60, 100 },
            { 122, 0.07f,  60, 100 },
            { 132, 0.07f,  60, 100 },
            { 142, 0.07f,  60, 100 },
            { 203, 0.1f,  40, 100 },
            { 213, 0.1f,  40, 100 },
            { 222, 0.09f,  40,  80 },
        };
        quests_.push_back(quest);
    }

    //==========================================================================
    // クエスト 4 : 聖剣の試練（ユニークスキル「神聖剣」専用の特別クエスト）
    //   ボス戦のみ。専用のセット武器が 100% ドロップする。
    //   クエスト選択タブには出さず、スキルツリーから 1 度だけ挑める。
    //==========================================================================
    {
        QuestDef quest;
        quest.id = kHolySwordQuestId;
        quest.name = "聖剣の試練";
        quest.subtitle = "TRIAL OF THE SACRED BLADE";
        quest.description = "神聖剣に選ばれた者だけが立ち入れる聖堂。守護者との一騎討ち。";
        quest.bossName = "聖剣の守護者 セイクリッド・ガーディアン";
        quest.difficulty = 3;
        quest.recommendedPower = 1200;
        quest.enemyPowerScale = 1.0f;
        quest.colReward = 800;
        quest.expReward = 900;
        quest.firstClearCol = 3000;
        quest.special = true;

        {
            FloorDef floor = MakeFloor("試練の間", 3000.0f, StageTheme::Altar);
            floor.bossId = 4;
            quest.floors.push_back(floor);
        }

        // 専用のセット武器は 100% ドロップ（個体値も高めで固定）
        quest.bossDrops = {
            { kHolySwordSwordId,  1.00f, 80, 100 },
            { kHolySwordShieldId, 1.00f, 80, 100 },
        };
        quests_.push_back(quest);
    }

    //==========================================================================
    // クエスト 5 : 竜王の火山
    //   推奨戦力 14,000 の最上位クエスト。5 フロア構成。
    //   ボスドロップは部位ごとに 1 つずつ用意してあり、
    //   1 回の撃破で手に入るのはそのうち 1 部位だけ。
    //==========================================================================
    {
        QuestDef quest;
        quest.id = 5;
        quest.name = "竜王の火山";
        quest.subtitle = "VOLCANO OF THE DRAGON KING";
        quest.description = "溶岩が脈打つ火口。頂にはこの層の主が眠っている。";
        quest.bossName = "竜王 ヴァルグリム・ザ・エンシェントドラゴン";
        quest.difficulty = 5;
        quest.recommendedPower = 14000;
        quest.enemyPowerScale = 1.0f;
        quest.colReward = 4200;
        quest.expReward = 6500;
        quest.firstClearCol = 15000;
        // 何が落ちるかはクエスト詳細に出さない
        quest.hideDrops = true;

        {
            FloorDef floor = MakeFloor("FLOOR 1 - 焼けた裾野", 4200.0f, StageTheme::Volcano);
            floor.spawns = {
                EnemySpawn(1100.0f, 8),
                EnemySpawn(1700.0f, 8),
                EnemySpawn(2400.0f, 9),
                EnemySpawn(3000.0f, 7, 620.0f),
                EnemySpawn(3600.0f, 8),
            };
            quest.floors.push_back(floor);
        }
        {
            FloorDef floor = MakeFloor("FLOOR 2 - 溶岩の回廊", 4400.0f, StageTheme::Volcano);
            floor.spawns = {
                EnemySpawn(1000.0f, 9),
                EnemySpawn(1600.0f, 8),
                EnemySpawn(2100.0f, 7, 660.0f),
                EnemySpawn(2800.0f, 9),
                EnemySpawn(3500.0f, 8),
                EnemySpawn(4000.0f, 7, 600.0f),
            };
            quest.floors.push_back(floor);
        }
        {
            FloorDef floor = MakeFloor("FLOOR 3 - 竜の巣", 4600.0f, StageTheme::Volcano);
            floor.spawns = {
                EnemySpawn(1100.0f, 8),
                EnemySpawn(1500.0f, 8),
                EnemySpawn(2200.0f, 9),
                EnemySpawn(2700.0f, 7, 640.0f),
                EnemySpawn(3300.0f, 9),
                EnemySpawn(4000.0f, 8),
            };
            quest.floors.push_back(floor);
        }
        {
            FloorDef floor = MakeFloor("FLOOR 4 - 火口への道", 4200.0f, StageTheme::Volcano);
            floor.spawns = {
                EnemySpawn(1200.0f, 9),
                EnemySpawn(1800.0f, 7, 600.0f),
                EnemySpawn(2400.0f, 9),
                EnemySpawn(2900.0f, 7, 680.0f),
                EnemySpawn(3500.0f, 9),
            };
            quest.floors.push_back(floor);
        }
        {
            FloorDef floor = MakeFloor("FINAL FLOOR - 竜王の玉座", 3600.0f, StageTheme::Volcano);
            floor.bossId = 5;
            quest.floors.push_back(floor);
        }

        // 雑魚敵はドロップなし
        quest.floorDrops.clear();

        // 部位ごとに 1 つずつ。撃破 1 回で落ちるのはどれか 1 部位だけ。
        quest.bossDrops = {
            { kDragonSwordId,  0.55f, 70, 100 },
            { kDragonHelmId,   0.55f, 70, 100 },
            { kDragonMailId,   0.55f, 70, 100 },
            { kDragonShieldId, 0.55f, 70, 100 },
            { kDragonArmId,    0.55f, 70, 100 },
            { kDragonGloveId,  0.55f, 70, 100 },
            { kDragonBootsId,  0.55f, 70, 100 },
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

int QuestDatabase::RollIv(const DropEntry& entry, int difficulty) const
{
    const int minIv = ClampIv(entry.minIv);
    const int maxIv = ClampIv(entry.maxIv);
    if (maxIv <= minIv) return minIv;

    // 難易度が高いほど多めに引いて、その中の最大値を採用する（上振れしやすくなる）
    const int samples = 1 + math::ClampInt(difficulty, 0, 4);
    int best = minIv;
    for (int i = 0; i < samples; ++i) {
        const int roll = math::RandInt(minIv, maxIv);
        if (roll > best) best = roll;
    }
    return best;
}

std::vector<EquipmentItem> QuestDatabase::RollDrops(const QuestDef& quest, bool bossDefeated,
                                                    int enemiesDefeated, float dropRate) const
{
    std::vector<EquipmentItem> drops;
    const ItemDatabase& items = ItemDatabase::Instance();

    // LUK による倍率（確率は 100% を超えない）
    const float rate = math::MaxF(0.0f, dropRate);
    auto roll = [rate](float chance) {
        return math::RandChance(math::MinF(1.0f, chance * rate));
    };

    // 道中ドロップ（倒した数だけ抽選）
    const int rolls = math::MinI(enemiesDefeated, 12);
    for (int i = 0; i < rolls; ++i) {
        for (const DropEntry& entry : quest.floorDrops) {
            if (!roll(entry.chance)) continue;
            EquipmentItem item = items.Create(entry.templateId, RollIv(entry, quest.difficulty));
            if (item.IsValid()) drops.push_back(item);
            break; // 1 回の抽選につき最大 1 個
        }
    }

    // --- ボスドロップ --------------------------------------------------------
    //   まず「武器 / 頭 / 体 / 盾 / 腕 / 手 / 足」の中から 1 つの部位を抽選し、
    //   その部位の候補だけを引く。頭装備が当たったフロアでは、武器や体装備は
    //   絶対に落ちない。
    //   特別クエスト（ユニークスキル用のセット武器）はセットで渡したいので、
    //   この部位抽選を通さず今までどおり全候補を引く。
    if (bossDefeated && !quest.bossDrops.empty()) {
        if (quest.special) {
            bool gotAny = false;
            for (const DropEntry& entry : quest.bossDrops) {
                if (!roll(entry.chance)) continue;
                EquipmentItem item = items.Create(entry.templateId, RollIv(entry, quest.difficulty));
                if (item.IsValid()) {
                    drops.push_back(item);
                    gotAny = true;
                }
            }
            // 特別クエストだけは必ず 1 個落とす
            if (!gotAny) {
                const DropEntry& entry = quest.bossDrops[0];
                EquipmentItem item = items.Create(entry.templateId, RollIv(entry, quest.difficulty));
                if (item.IsValid()) drops.push_back(item);
            }
        } else {
            // 候補に含まれている部位を集める（武器は左右をまとめて 1 部位とする）
            std::vector<EquipSlot> slots;
            for (const DropEntry& entry : quest.bossDrops) {
                const ItemTemplate* tmpl = items.Find(entry.templateId);
                if (!tmpl) continue;
                const EquipSlot slot = IsWeaponSlot(tmpl->slot) ? EquipSlot::WeaponRight : tmpl->slot;
                bool known = false;
                for (EquipSlot s : slots) known = known || (s == slot);
                if (!known) slots.push_back(slot);
            }

            if (!slots.empty()) {
                const EquipSlot picked =
                    slots[static_cast<size_t>(math::RandInt(0, static_cast<int>(slots.size()) - 1))];

                // 選ばれた部位の候補だけを、定義どおりの確率で引く
                for (const DropEntry& entry : quest.bossDrops) {
                    const ItemTemplate* tmpl = items.Find(entry.templateId);
                    if (!tmpl) continue;
                    const EquipSlot slot =
                        IsWeaponSlot(tmpl->slot) ? EquipSlot::WeaponRight : tmpl->slot;
                    if (slot != picked) continue;
                    if (!roll(entry.chance)) continue;
                    EquipmentItem item = items.Create(entry.templateId,
                                                     RollIv(entry, quest.difficulty));
                    if (item.IsValid()) {
                        drops.push_back(item);
                        break;   // 1 部位につき 1 個まで
                    }
                }
            }
        }
    }

    // 一度に持ち帰れる数は制限する
    if (drops.size() > 12) drops.resize(12);
    return drops;
}

} // namespace ecl
