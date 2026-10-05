#include "Game/ItemDatabase.h"

#include "Game/UniqueSkill.h"

#include "Common/MathUtil.h"

namespace ecl {

bool IsDragonSetItem(int templateId)
{
    return templateId == kDragonSwordId || templateId == kDragonHelmId
        || templateId == kDragonMailId  || templateId == kDragonShieldId
        || templateId == kDragonArmId   || templateId == kDragonGloveId
        || templateId == kDragonBootsId;
}

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

//------------------------------------------------------------------------------
// アクセサリーの定義を組む
//   rates : 倍率で効くバフ（攻撃力 +5% など）
//   flat  : そのまま加算される値（クリティカル率 / 魔法 / 属性 / 命中）
//------------------------------------------------------------------------------
ItemTemplate MakeAccessory(int id, const char* name, const char* flavor, AccessoryKind kind,
                           const char* effect, int price, const StatRates& rates,
                           const Stats& flat = Stats())
{
    ItemTemplate t;
    t.id = id;
    t.name = name;
    t.flavor = flavor;
    t.slot = EquipSlot::Accessory;
    t.tier = 1;
    t.base = flat;
    t.price = price;
    t.accessory = kind;
    t.rates = rates;
    t.effect = effect;
    return t;
}

StatRates Rate(float attack, float defense, float maxHp, float maxMp, float moveSpeed)
{
    StatRates r;
    r.attack = attack;
    r.defense = defense;
    r.maxHp = maxHp;
    r.maxMp = maxMp;
    r.moveSpeed = moveSpeed;
    return r;
}

// 加算で効く値だけを持つステータス
Stats FlatBonus(float critRate = 0.0f, float magicAttack = 0.0f, float magicDefense = 0.0f,
                float accuracy = 0.0f, float fireResist = 0.0f, float waterResist = 0.0f)
{
    Stats s;
    s.critRate = critRate;
    s.magicAttack = magicAttack;
    s.magicDefense = magicDefense;
    s.accuracy = accuracy;
    s.fireResist = fireResist;
    s.waterResist = waterResist;
    return s;
}

//------------------------------------------------------------------------------
// ショップに並べる品と価格
//   ID が kShopOnlyIdBase 以上のものはショップ専用（ドロップには出ない）。
//   それ未満のものはドロップでも手に入る装備で、ショップでも買えるようにする。
//------------------------------------------------------------------------------
constexpr int kShopOnlyIdBase = 300;

struct ShopPrice
{
    int id;
    int price;
};

const ShopPrice kShopPrices[] = {
    // --- ドロップにも出る装備（ショップでも買える）---------------------------
    { 110, 800 },  // アイアンメイス
    { 201, 1500 }, // アイアンヘルム
    { 211, 1900 }, // チェインメイル
    { 221, 1800 }, // カイトシールド

    // --- ショップ専用の武器 ---------------------------------------------------
    { 300, 700 },  { 301, 1100 }, { 302, 2600 },           // 片手剣
    { 310, 2400 }, { 311, 2800 },                          // 片手棍
    { 320, 650 },  { 321, 1000 }, { 322, 2700 },           // 細剣
    { 330, 750 },  { 331, 2300 }, { 332, 3000 },           // 槍
    { 340, 600 },  { 341, 950 },  { 342, 2500 },           // 短剣

    // --- ショップ専用の防具 ---------------------------------------------------
    { 350, 400 },  { 351, 2600 },                          // 頭
    { 360, 300 },  { 361, 800 },  { 362, 3400 },           // 体
    { 370, 450 },  { 371, 1100 },                          // 盾
    { 380, 350 },  { 381, 900 },  { 382, 2000 },           // 腕
    { 390, 300 },  { 391, 750 },  { 392, 1700 },           // 手
    { 395, 380 },  { 396, 950 },  { 397, 2100 },           // 足
};

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
        { 100, "アイアンソード",       "鍛冶屋の定番。癖がなく扱いやすい。",       EquipSlot::WeaponRight, SWD, 1, WeaponStats(28.0f, 0.05f, 0.10f, 0.00f) },
        { 101, "ブレイズエッジ",       "刃に熱を宿した中級剣。",                   EquipSlot::WeaponRight, SWD, 2, WeaponStats(44.0f, 0.07f, 0.14f, 0.02f) },
        { 102, "エクリプス・ブレード", "蝕の夜にのみ研がれるという長剣。",         EquipSlot::WeaponRight, SWD, 3, WeaponStats(62.0f, 0.09f, 0.20f, 0.04f) },

