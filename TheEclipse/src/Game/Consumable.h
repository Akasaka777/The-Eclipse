//==============================================================================
// Consumable.h : 消費アイテムと素材のマスターデータ
//   回復アイテム : 使うと HP / MP を回復する
//   バフアイテム : 使うと一定時間だけ能力が上がる（使ったアイテムは無くなる）
//   素材         : 武器・防具の作成に使う（作成システムは今後実装）
//   装備品（EquipmentItem）とは別に、ID ごとの個数だけで管理する。
//==============================================================================
#pragma once

#include "Common/Types.h"
#include "Game/Stats.h"

#include <string>
#include <vector>

namespace ecl {

//------------------------------------------------------------------------------
// 種類
//------------------------------------------------------------------------------
enum class ConsumableKind
{
    Recovery,   // 回復
    Buff,       // バフ
    Material,   // 素材
    Count
};

const char* ConsumableKindName(ConsumableKind kind);

//------------------------------------------------------------------------------
// 戦闘中に使えるアイテムの枠（アイテムメニューで 1 つずつ装備する）
//------------------------------------------------------------------------------
enum class QuickSlot
{
    Recovery,   // 回復アイテム
    Buff,       // バフアイテム
    Count
};

constexpr int kQuickSlotCount = static_cast<int>(QuickSlot::Count);
const char* QuickSlotName(QuickSlot slot);
// この枠に入る種類
ConsumableKind QuickSlotKind(QuickSlot slot);

// 所持できる上限
constexpr int kConsumableMaxStack = 99;   // 回復・バフ
constexpr int kMaterialMaxStack = 999;    // 素材

// 戦闘中にアイテムを使ったあと、次に使えるまでの待ち時間（秒）
constexpr float kItemUseCooldown = 1.0f;

//------------------------------------------------------------------------------
// 定義
//------------------------------------------------------------------------------
struct ConsumableDef
{
    int            id = 0;
    std::string    name;
    std::string    description;   // 説明文
    std::string    effect;        // 「HP を 30% 回復」のような効果の要約
    std::string    shortEffect;   // 戦闘 HUD 用の短い要約（「HP 30%」など）
    ConsumableKind kind = ConsumableKind::Recovery;
    int            rarity = 1;    // 1〜3（素材の色分けに使う）
    // ショップの価格（0 なら非売品）
    int            price = 0;
    ColorRGB       color = ColorRGB(120, 230, 140);

    // --- 回復（最大値に対する割合）---------------------------------------------
    float hpRate = 0.0f;
    float mpRate = 0.0f;

    // --- バフ（効果時間のあいだだけ効く）----------------------------------------
    StatRates rates;              // 攻撃力・防御力・移動速度の倍率
    float     critRate = 0.0f;    // クリティカル率の加算（0.10 = +10%）
    float     duration = 0.0f;    // 効果時間（秒）

    bool InShop() const { return price > 0; }
    int  MaxStack() const
    {
        return (kind == ConsumableKind::Material) ? kMaterialMaxStack : kConsumableMaxStack;
    }
};

//------------------------------------------------------------------------------
// アイテム ID と個数の組（ドロップやリザルト表示に使う）
//------------------------------------------------------------------------------
struct ItemStack
{
    int itemId = 0;
    int count = 0;
};

//------------------------------------------------------------------------------
// データベース
//------------------------------------------------------------------------------
class ConsumableDatabase
{
public:
    static const ConsumableDatabase& Instance();

    const std::vector<ConsumableDef>& All() const { return defs_; }
    const ConsumableDef* Find(int id) const;
    std::vector<const ConsumableDef*> OfKind(ConsumableKind kind) const;
    // ショップに並ぶもの（回復・バフ。素材は売らない）
    std::vector<const ConsumableDef*> ShopItems() const;

private:
    ConsumableDatabase();

    std::vector<ConsumableDef> defs_;
};

//------------------------------------------------------------------------------
// 回復・バフアイテムの ID
//------------------------------------------------------------------------------
constexpr int kItemPotion       = 1000;  // ポーション
constexpr int kItemHiPotion     = 1001;  // ハイポーション
constexpr int kItemHealCrystal  = 1002;  // 回復結晶
constexpr int kItemManaPotion   = 1003;  // マナポーション
constexpr int kItemElixir       = 1004;  // エリクサー
constexpr int kItemPowerTonic   = 1100;  // 剛力の秘薬
constexpr int kItemGuardTonic   = 1101;  // 堅牢の秘薬
constexpr int kItemSwiftTonic   = 1102;  // 疾風の秘薬
constexpr int kItemCritTonic    = 1103;  // 会心の秘薬
constexpr int kItemHeroTonic    = 1104;  // 英雄の霊薬

// 新規ゲームで配る回復アイテム（回復枠に装備した状態で始まる）
constexpr int kStarterPotionId = kItemPotion;
constexpr int kStarterPotionCount = 5;

//------------------------------------------------------------------------------
// 素材の ID（クエストのドロップ表から参照する）
//------------------------------------------------------------------------------
constexpr int kMatIronOre      = 1200;  // 鉄鉱石
constexpr int kMatBeastHide    = 1201;  // 獣の皮
constexpr int kMatSpiritBranch = 1202;  // 霊木の枝
constexpr int kMatWolfFang     = 1203;  // 狼王の牙
constexpr int kMatHardBone     = 1204;  // 硬い骨
constexpr int kMatSilverOre    = 1205;  // 銀鉱石
constexpr int kMatGolemCore    = 1206;  // ゴーレムの核
constexpr int kMatEclipseShard = 1207;  // 蝕の欠片
constexpr int kMatMagicStone   = 1208;  // 魔石
constexpr int kMatDarkSteel    = 1209;  // 黒騎士の鋼
constexpr int kMatHolySilver   = 1210;  // 聖銀
constexpr int kMatObsidian     = 1211;  // 黒曜石
constexpr int kMatFireScale    = 1212;  // 火竜の鱗
constexpr int kMatDragonScale  = 1213;  // 竜王の逆鱗

} // namespace ecl
