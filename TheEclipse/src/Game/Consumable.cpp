#include "Game/Consumable.h"

namespace ecl {

const char* ConsumableKindName(ConsumableKind kind)
{
    switch (kind) {
    case ConsumableKind::Recovery: return "回復";
    case ConsumableKind::Buff:     return "バフ";
    case ConsumableKind::Material: return "素材";
    default: return "アイテム";
    }
}

const char* QuickSlotName(QuickSlot slot)
{
    switch (slot) {
    case QuickSlot::Recovery: return "回復";
    case QuickSlot::Buff:     return "バフ";
    default: return "アイテム";
    }
}

ConsumableKind QuickSlotKind(QuickSlot slot)
{
    return (slot == QuickSlot::Buff) ? ConsumableKind::Buff : ConsumableKind::Recovery;
}

namespace {

const ColorRGB kHpColor(110, 230, 130);
const ColorRGB kMpColor(90, 170, 255);
const ColorRGB kElixirColor(220, 140, 255);

ConsumableDef MakeRecovery(int id, const char* name, const char* description, const char* effect,
                           const char* shortEffect, float hpRate, float mpRate, int price, const ColorRGB& color)
{
    ConsumableDef def;
    def.id = id;
    def.name = name;
    def.description = description;
    def.effect = effect;
    def.shortEffect = shortEffect;
    def.kind = ConsumableKind::Recovery;
    def.hpRate = hpRate;
    def.mpRate = mpRate;
    def.price = price;
    def.color = color;
    return def;
}

ConsumableDef MakeBuff(int id, const char* name, const char* description, const char* effect,
                       const char* shortEffect, const StatRates& rates, float critRate, float duration, int price,
                       const ColorRGB& color)
{
    ConsumableDef def;
    def.id = id;
    def.name = name;
    def.description = description;
    def.effect = effect;
    def.shortEffect = shortEffect;
    def.kind = ConsumableKind::Buff;
    def.rates = rates;
    def.critRate = critRate;
    def.duration = duration;
    def.price = price;
    def.color = color;
    return def;
}

ConsumableDef MakeMaterial(int id, const char* name, const char* description, int rarity)
{
    // レア度で色を変える（1: 灰 / 2: 青 / 3: 金）
    static const ColorRGB kRarityColor[3] = {
        ColorRGB(190, 196, 206), ColorRGB(110, 180, 255), ColorRGB(255, 196, 80),
    };

    ConsumableDef def;
    def.id = id;
    def.name = name;
    def.description = description;
    def.effect = "武器・防具の作成に使う素材";
    def.shortEffect = "素材";
    def.kind = ConsumableKind::Material;
    def.rarity = rarity;
    def.color = kRarityColor[(rarity < 1) ? 0 : (rarity > 3 ? 2 : rarity - 1)];
    return def;
}

StatRates AttackRate(float rate)
{
    StatRates r;
    r.attack = rate;
    return r;
}

StatRates DefenseRate(float rate)
{
    StatRates r;
    r.defense = rate;
    return r;
}

StatRates SpeedRate(float rate)
{
    StatRates r;
    r.moveSpeed = rate;
    return r;
}

} // namespace

ConsumableDatabase::ConsumableDatabase()
{
    //==========================================================================
    // 回復アイテム（1000〜）
    //==========================================================================
    defs_.push_back(MakeRecovery(kItemPotion, "ポーション",
                                 "傷を癒やす基本の回復薬。", "HP を 30% 回復", "HP 30% 回復",
                                 0.30f, 0.0f, 100, kHpColor));
    defs_.push_back(MakeRecovery(kItemHiPotion, "ハイポーション",
                                 "濃く煮詰めた回復薬。深い傷にも効く。", "HP を 60% 回復", "HP 60% 回復",
                                 0.60f, 0.0f, 400, kHpColor));
    defs_.push_back(MakeRecovery(kItemHealCrystal, "回復結晶",
                                 "砕くと光が全身を包む結晶。高価だが一瞬で全快する。",
                                 "HP を 100% 回復", "HP 全回復", 1.00f, 0.0f, 1500, kHpColor));
    defs_.push_back(MakeRecovery(kItemManaPotion, "マナポーション",
                                 "青く澄んだ薬。ソードスキルの消耗を癒やす。", "MP を 40% 回復", "MP 40% 回復",
                                 0.0f, 0.40f, 200, kMpColor));
    defs_.push_back(MakeRecovery(kItemElixir, "エリクサー",
                                 "体力と気力を同時に満たす秘伝の霊薬。",
                                 "HP と MP を 60% 回復", "HP・MP 60%", 0.60f, 0.60f, 1200, kElixirColor));

    //==========================================================================
    // バフアイテム（1100〜）
    //   使うと消え、効果時間のあいだだけ能力が上がる。
    //   重ねがけはできず、別のバフを使うと上書きされる。
    //==========================================================================
    defs_.push_back(MakeBuff(kItemPowerTonic, "剛力の秘薬",
                             "飲むと腕に力がみなぎる赤い秘薬。", "攻撃力 +15%（60 秒）", "攻撃 +15% 60s",
                             AttackRate(0.15f), 0.0f, 60.0f, 400, ColorRGB(255, 110, 90)));
    defs_.push_back(MakeBuff(kItemGuardTonic, "堅牢の秘薬",
                             "皮膚が岩のように硬くなる秘薬。", "防御力 +20%（60 秒）", "防御 +20% 60s",
                             DefenseRate(0.20f), 0.0f, 60.0f, 400, ColorRGB(120, 200, 255)));
    defs_.push_back(MakeBuff(kItemSwiftTonic, "疾風の秘薬",
                             "足取りが風のように軽くなる秘薬。", "移動速度 +15%（60 秒）", "速度 +15% 60s",
                             SpeedRate(0.15f), 0.0f, 60.0f, 300, ColorRGB(140, 255, 200)));
    defs_.push_back(MakeBuff(kItemCritTonic, "会心の秘薬",
                             "感覚を研ぎ澄まし、急所を見抜く秘薬。",
                             "クリティカル率 +10%（60 秒）", "会心 +10% 60s",
                             StatRates(), 0.10f, 60.0f, 500, ColorRGB(255, 220, 90)));
    {
        StatRates rates;
        rates.attack = 0.10f;
        rates.defense = 0.10f;
        defs_.push_back(MakeBuff(kItemHeroTonic, "英雄の霊薬",
                                 "古の英雄が戦の前に口にしたという霊薬。",
                                 "攻撃力・防御力 +10%（90 秒）", "攻防 +10% 90s",
                                 rates, 0.0f, 90.0f, 1200, ColorRGB(255, 170, 60)));
    }

    //==========================================================================
    // 素材（1200〜）
    //   武器・防具の作成に使う予定。今はクエストで集めるだけ。
    //==========================================================================
    defs_.push_back(MakeMaterial(kMatIronOre, "鉄鉱石",
                                 "どこでも採れるありふれた鉱石。武具作りの基本。", 1));
    defs_.push_back(MakeMaterial(kMatBeastHide, "獣の皮",
                                 "森の獣から剥いだ丈夫な皮。軽い防具に向く。", 1));
    defs_.push_back(MakeMaterial(kMatSpiritBranch, "霊木の枝",
                                 "はじまりの森の古木の枝。かすかに魔力を帯びる。", 1));
    defs_.push_back(MakeMaterial(kMatWolfFang, "狼王の牙",
                                 "深緑の狼王ファングルフの鋭い牙。", 2));
    defs_.push_back(MakeMaterial(kMatHardBone, "硬い骨",
                                 "石牢に眠る魔物の骨。鉄のように硬い。", 1));
    defs_.push_back(MakeMaterial(kMatSilverOre, "銀鉱石",
                                 "深い層で採れる銀色の鉱石。魔を払う力がある。", 2));
    defs_.push_back(MakeMaterial(kMatGolemCore, "ゴーレムの核",
                                 "ゴーレム・ガルドを動かしていた石の心臓。", 2));
    defs_.push_back(MakeMaterial(kMatEclipseShard, "蝕の欠片",
                                 "祭壇に散らばる黒い結晶の欠片。光を吸い込む。", 2));
    defs_.push_back(MakeMaterial(kMatMagicStone, "魔石",
                                 "魔力が凝り固まってできた石。", 2));
    defs_.push_back(MakeMaterial(kMatDarkSteel, "黒騎士の鋼",
                                 "蝕の騎士の鎧から削り出した漆黒の鋼。", 3));
    defs_.push_back(MakeMaterial(kMatHolySilver, "聖銀",
                                 "聖堂の守護者が纏っていた、清らかに輝く銀。", 3));
    defs_.push_back(MakeMaterial(kMatObsidian, "黒曜石",
                                 "溶岩が冷えて固まった黒いガラス質の石。", 2));
    defs_.push_back(MakeMaterial(kMatFireScale, "火竜の鱗",
                                 "火山に棲む竜の鱗。熱をよく通さない。", 2));
    defs_.push_back(MakeMaterial(kMatDragonScale, "竜王の逆鱗",
                                 "竜王ヴァルグリムの喉元にあった一枚の鱗。", 3));
}

const ConsumableDatabase& ConsumableDatabase::Instance()
{
    static ConsumableDatabase instance;
    return instance;
}

const ConsumableDef* ConsumableDatabase::Find(int id) const
{
    for (const ConsumableDef& def : defs_) {
        if (def.id == id) return &def;
    }
    return nullptr;
}

std::vector<const ConsumableDef*> ConsumableDatabase::OfKind(ConsumableKind kind) const
{
    std::vector<const ConsumableDef*> list;
    for (const ConsumableDef& def : defs_) {
        if (def.kind == kind) list.push_back(&def);
    }
    return list;
}

std::vector<const ConsumableDef*> ConsumableDatabase::ShopItems() const
{
    std::vector<const ConsumableDef*> list;
    for (const ConsumableDef& def : defs_) {
        if (def.InShop()) list.push_back(&def);
    }
    return list;
}

} // namespace ecl
