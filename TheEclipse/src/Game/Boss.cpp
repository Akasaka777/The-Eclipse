#include "Game/Boss.h"

#include "Common/MathUtil.h"
#include "Core/DxInclude.h"
#include "Game/ActorAssets.h"
#include "Game/WeaponMotion.h"
#include "Core/GameConfig.h"
#include "Graphics/DrawUtil.h"

#include <cmath>

namespace ecl {

namespace {

const char* AttackName(BossAttackKind kind)
{
    switch (kind) {
    case BossAttackKind::Swipe:    return "薙ぎ払い";
    case BossAttackKind::Charge:   return "突進";
    case BossAttackKind::JumpSlam: return "叩きつけ";
    case BossAttackKind::Shot:     return "邪弾";
    case BossAttackKind::Combo:    return "連続斬り";
    case BossAttackKind::Burst:    return "咆哮";
    case BossAttackKind::SkyVolley:  return "金剛杵の雨";
    case BossAttackKind::SkyThunder: return "天雷";
    case BossAttackKind::SkyHalo:    return "光輪";
    case BossAttackKind::DiveSlam:   return "降魔の掌";
    default: return "攻撃";
    }
}

struct PatternTiming
{
    float windup;
    float attack;
    float recover;
};

PatternTiming TimingFor(BossAttackKind kind)
{
    switch (kind) {
    case BossAttackKind::Swipe:    return { 0.55f, 0.55f, 0.75f };
    case BossAttackKind::Charge:   return { 0.70f, 0.85f, 0.95f };
    case BossAttackKind::JumpSlam: return { 0.60f, 0.90f, 1.00f };
    case BossAttackKind::Shot:     return { 0.65f, 0.50f, 0.80f };
    case BossAttackKind::Combo:    return { 0.50f, 0.95f, 0.85f };
    case BossAttackKind::Burst:    return { 0.85f, 0.45f, 1.10f };
    case BossAttackKind::SkyVolley:  return { 0.75f, 0.55f, 0.65f };
    case BossAttackKind::SkyThunder: return { 1.05f, 0.55f, 0.75f };   // 落ちる場所を長めに予告する
    case BossAttackKind::SkyHalo:    return { 0.80f, 0.50f, 0.70f };
    case BossAttackKind::DiveSlam:   return { 0.85f, 0.80f, 1.90f };   // 着地したあと長く地上にとどまる（反撃のチャンス）
    default: return { 0.6f, 0.6f, 0.8f };
    }
}

// パターンごとの得意間合い
bool RangeMatches(BossAttackKind kind, float distance)
{
    switch (kind) {
    case BossAttackKind::Swipe:    return distance < 260.0f;
    case BossAttackKind::Combo:    return distance < 300.0f;
    case BossAttackKind::Burst:    return distance < 420.0f;
    case BossAttackKind::Charge:   return distance > 240.0f;
    case BossAttackKind::JumpSlam: return distance > 200.0f;
    case BossAttackKind::Shot:     return distance > 320.0f;
    case BossAttackKind::DiveSlam: return distance < 760.0f;
    default: return true;   // 空からの遠距離攻撃はどの間合いでも使う
    }
}

// 仏像の攻撃の構え（CharacterArt の kStatueMotion*）
int StatueMotionFor(BossAttackKind kind)
{
    switch (kind) {
    case BossAttackKind::SkyVolley:  return kStatueMotionVolley;
    case BossAttackKind::SkyThunder: return kStatueMotionThunder;
    case BossAttackKind::SkyHalo:    return kStatueMotionHalo;
    case BossAttackKind::DiveSlam:   return kStatueMotionDive;
    default:                         return kStatueMotionNone;
    }
}

// --- 空を飛ぶボスの動き ---------------------------------------------------------
constexpr float kFlightBob = 12.0f;          // 浮遊の上下の揺れ幅
constexpr float kFlightRiseSpeed = 380.0f;   // 浮き上がる速さ
constexpr float kFlightSinkSpeed = 320.0f;   // ゆっくり下がる速さ
constexpr float kDiveSpeed = 1500.0f;        // 降魔の掌の急降下の速さ
constexpr float kFlightNear = 420.0f;        // これより近いと離れる
constexpr float kFlightFar = 720.0f;         // これより遠いと近づく
constexpr int   kForceDiveAfter = 4;         // 遠距離攻撃がこの回数続いたら、次は必ず降りてくる
constexpr float kDiveChance = 0.20f;         // 降魔の掌を選ぶ確率（最後の形態で +10%）

} // namespace

//==============================================================================
BossDatabase::BossDatabase()
{
    {
        BossDef b;
        b.id = 1;
        b.name = "ファングルフ";
        b.title = "深緑の狼王";
        b.style = ArtStyle::Beast;
        b.main = ColorRGB(62, 74, 68);
        b.accent = ColorRGB(214, 226, 218);
        b.trim = ColorRGB(255, 186, 88);
        b.maxHp = 4600.0f; b.attack = 72.0f; b.defense = 26.0f; b.moveSpeed = 250.0f;
        b.halfWidth = 82.0f; b.height = 190.0f;
        b.assetFolder = "fangwolf";
        b.expReward = 340; b.colReward = 560;
        b.patterns = { BossAttackKind::Swipe, BossAttackKind::Charge, BossAttackKind::Burst,
                       BossAttackKind::JumpSlam };
        bosses_.push_back(b);
    }
    {
        BossDef b;
        b.id = 2;
        b.name = "ゴーレム・ガルド";
        b.title = "石牢の守護者";
        b.style = ArtStyle::Golem;
        b.main = ColorRGB(98, 96, 110);
        b.accent = ColorRGB(198, 198, 212);
        b.trim = ColorRGB(120, 210, 255);
        b.maxHp = 8200.0f; b.attack = 96.0f; b.defense = 52.0f; b.moveSpeed = 150.0f;
        b.halfWidth = 90.0f; b.height = 272.0f;
        b.assetFolder = "golem_guard";
        b.expReward = 620; b.colReward = 980;
        b.patterns = { BossAttackKind::Swipe, BossAttackKind::JumpSlam, BossAttackKind::Shot,
                       BossAttackKind::Burst };
        bosses_.push_back(b);
    }
    {
        BossDef b;
        b.id = 3;
        b.name = "エクリプス・ナイト";
        b.title = "蝕の騎士";
        b.style = ArtStyle::Knight;
        b.main = ColorRGB(38, 32, 58);
        b.accent = ColorRGB(214, 206, 255);
        b.trim = ColorRGB(198, 108, 255);
        b.weapon = WeaponType::OneHandSword;
        b.maxHp = 13000.0f; b.attack = 124.0f; b.defense = 64.0f; b.moveSpeed = 230.0f;
        b.halfWidth = 62.0f; b.height = 250.0f;
        b.assetFolder = "eclipse_knight";
        b.expReward = 1150; b.colReward = 1800;
        b.patterns = { BossAttackKind::Combo, BossAttackKind::Charge, BossAttackKind::Shot,
                       BossAttackKind::JumpSlam, BossAttackKind::Burst, BossAttackKind::Swipe };
        bosses_.push_back(b);
    }
    {
        // ユニークスキル「神聖剣」の特別クエスト用ボス
        BossDef b;
        b.id = 4;
        b.name = "セイクリッド・ガーディアン";
        b.title = "聖剣の守護者";
        b.style = ArtStyle::Knight;
        b.main = ColorRGB(56, 60, 82);
        b.accent = ColorRGB(248, 244, 226);
        b.trim = ColorRGB(255, 214, 128);
        b.weapon = WeaponType::OneHandSword;
        b.maxHp = 11000.0f; b.attack = 112.0f; b.defense = 58.0f; b.moveSpeed = 215.0f;
        b.halfWidth = 60.0f; b.height = 244.0f;
        b.assetFolder = "sacred_guardian";
        b.expReward = 900; b.colReward = 1500;
        b.patterns = { BossAttackKind::Swipe, BossAttackKind::Combo, BossAttackKind::Charge,
                       BossAttackKind::JumpSlam, BossAttackKind::Shot };
        bosses_.push_back(b);
    }
    {
        // 竜王の火山のボス。全パターンを使う最上位個体。
        BossDef b;
        b.id = 5;
        b.name = "ヴァルグリム・ザ・エンシェントドラゴン";
        b.title = "竜王";
        b.style = ArtStyle::Dragon;
        b.main = ColorRGB(94, 30, 26);
        b.accent = ColorRGB(255, 200, 128);
        b.trim = ColorRGB(255, 108, 40);
        b.weapon = WeaponType::OneHandSword;
        b.maxHp = 68000.0f; b.attack = 8600.0f; b.defense = 880.0f; b.moveSpeed = 245.0f;
        b.halfWidth = 86.0f; b.height = 300.0f;
        b.assetFolder = "valgrim";
        b.expReward = 9200; b.colReward = 12000;
        b.patterns = { BossAttackKind::Swipe, BossAttackKind::Charge, BossAttackKind::JumpSlam,
                       BossAttackKind::Shot, BossAttackKind::Combo, BossAttackKind::Burst };
        bosses_.push_back(b);
    }
    {
        // 竜王の火山の隠しボス。上半身だけの金属の仏像で、腕が六本（二本は腕組み）。
        // 空に浮いたまま遠くから攻撃し、ときどき降りてきて掌で叩きつける。
        // 浮いている間はジャンプ攻撃でしか届かない。降りてきて地上にいる間は普通に当たる。
        BossDef b;
        b.id = kVajraBossId;
        b.name = "ヴァジュラ・ラグナシュラ";
        b.title = "六臂の金剛像";
        b.style = ArtStyle::Statue;
        b.main = ColorRGB(156, 116, 58);     // 青銅に金をかぶせた地
        b.accent = ColorRGB(246, 214, 138);  // 磨かれた面
        b.trim = ColorRGB(255, 196, 92);     // 光（光背・攻撃）
        b.maxHp = 76000.0f; b.attack = 9400.0f; b.defense = 940.0f; b.moveSpeed = 230.0f;
        b.halfWidth = 96.0f; b.height = 236.0f;
        b.flying = true;
        b.hoverHeight = 240.0f;   // 地上の攻撃（ソードスキルでも高さ約 210 まで）は届かず、ジャンプ攻撃は届く高さ
        b.assetFolder = "vajra";
        b.expReward = 12500; b.colReward = 16000;
        b.patterns = { BossAttackKind::SkyVolley, BossAttackKind::SkyThunder, BossAttackKind::SkyHalo,
                       BossAttackKind::DiveSlam };
        bosses_.push_back(b);
    }
}

const BossDatabase& BossDatabase::Instance()
{
    static BossDatabase instance;
    return instance;
}

const BossDef* BossDatabase::Find(int id) const
{
    for (const BossDef& b : bosses_) {
        if (b.id == id) return &b;
    }
    return nullptr;
}

//==============================================================================
void Boss::Setup(const BossDef& def, const Vec2& position, float powerScale, int hpBars)
{
    def_ = &def;
    hpBars_ = math::ClampInt(hpBars, kMinBossHpBars, kMaxBossHpBars);
    id = IssueActorId();
    team = Team::Enemy;
    powerScale_ = math::MaxF(0.2f, powerScale);

    pos = position;
    velocity = Vec2(0.0f, 0.0f);
    halfWidth = def.halfWidth;
    height = def.height;

    maxHp = def.maxHp * powerScale_;
    hp = maxHp;
    hpDisplay_ = 1.0f;

    stats = Stats();
    stats.attack = def.attack * powerScale_;
    stats.defense = def.defense * powerScale_;
    stats.moveSpeed = def.moveSpeed;
    stats.critRate = 0.06f;
    stats.critDamage = 0.35f;

    art.style = def.style;
    art.main = def.main;
    art.accent = def.accent;
    art.trim = def.trim;
    art.weapon = def.weapon;
    art.hasWeapon = (def.style == ArtStyle::Knight || def.style == ArtStyle::Humanoid);

    animator.SetSet(ActorAssets::Instance().BossSet(def.id, def.assetFolder));

    // 空を飛ぶボスは浮いた高さから始める
    altitude_ = def.flying ? def.hoverHeight : 0.0f;
    altitudeTarget_ = altitude_;
    groundY_ = position.y;
    stageGroundY_ = position.y;
    pos.y = position.y - altitude_;
    flightTime_ = 0.0f;
    diveTargetX_ = position.x;
    diveLanded_ = false;
    attacksSinceDive_ = 0;
    thunderTargets_.clear();
    lastPlayerPos_ = position;

    alive = true;
    state_ = BossState::Intro;
    stateTimer_ = 0.0f;
    introHeld_ = false;
    barsRevealing_ = false;
    introRoar_ = false;
    roarTimer_ = 0.0f;
    barReveal_ = 1.0f;
    phase_ = 1;
    damageBoost_ = 1.0f;
    speedBoost_ = 1.0f;
    facing = -1;
    headSide_ = -1.0f;
    actionName_.clear();
}

float Boss::PhaseFloorHp() const
{
    // phase_ 本目のバーの下端。最後のバーなら 0（倒れるまで削る）
    const int left = math::MaxI(0, hpBars_ - phase_);
    return maxHp * static_cast<float>(left) / static_cast<float>(hpBars_);
}

void Boss::HoldIntro(bool hold)
{
    if (hold == introHeld_) return;
    introHeld_ = hold;
    if (hold) {
        state_ = BossState::Intro;
        stateTimer_ = 0.0f;
        barReveal_ = 0.0f;
        barsRevealing_ = false;
    } else {
        introRoar_ = false;
        // 演出が終わったら一呼吸おいて動き出す（スキップしたときも同じ）
        stateTimer_ = math::MaxF(stateTimer_, 1.8f);
    }
}

void Boss::RevealHpBars()
{
    barsRevealing_ = true;
}

void Boss::SetIntroRoar(bool roar)
{
    if (roar && !introRoar_) roarTimer_ = 0.0f;
    introRoar_ = roar;
}

int Boss::ApplyDirectDamage(int damage, float knockbackX, CombatSystem& combat)
{
    if (state_ == BossState::Intro || state_ == BossState::PhaseShift) return 0;

    // 1 本のバーを削りきったところで止め、形態変化を必ず挟む
    //   （大技でバーを 2 本まとめて飛ばしたり、形態変化を素通りしたりしない）
    if (alive && phase_ < hpBars_) {
        const float floorHp = PhaseFloorHp();
        const int toFloor = math::MaxI(1, static_cast<int>(std::ceil(hp - floorHp)));
        damage = math::MinI(damage, toFloor);
    }

    // ボスはのけぞらない（ノックバックを大幅に軽減）
    const int dealt = Actor::ApplyDirectDamage(damage, knockbackX * 0.08f, combat);
    if (dealt > 0) CheckPhaseShift(combat);
    return dealt;
}

void Boss::ForceKill(CombatSystem& combat)
{
    if (!alive) return;
    Actor::ApplyDirectDamage(static_cast<int>(std::ceil(hp)) + 1, 0.0f, combat);
}

void Boss::Stagger(float duration)
{
    // 登場演出中とフェーズ変化中は中断できない
    if (!alive || state_ == BossState::Intro || state_ == BossState::PhaseShift) return;

    state_ = BossState::Recover;
    stateTimer_ = 0.0f;
    recoverTime_ = math::MaxF(recoverTime_, duration);
    staggered_ = true;
    velocity.x = -static_cast<float>(facing) * 120.0f;
    actionName_ = "よろけ";
}

void Boss::OnDeath(CombatSystem& combat)
{
    combat.AddImpact(Vec2(pos.x, pos.y - height * 0.5f), art.trim, 60, 800.0f);
    combat.AddRing(Vec2(pos.x, pos.y), 460.0f, art.trim, 0.9f);
    state_ = BossState::Dead;
}

void Boss::CheckPhaseShift(CombatSystem& combat)
{
    // 今のバーを削りきったら次の形態へ（バー 1 本につき 1 回）
    if (!alive || phase_ >= hpBars_ || hp > PhaseFloorHp()) return;

    ++phase_;
    // 最後のバーで攻撃 +36% / 速さ +44%。本数が多いボスほど少しずつ強くなる
    const float progress = static_cast<float>(phase_ - 1) / static_cast<float>(hpBars_ - 1);
    damageBoost_ = 1.0f + 0.36f * progress;
    speedBoost_ = 1.0f + 0.44f * progress;

    state_ = BossState::PhaseShift;
    stateTimer_ = 0.0f;
    velocity.x = 0.0f;
    invincibleTimer = 1.1f;

    combat.AddRing(Vec2(pos.x, pos.y), 520.0f, art.trim, 0.7f);
    combat.AddImpact(Vec2(pos.x, pos.y - height * 0.5f), art.trim, 40, 620.0f);
    combat.AddPopup(Vec2(pos.x, pos.y - height - 60.0f),
                    (phase_ >= hpBars_) ? "激昂！" : "形態変化", art.trim, true);
}

void Boss::ChoosePattern(float distance)
{
    if (!def_ || def_->patterns.empty()) return;

    if (def_->flying) {
        // 空を飛ぶボス：基本は空からの遠距離攻撃。ときどき（約 2 割、最後の形態で約 3 割）降りてきて
        // 掌で叩く。遠距離攻撃が続いたら必ず降りてくる（地上で攻撃できる機会を作る）
        int diveIndex = -1;
        std::vector<int> ranged;
        for (size_t i = 0; i < def_->patterns.size(); ++i) {
            if (def_->patterns[i] == BossAttackKind::DiveSlam) diveIndex = static_cast<int>(i);
            else if (static_cast<int>(i) != lastPatternIndex_) ranged.push_back(static_cast<int>(i));
        }
        if (ranged.empty()) {
            for (size_t i = 0; i < def_->patterns.size(); ++i) {
                if (def_->patterns[i] != BossAttackKind::DiveSlam) ranged.push_back(static_cast<int>(i));
            }
        }
        const float progress = (hpBars_ > 1) ? static_cast<float>(phase_ - 1) / static_cast<float>(hpBars_ - 1) : 0.0f;
        const bool lastWasDive = (lastPatternIndex_ == diveIndex);
        const bool dive = diveIndex >= 0 && !lastWasDive
                       && (ranged.empty() || attacksSinceDive_ >= kForceDiveAfter
                           || (RangeMatches(BossAttackKind::DiveSlam, distance)
                               && math::RandChance(kDiveChance + 0.1f * progress)));
        int pick = diveIndex;
        if (dive) {
            attacksSinceDive_ = 0;
        } else {
            pick = ranged[static_cast<size_t>(math::RandInt(0, static_cast<int>(ranged.size()) - 1))];
            ++attacksSinceDive_;
        }
        lastPatternIndex_ = pick;
        StartWindup(def_->patterns[static_cast<size_t>(pick)]);
        return;
    }

    // 間合いに合うパターンを優先し、直前と同じものは避ける
    std::vector<int> candidates;
    for (size_t i = 0; i < def_->patterns.size(); ++i) {
        if (static_cast<int>(i) == lastPatternIndex_) continue;
        if (RangeMatches(def_->patterns[i], distance)) candidates.push_back(static_cast<int>(i));
    }
    if (candidates.empty()) {
        for (size_t i = 0; i < def_->patterns.size(); ++i) candidates.push_back(static_cast<int>(i));
    }

    const int pick = candidates[static_cast<size_t>(math::RandInt(0, static_cast<int>(candidates.size()) - 1))];
    lastPatternIndex_ = pick;
    StartWindup(def_->patterns[static_cast<size_t>(pick)]);
}

// 予備動作を始める。
//   この時点で向いている方向が、そのまま「この攻撃の狙った方向」になる
//   （予備動作中は振り向かない）。
void Boss::StartWindup(BossAttackKind kind)
{
    currentKind_ = kind;
    actionName_ = AttackName(kind);

    const PatternTiming timing = TimingFor(kind);
    // フェーズが上がるほど予備動作が短くなる
    const float rate = 1.0f / speedBoost_;
    windupTime_ = timing.windup * rate;
    attackTime_ = timing.attack * rate;
    recoverTime_ = timing.recover * rate;

    state_ = BossState::Windup;
    stateTimer_ = 0.0f;
    attackStep_ = 0;
    nextStepTime_ = 0.0f;
    velocity.x = 0.0f;
    animator.Play(PoseKind::Attack, true);

    // 空を飛ぶボスの狙い：予備動作に入った時点のプレイヤーの位置へ降りる / 雷を落とす
    auto clampX = [this](float x) {
        if (stageWidth_ <= 0.0f) return x;
        return math::Clamp(x, halfWidth + 40.0f, stageWidth_ - halfWidth - 40.0f);
    };
    if (kind == BossAttackKind::DiveSlam) {
        diveTargetX_ = clampX(lastPlayerPos_.x);
        diveLanded_ = false;
    } else if (kind == BossAttackKind::SkyThunder) {
        // 足元 → 左右の順に落とす
        thunderTargets_ = { clampX(lastPlayerPos_.x), clampX(lastPlayerPos_.x - 300.0f),
                            clampX(lastPlayerPos_.x + 300.0f) };
        thunderZ_ = lastPlayerZ_;
    }
}

Projectile Boss::MakeProjectile(const Vec2& from, const Vec2& shotVelocity, float radius, float damageMultiplier,
                                float knockback, int kind) const
{
    Projectile projectile;
    projectile.pos = from;
    projectile.velocity = shotVelocity;
    projectile.radius = radius;
    projectile.team = Team::Enemy;
    projectile.sourceId = id;
    projectile.attack = stats.attack * damageBoost_;
    projectile.damageMultiplier = damageMultiplier;
    projectile.critRate = stats.critRate;
    projectile.critDamage = stats.critDamage;
    projectile.knockback = knockback;
    projectile.z = z;
    projectile.life = 3.4f;
    projectile.kind = kind;
    projectile.color = art.trim;
    return projectile;
}

void Boss::SpawnThunder(CombatSystem& combat, float x)
{
    // 予告した奥行きの地面に落とす（雷の柱は高いので、ジャンプしてもよけられない。奥行きをずらしてよける）
    const float ground = stageGroundY_ - thunderZ_;
    HitBox hitBox;
    hitBox.area = Rect(x - 64.0f, ground - 760.0f, x + 64.0f, ground);
    hitBox.team = Team::Enemy;
    hitBox.sourceId = id;
    hitBox.attack = stats.attack * damageBoost_;
    hitBox.damageMultiplier = 1.3f;
    hitBox.critRate = stats.critRate;
    hitBox.critDamage = stats.critDamage;
    hitBox.knockback = 300.0f;
    hitBox.launch = true;
    hitBox.z = thunderZ_;
    hitBox.zRange = config::kHitDepthRange * 1.6f;
    hitBox.life = 0.2f;
    hitBox.color = art.trim;
    combat.AddHitBox(hitBox);
    combat.AddBolt(x, ground - 1100.0f, ground, art.trim, 0.4f);
    combat.AddImpact(Vec2(x, ground - 10.0f), art.trim, 14, 420.0f);
    combat.AddRing(Vec2(x, ground), 160.0f, art.trim, 0.35f);
}

void Boss::SpawnMeleeHit(CombatSystem& combat, float reachScale, float heightScale,
                         float damageMultiplier, float knockback, bool launch)
{
    const float reach = halfWidth * 2.0f * reachScale;
    const float centerX = pos.x + static_cast<float>(facing) * (halfWidth + reach * 0.4f);
    const float centerY = pos.y - height * 0.5f;

    HitBox hitBox;
    hitBox.area = Rect::FromCenter(centerX, centerY, reach, height * heightScale);
    hitBox.team = Team::Enemy;
    hitBox.sourceId = id;
    hitBox.attack = stats.attack * damageBoost_;
    hitBox.damageMultiplier = damageMultiplier;
    hitBox.critRate = stats.critRate;
    hitBox.critDamage = stats.critDamage;
    hitBox.knockback = knockback;
    hitBox.z = z;
    hitBox.zRange = config::kHitDepthRange * 1.2f;
    hitBox.life = 0.16f;
    hitBox.launch = launch;
    hitBox.color = art.trim;
    combat.AddHitBox(hitBox);

    combat.AddSlash(Vec2(centerX, centerY), facing, reach * 0.7f, art.trim, 1);
}

void Boss::ExecuteAttack(CombatSystem& combat)
{
    switch (currentKind_) {
    case BossAttackKind::Swipe:
        SpawnMeleeHit(combat, 1.6f, 1.0f, 1.2f, 360.0f, false);
        break;

    case BossAttackKind::Charge: {
        velocity.x = static_cast<float>(facing) * 1150.0f * speedBoost_;
        // 突進中は当たり続ける判定を本体に貼り付ける
        HitBox hitBox;
        hitBox.area = Rect::FromFoot(pos.x, pos.y, halfWidth * 1.25f, height);
        hitBox.team = Team::Enemy;
        hitBox.sourceId = id;
        hitBox.attack = stats.attack * damageBoost_;
        hitBox.damageMultiplier = 1.15f;
        hitBox.critRate = stats.critRate;
        hitBox.critDamage = stats.critDamage;
        hitBox.knockback = 480.0f;
        hitBox.z = z;
        hitBox.zRange = config::kHitDepthRange * 1.2f;
        hitBox.life = attackTime_;
        hitBox.multiHit = true;
        hitBox.hitInterval = 0.45f;
        hitBox.color = art.trim;
        combat.AddHitBox(hitBox);
        combat.AddDust(Vec2(pos.x, pos.y), facing, 12);
        break;
    }
    case BossAttackKind::JumpSlam:
        velocity.y = -1250.0f;
        velocity.x = static_cast<float>(facing) * 420.0f;
        break;

    case BossAttackKind::Shot: {
        for (int i = -1; i <= 1; ++i) {
            Projectile projectile;
            projectile.pos = Vec2(pos.x + static_cast<float>(facing) * halfWidth, pos.y - height * 0.6f);
            projectile.velocity = Vec2(static_cast<float>(facing) * 620.0f, static_cast<float>(i) * 210.0f);
            projectile.radius = 22.0f;
            projectile.team = Team::Enemy;
            projectile.sourceId = id;
            projectile.attack = stats.attack * damageBoost_;
            projectile.damageMultiplier = 0.9f;
            projectile.critRate = stats.critRate;
            projectile.critDamage = stats.critDamage;
            projectile.knockback = 280.0f;
            projectile.z = z;
            projectile.life = 3.0f;
            projectile.kind = 2;
            projectile.color = art.trim;
            combat.AddProjectile(projectile);
        }
        break;
    }
    case BossAttackKind::Combo:
        SpawnMeleeHit(combat, 1.4f, 0.9f, 0.9f, 180.0f, false);
        break;

    case BossAttackKind::Burst: {
        combat.AddRing(Vec2(pos.x, pos.y), 520.0f, art.trim, 0.55f);
        HitBox hitBox;
        hitBox.area = Rect::FromCenter(pos.x, pos.y - height * 0.4f, 900.0f, height * 1.6f);
        hitBox.team = Team::Enemy;
        hitBox.sourceId = id;
        hitBox.attack = stats.attack * damageBoost_;
        hitBox.damageMultiplier = 1.0f;
        hitBox.critRate = stats.critRate;
        hitBox.critDamage = stats.critDamage;
        hitBox.knockback = 520.0f;
        hitBox.launch = true;
        hitBox.ignoreDepth = true;   // 咆哮は奥行き全域を巻き込む
        hitBox.life = 0.2f;
        hitBox.color = art.trim;
        combat.AddHitBox(hitBox);
        break;
    }
    case BossAttackKind::SkyVolley: {
        // 上の腕から、プレイヤーの胸元へ向けて 5 本を扇状に撃ち下ろす
        const Vec2 from(pos.x + static_cast<float>(facing) * halfWidth * 0.5f, pos.y - height * 0.55f);
        const Vec2 target(lastPlayerPos_.x, lastPlayerPos_.y - 80.0f);
        const float base = std::atan2(target.y - from.y, target.x - from.x);
        for (int i = -2; i <= 2; ++i) {
            const float angle = base + math::DegToRad(9.0f * static_cast<float>(i));
            const Vec2 velocityDir(std::cos(angle), std::sin(angle));
            combat.AddProjectile(MakeProjectile(from, velocityDir * 640.0f, 18.0f, 0.75f, 260.0f, 3));
        }
        combat.AddImpact(from, art.trim, 10, 300.0f);
        break;
    }
    case BossAttackKind::SkyHalo:
        // 地面すれすれを左右へ走る光輪（ジャンプで飛び越える）
        for (int side = -1; side <= 1; side += 2) {
            combat.AddProjectile(MakeProjectile(Vec2(pos.x + static_cast<float>(side) * 30.0f, groundY_ - 72.0f),
                                                Vec2(static_cast<float>(side) * 560.0f, 0.0f), 46.0f, 0.9f, 320.0f, 4));
        }
        combat.AddRing(Vec2(pos.x, groundY_), 260.0f, art.trim, 0.45f);
        break;

    case BossAttackKind::SkyThunder:   // 雷は UpdateAttackPhase で順に落とす
    case BossAttackKind::DiveSlam:     // 急降下も UpdateAttackPhase で進める
        velocity.x = 0.0f;
        break;

    default:
        break;
    }
}

void Boss::UpdateAttackPhase(float dt, CombatSystem& combat, const Vec2& playerPos)
{
    (void)playerPos;

    switch (currentKind_) {
    case BossAttackKind::Charge:
        // 突進の減速
        velocity.x *= 0.985f;
        // 判定を本体に追従させる（置いていくと突進が当たらなくなる）
        combat.MoveHitBoxes(id, Rect::FromFoot(pos.x, pos.y, halfWidth * 1.25f, height), z);
        combat.AddDust(Vec2(pos.x, pos.y), facing, 1);
        break;

    case BossAttackKind::JumpSlam:
        // 着地した瞬間に衝撃波
        if (onGround && attackStep_ == 0 && stateTimer_ > 0.18f) {
            attackStep_ = 1;
            SpawnMeleeHit(combat, 2.2f, 1.2f, 1.6f, 420.0f, true);
            combat.AddRing(Vec2(pos.x, pos.y), 420.0f, art.trim, 0.5f);
            combat.AddDust(Vec2(pos.x, pos.y), 1, 16);

            for (int side = -1; side <= 1; side += 2) {
                Projectile wave;
                wave.pos = Vec2(pos.x + static_cast<float>(side) * halfWidth, pos.y - 30.0f);
                wave.velocity = Vec2(static_cast<float>(side) * 560.0f, 0.0f);
                wave.radius = 34.0f;
                wave.team = Team::Enemy;
                wave.sourceId = id;
                wave.attack = stats.attack * damageBoost_;
                wave.damageMultiplier = 0.8f;
                wave.critRate = stats.critRate;
                wave.critDamage = stats.critDamage;
                wave.knockback = 320.0f;
                wave.z = z;
                wave.life = 1.6f;
                wave.kind = 1;
                wave.color = art.trim;
                combat.AddProjectile(wave);
            }
        }
        break;

    case BossAttackKind::Combo:
        // 3 連撃
        if (stateTimer_ >= nextStepTime_ && attackStep_ < 3) {
            ++attackStep_;
            nextStepTime_ += 0.26f;
            const bool finisher = (attackStep_ == 3);
            SpawnMeleeHit(combat, finisher ? 1.8f : 1.4f, finisher ? 1.1f : 0.9f,
                          finisher ? 1.6f : 0.85f, finisher ? 420.0f : 140.0f, finisher);
            velocity.x = static_cast<float>(facing) * (finisher ? 420.0f : 240.0f);
        }
        break;

    case BossAttackKind::Swipe:
        // 2 連の薙ぎ払い
        if (attackStep_ == 0 && stateTimer_ >= 0.30f) {
            attackStep_ = 1;
            SpawnMeleeHit(combat, 1.7f, 1.1f, 1.5f, 420.0f, false);
        }
        break;

    case BossAttackKind::SkyThunder:
        // 足元 → 左右の順に 0.16 秒おきに落とす
        while (attackStep_ < static_cast<int>(thunderTargets_.size())
               && stateTimer_ >= 0.16f * static_cast<float>(attackStep_)) {
            SpawnThunder(combat, thunderTargets_[static_cast<size_t>(attackStep_)]);
            ++attackStep_;
        }
        velocity.x *= 0.9f;
        break;

    case BossAttackKind::DiveSlam:
        if (!diveLanded_) {
            // 予告した位置へ寄りながら急降下する
            velocity.x = math::Clamp((diveTargetX_ - pos.x) * 7.0f, -1200.0f, 1200.0f);
            if (altitude_ <= 0.5f) {
                // 着地：掌で叩きつけ、左右へ衝撃波。このあと硬直が終わるまで地上にとどまる
                diveLanded_ = true;
                attackStep_ = 1;
                velocity.x = 0.0f;
                SpawnMeleeHit(combat, 2.3f, 1.15f, 1.65f, 460.0f, true);
                combat.AddRing(Vec2(pos.x, pos.y), 460.0f, art.trim, 0.5f);
                combat.AddDust(Vec2(pos.x, pos.y), 1, 18);
                for (int side = -1; side <= 1; side += 2) {
                    combat.AddProjectile(MakeProjectile(Vec2(pos.x + static_cast<float>(side) * halfWidth, pos.y - 30.0f),
                                                        Vec2(static_cast<float>(side) * 560.0f, 0.0f), 34.0f, 0.8f,
                                                        320.0f, 1));
                }
            }
        } else {
            velocity.x *= 0.8f;
        }
        break;

    default:
        velocity.x *= 0.9f;
        break;
    }
    (void)dt;
}

void Boss::TrackDepth(float dt, float playerZ, const Stage& stage)
{
    const float gap = playerZ - z;
    if (math::Abs(gap) <= 6.0f) return;

    const float step = def_->moveSpeed * config::kDepthMoveRate * speedBoost_ * dt;
    MoveDepth(math::Clamp(gap, -step, step), stage);
}

void Boss::UpdateFlightAltitudeTarget()
{
    // ふだんは決めた高さでゆっくり上下に揺れる
    altitudeTarget_ = def_->hoverHeight + std::sin(flightTime_ * 1.7f) * kFlightBob;
    if (currentKind_ == BossAttackKind::DiveSlam) {
        if (state_ == BossState::Windup) altitudeTarget_ = def_->hoverHeight + 50.0f;     // 構え：少し浮き上がる
        else if (state_ == BossState::Attack) altitudeTarget_ = 0.0f;                     // 急降下
        else if (state_ == BossState::Recover && diveLanded_) altitudeTarget_ = 0.0f;     // 硬直の間は地上にとどまる
    }
}

void Boss::KeepFlightDistance(float distance)
{
    // 遠距離攻撃しやすい間合い（420〜720）を保つ。近づかれたら離れ、遠ければ寄る
    const float toward = (lastPlayerPos_.x >= pos.x) ? 1.0f : -1.0f;
    float desired = 0.0f;
    if (distance < kFlightNear) desired = -toward * def_->moveSpeed * 0.9f;
    else if (distance > kFlightFar) desired = toward * def_->moveSpeed;
    velocity.x += (desired * speedBoost_ - velocity.x) * 0.08f;
}

void Boss::ApplyFlightPhysics(float dt, const Stage& stage)
{
    stageGroundY_ = stage.GroundYAt(0.0f);
    groundY_ = stage.GroundYAt(z);
    stageWidth_ = stage.Width();

    const bool diving = (state_ == BossState::Attack && currentKind_ == BossAttackKind::DiveSlam);
    const float speed = (altitudeTarget_ < altitude_) ? (diving ? kDiveSpeed : kFlightSinkSpeed) : kFlightRiseSpeed;
    altitude_ = math::Approach(altitude_, altitudeTarget_, speed * dt);

    pos.x = stage.ClampX(pos.x + velocity.x * dt, halfWidth);
    pos.y = groundY_ - altitude_;
    velocity.y = 0.0f;
    onGround = altitude_ <= 0.5f;
}

void Boss::Update(float dt, const Stage& stage, CombatSystem& combat,
                  const Vec2& playerPos, bool playerAlive, float playerZ)
{
    if (!def_) return;

    lastPlayerPos_ = playerPos;
    lastPlayerZ_ = playerZ;
    flightTime_ += dt;
    if (stageWidth_ <= 0.0f) stageWidth_ = stage.Width();
    stateTimer_ += dt;
    hpDisplay_ = math::Approach(hpDisplay_, HpRatio(), dt * 0.55f);
    roarTimer_ += dt;
    headSide_ = math::Approach(headSide_, static_cast<float>(facing), dt * 5.0f);
    // 登場演出の見せ場（または演出の終わり）から、空の HP バーを満タンまで溜める
    if (!introHeld_ || barsRevealing_) barReveal_ = math::Approach(barReveal_, 1.0f, dt / 0.9f);

    if (!alive) {
        state_ = BossState::Dead;
        velocity.x *= 0.9f;
        ApplyPhysics(dt, stage);   // 空を飛ぶボスも、倒れたら地面へ落ちる
        if (IsFlying()) {
            groundY_ = stage.GroundYAt(z);
            altitude_ = math::MaxF(0.0f, groundY_ - pos.y);
        }
        UpdateTimers(dt);
        pose = PoseKind::Dead;

        // 死亡演出中に断続的な爆発
        if (deathTimer < 2.0f && math::RandChance(dt * 6.0f)) {
            combat.AddImpact(Vec2(pos.x + math::RandFloat(-halfWidth, halfWidth),
                                  pos.y - math::RandFloat(0.0f, height)),
                             art.trim, 12, 420.0f);
        }
        return;
    }

    const float distance = math::Abs(playerPos.x - pos.x);

    switch (state_) {
    case BossState::Intro: {
        velocity.x *= 0.9f;
        pose = PoseKind::Idle;
        FaceTowards(playerPos.x);
        if (!introHeld_ && stateTimer_ >= 2.2f) {
            state_ = BossState::Idle;
            stateTimer_ = 0.0f;
        }
        break;
    }
    case BossState::PhaseShift: {
        velocity.x *= 0.85f;
        pose = PoseKind::Guard;
        if (stateTimer_ >= 1.1f) {
            state_ = BossState::Idle;
            stateTimer_ = 0.0f;
        }
        break;
    }
    case BossState::Idle: {
        if (IsFlying()) KeepFlightDistance(distance);
        else velocity.x *= 0.86f;
        FaceTowards(playerPos.x);
        TrackDepth(dt, playerZ, stage);
        if (!playerAlive) break;

        // 少し間を置いてから次の行動を決める
        const float waitTime = math::MaxF(0.25f, 0.85f / speedBoost_);
        if (stateTimer_ >= waitTime) {
            if (!IsFlying() && distance > 320.0f && math::RandChance(0.45f)) {
                state_ = BossState::Move;
                stateTimer_ = 0.0f;
                moveTarget_ = playerPos.x;
            } else {
                ChoosePattern(distance);
            }
        }
        break;
    }
    case BossState::Move: {
        FaceTowards(playerPos.x);
        TrackDepth(dt, playerZ, stage);
        velocity.x = static_cast<float>(facing) * def_->moveSpeed * speedBoost_;
        if (stateTimer_ >= 0.9f || distance < 220.0f) {
            state_ = BossState::Idle;
            stateTimer_ = 0.0f;
        }
        break;
    }
    case BossState::Windup: {
        // 攻撃モーションに入った時点で向きを固定する（振り向きながら当ててこない）。
        // 奥行きだけは予備動作の前半で寄せる。
        velocity.x *= 0.85f;
        if (stateTimer_ < windupTime_ * 0.5f) {
            TrackDepth(dt, playerZ, stage);
        }
        if (stateTimer_ >= windupTime_) {
            state_ = BossState::Attack;
            stateTimer_ = 0.0f;
            attackStep_ = 0;
            nextStepTime_ = 0.0f;
            ExecuteAttack(combat);
        }
        break;
    }
    case BossState::Attack: {
        UpdateAttackPhase(dt, combat, playerPos);
        if (stateTimer_ >= attackTime_) {
            state_ = BossState::Recover;
            stateTimer_ = 0.0f;
        }
        break;
    }
    case BossState::Recover: {
        velocity.x *= 0.88f;
        if (stateTimer_ >= recoverTime_) {
            state_ = BossState::Idle;
            stateTimer_ = 0.0f;
            staggered_ = false;
        }
        break;
    }
    default:
        break;
    }

    if (IsFlying()) {
        UpdateFlightAltitudeTarget();
        ApplyFlightPhysics(dt, stage);
    } else {
        ApplyPhysics(dt, stage);
    }
    UpdateTimers(dt);
    UpdatePose();
}

void Boss::UpdatePose()
{
    // 攻撃モーションの進行度（予備動作で 0 → kStrikePhase、そこから振り切る）
    constexpr float kStrikePhase = 0.36f;
    motionPhase = -1.0f;
    // 技ごとに違う振り方になるようモーションを割り当てる
    motionVariant = static_cast<int>(currentKind_)
                  % math::MaxI(1, WeaponComboLength(art.weapon));
    const bool statue = (art.style == ArtStyle::Statue);
    if (statue) motionVariant = StatueMotionFor(currentKind_);

    switch (state_) {
    case BossState::Intro:
        // 登場演出の咆哮：口を開き（武器持ちは振りかぶり）、そのまま吠え続ける
        if (introRoar_) {
            pose = PoseKind::Attack;
            motionVariant = statue ? kStatueMotionThunder
                                   : static_cast<int>(BossAttackKind::Burst) % math::MaxI(1, WeaponComboLength(art.weapon));
            motionPhase = math::Clamp(0.15f + roarTimer_ * 1.2f, 0.15f, 0.55f);
        } else {
            pose = PoseKind::Idle;
        }
        break;
    case BossState::Windup:
        pose = PoseKind::Attack;
        if (windupTime_ > 0.0f) {
            motionPhase = kStrikePhase * math::Clamp(stateTimer_ / windupTime_, 0.0f, 1.0f);
        }
        break;
    case BossState::Attack:
        pose = PoseKind::Attack;
        if (attackTime_ > 0.0f) {
            motionPhase = kStrikePhase
                        + (1.0f - kStrikePhase) * math::Clamp(stateTimer_ / attackTime_, 0.0f, 1.0f);
        }
        break;
    case BossState::PhaseShift:
        pose = PoseKind::Guard;
        break;
    case BossState::Recover:
        pose = staggered_ ? PoseKind::Hurt : PoseKind::Idle;
        break;
    case BossState::Move:
        pose = PoseKind::Run;
        break;
    default:
        pose = (math::Abs(velocity.x) > 40.0f) ? PoseKind::Run : PoseKind::Idle;
        break;
    }
    // 空を飛ぶボスは浮いていても跳躍・落下のポーズにしない
    if (!onGround && alive && !IsFlying()) pose = (velocity.y < 0.0f) ? PoseKind::Jump : PoseKind::Fall;
    animator.Play(pose, false);
}

void Boss::Draw(const Camera& camera) const
{
    if (!def_) return;

    // 予備動作の警告（危険度が高いので大きく表示）
    if (alive && state_ == BossState::Windup) {
        const float t = math::Clamp(stateTimer_ / math::MaxF(windupTime_, 0.01f), 0.0f, 1.0f);
        Vec2 screen = camera.WorldToScreen(Vec2(pos.x, pos.y - height - 46.0f));
        // HP バーと名前が出ているときは、名前の上の目印よりさらに上に出す（重ならないように）
        std::vector<Rect> bars;
        Vec2 namePos;
        if (HpBarLayout(camera, bars, namePos)) screen = Vec2(namePos.x, namePos.y - 56.0f);
        draw::Circle(screen.x, screen.y, 22.0f, palette::kDanger, false, 3.0f, 220);
        draw::Circle(screen.x, screen.y, 22.0f * t, palette::kDanger, true, 1.0f, 200);
        draw::Text(FontSize::Small, screen.x, screen.y - 46.0f, palette::kDanger,
                   actionName_, draw::TextAlign::Center);

        // 攻撃範囲の予告
        if (currentKind_ == BossAttackKind::Charge) {
            const Rect area = Rect::FromFoot(pos.x + static_cast<float>(facing) * 500.0f, pos.y,
                                             500.0f, height * 0.9f);
            const Rect screenArea = camera.WorldToScreen(area);
            draw::FillRect(screenArea, palette::kDanger, static_cast<int>(50.0f * t));
            draw::StrokeRect(screenArea, palette::kDanger, 2.0f, static_cast<int>(160.0f * t));
        } else if (currentKind_ == BossAttackKind::Burst) {
            const Vec2 center = camera.WorldToScreen(Vec2(pos.x, pos.y));
            draw::Circle(center.x, center.y, 450.0f * t, palette::kDanger, false, 3.0f, 150);
        } else if (currentKind_ == BossAttackKind::DiveSlam) {
            // 降りてくる場所（地面の帯）
            const Rect area = Rect::FromFoot(diveTargetX_, groundY_, 250.0f, 46.0f);
            const Rect screenArea = camera.WorldToScreen(area);
            draw::FillRect(screenArea, palette::kDanger, static_cast<int>(70.0f * t));
            draw::StrokeRect(screenArea, palette::kDanger, 2.0f, static_cast<int>(200.0f * t));
            const Vec2 mark = camera.WorldToScreen(Vec2(diveTargetX_, groundY_));
            DrawOvalAA(mark.x, mark.y, 120.0f, 30.0f, 32, draw::ToDx(palette::kDanger), FALSE, 3.0f);
        } else if (currentKind_ == BossAttackKind::SkyHalo) {
            // 光輪が走る高さ（左右へ）
            const Rect lane(pos.x - 900.0f, groundY_ - 118.0f, pos.x + 900.0f, groundY_ - 26.0f);
            draw::FillRect(camera.WorldToScreen(lane), palette::kDanger, static_cast<int>(40.0f * t));
        }
    }

    // 天雷の予告（予備動作の間と、まだ落ちていない場所）
    if (alive && currentKind_ == BossAttackKind::SkyThunder
        && (state_ == BossState::Windup || state_ == BossState::Attack)) {
        const float t = (state_ == BossState::Windup)
                      ? math::Clamp(stateTimer_ / math::MaxF(windupTime_, 0.01f), 0.0f, 1.0f) : 1.0f;
        const float ground = stageGroundY_ - thunderZ_;
        const int first = (state_ == BossState::Attack) ? attackStep_ : 0;
        for (int i = first; i < static_cast<int>(thunderTargets_.size()); ++i) {
            const float x = thunderTargets_[static_cast<size_t>(i)];
            const Rect column = camera.WorldToScreen(Rect(x - 64.0f, ground - 760.0f, x + 64.0f, ground));
            draw::FillRect(column, palette::kDanger, static_cast<int>(36.0f * t));
            const Vec2 mark = camera.WorldToScreen(Vec2(x, ground));
            DrawOvalAA(mark.x, mark.y, 70.0f * (0.4f + 0.6f * t), 20.0f * (0.4f + 0.6f * t), 32,
                       draw::ToDx(palette::kDanger), FALSE, 3.0f);
        }
    }

    // 形態変化のたびにオーラが濃くなる（最後のバーで最大）
    if (alive && phase_ >= 2) {
        const float progress = static_cast<float>(phase_ - 1) / static_cast<float>(hpBars_ - 1);
        const Vec2 center = camera.WorldToScreen(Center());
        draw::Glow(center.x, center.y, height * (0.40f + 0.15f * progress), art.trim,
                   static_cast<int>(30.0f + 60.0f * progress), 4);
    }

    // 空を飛ぶボスの影は、いる場所の真下の地面に描く（高く浮くほど小さく薄く）
    const bool flying = IsFlying();
    if (flying) {
        const float scale = DepthScale();
        const Vec2 ground = camera.WorldToScreen(Vec2(pos.x, groundY_));
        const float lift = math::Clamp(altitude_ / 500.0f, 0.0f, 0.7f);
        const float fade = alive ? 1.0f : 1.0f - math::Clamp((deathTimer - 0.6f) / 0.6f, 0.0f, 1.0f);
        DrawActorShadow(ground.x, ground.y, halfWidth * 2.2f * scale * (1.0f - lift * 0.5f),
                        static_cast<int>(90.0f * (1.0f - lift) * fade));
    }

    DrawBody(camera, 255, !flying);
    DrawHpBars(camera);
}

//------------------------------------------------------------------------------
// HP バー
//   ボスの顔の右横に、小さなまっすぐのバーを 1 本ずつ離して重ねる（いちばん下のバーが顔の高さ）。
//   名前はバーの上に、バーの真ん中にそろえて小さめの文字で出し、その上に赤いひし形の目印を置く。
//   上のバーから、右端から左へ減っていく。
//------------------------------------------------------------------------------
namespace {
constexpr float kHpBarWidth = 72.0f;       // バーの長さ
constexpr float kHpBarHeight = 7.0f;       // バーの太さ
constexpr float kHpBarGap = 7.0f;          // バー同士の間隔（縁取りを除いても 1 本ずつ離れて見えるように）
constexpr float kHpBarStagger = 2.0f;      // 下のバーほど左へずらす量（段々に見せる）
constexpr float kHpBarFaceGap = 12.0f;     // 顔の輪郭とバーの左端の間
constexpr float kHpBarNameGap = 6.0f;      // いちばん上のバーと名前の下端の間
constexpr float kHpBarNameMargin = 8.0f;   // 名前を画面の端から離す量
constexpr FontSize kHpBarNameFont = FontSize::Tiny;   // 名前の文字（長い名前でもバーから大きくはみ出さない大きさ）
constexpr float kMarkerWidth = 14.0f;      // 名前の上の目印（赤いひし形）
constexpr float kMarkerHeight = 22.0f;
constexpr float kMarkerGap = 4.0f;         // 目印と名前の間
const ColorRGB  kHpBarEdge(10, 10, 14);    // 縁取り
const ColorRGB  kHpBarEmpty(40, 40, 48);   // 減った部分
const ColorRGB  kMarkerColor(236, 64, 72);

// 名前の上の目印（縦長の赤いひし形。左上の面を明るくして宝石のように見せる）
void DrawMarker(const Vec2& center)
{
    const float hw = kMarkerWidth * 0.5f;
    const float hh = kMarkerHeight * 0.5f;
    const Vec2 top(center.x, center.y - hh);
    const Vec2 bottom(center.x, center.y + hh);
    const Vec2 left(center.x - hw, center.y);
    const Vec2 right(center.x + hw, center.y);
    const float e = 1.5f;
    draw::Triangle(Vec2(top.x, top.y - e), Vec2(left.x - e, left.y), Vec2(right.x + e, right.y), kHpBarEdge);
    draw::Triangle(Vec2(bottom.x, bottom.y + e), Vec2(left.x - e, left.y), Vec2(right.x + e, right.y), kHpBarEdge);
    draw::Triangle(top, left, right, kMarkerColor.Scaled(1.15f));
    draw::Triangle(bottom, left, right, kMarkerColor.Scaled(0.75f));
    draw::Triangle(top, left, center, ColorRGB::Lerp(kMarkerColor, palette::kWhite, 0.45f));
}
} // namespace

float Boss::HpBarFill(float ratio, int index, int bars)
{
    // index 本目が受け持つ範囲：上から順に [1 - (index+1)/N, 1 - index/N]
    const float share = 1.0f / static_cast<float>(math::MaxI(1, bars));
    const float low = 1.0f - share * static_cast<float>(index + 1);
    return math::Clamp((ratio - low) / share, 0.0f, 1.0f);
}

void Boss::HeadCircle(Vec2& center, float& radius) const
{
    // 図形で描くモデル（CharacterArt.cpp）の頭の位置に合わせてある
    const float scale = DepthScale();
    const float w = halfWidth * 2.0f * scale;
    const float h = height * scale;
    const float side = headSide_;
    switch (art.style) {
    case ArtStyle::Dragon:   // 長い首の先、体の前に突き出た頭
        center = Vec2(pos.x + side * (w * 0.44f + h * 0.09f), pos.y - h * 0.82f);
        radius = h * 0.14f;
        break;
    case ArtStyle::Beast:    // 胴の前の頭（たてがみの先）
        center = Vec2(pos.x + side * w * 0.50f, pos.y - h * 0.82f);
        radius = h * 0.13f;
        break;
    case ArtStyle::Golem:    // 肩に埋まった小さな頭
        center = Vec2(pos.x + side * w * 0.06f, pos.y - h * 0.78f);
        radius = w * 0.14f;
        break;
    case ArtStyle::Wisp:
        center = Vec2(pos.x, pos.y - h * 0.62f);
        radius = w * 0.30f;
        break;
    case ArtStyle::Statue:   // 上半身の上の頭（光背の中心）
        center = Vec2(pos.x + side * w * 0.03f, pos.y - h * 0.80f);
        radius = h * 0.12f;
        break;
    default:                 // 人型・騎士
        center = Vec2(pos.x + side * w * 0.04f, pos.y - h * 0.88f);
        radius = h * 0.105f;
        break;
    }
}

bool Boss::HpBarLayout(const Camera& camera, std::vector<Rect>& bars, Vec2& namePos) const
{
    bars.clear();
    // 撃破後と、登場演出で出すまでの間は描かない
    if (!def_ || !alive || (introHeld_ && !barsRevealing_)) return false;

    // 顔が画面に入っているときだけ出す（画面外のボスのバーは出さない）
    Vec2 faceWorld;
    float faceRadius = 0.0f;
    HeadCircle(faceWorld, faceRadius);
    const Vec2 face = camera.WorldToScreen(faceWorld);
    const float screenW = static_cast<float>(config::kScreenWidth);
    const float screenH = static_cast<float>(config::kScreenHeight);
    if (face.x < 0.0f || face.x > screenW || face.y < 0.0f || face.y > screenH) return false;

    // バーは顔の右横。いちばん下のバーを顔の高さに置き、上へ重ねる
    const int count = HpBarCount();
    const float pitch = kHpBarHeight + kHpBarGap;
    const float left = face.x + faceRadius + kHpBarFaceGap;
    const float bottomTop = face.y - kHpBarHeight * 0.5f;   // いちばん下のバーの上端
    for (int i = 0; i < count; ++i) {
        const int fromBottom = count - 1 - i;
        const float x = left + kHpBarStagger * static_cast<float>(fromBottom);
        const float y = bottomTop - pitch * static_cast<float>(fromBottom);
        bars.push_back(Rect(x, y, x + kHpBarWidth, y + kHpBarHeight));
    }

    // 名前はいちばん上のバーの上で、バー全体の真ん中にそろえる（画面の端からははみ出さない）
    const float barsLeft = bars.back().left;     // いちばん下のバーがいちばん左
    const float barsRight = bars.front().right;  // いちばん上のバーがいちばん右
    const float nameW = static_cast<float>(draw::TextWidth(kHpBarNameFont, def_->name));
    const float nameH = static_cast<float>(draw::TextHeight(kHpBarNameFont));
    const float halfW = nameW * 0.5f + kHpBarNameMargin;
    const float centerX = (halfW * 2.0f < screenW)
                        ? math::Clamp((barsLeft + barsRight) * 0.5f, halfW, screenW - halfW)
                        : screenW * 0.5f;
    namePos = Vec2(centerX, bars.front().top - 1.5f - kHpBarNameGap - nameH);
    return true;
}

void Boss::DrawHpBars(const Camera& camera) const
{
    std::vector<Rect> bars;
    Vec2 namePos;
    if (!HpBarLayout(camera, bars, namePos)) return;
    // 登場演出の直後は、空のバーが下から順に溜まっていく
    const float reveal = barReveal_;

    // 目印と名前（影つき。名前の真上に目印）
    DrawMarker(Vec2(namePos.x, namePos.y - kMarkerGap - kMarkerHeight * 0.5f));
    draw::Text(kHpBarNameFont, namePos.x + 1.0f, namePos.y + 1.0f, palette::kBlack, def_->name,
               draw::TextAlign::Center);
    draw::Text(kHpBarNameFont, namePos.x, namePos.y, def_->trim, def_->name, draw::TextAlign::Center);

    const float ratio = math::MinF(HpRatio(), reveal);
    const float delayed = math::MinF(hpDisplay_, reveal);
    const ColorRGB color = palette::HpColor(ratio);
    const int count = static_cast<int>(bars.size());
    for (int i = 0; i < count; ++i) {
        const Rect& bar = bars[static_cast<size_t>(i)];
        // 縁取りで背景（ボスの体など）から浮かせて、ぱっと見で読めるようにする
        draw::FillRect(bar.Expanded(1.5f), kHpBarEdge, 230);
        draw::Bar(bar, HpBarFill(ratio, i, count), color, kHpBarEmpty, HpBarFill(delayed, i, count),
                  palette::kHpLoss);
    }
}

} // namespace ecl
