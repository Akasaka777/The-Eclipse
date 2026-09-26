#include "Game/Boss.h"

#include "Common/MathUtil.h"
#include "Game/ActorAssets.h"
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
    default: return true;
    }
}

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
        b.style = ArtStyle::Beast;
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
void Boss::Setup(const BossDef& def, const Vec2& position, float powerScale)
{
    def_ = &def;
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

    alive = true;
    state_ = BossState::Intro;
    stateTimer_ = 0.0f;
    phase_ = 1;
    damageBoost_ = 1.0f;
    speedBoost_ = 1.0f;
    facing = -1;
    actionName_.clear();
}

int Boss::ApplyDirectDamage(int damage, float knockbackX, CombatSystem& combat)
{
    if (state_ == BossState::Intro || state_ == BossState::PhaseShift) return 0;

    // ボスはのけぞらない（ノックバックを大幅に軽減）
    const int dealt = Actor::ApplyDirectDamage(damage, knockbackX * 0.08f, combat);
    if (dealt > 0) CheckPhaseShift(combat);
    return dealt;
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
    const float ratio = HpRatio();
    const int nextPhase = (ratio <= 0.35f) ? 3 : (ratio <= 0.70f ? 2 : 1);
    if (nextPhase <= phase_ || !alive) return;

    phase_ = nextPhase;
    damageBoost_ = 1.0f + 0.18f * static_cast<float>(phase_ - 1);
    speedBoost_ = 1.0f + 0.22f * static_cast<float>(phase_ - 1);

    state_ = BossState::PhaseShift;
    stateTimer_ = 0.0f;
    velocity.x = 0.0f;
    invincibleTimer = 1.1f;

    combat.AddRing(Vec2(pos.x, pos.y), 520.0f, art.trim, 0.7f);
    combat.AddImpact(Vec2(pos.x, pos.y - height * 0.5f), art.trim, 40, 620.0f);
    combat.AddPopup(Vec2(pos.x, pos.y - height - 60.0f),
                    (phase_ >= 3) ? "激昂！" : "形態変化", art.trim, true);
}

void Boss::ChoosePattern(float distance)
{
    if (!def_ || def_->patterns.empty()) return;

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

void Boss::Update(float dt, const Stage& stage, CombatSystem& combat,
                  const Vec2& playerPos, bool playerAlive, float playerZ)
{
    if (!def_) return;

    stateTimer_ += dt;
    hpDisplay_ = math::Approach(hpDisplay_, HpRatio(), dt * 0.55f);

    if (!alive) {
        state_ = BossState::Dead;
        velocity.x *= 0.9f;
        ApplyPhysics(dt, stage);
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
        if (stateTimer_ >= 2.2f) {
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
        velocity.x *= 0.86f;
        FaceTowards(playerPos.x);
        TrackDepth(dt, playerZ, stage);
        if (!playerAlive) break;

        // 少し間を置いてから次の行動を決める
        const float waitTime = math::MaxF(0.25f, 0.85f / speedBoost_);
        if (stateTimer_ >= waitTime) {
            if (distance > 320.0f && math::RandChance(0.45f)) {
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

    ApplyPhysics(dt, stage);
    UpdateTimers(dt);
    UpdatePose();
}

void Boss::UpdatePose()
{
    switch (state_) {
    case BossState::Windup:
    case BossState::Attack:
        pose = PoseKind::Attack;
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
    if (!onGround && alive) pose = (velocity.y < 0.0f) ? PoseKind::Jump : PoseKind::Fall;
    animator.Play(pose, false);
}

void Boss::Draw(const Camera& camera) const
{
    if (!def_) return;

    // 予備動作の警告（危険度が高いので大きく表示）
    if (alive && state_ == BossState::Windup) {
        const float t = math::Clamp(stateTimer_ / math::MaxF(windupTime_, 0.01f), 0.0f, 1.0f);
        const Vec2 screen = camera.WorldToScreen(Vec2(pos.x, pos.y - height - 46.0f));
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
        }
    }

    // フェーズ 3 ではオーラを纏う
    if (alive && phase_ >= 3) {
        const Vec2 center = camera.WorldToScreen(Center());
        draw::Glow(center.x, center.y, height * 0.55f, art.trim, 90, 4);
    }

    DrawBody(camera);
}

} // namespace ecl
