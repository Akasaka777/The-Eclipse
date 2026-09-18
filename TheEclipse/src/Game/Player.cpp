#include "Game/Player.h"

#include "Common/MathUtil.h"
#include "Game/ActorAssets.h"
#include "Core/GameConfig.h"
#include "Graphics/DrawUtil.h"

#include <cmath>

namespace ecl {

namespace {

// 通常攻撃 1 段分の定義
struct ComboStep
{
    float duration;
    float hitTime;
    float damageMultiplier;
    float reach;
    float knockback;
    float forwardImpulse;
    int   effectStyle;
};

const ComboStep kCombo[3] = {
    { 0.34f, 0.11f, 1.00f, 1.30f, 140.0f, 180.0f, 0 },
    { 0.36f, 0.12f, 1.15f, 1.35f, 160.0f, 200.0f, 1 },
    { 0.54f, 0.17f, 1.85f, 1.55f, 380.0f, 260.0f, 0 },
};

// パリィ
constexpr float kParryWindow = 0.18f;    // 受け流しの受付時間
constexpr float kParryRecovery = 0.50f;  // 失敗時の硬直
constexpr float kParrySuccessCooldown = 0.12f; // 成功時は素早く再発動できる
constexpr float kParryMpGain = 14.0f;    // 成功時の MP 回復
constexpr float kParryInvincible = 0.30f;

constexpr float kDashDuration = 0.22f;
constexpr float kDashSpeed = 1150.0f;
constexpr float kDashCooldown = 0.85f;
constexpr float kComboWindow = 0.55f;

} // namespace

Player::Player()
{
    id = IssueActorId();
    team = Team::Player;
    halfWidth = 30.0f;
    height = 156.0f;

    art.style = ArtStyle::Humanoid;
    art.main = ColorRGB(48, 58, 84);
    art.accent = ColorRGB(226, 234, 248);
    art.trim = ColorRGB(64, 206, 255);
    art.hasWeapon = true;
}

void Player::Setup(const PlayerData& data)
{
    ApplyEquipment(data);

    hp = maxHp;
    mp_ = maxMp_;

    for (int i = 0; i < kSkillSlotCount; ++i) cooldowns_[i] = 0.0f;

    state_ = PlayerState::Normal;
    currentSkill_ = nullptr;
    comboIndex_ = 0;
    parryWindow_ = 0.0f;
    parryCooldown_ = 0.0f;
    parryFlash_ = 0.0f;
    parrySignal_ = false;
    alive = true;
    deathTimer = 0.0f;
}

void Player::RefreshEquipment(const PlayerData& data)
{
    ApplyEquipment(data);

    // 装備が変わって最大値が下がっても、現在値は維持したまま収める
    hp = math::Clamp(hp, 0.0f, maxHp);
    mp_ = math::Clamp(mp_, 0.0f, maxMp_);

    // 外れたスキルのクールダウンは持ち越さない
    for (int i = 0; i < kSkillSlotCount; ++i) {
        if (skills_[i] == nullptr) cooldowns_[i] = 0.0f;
    }
}

void Player::ApplyEquipment(const PlayerData& data)
{
    stats = data.TotalStats();
    maxHp = math::MaxF(1.0f, stats.maxHp);
    maxMp_ = math::MaxF(1.0f, stats.maxMp);

    weapon_ = data.CurrentWeaponType();

    // 装備スキンから見た目を組み立てる
    art = data.GetInventory().BuildAppearance();
    art.weapon = weapon_;

    attackSpeedFactor_ = math::MaxF(0.4f, WeaponSpeedScale(weapon_) * (1.0f + stats.attackSpeed));

    for (int i = 0; i < kSkillSlotCount; ++i) {
        skills_[i] = data.SkillAt(i);
    }

    animator.SetSet(ActorAssets::Instance().PlayerSet());
}

void Player::PlaceAt(const Vec2& position)
{
    pos = position;
    velocity = Vec2(0.0f, 0.0f);
    state_ = PlayerState::Normal;
    currentSkill_ = nullptr;
    comboIndex_ = 0;
    attackTimer_ = 0.0f;
    dashTimer_ = 0.0f;
    guarding_ = false;
    parryWindow_ = 0.0f;
    parryCooldown_ = 0.0f;
    parryFlash_ = 0.0f;
    parrySignal_ = false;
    facing = 1;
    pose = PoseKind::Idle;
}

void Player::FullHeal()
{
    hp = maxHp;
    mp_ = maxMp_;
    alive = true;
    deathTimer = 0.0f;
    state_ = PlayerState::Normal;
}

float Player::MpRatio() const
{
    if (maxMp_ <= 0.0f) return 0.0f;
    return math::Clamp(mp_ / maxMp_, 0.0f, 1.0f);
}

const SwordSkill* Player::Skill(int index) const
{
    if (index < 0 || index >= 4) return nullptr;
    return skills_[index];
}

float Player::SkillCooldown(int index) const
{
    if (index < 0 || index >= 4) return 0.0f;
    return cooldowns_[index];
}

float Player::SkillCooldownRatio(int index) const
{
    const SwordSkill* skill = Skill(index);
    if (!skill || skill->cooldown <= 0.0f) return 0.0f;
    return math::Clamp(SkillCooldown(index) / skill->cooldown, 0.0f, 1.0f);
}

bool Player::CanUseSkill(int index) const
{
    // 装備されていないスロット（ユニークスキルで減った枠を含む）は使用できない
    const SwordSkill* skill = Skill(index);
    if (!skill || !alive) return false;
    if (cooldowns_[index] > 0.0f) return false;
    if (mp_ < skill->mpCost) return false;
    return state_ == PlayerState::Normal || state_ == PlayerState::Attack;
}

bool Player::UseSkill(int index, CombatSystem& combat)
{
    if (!CanUseSkill(index)) return false;

    const SwordSkill* skill = Skill(index);
    mp_ -= skill->mpCost;
    cooldowns_[index] = skill->cooldown;

    currentSkill_ = skill;
    skillTimer_ = 0.0f;
    nextStrikeIndex_ = 0;
    state_ = PlayerState::Skill;
    guarding_ = false;
    comboIndex_ = 0;

    if (skill->invincibleUntil > 0.0f) {
        invincibleTimer = math::MaxF(invincibleTimer, skill->invincibleUntil);
    }

    animator.Play(PoseKind::Skill, true);
    combat.AddPopup(Vec2(pos.x, pos.y - height - 30.0f), skill->name, skill->effectColor, false);
    combat.AddRing(Vec2(pos.x, pos.y), 160.0f, skill->effectColor, 0.35f);
    return true;
}

bool Player::CanParry() const
{
    // ガード中かつ硬直が明けていること
    return alive && guarding_ && parryCooldown_ <= 0.0f && parryWindow_ <= 0.0f
        && state_ == PlayerState::Normal;
}

float Player::ParryWindowRatio() const
{
    return math::Clamp(parryWindow_ / kParryWindow, 0.0f, 1.0f);
}

bool Player::TryParry(CombatSystem& combat)
{
    if (!CanParry()) return false;

    parryWindow_ = kParryWindow;
    // 失敗した場合はそのまま硬直に移行する
    parryCooldown_ = kParryWindow + kParryRecovery;

    // 構えの演出
    const Vec2 front(pos.x + static_cast<float>(facing) * 46.0f, pos.y - height * 0.55f);
    combat.AddRing(front, 90.0f, palette::kAccent, 0.22f);
    return true;
}

bool Player::ConsumeParrySignal(int* outSourceId)
{
    if (!parrySignal_) return false;
    parrySignal_ = false;
    if (outSourceId) *outSourceId = parrySourceId_;
    return true;
}

void Player::Update(float dt, const Stage& stage, CombatSystem& combat,
                    const Input& input, bool controlEnabled)
{
    justAttacked_ = false;

    // --- クールダウン / MP 回復 -----------------------------------------------
    for (int i = 0; i < 4; ++i) {
        cooldowns_[i] = math::MaxF(0.0f, cooldowns_[i] - dt);
    }
    mp_ = math::MinF(maxMp_, mp_ + stats.mpRegen * dt);

    comboResetTimer_ = math::MaxF(0.0f, comboResetTimer_ - dt);
    if (comboResetTimer_ <= 0.0f && state_ != PlayerState::Attack) comboIndex_ = 0;

    dashCooldown_ = math::MaxF(0.0f, dashCooldown_ - dt);
    parryWindow_ = math::MaxF(0.0f, parryWindow_ - dt);
    parryCooldown_ = math::MaxF(0.0f, parryCooldown_ - dt);
    parryFlash_ = math::MaxF(0.0f, parryFlash_ - dt);
    jumpBuffer_ = math::MaxF(0.0f, jumpBuffer_ - dt);
    coyoteTimer_ = onGround ? 0.12f : math::MaxF(0.0f, coyoteTimer_ - dt);

    if (!alive) {
        state_ = PlayerState::Dead;
        pose = PoseKind::Dead;
        velocity.x *= 0.86f;
        ApplyPhysics(dt, stage);
        UpdateTimers(dt);
        return;
    }

    // --- 状態別の更新 --------------------------------------------------------
    switch (state_) {
    case PlayerState::Attack: UpdateAttack(dt, combat, input); break;
    case PlayerState::Skill:  UpdateSkill(dt, combat); break;
    case PlayerState::Dash:   UpdateDash(dt); break;
    case PlayerState::Hurt:
        if (hurtTimer <= 0.0f) state_ = PlayerState::Normal;
        velocity.x *= 0.88f;
        break;
    case PlayerState::Normal:
    default:
        UpdateNormal(dt, combat, input, controlEnabled, stage);
        break;
    }

    // --- 攻撃 / スキル入力（先行入力） ----------------------------------------
    if (controlEnabled) {
        if (input.Pressed(GameAction::Attack)) {
            if (guarding_) {
                // ガード中の攻撃ボタンはパリィ（攻撃には派生しない）
                TryParry(combat);
            } else if (state_ == PlayerState::Normal) {
                StartAttack(combat);
            } else if (state_ == PlayerState::Attack) {
                attackBuffered_ = true;
            }
        }
        const GameAction skillKeys[4] = { GameAction::Skill1, GameAction::Skill2,
                                          GameAction::Skill3, GameAction::Skill4 };
        for (int i = 0; i < 4; ++i) {
            if (input.Pressed(skillKeys[i])) UseSkill(i, combat);
        }
    }

    ApplyPhysics(dt, stage);
    UpdateTimers(dt);
    UpdatePose();

    // ダッシュ中の残像
    if (state_ == PlayerState::Dash) {
        afterImageTimer_ -= dt;
        if (afterImageTimer_ <= 0.0f) {
            afterImageTimer_ = 0.04f;
            combat.AddDust(Vec2(pos.x, pos.y), facing, 2);
        }
    }
}

void Player::UpdateNormal(float dt, CombatSystem& combat, const Input& input, bool controlEnabled,
                          const Stage& stage)
{
    depthMoving_ = false;

    if (!controlEnabled) {
        velocity.x *= 0.82f;
        guarding_ = false;
        return;
    }

    guarding_ = input.Down(GameAction::Guard) && onGround;

    const float axis = input.MoveAxisX();
    const float speed = stats.moveSpeed * (guarding_ ? 0.0f : 1.0f);

    if (math::Abs(axis) > 0.1f && !guarding_) {
        velocity.x = axis * speed;
        facing = (axis > 0.0f) ? 1 : -1;
    } else {
        // フレームレートに依存しない減速
        velocity.x *= 1.0f - math::DampFactor(onGround ? 20.0f : 5.0f, dt);
    }

    // --- 奥行き移動（接地時のみ） --------------------------------------------
    if (onGround && !guarding_) {
        float depthAxis = 0.0f;
        if (input.Down(GameAction::MoveUp)) depthAxis += 1.0f;   // 奥へ
        if (input.Down(GameAction::MoveDown)) depthAxis -= 1.0f; // 手前へ
        if (depthAxis != 0.0f) {
            MoveDepth(depthAxis * stats.moveSpeed * config::kDepthMoveRate * dt, stage);
            depthMoving_ = true;
        }
    }

    // ジャンプ（先行入力＋コヨーテタイム）
    if (input.Pressed(GameAction::Jump)) jumpBuffer_ = 0.12f;
    if (jumpBuffer_ > 0.0f && coyoteTimer_ > 0.0f && !guarding_) {
        velocity.y = config::kJumpVelocity;
        jumpBuffer_ = 0.0f;
        coyoteTimer_ = 0.0f;
        onGround = false;
        combat.AddDust(Vec2(pos.x, pos.y), facing, 5);
    }

    // 回避ダッシュ
    if (input.Pressed(GameAction::Dash) && dashCooldown_ <= 0.0f && !guarding_) {
        state_ = PlayerState::Dash;
        dashTimer_ = kDashDuration;
        dashCooldown_ = kDashCooldown;
        invincibleTimer = math::MaxF(invincibleTimer, kDashDuration * 0.85f);
        velocity.x = static_cast<float>(facing) * kDashSpeed;
        velocity.y = 0.0f;
        animator.Play(PoseKind::Dash, true);
        combat.AddDust(Vec2(pos.x, pos.y), facing, 8);
    }
}

void Player::StartAttack(CombatSystem& combat)
{
    (void)combat;
    state_ = PlayerState::Attack;
    attackTimer_ = 0.0f;
    attackHitSpawned_ = false;
    attackBuffered_ = false;
    guarding_ = false;

    const ComboStep& step = kCombo[math::ClampInt(comboIndex_, 0, 2)];
    attackDuration_ = step.duration / attackSpeedFactor_;
    animator.Play(PoseKind::Attack, true);
}

void Player::UpdateAttack(float dt, CombatSystem& combat, const Input& input)
{
    (void)input;
    attackTimer_ += dt;

    const ComboStep& step = kCombo[math::ClampInt(comboIndex_, 0, 2)];
    const float hitTime = step.hitTime / attackSpeedFactor_;

    // 前進しながら斬る
    if (attackTimer_ < hitTime * 1.6f) {
        velocity.x = static_cast<float>(facing) * step.forwardImpulse;
    } else {
        velocity.x *= 0.80f;
    }

    if (!attackHitSpawned_ && attackTimer_ >= hitTime) {
        attackHitSpawned_ = true;
        SpawnComboHitBox(combat);
    }

    if (attackTimer_ >= attackDuration_) {
        comboResetTimer_ = kComboWindow;
        comboIndex_ = (comboIndex_ + 1) % 3;
        if (attackBuffered_) {
            StartAttack(combat);
        } else {
            state_ = PlayerState::Normal;
        }
    }
}

void Player::SpawnComboHitBox(CombatSystem& combat)
{
    const ComboStep& step = kCombo[math::ClampInt(comboIndex_, 0, 2)];
    const float reach = halfWidth * 2.0f * step.reach * WeaponReachScale(weapon_);
    const float centerX = pos.x + static_cast<float>(facing) * (halfWidth + reach * 0.5f);
    const float centerY = pos.y - height * 0.55f;

    HitBox hitBox;
    hitBox.area = Rect::FromCenter(centerX, centerY, reach, height * 0.9f);
    hitBox.team = Team::Player;
    hitBox.sourceId = id;
    hitBox.attack = stats.attack;
    hitBox.damageMultiplier = step.damageMultiplier;
    hitBox.critRate = stats.critRate;
    hitBox.critDamage = stats.critDamage;
    hitBox.knockback = step.knockback;
    hitBox.z = z;
    hitBox.zRange = config::kHitDepthRange;
    hitBox.hitStop = (comboIndex_ == 2) ? 0.07f : 0.035f;
    hitBox.life = 0.09f;
    hitBox.color = art.trim;
    combat.AddHitBox(hitBox);

    combat.AddSlash(Vec2(centerX, centerY), facing, reach * 0.9f, art.trim, step.effectStyle);
    justAttacked_ = true;
}

void Player::UpdateSkill(float dt, CombatSystem& combat)
{
    if (!currentSkill_) {
        state_ = PlayerState::Normal;
        return;
    }

    skillTimer_ += dt;

    // 定義された時刻になった攻撃を順に発生させる
    while (nextStrikeIndex_ < currentSkill_->strikes.size() &&
           skillTimer_ >= currentSkill_->strikes[nextStrikeIndex_].time) {
        SpawnSkillStrike(currentSkill_->strikes[nextStrikeIndex_], combat);
        ++nextStrikeIndex_;
    }

    velocity.x *= 0.88f;

    if (skillTimer_ >= currentSkill_->duration) {
        currentSkill_ = nullptr;
        state_ = PlayerState::Normal;
        comboIndex_ = 0;
    }
}

void Player::SpawnSkillStrike(const SkillStrike& strike, CombatSystem& combat)
{
    if (!currentSkill_) return;

    const float reach = halfWidth * 2.0f * strike.reach * WeaponReachScale(weapon_);
    const float centerX = pos.x + static_cast<float>(facing) * (halfWidth + reach * 0.5f);
    const float centerY = pos.y - height * (0.55f + strike.offsetY);

    HitBox hitBox;
    hitBox.area = Rect::FromCenter(centerX, centerY, reach, height * strike.height);
    hitBox.team = Team::Player;
    hitBox.sourceId = id;
    hitBox.attack = stats.attack;
    hitBox.damageMultiplier = strike.damageMultiplier;
    hitBox.critRate = stats.critRate;
    hitBox.critDamage = stats.critDamage;
    hitBox.knockback = strike.knockback;
    hitBox.z = z;
    // 回転系のスキルは奥行き方向にも広く当たる
    hitBox.zRange = config::kHitDepthRange * ((currentSkill_->effectStyle == 4) ? 2.2f : 1.0f);
    hitBox.hitStop = strike.hitStop;
    hitBox.launch = strike.launch;
    hitBox.life = 0.10f;
    hitBox.color = currentSkill_->effectColor;
    combat.AddHitBox(hitBox);

    if (strike.forwardImpulse != 0.0f) {
        velocity.x = static_cast<float>(facing) * strike.forwardImpulse;
    }

    combat.AddSlash(Vec2(centerX, centerY), facing, reach * 0.95f,
                    currentSkill_->effectColor, currentSkill_->effectStyle);
    if (currentSkill_->effectStyle == 4) {
        combat.AddRing(Vec2(pos.x, pos.y), reach * 1.2f, currentSkill_->effectColor, 0.3f);
    }
    justAttacked_ = true;
}

void Player::UpdateDash(float dt)
{
    dashTimer_ -= dt;
    velocity.y = 0.0f;
    velocity.x = static_cast<float>(facing) * kDashSpeed * math::Clamp(dashTimer_ / kDashDuration, 0.2f, 1.0f);

    if (dashTimer_ <= 0.0f) {
        state_ = PlayerState::Normal;
        velocity.x *= 0.4f;
    }
}

void Player::UpdatePose()
{
    switch (state_) {
    case PlayerState::Dead:   pose = PoseKind::Dead; return;
    case PlayerState::Attack: pose = PoseKind::Attack; return;
    case PlayerState::Skill:  pose = PoseKind::Skill; return;
    case PlayerState::Dash:   pose = PoseKind::Dash; return;
    case PlayerState::Hurt:   pose = PoseKind::Hurt; return;
    default: break;
    }

    if (guarding_) pose = PoseKind::Guard;
    else if (!onGround) pose = (velocity.y < 0.0f) ? PoseKind::Jump : PoseKind::Fall;
    else if (math::Abs(velocity.x) > 40.0f) pose = PoseKind::Run;
    else pose = PoseKind::Idle;

    animator.Play(pose, false);
}

int Player::ApplyHit(const HitBox& hitBox, CombatSystem& combat)
{
    if (!alive || IsInvincible()) return 0;

    // 正面から来た攻撃かどうか（ガード / パリィの成立条件）
    const bool fromFront = (hitBox.area.CenterX() > pos.x) == (facing > 0);

    // --- パリィ判定（受付時間中に正面から攻撃を受けた） -------------------------
    if (parryWindow_ > 0.0f && fromFront) {
        parryWindow_ = 0.0f;
        parryCooldown_ = kParrySuccessCooldown;
        parryFlash_ = 0.45f;
        parrySignal_ = true;
        parrySourceId_ = hitBox.sourceId;

        invincibleTimer = math::MaxF(invincibleTimer, kParryInvincible);
        mp_ = math::MinF(maxMp_, mp_ + kParryMpGain);
        velocity.x = 0.0f;

        // 演出
        const Vec2 front(pos.x + static_cast<float>(facing) * 52.0f, pos.y - height * 0.55f);
        combat.AddPopup(Vec2(pos.x, pos.y - height - 40.0f), "PARRY!", palette::kCritical, true);
        combat.AddRing(front, 260.0f, palette::kCritical, 0.35f);
        combat.AddImpact(front, palette::kCritical, 26, 620.0f);
        combat.AddSlash(front, facing, 130.0f, palette::kWhite, 4);
        combat.AddHealNumber(Vec2(pos.x, pos.y - height * 1.1f), static_cast<int>(kParryMpGain));
        return 0;
    }

    DamageResult result = CalculateDamage(hitBox.attack, stats.defense, hitBox.damageMultiplier,
                                          hitBox.critRate, hitBox.critDamage);

    // ガード中は大幅に軽減し、のけぞりも短い
    bool guarded = false;
    if (guarding_ && fromFront) {
        guarded = true;
        result.value = math::MaxI(1, static_cast<int>(static_cast<float>(result.value) * 0.28f));
    }

    const float direction = (hitBox.area.CenterX() <= pos.x) ? 1.0f : -1.0f;
    hp -= static_cast<float>(result.value);
    flashTimer = 0.2f;
    invincibleTimer = guarded ? 0.25f : 0.55f;
    velocity.x = hitBox.knockback * direction * (guarded ? 0.35f : 1.0f);

    if (!guarded) {
        hurtTimer = 0.28f;
        // パリィに失敗して被弾した場合は受付も打ち切る
        parryWindow_ = 0.0f;
        if (state_ != PlayerState::Skill) state_ = PlayerState::Hurt;
    }

    const Vec2 popupPos(pos.x, pos.y - height * 0.95f);
    combat.AddDamageNumber(popupPos, result.value, false,
                           guarded ? palette::kTextDim : palette::kDanger);
    combat.AddImpact(Vec2(pos.x, pos.y - height * 0.6f),
                     guarded ? palette::kAccent : ColorRGB(255, 120, 120), 12, 380.0f);
    if (guarded) combat.AddPopup(popupPos, "GUARD", palette::kAccent, false);

    if (hp <= 0.0f) {
        hp = 0.0f;
        alive = false;
        deathTimer = 0.0f;
        state_ = PlayerState::Dead;
        OnDeath(combat);
    }
    return result.value;
}

void Player::OnDeath(CombatSystem& combat)
{
    combat.AddImpact(Vec2(pos.x, pos.y - height * 0.5f), palette::kDanger, 30, 600.0f);
    combat.AddPopup(Vec2(pos.x, pos.y - height - 40.0f), "戦闘不能", palette::kDanger, true);
}

void Player::Draw(const Camera& camera) const
{
    DrawBody(camera);
    if (!alive) return;

    const Vec2 guardPos = camera.WorldToScreen(Vec2(pos.x + static_cast<float>(facing) * 40.0f,
                                                    pos.y - height * 0.55f));

    // ガード中のシールドエフェクト
    if (guarding_) {
        const bool ready = parryCooldown_ <= 0.0f;
        const ColorRGB shield = ready ? palette::kAccent : palette::kTextDisabled;
        draw::Circle(guardPos.x, guardPos.y, 62.0f, shield, false, 3.0f, 170);
        draw::Glow(guardPos.x, guardPos.y, 40.0f, shield, 90, 3);
    }

    // パリィ受付中：内側から閉じるリングでタイミングを示す
    if (parryWindow_ > 0.0f) {
        const float t = ParryWindowRatio();
        draw::Circle(guardPos.x, guardPos.y, 40.0f + 46.0f * t, palette::kCritical, false, 4.0f, 230);
        draw::Glow(guardPos.x, guardPos.y, 54.0f, palette::kCritical, 150, 4);
    }

    // パリィ成功：閃光
    if (parryFlash_ > 0.0f) {
        const float t = math::Clamp(parryFlash_ / 0.45f, 0.0f, 1.0f);
        draw::Glow(guardPos.x, guardPos.y, 120.0f * (1.3f - t), palette::kCritical,
                   static_cast<int>(200.0f * t), 5);
        draw::Circle(guardPos.x, guardPos.y, 150.0f * (1.0f - t), palette::kWhite, false,
                     5.0f * t, static_cast<int>(230.0f * t));
    }
}

} // namespace ecl
