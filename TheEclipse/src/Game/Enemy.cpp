#include "Game/Enemy.h"

#include "Common/MathUtil.h"
#include "Game/ActorAssets.h"
#include "Core/GameConfig.h"
#include "Graphics/DrawUtil.h"

#include <cmath>

namespace ecl {

void Enemy::Setup(const EnemyDef& def, const Vec2& position, float powerScale)
{
    def_ = &def;
    id = IssueActorId();
    team = Team::Enemy;
    powerScale_ = math::MaxF(0.2f, powerScale);

    pos = position;
    baseY_ = position.y;
    hoverY_ = position.y;
    velocity = Vec2(0.0f, 0.0f);
    halfWidth = def.halfWidth;
    height = def.height;

    maxHp = def.maxHp * powerScale_;
    hp = maxHp;

    stats = Stats();
    stats.attack = def.attack * powerScale_;
    stats.defense = def.defense * powerScale_;
    stats.moveSpeed = def.moveSpeed;
    stats.critRate = 0.04f;
    stats.critDamage = 0.25f;

    art.style = def.style;
    art.main = def.main;
    art.accent = def.accent;
    art.trim = def.trim;
    art.hasWeapon = (def.style == ArtStyle::Humanoid || def.style == ArtStyle::Knight);
    art.weapon = (def.style == ArtStyle::Knight) ? WeaponType::OneHandSword : WeaponType::OneHandMace;

    animator.SetSet(ActorAssets::Instance().EnemySet(def.id, def.assetFolder));

    alive = true;
    rewardGranted = false;
    state_ = EnemyState::Idle;
    stateTimer_ = math::RandFloat(0.0f, 0.6f);
    hoverPhase_ = math::RandFloat(0.0f, 6.28f);
    facing = -1;
}

int Enemy::ExpReward() const
{
    if (!def_) return 0;
    return static_cast<int>(static_cast<float>(def_->expReward) * powerScale_);
}

int Enemy::ColReward() const
{
    if (!def_) return 0;
    return static_cast<int>(static_cast<float>(def_->colReward) * powerScale_);
}

bool Enemy::IsRemovable() const
{
    return !alive && deathTimer > 1.3f;
}

int Enemy::ApplyDirectDamage(int damage, float knockbackX, CombatSystem& combat)
{
    if (def_) knockbackX *= (1.0f - math::Clamp(def_->knockbackResist, 0.0f, 1.0f));
    const int dealt = Actor::ApplyDirectDamage(damage, knockbackX, combat);
    if (dealt > 0) {
        healthBarTimer_ = 3.0f;
        // 攻撃準備中に被弾すると中断する
        if (state_ == EnemyState::Windup && math::RandChance(0.5f)) {
            state_ = EnemyState::Hurt;
            stateTimer_ = 0.0f;
        }
    }
    return dealt;
}

void Enemy::OnDeath(CombatSystem& combat)
{
    Actor::OnDeath(combat);
    combat.AddRing(Vec2(pos.x, pos.y), 120.0f, art.trim, 0.35f);
}

void Enemy::Update(float dt, const Stage& stage, CombatSystem& combat,
                   const Vec2& playerPos, bool playerAlive)
{
    if (!def_) return;

    stateTimer_ += dt;
    attackCooldown_ = math::MaxF(0.0f, attackCooldown_ - dt);
    healthBarTimer_ = math::MaxF(0.0f, healthBarTimer_ - dt);
    hoverPhase_ += dt * 2.4f;

    if (!alive) {
        state_ = EnemyState::Dead;
        velocity.x *= 0.88f;
        if (!def_->floating) ApplyPhysics(dt, stage);
        UpdateTimers(dt);
        pose = PoseKind::Dead;
        return;
    }

    const float distanceX = playerPos.x - pos.x;
    const float distance = math::Abs(distanceX);
    const float verticalGap = math::Abs(playerPos.y - pos.y);

    switch (state_) {
    case EnemyState::Idle: {
        velocity.x *= 0.85f;
        if (playerAlive && distance < def_->aggroRange) {
            state_ = EnemyState::Chase;
            stateTimer_ = 0.0f;
        }
        break;
    }
    case EnemyState::Chase: {
        if (!playerAlive) {
            state_ = EnemyState::Idle;
            break;
        }
        FaceTowards(playerPos.x);

        const bool inRange = distance <= def_->attackRange && verticalGap < height * 1.4f;
        if (inRange && attackCooldown_ <= 0.0f) {
            state_ = EnemyState::Windup;
            stateTimer_ = 0.0f;
            velocity.x = 0.0f;
            animator.Play(PoseKind::Attack, true);
        } else if (distance > def_->attackRange * 0.75f) {
            velocity.x = static_cast<float>(facing) * def_->moveSpeed;
        } else {
            velocity.x *= 0.8f;
        }

        // 遠距離型は近づきすぎたら距離を取る
        if (def_->ranged && distance < 260.0f) {
            velocity.x = -static_cast<float>(facing) * def_->moveSpeed * 0.8f;
        }
        break;
    }
    case EnemyState::Windup: {
        velocity.x *= 0.82f;
        FaceTowards(playerPos.x);
        if (stateTimer_ >= def_->attackWindup) {
            SpawnAttack(combat);
            state_ = EnemyState::Attack;
            stateTimer_ = 0.0f;
        }
        break;
    }
    case EnemyState::Attack: {
        velocity.x *= 0.9f;
        if (stateTimer_ >= 0.12f) {
            state_ = EnemyState::Recover;
            stateTimer_ = 0.0f;
        }
        break;
    }
    case EnemyState::Recover: {
        velocity.x *= 0.85f;
        if (stateTimer_ >= def_->attackRecover) {
            state_ = EnemyState::Chase;
            stateTimer_ = 0.0f;
            attackCooldown_ = def_->attackCooldown;
        }
        break;
    }
    case EnemyState::Hurt: {
        velocity.x *= 0.9f;
        if (stateTimer_ >= 0.3f) {
            state_ = EnemyState::Chase;
            stateTimer_ = 0.0f;
        }
        break;
    }
    default:
        break;
    }

    if (def_->floating) {
        // 浮遊体は重力を無視する。戦闘中はプレイヤーが攻撃できる高度まで降りてくる
        pos.x += velocity.x * dt;
        pos.x = stage.ClampX(pos.x, halfWidth);

        float targetY = baseY_;
        if (playerAlive && distance < def_->aggroRange) {
            targetY = math::Clamp(playerPos.y - height * 0.2f,
                                  stage.GroundY() - 260.0f, stage.GroundY() - 20.0f);
        }
        hoverY_ = math::Approach(hoverY_, targetY, 180.0f * dt);
        pos.y = hoverY_ + std::sin(hoverPhase_) * 16.0f;
    } else {
        ApplyPhysics(dt, stage);
    }

    UpdateTimers(dt);
    UpdatePose();
}

void Enemy::SpawnAttack(CombatSystem& combat)
{
    if (!def_) return;

    if (def_->ranged) {
        Projectile projectile;
        projectile.pos = Vec2(pos.x + static_cast<float>(facing) * halfWidth, pos.y - height * 0.6f);
        projectile.velocity = Vec2(static_cast<float>(facing) * def_->projectileSpeed, 0.0f);
        projectile.radius = 18.0f;
        projectile.team = Team::Enemy;
        projectile.sourceId = id;
        projectile.attack = stats.attack;
        projectile.damageMultiplier = def_->damageMultiplier;
        projectile.critRate = stats.critRate;
        projectile.critDamage = stats.critDamage;
        projectile.knockback = def_->knockback;
        projectile.life = 2.6f;
        projectile.color = def_->trim;
        combat.AddProjectile(projectile);
        combat.AddImpact(projectile.pos, def_->trim, 6, 200.0f);
        return;
    }

    const float reach = def_->attackRange * 1.05f;
    const float centerX = pos.x + static_cast<float>(facing) * (halfWidth + reach * 0.4f);
    const float centerY = pos.y - height * 0.55f;

    HitBox hitBox;
    hitBox.area = Rect::FromCenter(centerX, centerY, reach, height * 0.95f);
    hitBox.team = Team::Enemy;
    hitBox.sourceId = id;
    hitBox.attack = stats.attack;
    hitBox.damageMultiplier = def_->damageMultiplier;
    hitBox.critRate = stats.critRate;
    hitBox.critDamage = stats.critDamage;
    hitBox.knockback = def_->knockback;
    hitBox.life = 0.14f;
    hitBox.hitStop = 0.04f;
    hitBox.color = def_->trim;
    combat.AddHitBox(hitBox);

    combat.AddSlash(Vec2(centerX, centerY), facing, reach * 0.8f, def_->trim,
                    (def_->style == ArtStyle::Beast) ? 3 : 1);
}

void Enemy::UpdatePose()
{
    switch (state_) {
    case EnemyState::Windup:
    case EnemyState::Attack:
        pose = PoseKind::Attack;
        break;
    case EnemyState::Hurt:
        pose = PoseKind::Hurt;
        break;
    case EnemyState::Chase:
        pose = (math::Abs(velocity.x) > 30.0f) ? PoseKind::Run : PoseKind::Idle;
        break;
    default:
        pose = PoseKind::Idle;
        break;
    }
    animator.Play(pose, false);
}

void Enemy::Draw(const Camera& camera) const
{
    if (!def_) return;

    // 予備動作中は警告表示
    if (alive && state_ == EnemyState::Windup) {
        const Vec2 screen = camera.WorldToScreen(Vec2(pos.x, pos.y - height - 28.0f));
        const float t = math::Clamp(stateTimer_ / math::MaxF(def_->attackWindup, 0.01f), 0.0f, 1.0f);
        draw::Circle(screen.x, screen.y, 14.0f, palette::kDanger, false, 3.0f, 200);
        draw::Circle(screen.x, screen.y, 14.0f * t, palette::kDanger, true, 1.0f, 200);
    }

    DrawBody(camera);

    // HP バー（被弾から数秒間だけ表示）
    if (alive && healthBarTimer_ > 0.0f && HpRatio() < 1.0f) {
        const Vec2 screen = camera.WorldToScreen(Vec2(pos.x, pos.y - height - 14.0f));
        const Rect bar = Rect::FromCenter(screen.x, screen.y, 84.0f, 8.0f);
        const int alpha = static_cast<int>(math::Clamp(healthBarTimer_, 0.0f, 1.0f) * 255.0f);
        draw::FillRect(bar.Expanded(2.0f), palette::kBlack, math::ClampInt(alpha * 3 / 4, 0, 255));
        draw::Bar(bar, HpRatio(), palette::kHpLoss, ColorRGB(40, 40, 48), -1.0f,
                  palette::kHpLoss, alpha);
    }
}

} // namespace ecl
