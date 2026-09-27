#include "Game/WeaponMotion.h"

#include "Common/MathUtil.h"

#include <cstddef>

namespace ecl {

namespace {

//==============================================================================
// 通常攻撃のコンボ表
//   1 コンボあたりの「倍率の合計」を「所要時間の合計」で割った値はどの武器種でも
//   約 3.2 倍/秒 に揃えてあり、差が出るのは振りの速さと当たり方のみ。
//==============================================================================

// 片手剣：斬り下ろし → 斬り上げ → 横薙ぎ（素直な 3 段）
const ComboStep kSwordCombo[] = {
    { "斬り下ろし", 0.34f, 0.11f, 1.00f, 1.30f, 0.90f,  0.00f, 1.00f, 140.0f, 180.0f, 0.035f, 0 },
    { "斬り上げ",   0.36f, 0.12f, 1.15f, 1.35f, 0.95f,  0.04f, 1.00f, 160.0f, 200.0f, 0.035f, 0 },
    { "横薙ぎ",     0.54f, 0.17f, 1.85f, 1.55f, 0.85f,  0.00f, 1.45f, 380.0f, 260.0f, 0.070f, 1 },
};

// 片手棍：叩きつけ → 大振り（重い 2 段 / 吹き飛ばしが強い）
const ComboStep kMaceCombo[] = {
    { "叩きつけ",   0.46f, 0.20f, 1.38f, 1.25f, 1.05f, -0.02f, 1.15f, 240.0f, 150.0f, 0.055f, 0 },
    { "大振り",     0.62f, 0.24f, 2.10f, 1.45f, 0.95f,  0.00f, 1.70f, 480.0f, 210.0f, 0.090f, 1 },
};

// 短剣：刺突 → 斬り払い → 逆手斬り → 回転二連（手数で稼ぐ 4 段）
//   間合いは変更前（全武器共通コンボ）と同じ 1.30 / 1.35 / 1.55 を保つ。
const ComboStep kDaggerCombo[] = {
    { "刺突",       0.22f, 0.07f, 0.68f, 1.30f, 0.66f,  0.00f, 0.80f,  90.0f, 210.0f, 0.025f, 2 },
    { "斬り払い",   0.23f, 0.07f, 0.70f, 1.35f, 0.85f,  0.00f, 0.95f, 100.0f, 220.0f, 0.025f, 3 },
    { "逆手斬り",   0.24f, 0.08f, 0.68f, 1.35f, 0.90f,  0.02f, 0.95f, 100.0f, 220.0f, 0.025f, 3 },
    { "回転二連",   0.38f, 0.12f, 1.36f, 1.55f, 0.95f,  0.00f, 1.60f, 260.0f, 250.0f, 0.060f, 4 },
};

// 細剣：突き → 二段突き → 踏み込み刺突（判定は細いが伸びる 3 段）
const ComboStep kRapierCombo[] = {
    { "突き",         0.28f, 0.09f, 0.82f, 1.35f, 0.62f, 0.00f, 0.75f, 110.0f, 240.0f, 0.030f, 2 },
    { "二段突き",     0.28f, 0.09f, 0.86f, 1.40f, 0.62f, 0.04f, 0.75f, 120.0f, 250.0f, 0.030f, 2 },
    { "踏み込み刺突", 0.48f, 0.15f, 1.65f, 1.75f, 0.66f, 0.00f, 0.80f, 300.0f, 430.0f, 0.075f, 2 },
};

// 槍：長槍突き → 薙ぎ払い（間合いと範囲で押す 2 段）
const ComboStep kSpearCombo[] = {
    { "長槍突き",   0.42f, 0.14f, 1.40f, 1.55f, 0.66f, 0.00f, 0.80f, 200.0f, 200.0f, 0.050f, 2 },
    { "薙ぎ払い",   0.66f, 0.21f, 2.06f, 1.45f, 1.00f, 0.00f, 1.80f, 420.0f, 160.0f, 0.085f, 1 },
};

template <std::size_t N>
constexpr ComboChain MakeChain(const ComboStep (&steps)[N])
{
    return ComboChain{ steps, static_cast<int>(N) };
}

const ComboChain kChains[static_cast<int>(WeaponType::Count)] = {
    MakeChain(kSwordCombo),
    MakeChain(kMaceCombo),
    MakeChain(kDaggerCombo),
    MakeChain(kRapierCombo),
    MakeChain(kSpearCombo),
};

//==============================================================================
// モーションのキーフレーム
//   t は 0〜1 の進行度。キー間は線形に補間する。
//==============================================================================
struct SwingKey
{
    float t;
    float angleDeg;
    float reachOut;
    float liftUp;
    float lean;
    float crouch;
};

struct SwingMotion
{
    const SwingKey* keys = nullptr;
    int count = 0;
    float offHandDeg = 48.0f;
};

// --- 片手剣 -------------------------------------------------------------------
const SwingKey kSwordSlashDown[] = {
    { 0.00f,   70.0f, 0.00f,  0.00f,  0.00f, 0.02f },
    { 0.20f, -120.0f, 0.00f,  0.07f, -0.05f, 0.06f },  // 振りかぶり
    { 0.36f,   45.0f, 0.05f, -0.01f,  0.14f, 0.05f },  // 斬り下ろし
    { 0.50f,   88.0f, 0.03f, -0.02f,  0.11f, 0.06f },  // 振り切り
    { 0.70f,   78.0f, 0.01f,  0.00f,  0.05f, 0.03f },
    { 1.00f,   70.0f, 0.00f,  0.00f,  0.00f, 0.02f },
};
const SwingKey kSwordSlashUp[] = {
    { 0.00f,   70.0f, 0.00f,  0.00f,  0.00f, 0.02f },
    { 0.18f,  122.0f, 0.00f, -0.05f,  0.02f, 0.07f },  // 下段に沈める
    { 0.36f,  -30.0f, 0.06f,  0.05f,  0.12f, 0.02f },  // 斬り上げ
    { 0.50f,  -62.0f, 0.04f,  0.06f,  0.08f, 0.00f },
    { 0.72f,   40.0f, 0.01f,  0.01f,  0.04f, 0.02f },
    { 1.00f,   70.0f, 0.00f,  0.00f,  0.00f, 0.02f },
};
const SwingKey kSwordSweep[] = {
    { 0.00f,   70.0f, 0.00f,  0.00f,  0.00f, 0.03f },
    { 0.18f,  195.0f,-0.06f, -0.02f, -0.06f, 0.08f },  // 大きく引く
    { 0.34f,    0.0f, 0.09f,  0.02f,  0.18f, 0.04f },  // 薙ぎ払う
    { 0.48f,  -45.0f, 0.06f,  0.03f,  0.14f, 0.02f },
    { 0.72f,   30.0f, 0.02f,  0.00f,  0.06f, 0.03f },
    { 1.00f,   70.0f, 0.00f,  0.00f,  0.00f, 0.03f },
};

// --- 片手棍 -------------------------------------------------------------------
const SwingKey kMaceSmash[] = {
    { 0.00f,   70.0f, 0.00f,  0.00f,  0.00f, 0.03f },
    { 0.30f, -125.0f, 0.00f,  0.10f, -0.08f, 0.09f },  // ゆっくり担ぎ上げる
    { 0.34f, -135.0f, 0.00f,  0.11f, -0.09f, 0.10f },  // 溜め
    { 0.42f,   75.0f, 0.04f, -0.03f,  0.16f, 0.10f },  // 叩き落とす
    { 0.52f,   92.0f, 0.02f, -0.04f,  0.12f, 0.12f },
    { 0.72f,   84.0f, 0.01f, -0.01f,  0.06f, 0.06f },
    { 1.00f,   70.0f, 0.00f,  0.00f,  0.00f, 0.03f },
};
const SwingKey kMaceRoundSwing[] = {
    { 0.00f,   70.0f, 0.00f,  0.00f,  0.00f, 0.03f },
    { 0.22f,  205.0f,-0.08f, -0.04f, -0.08f, 0.09f },  // 遠心力を溜める
    { 0.42f,   30.0f, 0.10f,  0.00f,  0.20f, 0.06f },  // 振り抜く
    { 0.56f,  -30.0f, 0.07f,  0.03f,  0.16f, 0.03f },
    { 0.78f,   36.0f, 0.02f,  0.00f,  0.06f, 0.04f },
    { 1.00f,   70.0f, 0.00f,  0.00f,  0.00f, 0.03f },
};

// --- 短剣 ---------------------------------------------------------------------
const SwingKey kDaggerStab[] = {
    { 0.00f,   60.0f, 0.00f,  0.00f,  0.00f, 0.02f },
    { 0.14f,   30.0f,-0.05f,  0.02f, -0.03f, 0.04f },  // 引き手
    { 0.34f,   -2.0f, 0.15f,  0.03f,  0.12f, 0.02f },  // 突き出す
    { 0.52f,    8.0f, 0.10f,  0.02f,  0.08f, 0.02f },
    { 1.00f,   60.0f, 0.00f,  0.00f,  0.00f, 0.02f },
};
const SwingKey kDaggerSlash[] = {
    { 0.00f,   60.0f, 0.00f,  0.00f,  0.00f, 0.02f },
    { 0.14f,  -80.0f, 0.00f,  0.06f, -0.04f, 0.03f },
    { 0.34f,   55.0f, 0.08f, -0.01f,  0.13f, 0.02f },
    { 0.54f,   76.0f, 0.04f, -0.01f,  0.08f, 0.02f },
    { 1.00f,   60.0f, 0.00f,  0.00f,  0.00f, 0.02f },
};
const SwingKey kDaggerBackhand[] = {
    { 0.00f,   60.0f, 0.00f,  0.00f,  0.00f, 0.02f },
    { 0.14f,  128.0f,-0.02f, -0.05f,  0.02f, 0.05f },  // 逆手に構える
    { 0.36f,  -48.0f, 0.07f,  0.06f,  0.13f, 0.01f },  // 下から掻き上げる
    { 0.56f,  -66.0f, 0.04f,  0.06f,  0.08f, 0.00f },
    { 1.00f,   60.0f, 0.00f,  0.00f,  0.00f, 0.02f },
};
const SwingKey kDaggerSpin[] = {
    { 0.00f,   60.0f, 0.00f,  0.00f,  0.00f, 0.03f },
    { 0.12f,  -30.0f, 0.02f,  0.05f,  0.04f, 0.05f },
    { 0.34f,  240.0f, 0.06f,  0.00f,  0.10f, 0.04f },  // 一回転
    { 0.56f,  420.0f, 0.04f,  0.00f,  0.06f, 0.03f },  // もう一回転
    { 1.00f,  420.0f, 0.00f,  0.00f,  0.00f, 0.03f },
};

// --- 細剣 ---------------------------------------------------------------------
// 細剣は上体を立てたまま、高い位置から素早く刺す
const SwingKey kRapierThrust[] = {
    { 0.00f,   50.0f, 0.00f,  0.04f,  0.00f, 0.01f },
    { 0.16f,   14.0f,-0.07f,  0.09f, -0.03f, 0.03f },  // 構えて引く
    { 0.36f,   -6.0f, 0.20f,  0.11f,  0.12f, 0.01f },  // 刺す
    { 0.56f,    2.0f, 0.13f,  0.09f,  0.08f, 0.01f },
    { 1.00f,   50.0f, 0.00f,  0.04f,  0.00f, 0.01f },
};
const SwingKey kRapierHighThrust[] = {
    { 0.00f,   55.0f, 0.00f,  0.00f,  0.00f, 0.02f },
    { 0.14f,   -8.0f,-0.06f,  0.07f, -0.03f, 0.04f },  // 上段から狙う
    { 0.36f,  -12.0f, 0.20f,  0.08f,  0.15f, 0.02f },
    { 0.56f,   -4.0f, 0.13f,  0.06f,  0.10f, 0.02f },
    { 1.00f,   55.0f, 0.00f,  0.00f,  0.00f, 0.02f },
};
const SwingKey kRapierLunge[] = {
    { 0.00f,   55.0f, 0.00f,  0.00f,  0.00f, 0.03f },
    { 0.16f,   26.0f,-0.10f,  0.03f, -0.07f, 0.09f },  // 大きく沈む
    { 0.36f,   -4.0f, 0.30f,  0.04f,  0.24f, 0.05f },  // 踏み込んで刺す
    { 0.58f,    2.0f, 0.22f,  0.03f,  0.18f, 0.04f },
    { 0.80f,   30.0f, 0.08f,  0.01f,  0.07f, 0.03f },
    { 1.00f,   55.0f, 0.00f,  0.00f,  0.00f, 0.03f },
};

// --- 槍 -----------------------------------------------------------------------
// 槍は腰を落とし、両手で低く押し込む
const SwingKey kSpearThrust[] = {
    { 0.00f,   70.0f, 0.00f, -0.02f,  0.00f, 0.05f },
    { 0.18f,   26.0f,-0.12f, -0.04f, -0.06f, 0.10f },  // 石突を引き絞る
    { 0.38f,   10.0f, 0.28f, -0.01f,  0.20f, 0.06f },  // 長く押し出す
    { 0.58f,   14.0f, 0.19f, -0.02f,  0.14f, 0.06f },
    { 1.00f,   70.0f, 0.00f, -0.02f,  0.00f, 0.05f },
};
const SwingKey kSpearSweep[] = {
    { 0.00f,   62.0f, 0.00f,  0.00f,  0.00f, 0.03f },
    { 0.18f,  200.0f,-0.08f, -0.03f, -0.07f, 0.08f },  // 背後まで引く
    { 0.38f,   10.0f, 0.12f,  0.02f,  0.17f, 0.04f },  // 薙ぎ払う
    { 0.54f,  -26.0f, 0.08f,  0.03f,  0.12f, 0.02f },
    { 0.78f,   34.0f, 0.02f,  0.00f,  0.05f, 0.03f },
    { 1.00f,   62.0f, 0.00f,  0.00f,  0.00f, 0.03f },
};

// --- ソードスキル（演出種別ごと）-----------------------------------------------
const SwingKey kSkillVertical[] = {
    { 0.00f,   70.0f, 0.00f,  0.00f,  0.00f, 0.02f },
    { 0.22f, -140.0f, 0.00f,  0.12f, -0.10f, 0.07f },
    { 0.40f,   60.0f, 0.08f, -0.02f,  0.20f, 0.05f },
    { 0.55f,   95.0f, 0.05f, -0.03f,  0.15f, 0.07f },
    { 0.78f,   80.0f, 0.02f,  0.00f,  0.06f, 0.03f },
    { 1.00f,   70.0f, 0.00f,  0.00f,  0.00f, 0.02f },
};
const SwingKey kSkillHorizontal[] = {
    { 0.00f,   70.0f, 0.00f,  0.00f,  0.00f, 0.03f },
    { 0.20f,  210.0f,-0.08f, -0.04f, -0.09f, 0.09f },
    { 0.42f,   10.0f, 0.12f,  0.02f,  0.22f, 0.05f },
    { 0.58f,  -40.0f, 0.08f,  0.04f,  0.16f, 0.02f },
    { 0.80f,   34.0f, 0.02f,  0.00f,  0.06f, 0.03f },
    { 1.00f,   70.0f, 0.00f,  0.00f,  0.00f, 0.03f },
};
const SwingKey kSkillThrust[] = {
    { 0.00f,   60.0f, 0.00f,  0.00f,  0.00f, 0.03f },
    { 0.20f,   24.0f,-0.12f,  0.03f, -0.09f, 0.10f },
    { 0.40f,   -4.0f, 0.34f,  0.05f,  0.26f, 0.05f },
    { 0.62f,    0.0f, 0.26f,  0.04f,  0.20f, 0.04f },
    { 0.84f,   30.0f, 0.10f,  0.01f,  0.08f, 0.03f },
    { 1.00f,   60.0f, 0.00f,  0.00f,  0.00f, 0.03f },
};
const SwingKey kSkillRush[] = {
    { 0.00f,   60.0f, 0.00f,  0.00f,  0.00f, 0.02f },
    { 0.10f,   10.0f,-0.06f,  0.02f,  0.02f, 0.04f },
    { 0.22f,   -6.0f, 0.20f,  0.04f,  0.14f, 0.02f },
    { 0.34f,   20.0f,-0.04f,  0.02f,  0.06f, 0.04f },
    { 0.46f,   -8.0f, 0.22f,  0.05f,  0.16f, 0.02f },
    { 0.58f,   24.0f,-0.04f,  0.02f,  0.06f, 0.04f },
    { 0.70f,  -10.0f, 0.24f,  0.05f,  0.18f, 0.02f },
    { 0.86f,   30.0f, 0.08f,  0.02f,  0.08f, 0.03f },
    { 1.00f,   60.0f, 0.00f,  0.00f,  0.00f, 0.02f },
};
const SwingKey kSkillSpin[] = {
    { 0.00f,   70.0f, 0.00f,  0.00f,  0.00f, 0.03f },
    { 0.12f,  -40.0f, 0.04f,  0.06f,  0.05f, 0.06f },
    { 0.40f,  250.0f, 0.08f,  0.00f,  0.10f, 0.05f },
    { 0.68f,  610.0f, 0.08f,  0.00f,  0.10f, 0.05f },
    { 0.88f,  780.0f, 0.03f,  0.00f,  0.04f, 0.04f },
    { 1.00f,  790.0f, 0.00f,  0.00f,  0.00f, 0.03f },
};

template <std::size_t N>
constexpr SwingMotion MakeMotion(const SwingKey (&keys)[N], float offHandDeg)
{
    return SwingMotion{ keys, static_cast<int>(N), offHandDeg };
}

// 通常攻撃：武器種 × 段数
const SwingMotion kSwordSwings[] = {
    MakeMotion(kSwordSlashDown, 44.0f),
    MakeMotion(kSwordSlashUp, -40.0f),
    MakeMotion(kSwordSweep, 62.0f),
};
const SwingMotion kMaceSwings[] = {
    MakeMotion(kMaceSmash, 38.0f),
    MakeMotion(kMaceRoundSwing, 70.0f),
};
const SwingMotion kDaggerSwings[] = {
    MakeMotion(kDaggerStab, -34.0f),
    MakeMotion(kDaggerSlash, 52.0f),
    MakeMotion(kDaggerBackhand, -56.0f),
    MakeMotion(kDaggerSpin, 176.0f),
};
const SwingMotion kRapierSwings[] = {
    MakeMotion(kRapierThrust, -30.0f),
    MakeMotion(kRapierHighThrust, 34.0f),
    MakeMotion(kRapierLunge, -26.0f),
};
const SwingMotion kSpearSwings[] = {
    MakeMotion(kSpearThrust, -22.0f),
    MakeMotion(kSpearSweep, 66.0f),
};

struct SwingTable
{
    const SwingMotion* motions;
    int count;
};

// コンボの段数とモーションの本数は必ず一致させる
static_assert(sizeof(kSwordCombo) / sizeof(kSwordCombo[0])
              == sizeof(kSwordSwings) / sizeof(kSwordSwings[0]), "片手剣のモーション数が段数と合っていない");
static_assert(sizeof(kMaceCombo) / sizeof(kMaceCombo[0])
              == sizeof(kMaceSwings) / sizeof(kMaceSwings[0]), "片手棍のモーション数が段数と合っていない");
static_assert(sizeof(kDaggerCombo) / sizeof(kDaggerCombo[0])
              == sizeof(kDaggerSwings) / sizeof(kDaggerSwings[0]), "短剣のモーション数が段数と合っていない");
static_assert(sizeof(kRapierCombo) / sizeof(kRapierCombo[0])
              == sizeof(kRapierSwings) / sizeof(kRapierSwings[0]), "細剣のモーション数が段数と合っていない");
static_assert(sizeof(kSpearCombo) / sizeof(kSpearCombo[0])
              == sizeof(kSpearSwings) / sizeof(kSpearSwings[0]), "槍のモーション数が段数と合っていない");

const SwingTable kSwingTables[static_cast<int>(WeaponType::Count)] = {
    { kSwordSwings,  3 },
    { kMaceSwings,   2 },
    { kDaggerSwings, 4 },
    { kRapierSwings, 3 },
    { kSpearSwings,  2 },
};

// スキル：演出種別ごと（0:縦 1:横 2:突き 3:連撃 4:回転）
const SwingMotion kSkillSwings[] = {
    MakeMotion(kSkillVertical, 44.0f),
    MakeMotion(kSkillHorizontal, 64.0f),
    MakeMotion(kSkillThrust, -28.0f),
    MakeMotion(kSkillRush, -44.0f),
    MakeMotion(kSkillSpin, 182.0f),
};
constexpr int kSkillSwingCount = static_cast<int>(sizeof(kSkillSwings) / sizeof(kSkillSwings[0]));

// 武器種ごとの突き出しの伸び（長い武器ほど大きく前へ出る）
float ExtendScale(WeaponType type)
{
    switch (type) {
    case WeaponType::OneHandSword: return 1.00f;
    case WeaponType::OneHandMace:  return 0.90f;
    case WeaponType::Dagger:       return 0.85f;
    case WeaponType::Rapier:       return 1.15f;
    case WeaponType::Spear:        return 1.25f;
    default: return 1.0f;
    }
}

int WeaponIndex(WeaponType type)
{
    const int index = static_cast<int>(type);
    return math::ClampInt(index, 0, static_cast<int>(WeaponType::Count) - 1);
}

// キーフレーム列から進行度 t の姿勢を作る
WeaponSwing Sample(const SwingMotion& motion, float t, float extendScale)
{
    WeaponSwing swing;
    swing.offHandDeg = motion.offHandDeg;
    if (motion.count <= 0) return swing;

    const float clamped = math::Clamp(t, 0.0f, 1.0f);
    const SwingKey* keys = motion.keys;

    const SwingKey* from = &keys[0];
    const SwingKey* to = &keys[motion.count - 1];
    for (int i = 1; i < motion.count; ++i) {
        if (clamped <= keys[i].t) {
            from = &keys[i - 1];
            to = &keys[i];
            break;
        }
    }

    const float span = to->t - from->t;
    const float local = (span > 0.0001f) ? math::Clamp((clamped - from->t) / span, 0.0f, 1.0f) : 1.0f;

    swing.angleDeg = math::Lerp(from->angleDeg, to->angleDeg, local);
    swing.reachOut = math::Lerp(from->reachOut, to->reachOut, local) * extendScale;
    swing.liftUp = math::Lerp(from->liftUp, to->liftUp, local);
    swing.lean = math::Lerp(from->lean, to->lean, local);
    swing.crouch = math::Lerp(from->crouch, to->crouch, local);
    return swing;
}

} // namespace

//------------------------------------------------------------------------------
const ComboChain& WeaponCombo(WeaponType type)
{
    return kChains[WeaponIndex(type)];
}

int WeaponComboLength(WeaponType type)
{
    return WeaponCombo(type).count;
}

const ComboStep& WeaponComboStep(WeaponType type, int index)
{
    const ComboChain& chain = WeaponCombo(type);
    return chain.steps[math::ClampInt(index, 0, chain.count - 1)];
}

WeaponSwing SampleComboSwing(WeaponType type, int step, float t)
{
    const SwingTable& table = kSwingTables[WeaponIndex(type)];
    const int index = math::ClampInt(step, 0, table.count - 1);
    return Sample(table.motions[index], t, ExtendScale(type));
}

WeaponSwing SampleSkillSwing(WeaponType type, int style, float t)
{
    const int index = math::ClampInt(style, 0, kSkillSwingCount - 1);
    return Sample(kSkillSwings[index], t, ExtendScale(type));
}

} // namespace ecl
