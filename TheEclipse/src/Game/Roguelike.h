//==============================================================================
// Roguelike.h : ローグライクモード
//   ・クエスト一覧のクエストをすべてクリアすると解放される（街の「迷宮の門」から挑む）
//   ・本編で手に入れた武器・装備・アイテムは持ち込めない。Lv 1・初期装備の新しいキャラで始める
//   ・3 階層 × 4 フロア（各階層の最後はボス）を毎回ランダムに作る。ショップと鍛冶屋もランダムな場所に置く
//   ・敵は col・アイテム・強化結晶・スキルを落とす（雑魚敵は武器・防具を落とさない）。
//     階層のボスは装備を、最後のボス（ラスボス）は低い確率で迷宮の装備を落とす
//   ・拾ったスキルは 3 つの候補から 1 つを選んでスロットにセットする。持っているスキルを選ぶと Lv が上がる（Lv 5 まで）
//   ・パーマデス：倒れたら（リタイアしても）ランの成果はすべて失われる。
//     ラスボスを倒してクリアしたときだけ、ラスボスが落とした迷宮の装備を本編へ持ち帰れる
//==============================================================================
#pragma once

#include "Game/Consumable.h"
#include "Game/Equipment.h"
#include "Game/PlayerData.h"
#include "Game/QuestDatabase.h"
#include "Game/WeaponType.h"

#include <string>
#include <vector>

namespace ecl {

class GameContext;

namespace rogue {

//------------------------------------------------------------------------------
// 構成
//------------------------------------------------------------------------------
constexpr int kQuestId = 1000;              // 生成したクエストの ID（QuestDatabase には無い）
constexpr int kAreaCount = 3;               // 階層の数
constexpr int kBattleFloorsPerArea = 3;     // 階層ごとの戦闘フロア（そのあとにボスフロア）
constexpr int kFloorsPerArea = kBattleFloorsPerArea + 1;
constexpr int kFloorCount = kAreaCount * kFloorsPerArea;   // 12 フロア

// 敵の強さ：階層の最初のフロアの倍率と、階層の中で 1 フロア進むごとの増え方
constexpr float kAreaEnemyScale[kAreaCount] = { 1.0f, 1.8f, 2.8f };
constexpr float kFloorScaleStep = 0.14f;
// 階層ごとのボスの HP バーの本数
constexpr int kAreaBossHpBars[kAreaCount] = { 4, 5, 7 };

// 経験値：本編の必要経験値は Lv が上がるほど急に増えるので、ラン中は
//   「次のレベルまでに要る経験値」の割合で渡す（雑魚敵 1 体で 0.24 レベルぶん、ボスで 1.2 レベルぶん）。
//   全部倒して進むと、第 1 階層の終わりで Lv 6 前後・第 2 階層の終わりで Lv 12 前後・最後に Lv 18 前後
constexpr float kRunExpPerKill = 0.24f;
constexpr float kRunExpPerBoss = 1.2f;

// --- 開始時の持ち物（本編からは何も持ち込めない）------------------------------
constexpr int kStartCol = 0;
constexpr int kStartCrystals = 0;
constexpr int kStartPotions = 3;

// --- ドロップ ------------------------------------------------------------------
constexpr float kMobItemChance = 0.12f;      // 雑魚敵：回復・バフアイテム
constexpr float kMobCrystalChance = 0.25f;   // 雑魚敵：強化結晶
constexpr float kMobSkillChance = 0.07f;     // 雑魚敵：スキル
constexpr int   kSkillPityKills = 10;        // この数だけ続けて出なければ、次の雑魚敵は必ずスキルを落とす
constexpr float kAreaBossSecondGearChance = 0.30f;   // 階層のボス：2 つ目の装備
constexpr float kLastBossGearChance = 0.10f;          // ラスボス：迷宮の装備（低確率）
constexpr int   kSkillChoices = 3;           // スキルを拾ったときの候補の数

// 近くに敵がいるとショップ・鍛冶屋に入れない距離
constexpr float kFacilityEnemyRange = 500.0f;
// 施設の入口の前とみなす距離
constexpr float kFacilityRange = 120.0f;

//------------------------------------------------------------------------------
// 乱数（シードが同じなら同じマップになる）
//------------------------------------------------------------------------------
struct Rng
{
    unsigned int state = 1u;

