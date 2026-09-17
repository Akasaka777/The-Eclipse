#include "Game/Actor.h"

#include "Common/MathUtil.h"
#include "Core/GameConfig.h"
#include "Graphics/DrawUtil.h"

namespace ecl {

namespace {
int g_nextActorId = 1;
} // namespace

int IssueActorId()
{
    return g_nextActorId++;
}

Rect Actor::Bounds() const
{
    return Rect::FromFoot(pos.x, pos.y, halfWidth, height);
}

Vec2 Actor::Center() const
{
    return Vec2(pos.x, pos.y - height * 0.5f);
}

float Actor::DepthScale() const
{
    const float ratio = math::Clamp(z / config::kDefaultFieldDepth, 0.0f, 1.0f);
    return math::Lerp(1.0f, config::kFarScale, ratio);
}

void Actor::MoveDepth(float delta, const Stage& stage)
{
    if (delta == 0.0f) return;

    const float newZ = math::Clamp(z + delta, 0.0f, stage.Depth());
    const float applied = newZ - z;
    if (applied == 0.0f) return;

    z = newZ;
    // 奥へ進むほど画面上では持ち上がる（地面からの高さは保つ）
    pos.y -= applied;
}

bool Actor::WithinDepth(float otherZ, float range) const
{
    return math::Abs(z - otherZ) <= range;
}

float Actor::HpRatio() const
{
    if (maxHp <= 0.0f) return 0.0f;
    return math::Clamp(hp / maxHp, 0.0f, 1.0f);
}

float Actor::DeathPhase() const
{
    return math::Clamp(deathTimer / 0.6f, 0.0f, 1.0f);
}

int Actor::ApplyHit(const HitBox& hitBox, CombatSystem& combat)
{
    if (!alive || IsInvincible()) return 0;

    const DamageResult result = CalculateDamage(hitBox.attack, stats.defense, hitBox.damageMultiplier,
                                                hitBox.critRate, hitBox.critDamage);

    // 攻撃判定の中心から見て反対側へ吹き飛ばす
    const float direction = (hitBox.area.CenterX() <= pos.x) ? 1.0f : -1.0f;
    const int dealt = ApplyDirectDamage(result.value, hitBox.knockback * direction, combat);
    if (dealt <= 0) return 0;

    combat.AddDamageNumber(Vec2(pos.x, pos.y - height * 0.95f), dealt, result.critical,
                           result.critical ? palette::kCritical : palette::kWhite);
    combat.AddImpact(Vec2(pos.x, pos.y - height * 0.6f), hitBox.color, result.critical ? 18 : 10,
                     result.critical ? 520.0f : 360.0f);

    if (hitBox.launch && alive) velocity.y = -640.0f;
    return dealt;
}

int Actor::ApplyDirectDamage(int damage, float knockbackX, CombatSystem& combat)
{
    if (!alive || damage <= 0) return 0;

    hp -= static_cast<float>(damage);
    hurtTimer = 0.24f;
    flashTimer = 0.18f;
    velocity.x = knockbackX;

    combat.AddImpact(Vec2(pos.x, pos.y - height * 0.6f), ColorRGB(255, 200, 140), 8, 320.0f);

    if (hp <= 0.0f) {
        hp = 0.0f;
        alive = false;
        deathTimer = 0.0f;
        OnDeath(combat);
    }
    return damage;
}

void Actor::OnDeath(CombatSystem& combat)
{
    combat.AddImpact(Vec2(pos.x, pos.y - height * 0.5f), art.trim, 26, 560.0f);
}

void Actor::ApplyPhysics(float dt, const Stage& stage)
{
    const float prevBottom = pos.y;

    velocity.y += config::kGravity * dt;
    if (velocity.y > config::kMaxFallSpeed) velocity.y = config::kMaxFallSpeed;

    pos += velocity * dt;

    const float landing = stage.LandingY(pos.x, halfWidth, prevBottom, pos.y, z);
    if (velocity.y >= 0.0f && pos.y >= landing) {
        pos.y = landing;
        velocity.y = 0.0f;
        onGround = true;
    } else {
        onGround = false;
    }

    pos.x = stage.ClampX(pos.x, halfWidth);
}

void Actor::UpdateTimers(float dt)
{
    hurtTimer = math::MaxF(0.0f, hurtTimer - dt);
    invincibleTimer = math::MaxF(0.0f, invincibleTimer - dt);
    flashTimer = math::MaxF(0.0f, flashTimer - dt);
    if (!alive) deathTimer += dt;
    animator.Update(dt);
}

void Actor::FaceTowards(float targetX)
{
    if (targetX > pos.x + 4.0f) facing = 1;
    else if (targetX < pos.x - 4.0f) facing = -1;
}

void Actor::DrawBody(const Camera& camera, int alpha) const
{
    const Rect bounds = Bounds();
    if (!camera.IsVisible(bounds, 200.0f)) return;

    // 奥にいるほど小さく描く（足元を基準に縮める）
    const float scale = DepthScale();
    const Vec2 feet = camera.WorldToScreen(Vec2(pos.x, pos.y));
    const Rect screen = Rect::FromFoot(feet.x, feet.y, halfWidth * scale, height * scale);

    // 影
    if (alive || DeathPhase() < 1.0f) {
        DrawActorShadow(feet.x, feet.y, halfWidth * 2.2f * scale,
                        static_cast<int>(90.0f * (alive ? 1.0f : 1.0f - DeathPhase())));
    }

    int bodyAlpha = alpha;
    if (!alive) {
        // 死亡後はフェードアウト
        const float fade = math::Clamp((deathTimer - 0.6f) / 0.6f, 0.0f, 1.0f);
        bodyAlpha = static_cast<int>(static_cast<float>(alpha) * (1.0f - fade));
    } else if (invincibleTimer > 0.0f) {
        // 無敵中は点滅
        bodyAlpha = (static_cast<int>(invincibleTimer * 24.0f) % 2 == 0) ? alpha : alpha / 3;
    }
    if (bodyAlpha <= 0) return;

    const float phase = alive ? animator.Phase() : DeathPhase();

    // 奥にいるほどわずかに暗くして距離感を出す
    ActorArt shaded = art;
    const float depthRatio = math::Clamp(z / config::kDefaultFieldDepth, 0.0f, 1.0f);
    const float shade = 1.0f - depthRatio * 0.18f;
    shaded.main = shaded.main.Scaled(shade);
    shaded.accent = shaded.accent.Scaled(shade);

    DrawActor(screen, facing, pose, phase, shaded, &animator, bodyAlpha, flashTimer / 0.18f);
}

} // namespace ecl