        //--- 片手棍 : 高火力・低速 ----------------------------------------------
        { 110, "アイアンメイス",       "重量で叩き潰す打撃武器。",                 EquipSlot::WeaponRight, MCE, 1, WeaponStats(35.0f, 0.03f, 0.16f, -0.10f) },
        { 111, "ルーンクラブ",         "古代文字が刻まれた棍棒。",                 EquipSlot::WeaponRight, MCE, 2, WeaponStats(53.0f, 0.04f, 0.20f, -0.08f) },
        { 112, "蝕の戦槌",             "一撃で城門をも砕くと伝わる。",             EquipSlot::WeaponRight, MCE, 3, WeaponStats(74.0f, 0.05f, 0.26f, -0.06f) },

        //--- 短剣 : 手数とクリティカル ------------------------------------------
        { 120, "ショートダガー",       "懐に隠せる小振りの刃。",                   EquipSlot::WeaponRight, DGR, 1, WeaponStats(20.0f, 0.16f, 0.14f, 0.18f, 12.0f) },
        { 121, "シャドウファング",     "影を裂く牙。連撃に長ける。",               EquipSlot::WeaponRight, DGR, 2, WeaponStats(31.0f, 0.20f, 0.18f, 0.22f, 16.0f) },
        { 122, "蝕の牙",               "闇に紛れて急所を貫く。",                   EquipSlot::WeaponRight, DGR, 3, WeaponStats(43.0f, 0.24f, 0.24f, 0.26f, 20.0f) },

        //--- 細剣 : 高速・刺突 ---------------------------------------------------
        { 130, "フルーレ",             "軽量の刺突剣。初心者にも扱える。",         EquipSlot::WeaponRight, RPR, 1, WeaponStats(24.0f, 0.11f, 0.12f, 0.12f, 8.0f) },
        { 131, "ウィンドピアサー",     "風を裂く速度の細剣。",                     EquipSlot::WeaponRight, RPR, 2, WeaponStats(37.0f, 0.14f, 0.16f, 0.15f, 12.0f) },
        { 132, "蝕の刺剣",             "一点に集約された蝕の光を放つ。",     EquipSlot::WeaponRight, RPR, 3, WeaponStats(51.0f, 0.17f, 0.22f, 0.18f, 16.0f) },

        //--- 槍 : 長射程 ---------------------------------------------------------
        { 140, "アイアンランス",       "間合いを制する長柄武器。",                 EquipSlot::WeaponRight, SPR, 1, WeaponStats(31.0f, 0.06f, 0.12f, -0.04f) },
        { 141, "ストームパイク",       "突きの衝撃が空気を裂く。",                 EquipSlot::WeaponRight, SPR, 2, WeaponStats(47.0f, 0.08f, 0.16f, -0.02f) },
        { 142, "蝕の穿槍",             "届かぬものなしと謳われた穂先。",           EquipSlot::WeaponRight, SPR, 3, WeaponStats(66.0f, 0.10f, 0.22f,  0.00f) },

        //--- ユニークスキル「神聖剣」専用のセット武器 -----------------------------
        //   特別クエストでのみ入手できる。通常のドロップ抽選には出ない。
        { 150, "神聖剣グレイス",       "聖別された白刃。折れることなく持ち主を守る。", EquipSlot::WeaponRight, SWD, 3, WeaponStats(96.0f, 0.12f, 0.30f, 0.06f) },

        //--- 竜王の火山のボスドロップ（部位ごとに 1 つずつ）----------------------
        { 160, "ヴァルグリム・ドラゴンソード", "竜王の牙から鍛えた大剣。振るうたびに熱を帯びる。", EquipSlot::WeaponRight, SWD, 3, WeaponStats(132.0f, 0.11f, 0.34f, 0.05f) },

