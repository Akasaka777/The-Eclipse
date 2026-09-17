#include "Game/ItemDatabase.h"

#include "Common/MathUtil.h"

namespace ecl {

namespace {

// 武器用ステータス生成
Stats WeaponStats(float attack, float critRate, float critDamage, float attackSpeed, float moveSpeed = 0.0f)
{
    Stats s;
    s.attack = attack;
    s.critRate = critRate;
    s.critDamage = critDamage;
    s.attackSpeed = attackSpeed;
    s.moveSpeed = moveSpeed;
    return s;
}

// 防具用ステータス生成
Stats ArmorStats(float defense, float hp, float mp, float critRate = 0.0f,
                 float moveSpeed = 0.0f, float mpRegen = 0.0f)
{
    Stats s;
    s.defense = defense;
    s.maxHp = hp;
    s.maxMp = mp;
    s.critRate = critRate;
    s.moveSpeed = moveSpeed;
    s.mpRegen = mpRegen;
    return s;
}

// スキン生成ヘルパ
EquipSkin MakeSkin(SkinShape shape, ColorRGB primary, ColorRGB secondary, ColorRGB glow,
                   const char* folder = "", bool cape = false, bool helmet = false)
{
    EquipSkin skin;
    skin.shape = shape;
    skin.primary = primary;
    skin.secondary = secondary;
    skin.glow = glow;
    skin.spriteFolder = folder;
    skin.hasCape = cape;
    skin.hasHelmet = helmet;
    return skin;
}

} // namespace

ItemDatabase::ItemDatabase()
{
    const WeaponType SWD = WeaponType::OneHandSword;
    const WeaponType MCE = WeaponType::OneHandMace;
    const WeaponType DGR = WeaponType::Dagger;
    const WeaponType RPR = WeaponType::Rapier;
    const WeaponType SPR = WeaponType::Spear;

    templates_ = {
        //--- 片手剣 : 攻撃力とクリティカルのバランス型 ---------------------------
        { 100, "アイアンソード",       "鍛冶屋の定番。癖がなく扱いやすい。",       EquipSlot::Weapon, SWD, 1, WeaponStats(28.0f, 0.05f, 0.10f, 0.00f), EquipSkin() },
        { 101, "ブレイズエッジ",       "刃に熱を宿した中級剣。",                   EquipSlot::Weapon, SWD, 2, WeaponStats(44.0f, 0.07f, 0.14f, 0.02f), EquipSkin() },
        { 102, "エクリプス・ブレード", "蝕の夜にのみ研がれるという長剣。",         EquipSlot::Weapon, SWD, 3, WeaponStats(62.0f, 0.09f, 0.20f, 0.04f), EquipSkin() },

        //--- 片手棍 : 高火力・低速 ----------------------------------------------
        { 110, "アイアンメイス",       "重量で叩き潰す打撃武器。",                 EquipSlot::Weapon, MCE, 1, WeaponStats(35.0f, 0.03f, 0.16f, -0.10f), EquipSkin() },
        { 111, "ルーンクラブ",         "古代文字が刻まれた棍棒。",                 EquipSlot::Weapon, MCE, 2, WeaponStats(53.0f, 0.04f, 0.20f, -0.08f), EquipSkin() },
        { 112, "蝕の戦槌",             "一撃で城門をも砕くと伝わる。",             EquipSlot::Weapon, MCE, 3, WeaponStats(74.0f, 0.05f, 0.26f, -0.06f), EquipSkin() },

        //--- 短剣 : 手数とクリティカル ------------------------------------------
        { 120, "ショートダガー",       "懐に隠せる小振りの刃。",                   EquipSlot::Weapon, DGR, 1, WeaponStats(20.0f, 0.16f, 0.14f, 0.18f, 12.0f), EquipSkin() },
        { 121, "シャドウファング",     "影を裂く牙。連撃に長ける。",               EquipSlot::Weapon, DGR, 2, WeaponStats(31.0f, 0.20f, 0.18f, 0.22f, 16.0f), EquipSkin() },
        { 122, "蝕の牙",               "闇に紛れて急所を貫く。",                   EquipSlot::Weapon, DGR, 3, WeaponStats(43.0f, 0.24f, 0.24f, 0.26f, 20.0f), EquipSkin() },

        //--- 細剣 : 高速・刺突 ---------------------------------------------------
        { 130, "フルーレ",             "軽量の刺突剣。初心者にも扱える。",         EquipSlot::Weapon, RPR, 1, WeaponStats(24.0f, 0.11f, 0.12f, 0.12f, 8.0f), EquipSkin() },
        { 131, "ウィンドピアサー",     "風を裂く速度の細剣。",                     EquipSlot::Weapon, RPR, 2, WeaponStats(37.0f, 0.14f, 0.16f, 0.15f, 12.0f), EquipSkin() },
        { 132, "蝕の刺剣",             "一点に集約された蝕の光を放つ。",     EquipSlot::Weapon, RPR, 3, WeaponStats(51.0f, 0.17f, 0.22f, 0.18f, 16.0f), EquipSkin() },

        //--- 槍 : 長射程 ---------------------------------------------------------
        { 140, "アイアンランス",       "間合いを制する長柄武器。",                 EquipSlot::Weapon, SPR, 1, WeaponStats(31.0f, 0.06f, 0.12f, -0.04f), EquipSkin() },
        { 141, "ストームパイク",       "突きの衝撃が空気を裂く。",                 EquipSlot::Weapon, SPR, 2, WeaponStats(47.0f, 0.08f, 0.16f, -0.02f), EquipSkin() },
        { 142, "蝕の穿槍",             "届かぬものなしと謳われた穂先。",           EquipSlot::Weapon, SPR, 3, WeaponStats(66.0f, 0.10f, 0.22f,  0.00f), EquipSkin() },

        //--- 頭装備 -------------------------------------------------------------
        { 200, "レザーキャップ",       "軽い革の帽子。",                           EquipSlot::Head, SWD, 1, ArmorStats(6.0f,  40.0f, 10.0f), EquipSkin() },
        { 201, "アイアンヘルム",       "視界は狭いが頑丈。",                       EquipSlot::Head, SWD, 2, ArmorStats(12.0f, 75.0f,  6.0f), EquipSkin() },
        { 202, "月光のサークレット",   "MP の巡りを良くする装飾。",                 EquipSlot::Head, SWD, 2, ArmorStats(8.0f,  45.0f, 38.0f, 0.02f, 0.0f, 1.2f), EquipSkin() },
        { 203, "蝕の兜",               "闇を見通す視界を得る。",                   EquipSlot::Head, SWD, 3, ArmorStats(19.0f, 110.0f, 24.0f, 0.03f), EquipSkin() },

        //--- 体装備 -------------------------------------------------------------
        { 210, "レザーアーマー",       "動きを妨げない革鎧。",                     EquipSlot::Body, SWD, 1, ArmorStats(11.0f, 80.0f,  8.0f, 0.0f, 6.0f), EquipSkin() },
        { 211, "チェインメイル",       "斬撃に強い鎖帷子。",                       EquipSlot::Body, SWD, 2, ArmorStats(21.0f, 140.0f, 4.0f), EquipSkin() },
        { 212, "月光のコート",         "魔力を織り込んだ外套。",                   EquipSlot::Body, SWD, 2, ArmorStats(15.0f, 100.0f, 52.0f, 0.0f, 10.0f, 1.6f), EquipSkin() },
        { 213, "蝕の鎧",               "蝕の夜を纏うかのような漆黒。",             EquipSlot::Body, SWD, 3, ArmorStats(32.0f, 210.0f, 30.0f, 0.02f, 8.0f), EquipSkin() },

        //--- 盾 -----------------------------------------------------------------
        { 220, "ラウンドシールド",     "取り回しの良い小盾。",                     EquipSlot::Shield, SWD, 1, ArmorStats(9.0f,  55.0f, 0.0f), EquipSkin() },
        { 221, "カイトシールド",       "全身を隠せる大盾。",                       EquipSlot::Shield, SWD, 2, ArmorStats(18.0f, 105.0f, 0.0f, 0.0f, -6.0f), EquipSkin() },
        { 222, "蝕の盾",               "受けた衝撃を闇へ逃がす。",                 EquipSlot::Shield, SWD, 3, ArmorStats(28.0f, 160.0f, 18.0f), EquipSkin() },

        //--- 腕装備 -------------------------------------------------------------
        { 230, "レザーブレイサー",     "手首を守る革当て。",                       EquipSlot::Arm, SWD, 1, ArmorStats(5.0f, 30.0f, 6.0f, 0.02f), EquipSkin() },
        { 231, "鋼の篭手",             "打撃の威力を底上げする。",                 EquipSlot::Arm, SWD, 2, ArmorStats(10.0f, 55.0f, 0.0f, 0.04f), EquipSkin() },

        //--- 手装備 -------------------------------------------------------------
        { 240, "戦士のグローブ",       "武器を握る力が増す。",                     EquipSlot::Hand, SWD, 1, ArmorStats(4.0f, 24.0f, 4.0f, 0.03f), EquipSkin() },
        { 241, "疾風の手甲",           "振りの速度が上がる。",                     EquipSlot::Hand, SWD, 2, ArmorStats(7.0f, 38.0f, 8.0f, 0.03f), EquipSkin() },

        //--- 足装備 -------------------------------------------------------------
        { 250, "レザーブーツ",         "長時間の探索に向く。",                     EquipSlot::Foot, SWD, 1, ArmorStats(5.0f, 28.0f, 4.0f, 0.0f, 18.0f), EquipSkin() },
        { 251, "韋駄天のブーツ",       "駆け抜ける者のための靴。",                 EquipSlot::Foot, SWD, 2, ArmorStats(9.0f, 46.0f, 8.0f, 0.0f, 34.0f), EquipSkin() },
    };

    // 手装備の攻撃速度補正は個別に付与
    for (ItemTemplate& t : templates_) {
        if (t.slot == EquipSlot::Hand) t.base.attackSpeed = (t.id == 241) ? 0.10f : 0.05f;
    }

    //--- 見た目（スキン）の割り当て ---------------------------------------------
    struct SkinEntry { int id; EquipSkin skin; };
    const SkinEntry kSkins[] = {
        // 頭装備
        { 200, MakeSkin(SkinShape::Light,  ColorRGB(112, 86, 58), ColorRGB(208, 186, 150),
                        ColorRGB(180, 150, 100), "leather_cap") },
        { 201, MakeSkin(SkinShape::Heavy,  ColorRGB(120, 124, 136), ColorRGB(216, 222, 232),
                        ColorRGB(160, 180, 210), "iron_helm", false, true) },
        { 202, MakeSkin(SkinShape::Mystic, ColorRGB(86, 96, 150), ColorRGB(226, 232, 255),
                        ColorRGB(150, 200, 255), "moon_circlet") },
        { 203, MakeSkin(SkinShape::Eclipse, ColorRGB(38, 32, 56), ColorRGB(206, 196, 255),
                        ColorRGB(198, 108, 255), "eclipse_helm", false, true) },
        // 体装備
        { 210, MakeSkin(SkinShape::Light,  ColorRGB(104, 78, 54), ColorRGB(206, 182, 146),
                        ColorRGB(180, 150, 100), "leather_armor") },
        { 211, MakeSkin(SkinShape::Heavy,  ColorRGB(108, 114, 128), ColorRGB(214, 220, 232),
                        ColorRGB(150, 175, 205), "chain_mail") },
        { 212, MakeSkin(SkinShape::Mystic, ColorRGB(62, 72, 132), ColorRGB(220, 228, 255),
                        ColorRGB(140, 195, 255), "moon_coat", true) },
        { 213, MakeSkin(SkinShape::Eclipse, ColorRGB(30, 26, 48), ColorRGB(210, 200, 255),
                        ColorRGB(198, 108, 255), "eclipse_armor", true) },
        // 盾
        { 220, MakeSkin(SkinShape::Light,  ColorRGB(120, 94, 62), ColorRGB(212, 192, 158),
                        ColorRGB(190, 160, 110), "round_shield") },
        { 221, MakeSkin(SkinShape::Heavy,  ColorRGB(116, 122, 138), ColorRGB(220, 226, 238),
                        ColorRGB(150, 180, 220), "kite_shield") },
        { 222, MakeSkin(SkinShape::Eclipse, ColorRGB(34, 30, 52), ColorRGB(208, 198, 255),
                        ColorRGB(198, 108, 255), "eclipse_shield") },
        // 腕・手・足
        { 230, MakeSkin(SkinShape::Light,  ColorRGB(108, 82, 56), ColorRGB(204, 182, 148),
                        ColorRGB(180, 150, 100), "leather_bracer") },
        { 231, MakeSkin(SkinShape::Heavy,  ColorRGB(114, 120, 132), ColorRGB(216, 222, 234),
                        ColorRGB(150, 178, 210), "steel_gauntlet") },
        { 240, MakeSkin(SkinShape::Light,  ColorRGB(100, 80, 60), ColorRGB(202, 180, 148),
                        ColorRGB(180, 150, 100), "warrior_glove") },
        { 241, MakeSkin(SkinShape::Mystic, ColorRGB(72, 86, 140), ColorRGB(218, 228, 255),
                        ColorRGB(140, 200, 255), "gale_glove") },
        { 250, MakeSkin(SkinShape::Light,  ColorRGB(104, 80, 56), ColorRGB(204, 182, 148),
                        ColorRGB(180, 150, 100), "leather_boots") },
        { 251, MakeSkin(SkinShape::Mystic, ColorRGB(70, 92, 138), ColorRGB(220, 230, 255),
                        ColorRGB(140, 210, 255), "swift_boots") },
        // 武器（刀身の色に反映）
        { 100, MakeSkin(SkinShape::Heavy,  ColorRGB(150, 156, 168), ColorRGB(228, 234, 244),
                        ColorRGB(160, 200, 240), "iron_sword") },
        { 101, MakeSkin(SkinShape::Heavy,  ColorRGB(170, 120, 90), ColorRGB(255, 200, 150),
                        ColorRGB(255, 140, 70), "blaze_edge") },
        { 102, MakeSkin(SkinShape::Eclipse, ColorRGB(70, 60, 100), ColorRGB(220, 210, 255),
                        ColorRGB(198, 108, 255), "eclipse_blade") },
        { 112, MakeSkin(SkinShape::Eclipse, ColorRGB(72, 62, 96), ColorRGB(220, 210, 255),
                        ColorRGB(198, 108, 255), "eclipse_hammer") },
        { 122, MakeSkin(SkinShape::Eclipse, ColorRGB(68, 58, 94), ColorRGB(220, 210, 255),
                        ColorRGB(198, 108, 255), "eclipse_fang") },
        { 132, MakeSkin(SkinShape::Eclipse, ColorRGB(66, 58, 96), ColorRGB(222, 212, 255),
                        ColorRGB(198, 108, 255), "eclipse_rapier") },
        { 142, MakeSkin(SkinShape::Eclipse, ColorRGB(70, 60, 98), ColorRGB(220, 210, 255),
                        ColorRGB(198, 108, 255), "eclipse_spear") },
    };

    for (const SkinEntry& entry : kSkins) {
        for (ItemTemplate& t : templates_) {
            if (t.id == entry.id) {
                t.skin = entry.skin;
                break;
            }
        }
    }
}

const ItemDatabase& ItemDatabase::Instance()
{
    static ItemDatabase instance;
    return instance;
}

const ItemTemplate* ItemDatabase::Find(int templateId) const
{
    for (const ItemTemplate& t : templates_) {
        if (t.id == templateId) return &t;
    }
    return nullptr;
}

EquipmentItem ItemDatabase::Create(int templateId, Rarity rarity) const
{
    EquipmentItem item;
    const ItemTemplate* tmpl = Find(templateId);
    if (!tmpl) return item;

    item.uid = IssueItemUid();
    item.templateId = tmpl->id;
    item.name = tmpl->name;
    item.flavor = tmpl->flavor;
    item.slot = tmpl->slot;
    item.weaponType = tmpl->weaponType;
    item.rarity = rarity;
    item.upgradeLevel = 0;
    item.skin = tmpl->skin;

    // 高レアリティはレアリティ色を差し色に混ぜる
    if (static_cast<int>(rarity) >= static_cast<int>(Rarity::SR)) {
        const ColorRGB rarityColor = RarityColor(rarity);
        const float blend = 0.25f + 0.18f * static_cast<float>(static_cast<int>(rarity)
                                                              - static_cast<int>(Rarity::SR));
        item.skin.glow = ColorRGB::Lerp(item.skin.glow, rarityColor, blend);
        item.skin.secondary = ColorRGB::Lerp(item.skin.secondary, rarityColor, blend * 0.6f);
    }

    const float rarityMul = RarityMultiplier(rarity);
    const float variance = math::RandFloat(0.94f, 1.06f);
    item.baseStats = tmpl->base.Scaled(rarityMul * variance);

    // クリティカル率は倍率が効き過ぎないよう補正
    item.baseStats.critRate = tmpl->base.critRate * (1.0f + (rarityMul - 1.0f) * 0.45f);
    item.baseStats.attackSpeed = tmpl->base.attackSpeed * (1.0f + (rarityMul - 1.0f) * 0.30f);

    item.RestoreDurability();
    return item;
}

EquipmentItem ItemDatabase::CreateRandom(EquipSlot slot, Rarity rarity, int maxTier) const
{
    std::vector<int> candidates;
    for (const ItemTemplate& t : templates_) {
        if (t.slot == slot && t.tier <= maxTier) candidates.push_back(t.id);
    }
    if (candidates.empty()) return EquipmentItem();

    const int index = math::RandInt(0, static_cast<int>(candidates.size()) - 1);
    return Create(candidates[static_cast<size_t>(index)], rarity);
}

EquipmentItem ItemDatabase::CreateRandomAny(Rarity rarity, int maxTier) const
{
    std::vector<int> candidates;
    for (const ItemTemplate& t : templates_) {
        if (t.tier <= maxTier) candidates.push_back(t.id);
    }
    if (candidates.empty()) return EquipmentItem();

    const int index = math::RandInt(0, static_cast<int>(candidates.size()) - 1);
    return Create(candidates[static_cast<size_t>(index)], rarity);
}

std::vector<EquipmentItem> ItemDatabase::CreateStarterSet() const
{
    std::vector<EquipmentItem> items;
    items.push_back(Create(100, Rarity::N)); // アイアンソード
    items.push_back(Create(200, Rarity::N)); // レザーキャップ
    items.push_back(Create(210, Rarity::N)); // レザーアーマー
    items.push_back(Create(220, Rarity::N)); // ラウンドシールド
    // 他の武器種も試せるように 1 本ずつ配布
    items.push_back(Create(110, Rarity::N));
    items.push_back(Create(120, Rarity::N));
    items.push_back(Create(130, Rarity::N));
    items.push_back(Create(140, Rarity::N));
    return items;
}

} // namespace ecl
