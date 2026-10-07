//==============================================================================
// Boss.h : ボス敵（パターン選択型 AI ＋ HP によるフェーズ変化）
//==============================================================================
#pragma once

#include "Game/Actor.h"

#include <string>
#include <vector>

namespace ecl {

//------------------------------------------------------------------------------
// 攻撃パターン種別
//------------------------------------------------------------------------------
enum class BossAttackKind
{
    Swipe,    // 横薙ぎ（2 連）
    Charge,   // 突進
    JumpSlam, // 跳躍からの叩きつけ＋衝撃波
    Shot,     // 3 方向の遠距離弾
    Combo,    // 3 連撃
    Burst     // 咆哮＋全方位の衝撃
};

//------------------------------------------------------------------------------
// ボス定義
//------------------------------------------------------------------------------
struct BossDef
{
    int         id = 0;
    std::string name;
    std::string title;
    ArtStyle    style = ArtStyle::Beast;
    ColorRGB    main = ColorRGB(90, 70, 70);
    ColorRGB    accent = ColorRGB(230, 220, 220);
    ColorRGB    trim = ColorRGB(255, 120, 90);
    WeaponType  weapon = WeaponType::OneHandSword;

    float maxHp = 4200.0f;
    float attack = 68.0f;
    float defense = 30.0f;
    float moveSpeed = 200.0f;
    float halfWidth = 62.0f;
    float height = 230.0f;

    // HP バーを何本に分けて表示するか（上の 1 本から順に減っていく）
    int   hpBarCount = 3;

    int   expReward = 320;
    int   colReward = 520;

    // assets/bosses/<assetFolder> から素材を読み込む（空なら代替表示）
    std::string assetFolder;

    std::vector<BossAttackKind> patterns;
};

class BossDatabase
{
public:
    static const BossDatabase& Instance();
    const BossDef* Find(int id) const;
    const std::vector<BossDef>& All() const { return bosses_; }

private:
    BossDatabase();
    std::vector<BossDef> bosses_;
};

//------------------------------------------------------------------------------
// ボス本体
//------------------------------------------------------------------------------
enum class BossState
{
    Intro,
    Idle,
    Move,
    Windup,
    Attack,
    Recover,
    PhaseShift,
    Dead
};

class Boss : public Actor
{
public:
    void Setup(const BossDef& def, const Vec2& position, float powerScale);

    void Update(float dt, const Stage& stage, CombatSystem& combat,
                const Vec2& playerPos, bool playerAlive, float playerZ);
    void Draw(const Camera& camera) const;

    int  ApplyDirectDamage(int damage, float knockbackX, CombatSystem& combat) override;
    void OnDeath(CombatSystem& combat) override;
    void Stagger(float duration) override;

    const BossDef* Def() const { return def_; }
    int   Phase() const { return phase_; }
    bool  IsIntroFinished() const { return state_ != BossState::Intro; }
    bool  IsRemovable() const { return !alive && deathTimer > 2.6f; }
    const std::string& CurrentActionName() const { return actionName_; }
    // 今どの攻撃パターンを出しているか（HUD の表示やテストで使う）
    BossAttackKind CurrentAttack() const { return currentKind_; }
    bool IsAttacking() const { return state_ == BossState::Attack; }
    // 予備動作に入ってから攻撃が終わるまで（この間は向きを変えない）
    bool IsCommittedToAttack() const
    {
        return state_ == BossState::Windup || state_ == BossState::Attack;
    }

    // --- HP バー（雑魚敵と同じ見た目。頭上の右上に、下へ本数ぶん並べる）---------
    int   HpBarCount() const;
    // HP の割合 ratio のとき、上から index 本目（0 始まり）のバーがどれだけ残っているか
    static float HpBarFill(float ratio, int index, int bars);
    // HP バーと名前の位置（画面座標）。頭が画面外にあるときや撃破後は false（描かない）
    bool  HpBarLayout(const Camera& camera, std::vector<Rect>& bars, Vec2& namePos) const;

private:
    void ChoosePattern(float distance);
    void StartWindup(BossAttackKind kind);
    void ExecuteAttack(CombatSystem& combat);
    void UpdateAttackPhase(float dt, CombatSystem& combat, const Vec2& playerPos);
    // プレイヤーの奥行きへ寄せる
    void TrackDepth(float dt, float playerZ, const Stage& stage);
    void CheckPhaseShift(CombatSystem& combat);
    void SpawnMeleeHit(CombatSystem& combat, float reachScale, float heightScale,
                       float damageMultiplier, float knockback, bool launch);
    void UpdatePose();
    void DrawHpBars(const Camera& camera) const;

    const BossDef* def_ = nullptr;
    BossState state_ = BossState::Intro;
    bool  staggered_ = false;   // よろけ中（反撃のチャンス）
    BossAttackKind currentKind_ = BossAttackKind::Swipe;
    std::string actionName_;

    float stateTimer_ = 0.0f;
    float windupTime_ = 0.6f;
    float attackTime_ = 0.5f;
    float recoverTime_ = 0.8f;
    int   attackStep_ = 0;
    float nextStepTime_ = 0.0f;

    int   phase_ = 1;
    float powerScale_ = 1.0f;
    float damageBoost_ = 1.0f;
    float speedBoost_ = 1.0f;
    float hpDisplay_ = 1.0f;
    float moveTarget_ = 0.0f;
    int   lastPatternIndex_ = -1;
};

} // namespace ecl