        //--- 頭装備 -------------------------------------------------------------
        { 200, "レザーキャップ",       "軽い革の帽子。",                           EquipSlot::Head, SWD, 1, ArmorStats(6.0f,  40.0f, 10.0f) },
        { 201, "アイアンヘルム",       "視界は狭いが頑丈。",                       EquipSlot::Head, SWD, 2, ArmorStats(12.0f, 75.0f,  6.0f) },
        { 202, "月光のサークレット",   "MP の巡りを良くする装飾。",                 EquipSlot::Head, SWD, 2, ArmorStats(8.0f,  45.0f, 38.0f, 0.02f, 0.0f, 0.05f) },
        { 203, "蝕の兜",               "闇を見通す視界を得る。",                   EquipSlot::Head, SWD, 3, ArmorStats(19.0f, 110.0f, 24.0f, 0.03f) },
        { 204, "エンシェントドラゴンヘルム", "竜鱗を重ねた兜。灼熱の息すら通さない。", EquipSlot::Head, SWD, 3, ArmorStats(42.0f, 280.0f, 60.0f, 0.04f) },

        //--- 体装備 -------------------------------------------------------------
        { 210, "レザーアーマー",       "動きを妨げない革鎧。",                     EquipSlot::Body, SWD, 1, ArmorStats(11.0f, 80.0f,  8.0f, 0.0f, 6.0f) },
        { 211, "チェインメイル",       "斬撃に強い鎖帷子。",                       EquipSlot::Body, SWD, 2, ArmorStats(21.0f, 140.0f, 4.0f) },
        { 212, "月光のコート",         "魔力を織り込んだ外套。",                   EquipSlot::Body, SWD, 2, ArmorStats(15.0f, 100.0f, 52.0f, 0.0f, 10.0f, 0.08f) },
        { 213, "蝕の鎧",               "蝕の夜を纏うかのような漆黒。",             EquipSlot::Body, SWD, 3, ArmorStats(32.0f, 210.0f, 30.0f, 0.02f, 8.0f) },
        { 214, "エンシェントドラゴンメイル", "古竜の背鱗を並べた鎧。重さを感じさせない。", EquipSlot::Body, SWD, 3, ArmorStats(68.0f, 520.0f, 70.0f, 0.02f, 6.0f) },

        //--- 盾 -----------------------------------------------------------------
        { 220, "ラウンドシールド",     "取り回しの良い小盾。",                     EquipSlot::Shield, SWD, 1, ArmorStats(9.0f,  55.0f, 0.0f) },
        { 221, "カイトシールド",       "全身を隠せる大盾。",                       EquipSlot::Shield, SWD, 2, ArmorStats(18.0f, 105.0f, 0.0f, 0.0f, -6.0f) },
        { 222, "蝕の盾",               "受けた衝撃を闇へ逃がす。",                 EquipSlot::Shield, SWD, 3, ArmorStats(28.0f, 160.0f, 18.0f) },
        // ユニークスキル「神聖剣」専用のセット武器（特別クエストでのみ入手）
        { 223, "聖盾エーギス",         "神聖剣と対になる盾。あらゆる刃を受け止める。", EquipSlot::Shield, SWD, 3, ArmorStats(125.0f, 480.0f, 70.0f) },
        { 224, "エンシェントドラゴンシールド", "竜の翼膜を張った盾。炎を受け流す。", EquipSlot::Shield, SWD, 3, ArmorStats(58.0f, 380.0f, 40.0f) },

        //--- 腕装備 -------------------------------------------------------------
        { 230, "レザーブレイサー",     "手首を守る革当て。",                       EquipSlot::Arm, SWD, 1, ArmorStats(5.0f, 30.0f, 6.0f, 0.02f) },
        { 231, "鋼の篭手",             "打撃の威力を底上げする。",                 EquipSlot::Arm, SWD, 2, ArmorStats(10.0f, 55.0f, 0.0f, 0.04f) },
        { 232, "エンシェントドラゴンアーム", "竜爪を模した篭手。急所を的確に捉える。", EquipSlot::Arm, SWD, 3, ArmorStats(26.0f, 160.0f, 30.0f, 0.06f) },

        //--- 手装備 -------------------------------------------------------------
        { 240, "戦士のグローブ",       "武器を握る力が増す。",                     EquipSlot::Hand, SWD, 1, ArmorStats(4.0f, 24.0f, 4.0f, 0.03f) },
        { 241, "疾風の手甲",           "振りの速度が上がる。",                     EquipSlot::Hand, SWD, 2, ArmorStats(7.0f, 38.0f, 8.0f, 0.03f) },
        { 242, "エンシェントドラゴングローブ", "竜の握力を宿す手甲。武器がぴたりと吸い付く。", EquipSlot::Hand, SWD, 3, ArmorStats(20.0f, 120.0f, 24.0f, 0.05f) },

