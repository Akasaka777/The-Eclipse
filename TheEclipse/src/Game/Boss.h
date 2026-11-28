//==============================================================================
// Boss.h : ボス敵（パターン選択型 AI ＋ HP バー 1 本ごとの形態変化）
//==============================================================================
#pragma once

#include "Game/Actor.h"

#include <string>
#include <utility>
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
    Burst,    // 咆哮＋全方位の衝撃
    // --- 空を飛ぶボス（ヴァジュラ・ラグナシュラ）用 -----------------------------
    SkyVolley,   // 空中から金剛杵を扇状に撃ち下ろす（遠距離）
    SkyThunder,  // 予告した 3 か所に順に雷を落とす（遠距離）
    SkyHalo,     // 地面すれすれを左右へ走る光輪（遠距離。ジャンプでよける）
    DiveSlam     // 急降下して掌で叩きつける（近距離）。着地したあとしばらく地上にとどまる
};

// 竜王の火山の隠しボス「ヴァジュラ・ラグナシュラ」の ID
constexpr int kVajraBossId = 6;

// 100 層「世界終焉の玉座」のボス（1 フロアに 1 体ずつの中ボスと、最後のフロアの最後のボス）
constexpr int kWorldEaterBossId = 7;      // 1F ワールドイーター
constexpr int kAbyssDragonBossId = 8;     // 2F アビスドラゴン
constexpr int kChaosLordBossId = 9;       // 3F カオスロード
constexpr int kVoidColossusBossId = 10;   // 4F ヴォイド・コロッサス
constexpr int kNemesisBossId = 11;        // 5F ネメシス・インフェルノ
constexpr int kArcXenosBossId = 12;       // 5F の最後 アークゼノス・ザ・エンドオブワールド

// ローグライクモード用のボス（見た目と技は元のまま、強さだけ階層に合わせた写し）。ID は 100 + 元の ID
constexpr int kRoguelikeBossIdBase = 100;

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

    int   expReward = 320;
    int   colReward = 520;

    // assets/bosses/<assetFolder> から素材を読み込む（空なら代替表示）
    std::string assetFolder;

    // 空を飛ぶボス。浮いている間は地上からの攻撃が届かない
    //   （ジャンプ攻撃で当てるか、降りてきて地上にいる間に当てる）
    bool  flying = false;
    float hoverHeight = 0.0f;   // 地面から体の下端までの高さ

    std::vector<BossAttackKind> patterns;
    // 技の名前を差し替える（載っていない技は共通の名前）
    std::vector<std::pair<BossAttackKind, std::string>> attackNames;
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

//------------------------------------------------------------------------------
// HP バーの本数（クエストの強さで決まる。QuestDatabase::BossHpBarCount を参照）
//------------------------------------------------------------------------------
constexpr int kMinBossHpBars = 3;
constexpr int kMaxBossHpBars = 10;

class Boss : public Actor
{
public:
    // hpBars : HP バーの本数（3〜10）。1 本削りきるごとに形態変化する。
    void Setup(const BossDef& def, const Vec2& position, float powerScale, int hpBars = kMinBossHpBars);

    void Update(float dt, const Stage& stage, CombatSystem& combat,
                const Vec2& playerPos, bool playerAlive, float playerZ);
    void Draw(const Camera& camera) const;

    // 1 本のバーを削りきったところで止まる（あふれた分は次のバーに持ち越さない）
    int  ApplyDirectDamage(int damage, float knockbackX, CombatSystem& combat) override;
    void OnDeath(CombatSystem& combat) override;
    void Stagger(float duration) override;
    // 残りのバーをまとめて削って倒す（開発者モードの「殲滅」用）
    void ForceKill(CombatSystem& combat);

    // --- 登場演出（QuestScene が進める）-------------------------------------------
    // hold を立てている間は登場状態のまま待つ（攻撃せず、ダメージも受けない）。
    // 立てると HP バーは隠れる。
    void HoldIntro(bool hold);
    bool IsIntroHeld() const { return introHeld_; }
    // 隠していた HP バーを出し、空から満タンまで溜める（演出の見せ場で呼ぶ）。
    // 呼ばないまま hold を外したときも、外した時点から溜まる。
    void RevealHpBars();
    // 咆哮のポーズ（登場演出の見せ場）
    void SetIntroRoar(bool roar);
    bool IsIntroRoaring() const { return introRoar_; }
    // HP バーの溜まり具合（0〜1。演出の後に溜まる。普段は 1）
    float HpBarReveal() const { return barReveal_; }

