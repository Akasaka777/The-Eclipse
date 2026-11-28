#include "Game/Roguelike.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Game/Boss.h"
#include "Game/EnemyDatabase.h"
#include "Game/GameContext.h"
#include "Game/ItemDatabase.h"
#include "Game/SwordSkill.h"

#include <algorithm>
#include <cmath>

namespace ecl {
namespace rogue {

//==============================================================================
// 乱数（xorshift32）
//==============================================================================
unsigned int Rng::Next()
{
    unsigned int x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state = x ? x : 0x9e3779b9u;
    return state;
}

float Rng::Next01()
{
    return static_cast<float>(Next() % 1000000u) / 1000000.0f;
}

int Rng::Range(int lo, int hi)
{
    if (hi <= lo) return lo;
    return lo + static_cast<int>(Next() % static_cast<unsigned int>(hi - lo + 1));
}

float Rng::Range(float lo, float hi)
{
    return lo + (hi - lo) * Next01();
}

namespace {

constexpr float kGroundY = 880.0f;

//------------------------------------------------------------------------------
// 階層ごとの見た目・敵・ボス
//------------------------------------------------------------------------------
struct AreaTheme
{
    StageTheme  theme;
    const char* name;
};

// 階層ごとに 2 つの見た目からどちらかを選ぶ
const AreaTheme kAreaThemes[kAreaCount][2] = {
    { { StageTheme::Forest, "迷いの森" },      { StageTheme::Ruins, "崩れた回廊" } },
    { { StageTheme::Altar, "蝕の聖堂" },       { StageTheme::Volcano, "灼熱の坑道" } },
    { { StageTheme::Sanctum, "金色の伽藍" },   { StageTheme::Throne, "終焉の回廊" } },
};

// 出てくる雑魚敵（0 は空き）
const int kAreaEnemies[kAreaCount][5] = {
    { 1, 2, 3, 0, 0 },                                     // ウルフ・コボルド・ウィスプ
    { 2, 4, 5, 0, 0 },                                     // コボルド・センチネル・インプ
    { 5, 6, kRoguelikeEnemyIdBase + 7, kRoguelikeEnemyIdBase + 8, kRoguelikeEnemyIdBase + 9 },
};

// 階層の最後のボス（ローグライク用の写しの ID）
const int kAreaBosses[kAreaCount][3] = {
    { kRoguelikeBossIdBase + 1, kRoguelikeBossIdBase + 2, 0 },
    { kRoguelikeBossIdBase + 3, kRoguelikeBossIdBase + 4, kRoguelikeBossIdBase + kWorldEaterBossId },
    { kRoguelikeBossIdBase + 5, kRoguelikeBossIdBase + kAbyssDragonBossId, kRoguelikeBossIdBase + kArcXenosBossId },
};

template <size_t N>
int PickNonZero(const int (&ids)[N], Rng& rng)
{
    int count = 0;
    for (int id : ids) if (id != 0) ++count;
    int pick = rng.Range(0, count - 1);
    for (int id : ids) {
        if (id == 0) continue;
        if (pick-- == 0) return id;
    }
    return ids[0];
}

//------------------------------------------------------------------------------
// 消費アイテム（落ちるもの・ショップの値段）
//------------------------------------------------------------------------------
struct ConsumableEntry
{
    int id;
    int weight[kAreaCount];   // 階層ごとの出やすさ
    int price;                // ショップの値段（ラン中の col）
};

const ConsumableEntry kConsumables[] = {
    { kItemPotion,      { 50, 20, 8 },   40 },
    { kItemHiPotion,    { 6, 34, 30 },  120 },
    { kItemHealCrystal, { 0, 4, 12 },   380 },
    { kItemManaPotion,  { 18, 14, 14 },  60 },
    { kItemElixir,      { 0, 4, 10 },   360 },
    { kItemPowerTonic,  { 8, 8, 7 },    120 },
    { kItemGuardTonic,  { 8, 8, 7 },    120 },
    { kItemSwiftTonic,  { 8, 4, 4 },     90 },
    { kItemCritTonic,   { 2, 4, 5 },    150 },
    { kItemHeroTonic,   { 0, 0, 3 },    360 },
};

int RandomConsumable(Rng& rng, int area)
{
    int total = 0;
    for (const ConsumableEntry& e : kConsumables) total += e.weight[area];
    int roll = rng.Range(0, total - 1);
    for (const ConsumableEntry& e : kConsumables) {
        roll -= e.weight[area];
        if (roll < 0) return e.id;
    }
    return kItemPotion;
}

int ConsumablePrice(int id)
{
    for (const ConsumableEntry& e : kConsumables) {
        if (e.id == id) return e.price;
    }
    return 100;
}

//------------------------------------------------------------------------------
// 装備（武器はランの武器種だけ。スキルが武器種ごとなので、持ち替えて消えないように）
//------------------------------------------------------------------------------
EquipmentItem RandomGear(Rng& rng, int tier, int ivMin, int ivMax, WeaponType weapon)
{
    std::vector<const ItemTemplate*> pool;
    for (const ItemTemplate& t : ItemDatabase::Instance().Templates()) {
        if (t.special || t.tier != tier) continue;
        if (t.slot == EquipSlot::Accessory) continue;
        if (IsWeaponSlot(t.slot) && t.weaponType != weapon) continue;
        pool.push_back(&t);
    }
    if (pool.empty()) return EquipmentItem();
    const ItemTemplate* pick = pool[static_cast<size_t>(rng.Range(0, static_cast<int>(pool.size()) - 1))];
    return ItemDatabase::Instance().Create(pick->id, rng.Range(ivMin, ivMax));
}

// ラスボスの迷宮の装備（武器はランの武器種、または防具 6 部位のどれか）
EquipmentItem RandomLabyrinthItem(Rng& rng, WeaponType weapon)
{
    static const int kWeapons[] = { kLabyrinthSwordId, kLabyrinthMaceId, kLabyrinthDaggerId, kLabyrinthRapierId,
                                    kLabyrinthSpearId };
    static const int kArmors[] = { kLabyrinthHelmId, kLabyrinthMailId, kLabyrinthShieldId, kLabyrinthArmId,
                                   kLabyrinthGloveId, kLabyrinthBootsId };
    int id = kArmors[0];
    const int pick = rng.Range(0, 6);
    if (pick == 6) {
        for (int w : kWeapons) {
            const ItemTemplate* t = ItemDatabase::Instance().Find(w);
            if (t && t->weaponType == weapon) id = w;
        }
    } else {
        id = kArmors[pick];
    }
    return ItemDatabase::Instance().Create(id, rng.Range(90, 100));
}

//------------------------------------------------------------------------------
// フロアの生成
//------------------------------------------------------------------------------
FloorDef MakeFloor(const std::string& name, float width, StageTheme theme)
{
    FloorDef floor;
    floor.name = name;
    floor.width = width;
    floor.groundY = kGroundY;
    floor.theme = theme;
    return floor;
}

void AddPlatforms(FloorDef& floor, Rng& rng)
{
    // 1〜3 個。フロアを区切った中にばらして置く（スタート地点とゲートの前は空ける）
    const int count = rng.Range(1, 3);
    const float left = 800.0f;
    const float right = floor.width - 800.0f;
    const float span = (right - left) / static_cast<float>(count);
    for (int i = 0; i < count; ++i) {
        const float w = std::round(rng.Range(240.0f, 400.0f) / 10.0f) * 10.0f;
        const float x = left + span * static_cast<float>(i) + rng.Range(0.0f, math::MaxF(0.0f, span - w));
        const float top = floor.groundY - std::round(rng.Range(150.0f, 210.0f));
        floor.platforms.push_back(Platform(x, top, w, 26.0f));
    }
}

void AddEnemies(FloorDef& floor, int area, Rng& rng)
{
    const int count = 5 + area + rng.Range(0, 2);
    const float left = 900.0f;
    const float right = floor.width - 450.0f;
    for (int i = 0; i < count; ++i) {
        const float x = left + (right - left) * (static_cast<float>(i) + rng.Range(0.15f, 0.85f))
                      / static_cast<float>(count);
        floor.spawns.push_back(EnemySpawn(std::round(x), PickNonZero(kAreaEnemies[area], rng)));
    }
}

// 施設を置く（ほかの施設から 500px 以上離す）
void AddFacility(FloorDef& floor, FacilityKind kind, Rng& rng)
{
    const float lo = 700.0f;
    const float hi = floor.width - 900.0f;
    float x = std::round(rng.Range(lo, hi));
    for (int tries = 0; tries < 8; ++tries) {
        bool clash = false;
        for (const FacilitySpot& other : floor.facilities) {
            if (std::fabs(other.x - x) < 500.0f) clash = true;
        }
        if (!clash) break;
        x += 620.0f;
        if (x > hi) x = lo + (x - hi);
    }
    FacilitySpot spot;
    spot.kind = kind;
    spot.x = x;
    floor.facilities.push_back(spot);
}

} // namespace

//==============================================================================
// 解放
//==============================================================================
bool IsUnlocked(const PlayerData& player)
{
    for (const QuestDef& quest : QuestDatabase::Instance().Quests()) {
        if (quest.special) continue;
        if (!player.IsQuestCleared(quest.id)) return false;
    }
    return true;
}

void MarkAllQuestsCleared(PlayerData& player)
{
    for (const QuestDef& quest : QuestDatabase::Instance().Quests()) player.MarkQuestCleared(quest.id);
}

int AreaOf(int floorIndex)
{
    return math::ClampInt(floorIndex / kFloorsPerArea, 0, kAreaCount - 1);
}

bool IsBossFloor(int floorIndex)
{
    return floorIndex % kFloorsPerArea == kFloorsPerArea - 1;
}

bool IsLastFloor(int floorIndex)
{
    return floorIndex == kFloorCount - 1;
}

//==============================================================================
// マップの生成
//==============================================================================
QuestDef GenerateRun(unsigned int seed, WeaponType weapon)
{
    (void)weapon;
    Rng rng(seed);
    QuestDef quest;
    quest.id = kQuestId;
    quest.name = "ローグライク";
    quest.subtitle = "LABYRINTH";
    quest.description = "毎回形の変わる迷宮。倒れたらすべてを失う。";
    quest.difficulty = 5;
    quest.enemyPowerScale = 1.0f;
    quest.colReward = 0;
    quest.expReward = 0;
    quest.firstClearCol = 0;
    // 階層のボスを倒したらゲートが開いて次の階層へ（最後のボスを倒したらクリア）
    quest.bossRush = true;

    for (int area = 0; area < kAreaCount; ++area) {
        const AreaTheme& theme = kAreaThemes[area][rng.Range(0, 1)];

        // 戦闘フロア
        for (int f = 0; f < kBattleFloorsPerArea; ++f) {
            const float width = std::round(rng.Range(3400.0f, 5000.0f) / 100.0f) * 100.0f;
            FloorDef floor = MakeFloor(str::Format("第%d階層 %dF - %s", area + 1, f + 1, theme.name), width,
                                       theme.theme);
            floor.enemyPowerScale = kAreaEnemyScale[area] * (1.0f + kFloorScaleStep * static_cast<float>(f));
            AddPlatforms(floor, rng);
            AddEnemies(floor, area, rng);
            quest.floors.push_back(floor);
        }

        // ショップと鍛冶屋は、階層ごとにどれかの戦闘フロアに必ず 1 つずつ。さらにときどきもう 1 つ
        const size_t first = static_cast<size_t>(area * kFloorsPerArea);
        AddFacility(quest.floors[first + static_cast<size_t>(rng.Range(0, kBattleFloorsPerArea - 1))],
                    FacilityKind::Shop, rng);
        AddFacility(quest.floors[first + static_cast<size_t>(rng.Range(0, kBattleFloorsPerArea - 1))],
                    FacilityKind::Smith, rng);
        for (int f = 0; f < kBattleFloorsPerArea; ++f) {
            if (!rng.Chance(0.25f)) continue;
            FloorDef& floor = quest.floors[first + static_cast<size_t>(f)];
            if (floor.facilities.size() >= 2) continue;
            AddFacility(floor, rng.Chance(0.5f) ? FacilityKind::Shop : FacilityKind::Smith, rng);
        }

        // ボスフロア
        const bool last = (area == kAreaCount - 1);
        FloorDef boss = MakeFloor(str::Format("第%d階層 最深部 - %s", area + 1, last ? "迷宮の主の間" : "守護者の間"),
                                  3200.0f, theme.theme);
        boss.bossId = PickNonZero(kAreaBosses[area], rng);
        boss.bossHpBars = kAreaBossHpBars[area];
        boss.enemyPowerScale = 1.0f;   // ローグライク用の写しはもとから階層に合わせた強さ
        quest.floors.push_back(boss);
        if (last) {
            if (const BossDef* def = BossDatabase::Instance().Find(boss.bossId)) quest.bossName = def->name;
        }
    }
    quest.recommendedPower = 0;
    return quest;
}

//==============================================================================
// 開始・終了
//==============================================================================
void StartRun(GameContext& context, WeaponType weapon, unsigned int seed)
{
    RunState& run = context.roguelike;
    run = RunState();
    run.mainPlayer = context.player;
    run.mainSelectedQuestId = context.selectedQuestId;
    run.active = true;
    run.seed = seed;
    run.weapon = weapon;
    run.rng = Rng(seed * 2654435761u + 1u);
    run.quest = GenerateRun(seed, weapon);
    run.deepestFloor = 1;
    // 最初に倒した雑魚敵は必ずスキルを落とす（スキル無しで始まるため）
    run.killsSinceSkill = kSkillPityKills - 1;

    // 本編からは何も持ち込めない：Lv 1・初期装備・回復アイテム少しだけの新しいキャラ
    PlayerData fresh;
    fresh.SetName(run.mainPlayer.Name());
    fresh.SetupNewGame(weapon);
    fresh.ClearSkillsForRun();
    Inventory& inventory = fresh.GetInventory();
    inventory.SetCurrency(kStartCol, kStartCrystals);
    const int potions = inventory.ItemCount(kStarterPotionId);
    if (potions > kStartPotions) {
        for (int i = potions; i > kStartPotions; --i) inventory.ConsumeItem(kStarterPotionId);
    } else if (potions < kStartPotions) {
        inventory.AddItemCount(kStarterPotionId, kStartPotions - potions);
    }
    AutoAssignQuickSlots(inventory);

    context.player = fresh;
    context.selectedQuestId = kQuestId;
}

std::vector<EquipmentItem> EndRun(GameContext& context, bool cleared, int floorReached)
{
    RunState& run = context.roguelike;
    std::vector<EquipmentItem> carried;
    if (!run.active) return carried;
    if (cleared) carried = run.carryHome;

    // 本編のキャラに戻す（ランのキャラと持ち物はすべて捨てる）
    context.player = run.mainPlayer;
    context.selectedQuestId = run.mainSelectedQuestId;
    context.player.GetInventory().AddItems(carried);
    context.player.RecordRoguelikeRun(cleared, floorReached);

    run.active = false;
    run.mainPlayer = PlayerData();
    run.shops.clear();
    return carried;
}

void AutoAllocate(PlayerData& player)
{
    static const Ability kOrder[] = { Ability::Str, Ability::Vit, Ability::Str, Ability::Agi, Ability::Vit };
    int guard = 0;
    while (player.AbilityPoints() > 0 && guard++ < 1000) {
        int spent = 0;
        for (int i = 0; i < kAbilityCount; ++i) spent += player.Abilities().values[i];
        player.SpendAbilityPoint(kOrder[spent % 5]);
    }
}

void AutoAssignQuickSlots(Inventory& inventory)
{
    const struct { QuickSlot slot; ConsumableKind kind; } kSlots[] = {
        { QuickSlot::Recovery, ConsumableKind::Recovery },
        { QuickSlot::Buff, ConsumableKind::Buff },
    };
    for (const auto& s : kSlots) {
        const int current = inventory.QuickItem(s.slot);
        if (current != 0 && inventory.ItemCount(current) > 0) continue;
        int best = 0;
        int bestCount = 0;
        for (int id : inventory.OwnedItemIds(s.kind)) {
            const int count = inventory.ItemCount(id);
            if (count > bestCount) {
                best = id;
                bestCount = count;
            }
        }
        if (best != 0) inventory.SetQuickItem(s.slot, best);
    }
}

//==============================================================================
// スキル
//==============================================================================
int OwnedSkillCount(const PlayerData& player)
{
    int count = 0;
    for (int i = 0; i < player.SkillSlotLimit(); ++i) {
        if (player.SkillAt(i)) ++count;
    }
    return count;
}

std::vector<int> DraftSkills(const PlayerData& player, WeaponType weapon, int count, Rng& rng)
{
    std::vector<int> pool;
    for (const SwordSkill* skill : SkillDatabase::Instance().ForWeapon(weapon)) {
        if (player.IsSkillEquipped(skill->id) && player.SkillLevel(skill->id) >= kMaxSkillLevel) continue;
        pool.push_back(skill->id);
    }
    // 並べ替えてから先頭を取る
    for (size_t i = pool.size(); i > 1; --i) {
        const size_t j = static_cast<size_t>(rng.Range(0, static_cast<int>(i) - 1));
        std::swap(pool[i - 1], pool[j]);
    }
    if (static_cast<int>(pool.size()) > count) pool.resize(static_cast<size_t>(count));
    return pool;
}

SkillPick ApplySkillPick(PlayerData& player, int skillId, int replaceSlot)
{
    if (!SkillDatabase::Instance().Find(skillId)) return SkillPick::Invalid;

    // 持っているスキル：Lv を上げる
    if (player.IsSkillEquipped(skillId)) {
        const int level = player.SkillLevel(skillId);
        if (level >= kMaxSkillLevel) return SkillPick::Maxed;
        player.SetSkillLevel(skillId, level + 1);
        return SkillPick::LevelUp;
    }

    // 新しいスキル：空いているスロットへ。埋まっていれば選んだスロットのスキルと入れ替える
    int slot = -1;
    for (int i = 0; i < player.SkillSlotLimit(); ++i) {
        if (!player.SkillAt(i)) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        if (replaceSlot < 0 || replaceSlot >= player.SkillSlotLimit()) return SkillPick::NeedSlot;
        if (const SwordSkill* old = player.SkillAt(replaceSlot)) player.RevokeSkill(old->id);
        slot = replaceSlot;
    }
    player.GrantSkill(skillId);
    player.SetSkillLevel(skillId, 1);
    if (!player.SetSkillAt(slot, skillId)) return SkillPick::Invalid;
    return SkillPick::Learned;
}

//==============================================================================
// ドロップ
//==============================================================================
DropRoll RollMobDrop(RunState& run, int floorIndex)
{
    DropRoll drop;
    Rng& rng = run.rng;
    const int area = AreaOf(floorIndex);
    drop.col = static_cast<int>(std::lround((10.0f + 4.0f * static_cast<float>(floorIndex)) * rng.Range(0.8f, 1.2f)));
    if (rng.Chance(kMobItemChance)) drop.items.push_back(ItemStack{ RandomConsumable(rng, area), 1 });
    if (rng.Chance(kMobCrystalChance)) drop.crystals = rng.Chance(0.25f) ? 2 : 1;
    ++run.killsSinceSkill;
    if (rng.Chance(kMobSkillChance) || run.killsSinceSkill >= kSkillPityKills) {
        drop.skill = true;
        run.killsSinceSkill = 0;
    }
    return drop;
}

DropRoll RollBossDrop(RunState& run, int floorIndex, float dropRate)
{
    DropRoll drop;
    Rng& rng = run.rng;
    const int area = AreaOf(floorIndex);
    static const int kBossCol[kAreaCount] = { 300, 700, 1500 };
    drop.col = static_cast<int>(std::lround(static_cast<float>(kBossCol[area]) * rng.Range(0.9f, 1.1f)));
    for (int i = 0; i < 2; ++i) drop.items.push_back(ItemStack{ RandomConsumable(rng, area), 1 });
    drop.crystals = 4 + area * 2 + rng.Range(0, 2);
    drop.skill = true;
    run.killsSinceSkill = 0;

    if (!IsLastFloor(floorIndex)) {
        // 階層のボス：装備を 1 つ（ときどき 2 つ）
        const int tier = (area == 0) ? 2 : 3;
        drop.gear.push_back(RandomGear(rng, tier, 50, 85, run.weapon));
        if (rng.Chance(kAreaBossSecondGearChance)) drop.gear.push_back(RandomGear(rng, tier, 50, 85, run.weapon));
        drop.gear.erase(std::remove_if(drop.gear.begin(), drop.gear.end(),
                                       [](const EquipmentItem& item) { return !item.IsValid(); }),
                        drop.gear.end());
    } else if (rng.Chance(math::Clamp(kLastBossGearChance * dropRate, 0.0f, 1.0f))) {
        // ラスボス：低い確率で迷宮の装備（クリアしたら本編へ持ち帰る）
        drop.carryHome.push_back(RandomLabyrinthItem(rng, run.weapon));
    }
    return drop;
}

//==============================================================================
// ショップ
//==============================================================================
ShopStock& ShopAt(RunState& run, int floorIndex, int spotIndex)
{
    for (ShopStock& shop : run.shops) {
        if (shop.floorIndex == floorIndex && shop.spotIndex == spotIndex) return shop;
    }
    ShopStock shop;
    shop.floorIndex = floorIndex;
    shop.spotIndex = spotIndex;
    Rng& rng = run.rng;
    const int area = AreaOf(floorIndex);

    // 回復・バフアイテム 4 種（各 3 個まで）
    std::vector<int> ids;
    for (int tries = 0; tries < 40 && ids.size() < 4; ++tries) {
        const int id = RandomConsumable(rng, area);
        if (std::find(ids.begin(), ids.end(), id) == ids.end()) ids.push_back(id);
    }
    for (int id : ids) {
        ShopOffer offer;
        offer.kind = OfferKind::Consumable;
        offer.itemId = id;
        offer.price = ConsumablePrice(id);
        offer.stock = 3;
        shop.offers.push_back(offer);
    }
    // 装備 2 つ（階層に合った段・ランの武器種）
    static const int kTierPrice[4] = { 0, 220, 480, 900 };
    const int tier = math::ClampInt(area + 1, 1, 3);
    for (int i = 0; i < 2; ++i) {
        EquipmentItem item = RandomGear(rng, tier, 30 + area * 5, 60 + area * 5, run.weapon);
        if (!item.IsValid()) continue;
        ShopOffer offer;
        offer.kind = OfferKind::Equipment;
        offer.item = item;
        offer.price = static_cast<int>(std::lround(kTierPrice[tier] * (0.7f + item.iv / 100.0f) / 10.0f)) * 10;
        offer.stock = 1;
        shop.offers.push_back(offer);
    }
    // 強化結晶 3 個ずつ（3 回まで）
    {
        ShopOffer offer;
        offer.kind = OfferKind::Crystal;
        offer.amount = 3;
        offer.price = 90 + 30 * area;
        offer.stock = 3;
        shop.offers.push_back(offer);
    }
    run.shops.push_back(shop);
    return run.shops.back();
}

bool Buy(PlayerData& player, ShopStock& shop, int offerIndex)
{
    if (offerIndex < 0 || offerIndex >= static_cast<int>(shop.offers.size())) return false;
    ShopOffer& offer = shop.offers[static_cast<size_t>(offerIndex)];
    Inventory& inventory = player.GetInventory();
    if (offer.stock <= 0 || inventory.Col() < offer.price) return false;

    switch (offer.kind) {
    case OfferKind::Consumable:
        if (inventory.AddItemCount(offer.itemId, offer.amount) <= 0) return false;   // 持てる数の上限
        AutoAssignQuickSlots(inventory);
        break;
    case OfferKind::Equipment:
        inventory.AddItem(offer.item);
        break;
    case OfferKind::Crystal:
        inventory.AddMaterial(offer.amount);
        break;
    }
    inventory.AddCol(-offer.price);
    --offer.stock;
    return true;
}

bool IsLabyrinthItem(int templateId)
{
    return IsLabyrinthSetItem(templateId);
}

} // namespace rogue
} // namespace ecl