        //--- 足装備 -------------------------------------------------------------
        { 250, "レザーブーツ",         "長時間の探索に向く。",                     EquipSlot::Foot, SWD, 1, ArmorStats(5.0f, 28.0f, 4.0f, 0.0f, 18.0f) },
        { 251, "韋駄天のブーツ",       "駆け抜ける者のための靴。",                 EquipSlot::Foot, SWD, 2, ArmorStats(9.0f, 46.0f, 8.0f, 0.0f, 34.0f) },
        { 252, "エンシェントドラゴンブーツ", "溶岩の上でも足を取られない具足。", EquipSlot::Foot, SWD, 3, ArmorStats(24.0f, 150.0f, 26.0f, 0.0f, 52.0f) },

        //======================================================================
        // ショップ専用の武器（ドロップには出ない）
        //======================================================================
        //--- 片手剣 -------------------------------------------------------------
        { 300, "鉄製ロングソード",   "扱いやすい標準的な剣。",         EquipSlot::WeaponRight, SWD, 1, WeaponStats(30.0f, 0.05f, 0.10f,  0.00f) },
        { 301, "傭兵の剣",           "頑丈で実戦向き。",               EquipSlot::WeaponRight, SWD, 1, WeaponStats(34.0f, 0.05f, 0.12f, -0.01f) },
        { 302, "騎士見習いの剣",     "切れ味と見栄えを両立。",         EquipSlot::WeaponRight, SWD, 2, WeaponStats(42.0f, 0.07f, 0.14f,  0.01f) },

        //--- 片手棍 -------------------------------------------------------------
        { 310, "戦鎚",               "重い一撃を叩き込む。",           EquipSlot::WeaponRight, MCE, 2, WeaponStats(50.0f, 0.03f, 0.20f, -0.11f) },
        { 311, "聖職者のメイス",     "儀礼用にも使われる。",           EquipSlot::WeaponRight, MCE, 2, WeaponStats(43.0f, 0.05f, 0.16f, -0.07f) },

        //--- 細剣 ---------------------------------------------------------------
        { 320, "スティレット",       "細身で急所を狙いやすい。",       EquipSlot::WeaponRight, RPR, 1, WeaponStats(22.0f, 0.14f, 0.12f, 0.13f,  8.0f) },
        { 321, "フェンシングソード", "軽快な刺突武器。",               EquipSlot::WeaponRight, RPR, 1, WeaponStats(26.0f, 0.11f, 0.13f, 0.14f, 10.0f) },
        { 322, "貴族仕立ての細剣",   "装飾性の高い上品な剣。",         EquipSlot::WeaponRight, RPR, 2, WeaponStats(35.0f, 0.13f, 0.16f, 0.15f, 12.0f) },

        //--- 槍 -----------------------------------------------------------------
        { 330, "木柄の鉄槍",         "安価で扱いやすい。",             EquipSlot::WeaponRight, SPR, 1, WeaponStats(29.0f, 0.05f, 0.11f, -0.05f) },
        { 331, "パイク",             "長い間合いを活かす。",           EquipSlot::WeaponRight, SPR, 2, WeaponStats(44.0f, 0.06f, 0.15f, -0.03f) },
        { 332, "ハルバード",         "刺突と斬撃を兼ね備える。",       EquipSlot::WeaponRight, SPR, 2, WeaponStats(48.0f, 0.07f, 0.17f, -0.05f) },

        //--- 短剣 ---------------------------------------------------------------
        { 340, "ハンターナイフ",     "狩猟にも使える実用品。",         EquipSlot::WeaponRight, DGR, 1, WeaponStats(19.0f, 0.14f, 0.13f, 0.17f, 12.0f) },
        { 341, "鋼のダガー",         "冒険者向けの標準品。",           EquipSlot::WeaponRight, DGR, 1, WeaponStats(22.0f, 0.16f, 0.14f, 0.18f, 12.0f) },
        { 342, "暗殺者の短剣",       "細身で隠し持ちやすい。",         EquipSlot::WeaponRight, DGR, 2, WeaponStats(29.0f, 0.21f, 0.18f, 0.21f, 16.0f) },