    explicit Rng(unsigned int seed = 1u) : state(seed ? seed : 0x9e3779b9u) {}
    unsigned int Next();
    float Next01();                       // 0 以上 1 未満
    int   Range(int lo, int hi);          // lo 以上 hi 以下
    float Range(float lo, float hi);
    bool  Chance(float p) { return Next01() < p; }
};

//------------------------------------------------------------------------------
// ショップ
//------------------------------------------------------------------------------
enum class OfferKind
{
    Consumable,   // 回復・バフアイテム
    Equipment,    // 武器・防具
    Crystal       // 強化結晶（まとめ売り）
};

struct ShopOffer
{
    OfferKind     kind = OfferKind::Consumable;
    int           itemId = 0;      // 消費アイテムの ID
    EquipmentItem item;            // 装備
    int           amount = 1;      // 1 回で買える数（強化結晶は 3 個ずつ）
    int           price = 0;
    int           stock = 1;       // 残り
};

struct ShopStock
{
    int floorIndex = 0;
    int spotIndex = 0;
    std::vector<ShopOffer> offers;
};

//------------------------------------------------------------------------------
// ラン（1 回の挑戦）の状態
//------------------------------------------------------------------------------
struct RunState
{
    bool         active = false;
    unsigned int seed = 0;
    WeaponType   weapon = WeaponType::OneHandSword;
    QuestDef     quest;          // 生成したフロア
    // ラン中にしまっておく本編のキャラ（GameContext::player にはランのキャラが入る）。
    // セーブはこちらを書く（ランのキャラは保存しない）
    PlayerData   mainPlayer;
    int          mainSelectedQuestId = 1;   // ランの前に選んでいたクエスト（セーブはこちら）
    Rng          rng;
    std::vector<ShopStock> shops;

    int kills = 0;
    int killsSinceSkill = 0;     // 最後にスキルが出てから倒した雑魚敵の数
    int deepestFloor = 0;        // たどり着いたいちばん奥のフロア（1 始まり）
    int colEarned = 0;           // ラン中に拾った col の合計
    int expEarned = 0;
    // ラスボスが落とした迷宮の装備（クリアしたら本編へ持ち帰る）
    std::vector<EquipmentItem> carryHome;
};

//------------------------------------------------------------------------------
// 解放・開始・終了
//------------------------------------------------------------------------------
// クエスト一覧に出るクエスト（特別クエストを除く）をすべてクリアしたか
bool IsUnlocked(const PlayerData& player);
// 開発者モード：すべてのクエスト（特別クエストも）をクリア済みにする
void MarkAllQuestsCleared(PlayerData& player);

// フロアを作る（シードが同じなら同じになる）
QuestDef GenerateRun(unsigned int seed, WeaponType weapon);
// ランを始める：本編のキャラをしまい、Lv 1・初期装備だけの新しいキャラにする
void StartRun(GameContext& context, WeaponType weapon, unsigned int seed);
// ランを終える：本編のキャラに戻して記録を残す。クリアしたらラスボスの戦利品を持ち帰る。
//   持ち帰った装備を返す
std::vector<EquipmentItem> EndRun(GameContext& context, bool cleared, int floorReached);

// レベルが上がったときのステータスの自動振り分け（STR / VIT / STR / AGI / VIT の順）
void AutoAllocate(PlayerData& player);
// 回復とバフの枠が空（または 0 個）なら、持っているもので埋める
void AutoAssignQuickSlots(Inventory& inventory);

//------------------------------------------------------------------------------
// スキル
//------------------------------------------------------------------------------
// 候補を選ぶ（武器種のスキルから、Lv 5 になったものを除いて最大 count 個）
std::vector<int> DraftSkills(const PlayerData& player, WeaponType weapon, int count, Rng& rng);

enum class SkillPick
{
    Learned,     // 新しく覚えてスロットにセットした
    LevelUp,     // 持っていたスキルの Lv が上がった
    NeedSlot,    // スロットが埋まっている（入れ替える枠を選ぶ必要がある）
    Maxed,       // もう Lv 5
    Invalid
};
// 候補から 1 つ選んだときの処理。スロットが埋まっているときは replaceSlot（0〜3）の
// スキルを捨てて入れ替える（-1 なら NeedSlot を返して何もしない）
SkillPick ApplySkillPick(PlayerData& player, int skillId, int replaceSlot = -1);
// 持っている（スロットにある）スキルの数
int OwnedSkillCount(const PlayerData& player);

//------------------------------------------------------------------------------
// ドロップ
//------------------------------------------------------------------------------
struct DropRoll
{
    int col = 0;
    std::vector<ItemStack> items;            // 回復・バフアイテム
    int crystals = 0;                        // 強化結晶
    bool skill = false;                      // スキル
    std::vector<EquipmentItem> gear;         // 武器・防具（階層のボスだけ）
    std::vector<EquipmentItem> carryHome;    // 迷宮の装備（ラスボスだけ。低確率）
};

// 階層（0〜2）
int AreaOf(int floorIndex);
bool IsBossFloor(int floorIndex);
bool IsLastFloor(int floorIndex);

DropRoll RollMobDrop(RunState& run, int floorIndex);
DropRoll RollBossDrop(RunState& run, int floorIndex, float dropRate = 1.0f);

//------------------------------------------------------------------------------
// ショップ
//------------------------------------------------------------------------------
// その施設の品ぞろえ（初めて開いたときに作り、売り切れは残る）
ShopStock& ShopAt(RunState& run, int floorIndex, int spotIndex);
// 買う（col が足りない・売り切れなら false）
bool Buy(PlayerData& player, ShopStock& shop, int offerIndex);
// 迷宮の装備（ラスボスの戦利品）か
bool IsLabyrinthItem(int templateId);

} // namespace rogue
} // namespace ecl