    const BossDef* Def() const { return def_; }
    // 今の形態（1 本目のバーを削っている間が 1。最後のバーで HpBarCount() になる）
    int   Phase() const { return phase_; }
    // この形態のバーを削りきる HP（これを下回ると次の形態へ）
    float PhaseFloorHp() const;
    bool  IsPhaseShifting() const { return state_ == BossState::PhaseShift; }
    bool  IsIntroFinished() const { return state_ != BossState::Intro; }
    bool  IsRemovable() const { return !alive && deathTimer > 2.6f; }
    const std::string& CurrentActionName() const { return actionName_; }
    // 今どの攻撃パターンを出しているか（HUD の表示やテストで使う）
    BossAttackKind CurrentAttack() const { return currentKind_; }
    bool IsAttacking() const { return state_ == BossState::Attack; }
    BossState State() const { return state_; }

    // --- 空を飛ぶボス -------------------------------------------------------------
    bool  IsFlying() const { return def_ && def_->flying; }
    // 地面から体の下端までの高さ（地上にいれば 0）
    float Altitude() const { return altitude_; }
    // 降魔の掌で降りてきて、地上にとどまっているか（地上からの攻撃が当たる）
    bool  IsGrounded() const { return IsFlying() && altitude_ <= 1.0f; }
    // 天雷の落ちる位置（予備動作の間に予告する）
    const std::vector<float>& ThunderTargets() const { return thunderTargets_; }
    float DiveTargetX() const { return diveTargetX_; }
    // 予備動作に入ってから攻撃が終わるまで（この間は向きを変えない）
    bool IsCommittedToAttack() const
    {
        return state_ == BossState::Windup || state_ == BossState::Attack;
    }

    // --- HP バー（顔の右横に小さく重ね、名前はその左＝頭の上に出す）-----------------
    int   HpBarCount() const { return hpBars_; }
    // HP の割合 ratio のとき、上から index 本目（0 始まり）のバーがどれだけ残っているか
    static float HpBarFill(float ratio, int index, int bars);
    // HP バーと名前の位置（画面座標。namePos は名前の中央上端）。
    //   bars は上のバーから順に並ぶ（いちばん下のバーが顔の高さ）。
    //   頭が画面外にあるときや撃破後は false（描かない）
    bool  HpBarLayout(const Camera& camera, std::vector<Rect>& bars, Vec2& namePos) const;
    // 顔（頭）の中心と大きさ（ワールド座標）。HP バーはこの横に置く。
    //   竜や獣は頭が体の前にあるので、向いている側へずらす（振り向くとなめらかに移る）
    void  HeadCircle(Vec2& center, float& radius) const;

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
    // 空を飛ぶボスの移動（重力を受けず、決めた高さへ寄せる）と、浮いている間の位置取り
    void ApplyFlightPhysics(float dt, const Stage& stage);
    void UpdateFlightAltitudeTarget();
    void KeepFlightDistance(float distance);
    void SpawnThunder(CombatSystem& combat, float x);
    // ボスの攻撃の弾（共通の値を入れて返す）
    Projectile MakeProjectile(const Vec2& from, const Vec2& shotVelocity, float radius, float damageMultiplier,
                              float knockback, int kind) const;

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
    int   hpBars_ = kMinBossHpBars;
    float powerScale_ = 1.0f;
    float damageBoost_ = 1.0f;
    float speedBoost_ = 1.0f;
    float hpDisplay_ = 1.0f;
    float moveTarget_ = 0.0f;
    int   lastPatternIndex_ = -1;
    // 頭の位置を向きに合わせてずらす量（-1〜1。振り向くとなめらかに入れ替わる）
    float headSide_ = -1.0f;

    // 空を飛ぶボス
    float altitude_ = 0.0f;         // 地面から体の下端までの高さ
    float altitudeTarget_ = 0.0f;
    float groundY_ = 0.0f;          // いる奥行きの地面の Y
    float stageGroundY_ = 0.0f;     // いちばん手前（奥行き 0）の地面の Y
    float stageWidth_ = 0.0f;
    float flightTime_ = 0.0f;       // 浮遊の上下の揺れ
    float diveTargetX_ = 0.0f;      // 降魔の掌で降りる位置
    bool  diveLanded_ = false;      // 降魔の掌で地上に着いたか（硬直が終わるまで地上にとどまる）
    int   attacksSinceDive_ = 0;    // 降魔の掌を出していない回数（続いたら必ず降りる）
    std::vector<float> thunderTargets_;
    float thunderZ_ = 0.0f;
    Vec2  lastPlayerPos_;
    float lastPlayerZ_ = 0.0f;

    // 登場演出
    bool  introHeld_ = false;
    bool  barsRevealing_ = false;
    bool  introRoar_ = false;
    float roarTimer_ = 0.0f;
    float barReveal_ = 1.0f;
};

} // namespace ecl