        //======================================================================
        // ショップ専用の防具（ドロップには出ない）
        //======================================================================
        { 350, "革の帽子",           "軽量で最低限の防護。",           EquipSlot::Head, SWD, 1, ArmorStats( 5.0f, 32.0f,  8.0f) },
        { 351, "騎士の兜",           "防御力の高い重装備。",           EquipSlot::Head, SWD, 2, ArmorStats(15.0f, 92.0f,  4.0f) },

        { 360, "布の服",             "防御力は低いが安価。",           EquipSlot::Body, SWD, 1, ArmorStats( 6.0f,  50.0f, 18.0f) },
        { 361, "革鎧",               "軽装冒険者の定番。",             EquipSlot::Body, SWD, 1, ArmorStats(12.0f,  84.0f,  8.0f, 0.0f,   6.0f) },
        { 362, "プレートアーマー",   "重厚で高い防御力。",             EquipSlot::Body, SWD, 2, ArmorStats(26.0f, 165.0f,  0.0f, 0.0f, -10.0f) },

        { 370, "木製ラウンドシールド", "軽くて扱いやすい。",           EquipSlot::Shield, SWD, 1, ArmorStats( 7.0f, 44.0f, 0.0f) },
        { 371, "アイアンシールド",   "頑丈な金属製の盾。",             EquipSlot::Shield, SWD, 1, ArmorStats(13.0f, 78.0f, 0.0f, 0.0f, -3.0f) },

        { 380, "革の腕当て",         "軽装向けの基本装備。",           EquipSlot::Arm, SWD, 1, ArmorStats( 4.0f, 26.0f, 5.0f, 0.02f) },
        { 381, "アイアンアームガード", "腕を金属で保護。",             EquipSlot::Arm, SWD, 1, ArmorStats( 8.0f, 44.0f, 0.0f, 0.03f) },
        { 382, "騎士の小手甲",       "重装備用の腕防具。",             EquipSlot::Arm, SWD, 2, ArmorStats(13.0f, 68.0f, 0.0f, 0.04f) },

        { 390, "革手袋",             "安価で使いやすい。",             EquipSlot::Hand, SWD, 1, ArmorStats( 3.0f, 20.0f, 4.0f, 0.02f) },
        { 391, "ハードレザーグローブ", "戦闘向けの丈夫な手袋。",       EquipSlot::Hand, SWD, 1, ArmorStats( 5.0f, 30.0f, 6.0f, 0.03f) },
        { 392, "アイアングローブ",   "手を金属で覆う重装備。",         EquipSlot::Hand, SWD, 2, ArmorStats( 9.0f, 46.0f, 0.0f, 0.03f) },

        { 395, "革のブーツ",         "冒険者の基本装備。",             EquipSlot::Foot, SWD, 1, ArmorStats( 4.0f, 24.0f, 4.0f, 0.0f, 16.0f) },
        { 396, "グリーブ",           "脛を金属で防護。",               EquipSlot::Foot, SWD, 1, ArmorStats( 8.0f, 42.0f, 0.0f, 0.0f, 10.0f) },
        { 397, "騎士の鉄靴",         "重装兵向けの防具。",             EquipSlot::Foot, SWD, 2, ArmorStats(13.0f, 62.0f, 0.0f, 0.0f,  4.0f) },
    };

    //==========================================================================
    // アクセサリー（ショップ専用）
    //   倍率バフは個体値でも強化でも変わらない固定値にしている。
    //==========================================================================
    const std::vector<ItemTemplate> accessories = {
        MakeAccessory(400, "力の腕輪",           "腕に力が満ちる金属の輪。",
                      AccessoryKind::Bracelet, "攻撃力 +5%",   2500, Rate(0.05f, 0.0f, 0.0f, 0.0f, 0.0f)),
        MakeAccessory(401, "守りの腕輪",         "打撃を逃がす厚い腕輪。",
                      AccessoryKind::Bracelet, "防御力 +5%",   2500, Rate(0.0f, 0.05f, 0.0f, 0.0f, 0.0f)),
        MakeAccessory(402, "魔力の指輪",         "魔力を増幅させる銀の指輪。",
                      AccessoryKind::Ring,     "魔法攻撃力 +5%", 1800, StatRates(),
                      FlatBonus(0.0f, 0.05f)),
        MakeAccessory(403, "精神の指輪",         "心を鎮め、魔を退ける指輪。",
                      AccessoryKind::Ring,     "魔法防御力 +5%", 1800, StatRates(),
                      FlatBonus(0.0f, 0.0f, 0.05f)),
        MakeAccessory(404, "俊足のアンクレット", "足取りが軽くなる足飾り。",
                      AccessoryKind::Anklet,   "素早さ +5%",   2500, Rate(0.0f, 0.0f, 0.0f, 0.0f, 0.05f)),
        MakeAccessory(405, "生命のペンダント",   "持ち主の生命力を支える石。",
                      AccessoryKind::Pendant,  "最大HP +5%",   2800, Rate(0.0f, 0.0f, 0.05f, 0.0f, 0.0f)),
        MakeAccessory(406, "魔力のペンダント",   "魔力の器を広げる石。",
                      AccessoryKind::Pendant,  "最大MP +5%",   2200, Rate(0.0f, 0.0f, 0.0f, 0.05f, 0.0f)),
        MakeAccessory(407, "会心のお守り",       "急所を見抜く勘が冴える。",
                      AccessoryKind::Charm,    "クリティカル率 +3%", 4000, StatRates(),
                      FlatBonus(0.03f)),
        MakeAccessory(408, "火除けの耳飾り",     "炎の熱を和らげる耳飾り。",
                      AccessoryKind::Earring,  "火属性ダメージ -5%", 1800, StatRates(),
                      FlatBonus(0.0f, 0.0f, 0.0f, 0.0f, 0.05f)),
        MakeAccessory(409, "水除けの耳飾り",     "水気を払う耳飾り。",
                      AccessoryKind::Earring,  "水属性ダメージ -5%", 1800, StatRates(),
                      FlatBonus(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.05f)),
        MakeAccessory(410, "集中の眼鏡",         "狙いが定まる薄い眼鏡。",
                      AccessoryKind::Glasses,  "命中率 +5%",   1800, StatRates(),
                      FlatBonus(0.0f, 0.0f, 0.0f, 0.05f)),
        MakeAccessory(411, "旅人の護符",         "旅立ちを後押しする古い護符。",
                      AccessoryKind::Talisman, "戦闘開始時、攻撃力 +5%（20 秒）", 3200, StatRates()),
    };
    templates_.insert(templates_.end(), accessories.begin(), accessories.end());

    // 旅人の護符だけは戦闘開始時の時限バフを持つ
    for (ItemTemplate& t : templates_) {
        if (t.id != 411) continue;
        t.openingAttackRate = 0.05f;
        t.openingDuration = 20.0f;
    }

    // ショップの価格を割り当てる
    for (const ShopPrice& entry : kShopPrices) {
        for (ItemTemplate& t : templates_) {
            if (t.id == entry.id) t.price = entry.price;
        }
    }

    // 手装備の攻撃速度補正は個別に付与
    for (ItemTemplate& t : templates_) {
        if (t.slot == EquipSlot::Hand) {
            if (t.id == 242) t.base.attackSpeed = 0.14f;
            else if (t.id == 241) t.base.attackSpeed = 0.10f;
            else t.base.attackSpeed = 0.05f;
        }
        // 神聖剣のセット武器は特別枠（ランダム抽選には出さない）。
        // さらに耐久力が 0 になっても消滅せず、性能が落ちるだけにする。
        if (t.id == kHolySwordSwordId || t.id == kHolySwordShieldId) {
            t.special = true;
            t.indestructible = true;
        }
        // 竜王のセットもボスドロップ専用（ランダム抽選には出さない）
        if (IsDragonSetItem(t.id)) t.special = true;
        // ショップ専用の装備とアクセサリーはドロップ抽選に出さない
        if (t.id >= kShopOnlyIdBase) t.special = true;
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

EquipmentItem ItemDatabase::Create(int templateId, int iv) const
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
    item.iv = ClampIv(iv);
    item.upgradeLevel = 0;
    item.indestructible = tmpl->indestructible;
    item.accessory = tmpl->accessory;
    item.rates = tmpl->rates;
    item.effect = tmpl->effect;
    item.openingAttackRate = tmpl->openingAttackRate;
    item.openingDuration = tmpl->openingDuration;

    if (tmpl->slot == EquipSlot::Accessory) {
        // アクセサリーは個体値で揺らがない（効果は表どおりの固定値）。
        // 摩耗も強化もしないので、耐久力は常に満タンのまま。
        item.iv = kAccessoryIv;
        item.baseStats = tmpl->base;
        item.RestoreDurability();
        return item;
    }

    // 能力値は個体値だけで決まる（同じ個体値なら必ず同じ性能になる）
    const float scale = IvStatScale(item.iv);
    item.baseStats = tmpl->base.Scaled(scale);

    // クリティカル率と攻撃速度は倍率が効き過ぎないよう補正
    item.baseStats.critRate = tmpl->base.critRate * (1.0f + (scale - 1.0f) * 0.45f);
    item.baseStats.attackSpeed = tmpl->base.attackSpeed * (1.0f + (scale - 1.0f) * 0.30f);

    item.RestoreDurability();
    return item;
}

std::vector<const ItemTemplate*> ItemDatabase::ShopItems() const
{
    std::vector<const ItemTemplate*> result;
    for (const ItemTemplate& t : templates_) {
        if (t.InShop()) result.push_back(&t);
    }
    return result;
}

std::vector<const ItemTemplate*> ItemDatabase::ShopItemsForSlot(EquipSlot slot) const
{
    std::vector<const ItemTemplate*> result;
    for (const ItemTemplate& t : templates_) {
        if (!t.InShop() || t.slot != slot) continue;
        result.push_back(&t);
    }
    return result;
}

std::vector<const ItemTemplate*> ItemDatabase::ShopArmors() const
{
    std::vector<const ItemTemplate*> result;
    for (const ItemTemplate& t : templates_) {
        if (!t.InShop()) continue;
        if (t.slot == EquipSlot::WeaponRight || t.slot == EquipSlot::WeaponLeft) continue;
        if (t.slot == EquipSlot::Accessory) continue;
        result.push_back(&t);
    }
    return result;
}

EquipmentItem ItemDatabase::CreateRandom(EquipSlot slot, int iv, int maxTier) const
{
    std::vector<int> candidates;
    for (const ItemTemplate& t : templates_) {
        if (t.special) continue;   // 特別枠はランダム抽選に出さない
        if (t.slot == slot && t.tier <= maxTier) candidates.push_back(t.id);
    }
    if (candidates.empty()) return EquipmentItem();

    const int index = math::RandInt(0, static_cast<int>(candidates.size()) - 1);
    return Create(candidates[static_cast<size_t>(index)], iv);
}

EquipmentItem ItemDatabase::CreateRandomAny(int iv, int maxTier) const
{
    std::vector<int> candidates;
    for (const ItemTemplate& t : templates_) {
        if (t.special) continue;   // 特別枠はランダム抽選に出さない
        if (t.tier <= maxTier) candidates.push_back(t.id);
    }
    if (candidates.empty()) return EquipmentItem();

    const int index = math::RandInt(0, static_cast<int>(candidates.size()) - 1);
    return Create(candidates[static_cast<size_t>(index)], iv);
}

int StarterWeaponId(WeaponType type)
{
    switch (type) {
    case WeaponType::OneHandSword: return 100; // アイアンソード
    case WeaponType::OneHandMace:  return 110; // アイアンメイス
    case WeaponType::Dagger:       return 120; // ショートダガー
    case WeaponType::Rapier:       return 130; // フルーレ
    case WeaponType::Spear:        return 140; // アイアンランス
    default:                       return 100;
    }
}

std::vector<EquipmentItem> ItemDatabase::CreateStarterSet(WeaponType weapon) const
{
    std::vector<EquipmentItem> items;
    // 選んだ武器種の武器だけを配る（選ばなかった武器種は手に入らない）
    items.push_back(Create(StarterWeaponId(weapon), kStarterIv));
    items.push_back(Create(200, kStarterIv)); // レザーキャップ
    items.push_back(Create(210, kStarterIv)); // レザーアーマー
    items.push_back(Create(220, kStarterIv)); // ラウンドシールド
    return items;
}

} // namespace ecl
