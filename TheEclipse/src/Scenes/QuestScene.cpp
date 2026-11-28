#include "Scenes/QuestScene.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/DxInclude.h"
#include "Core/GameConfig.h"
#include "Core/Input.h"
#include "Core/SceneManager.h"
#include "Game/EnemyDatabase.h"
#include "Game/GameContext.h"
#include "Game/ItemDatabase.h"
#include "Game/Roguelike.h"
#include "Game/UniqueSkill.h"
#include "Graphics/DrawUtil.h"
#include "Scenes/HomeTown.h"
#include "UI/UIWidgets.h"

#include <algorithm>
#include <cmath>

namespace ecl {

namespace {

constexpr float kScreenW = static_cast<float>(config::kScreenWidth);
constexpr float kScreenH = static_cast<float>(config::kScreenHeight);
constexpr float kComboHold = 2.4f;
constexpr float kParryHitStop = 0.22f;
constexpr float kParryStaggerEnemy = 1.0f;
constexpr float kParryStaggerBoss = 1.2f;

// --- 装備の摩耗量 -------------------------------------------------------------
// 攻撃を当てるたびに武器が、被弾するたびに防具が消耗する
constexpr float kWeaponWearPerHit = 0.30f;
constexpr float kArmorWearPerHit = 0.28f;
constexpr float kShieldWearPerGuard = 0.45f;

//------------------------------------------------------------------------------
// ボスの登場演出の流れ（秒）
//   暗転明け → 上下に黒帯 → カメラがボスへ → 咆哮 → 名前の札 → カメラが戻って戦闘開始
//   ENTER / 通常攻撃 / クリックで飛ばせる（戻りのパンだけは見せる）
//------------------------------------------------------------------------------
constexpr float kBossIntroFadeIn = 0.6f;    // 暗転から明ける
constexpr float kBossIntroBarTime = 0.5f;   // 黒帯が出入りする時間
constexpr float kBossIntroPanStart = 0.3f;  // ボスへのパン
constexpr float kBossIntroPanEnd = 1.8f;
constexpr float kBossIntroRoar = 1.8f;      // 咆哮（画面振動・衝撃波・閃光）
constexpr float kBossIntroRoarTime = 1.1f;
constexpr float kBossIntroBars = 2.3f;      // ボスの HP バーを出して溜める
constexpr float kBossIntroCardIn = 2.1f;    // 名前の札
constexpr float kBossIntroCardOut = 4.4f;
constexpr float kBossIntroOutro = 4.5f;     // プレイヤーへ戻るパン
constexpr float kBossIntroEnd = 5.4f;
constexpr float kBossIntroSkipAfter = 0.4f; // これより前の入力は飛ばしに使わない
// ボスラッシュ：最後のフロアの中ボスを倒してから、最後のボスが現れるまで（中ボスの撃破演出を見せる）
constexpr float kFinalBossDelay = 2.8f;
constexpr float kLetterboxHeight = 120.0f;

//------------------------------------------------------------------------------
// デバッグモードの情報パネル（左上の HUD の下）。下の段にフロア移動のボタンを並べる
//------------------------------------------------------------------------------
constexpr float kDebugPanelX = 28.0f;
constexpr float kDebugPanelY = 254.0f;
constexpr float kDebugPanelW = 420.0f;
constexpr float kDebugPanelH = 272.0f;
constexpr float kDebugFloorTextY = 158.0f;   // パネル上端から「フロア移動」の見出しまで
constexpr float kDebugFloorRowY = 182.0f;    // パネル上端からボタンの段まで
constexpr float kDebugFloorButtonH = 34.0f;
constexpr float kDebugArrowW = 44.0f;        // ◀ / ▶ の幅
constexpr float kDebugButtonGap = 6.0f;
constexpr float kDebugHiddenRowY = 226.0f;   // パネル上端から「隠しフロア」の段まで
constexpr float kDebugHiddenButtonW = 132.0f;

Rect DebugPanelRect()
{
    return Rect::FromXYWH(kDebugPanelX, kDebugPanelY, kDebugPanelW, kDebugPanelH);
}

float EaseInOut(float t)
{
    t = math::Clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

} // namespace

QuestScene::QuestScene() = default;

void QuestScene::OnEnter(GameContext& context)
{
    // ローグライクモードのラン中は、生成したフロアを進む
    roguelike_ = context.roguelike.active;
    quest_ = roguelike_ ? &context.roguelike.quest : QuestDatabase::Instance().Find(context.selectedQuestId);
    if (!quest_) quest_ = QuestDatabase::Instance().Find(1);
    LayoutDebugFloorButtons();
    menu_.SetRoguelike(roguelike_);
    pickups_.Clear();
    draft_.Close();
    shop_.Close();
    smith_.Close();
    pendingDrafts_ = 0;
    nearFacility_ = -1;
    runNotice_.clear();
    runNoticeTimer_ = 0.0f;
    worldTime_ = 0.0f;

    player_.Setup(context.player);
    player_.FullHeal();

    hud_.Reset();
    menu_.Close();
    combat_.Clear();

    enemiesDefeated_ = 0;
    totalDamage_ = 0;
    damageTaken_ = 0;
    expGained_ = 0;
    colGained_ = 0;
    combo_ = 0;
    maxCombo_ = 0;
    parryCount_ = 0;
    brokenItems_.clear();
    breakNotice_.clear();
    breakNoticeTimer_ = 0.0f;
    comboTimer_ = 0.0f;
    questTime_ = 0.0f;
    hitStop_ = 0.0f;
    transitioning_ = false;
    transitionAlpha_ = 0.0f;
    transitionText_.clear();
    debugClickConsumed_ = false;
    bossRewardGranted_ = false;

    // 隠しフロアの抽選（隠しフロアのあるクエストに挑むたびに数え、10〜30 回ごとに 1 回抽選する）
    hiddenLotteryHeld_ = false;
    hiddenRoute_ = false;
    mainBossDefeated_ = false;
    hiddenFloorReached_ = false;
    hiddenBossDefeated_ = false;
    if (quest_ && quest_->hasHiddenFloor) {
        const HiddenFloorDraw draw = context.player.BeginHiddenFloorAttempt(quest_->id);
        hiddenLotteryHeld_ = draw.held;
        hiddenRoute_ = draw.won;
    }

    camera_.SetShakeEnabled(context.settings.screenShake);
    LoadFloor(0, context);
}

const FloorDef& QuestScene::FloorAt(int index) const
{
    if (IsHiddenFloor(index)) return quest_->hiddenFloor;
    return quest_->floors[static_cast<size_t>(math::ClampInt(index, 0, quest_->FloorCount() - 1))];
}

int QuestScene::LastFloorIndex() const
{
    if (!quest_) return 0;
    return quest_->FloorCount() - 1 + (quest_->hasHiddenFloor ? 1 : 0);
}

bool QuestScene::IsHiddenFloor(int index) const
{
    return quest_ && quest_->hasHiddenFloor && index == quest_->FloorCount();
}

void QuestScene::OpenHiddenGate()
{
    // 竜王を倒した場所に、隠しフロアへのゲートが開く（通常の勝利の代わり）
    phase_ = Phase::FloorClear;
    phaseTimer_ = 0.0f;
    stage_.SetGateOpen(true);
    stage_.SetGateLabel("▶ 隠しフロアへ");
    bannerMain_ = "HIDDEN FLOOR";
    bannerSub_ = "隠しフロアへの扉が開いた……右のゲートへ";
    bannerTimer_ = 3.2f;
}

void QuestScene::OpenBossRushGate()
{
    phase_ = Phase::FloorClear;
    phaseTimer_ = 0.0f;
    stage_.SetGateOpen(true);
    bannerMain_ = "FLOOR CLEAR";
    bannerSub_ = roguelike_ ? "次の階層へ……右のゲートへ" : "次の守護者が待っている……右のゲートへ";
    bannerTimer_ = 2.6f;
}

bool QuestScene::FinalBossPending() const
{
    if (!quest_ || IsHiddenFloor(floorIndex_)) return false;
    return FloorAt(floorIndex_).finalBossId >= 0 && !finalBossSpawned_;
}

void QuestScene::SpawnFinalBoss()
{
    awaitingFinalBoss_ = false;
    finalBossSpawned_ = true;
    if (!quest_) return;
    const FloorDef& floor = FloorAt(floorIndex_);
    const BossDef* def = BossDatabase::Instance().Find(floor.finalBossId);
    if (!def) return;

    boss_.reset(new Boss());
    const float bossZ = stage_.Depth() * 0.5f;
    const int bars = (floor.finalBossHpBars > 0)
                   ? math::ClampInt(floor.finalBossHpBars, kMinBossHpBars, kMaxBossHpBars)
                   : QuestDatabase::BossHpBarCount(*quest_);
    // プレイヤーと反対側の、画面の少し外から現れる
    const float x = (player_.pos.x < stage_.Width() * 0.5f) ? stage_.Width() - 600.0f : 600.0f;
    boss_->Setup(*def, Vec2(x, stage_.GroundYAt(bossZ)), FloorPowerScale(floor), bars);
    boss_->z = bossZ;
    bossRewardGranted_ = false;
    // 中ボスの攻撃の残りは消す（登場演出の間に当たらないように）
    combat_.Clear();
    StartBossIntro();
}

void QuestScene::LoadFloor(int index, GameContext& context)
{
    if (!quest_) return;

    floorIndex_ = math::ClampInt(index, 0, LastFloorIndex());
    const FloorDef& floor = FloorAt(floorIndex_);
    if (IsHiddenFloor(floorIndex_)) hiddenFloorReached_ = true;

    stage_.Load(floor);
    enemies_.clear();
    boss_.reset();
    combat_.Clear();
    pickups_.Clear();   // 拾わなかったものはフロアに置いていく
    nearFacility_ = -1;
    if (roguelike_ && quest_) {
        // GameContext は const で渡されないので、たどり着いたフロアはここで記録する
        rogue::RunState& run = context.roguelike;
        run.deepestFloor = math::MaxI(run.deepestFloor, floorIndex_ + 1);
    }
    finalBossSpawned_ = false;
    awaitingFinalBoss_ = false;
    finalBossTimer_ = 0.0f;

    const float playerZ = stage_.Depth() * 0.5f;
    player_.PlaceAt(Vec2(220.0f, stage_.GroundYAt(playerZ)));
    player_.z = playerZ;

    camera_.Reset();
    camera_.SetWorldBounds(0.0f, stage_.Width(), 0.0f, kScreenH);
    camera_.SetTarget(Vec2(player_.pos.x, kScreenH * 0.5f));
    camera_.SnapToTarget();

    SpawnFloorEnemies(context);

    // ボスフロア
    if (floor.bossId >= 0) {
        const BossDef* def = BossDatabase::Instance().Find(floor.bossId);
        if (def) {
            boss_.reset(new Boss());
            const float bossZ = stage_.Depth() * 0.5f;
            // HP バーの本数はフロアで決めていればそれ、無ければクエストの強さから
            const int bars = (floor.bossHpBars > 0) ? math::ClampInt(floor.bossHpBars, kMinBossHpBars, kMaxBossHpBars)
                                                    : QuestDatabase::BossHpBarCount(*quest_);
            boss_->Setup(*def, Vec2(stage_.Width() - 600.0f, stage_.GroundYAt(bossZ)),
                         FloorPowerScale(floor), bars);
            boss_->z = bossZ;
            // ボスごとに撃破の報酬を渡す（隠しボスは竜王とは別に貰える）
            bossRewardGranted_ = false;
        }
    }

    phaseTimer_ = 0.0f;
    if (boss_) {
        // ボスフロアは登場演出から始める（フロア名のバナーの代わり）
        StartBossIntro();
    } else {
        bannerMain_ = floor.name;
        bannerSub_ = quest_->name;
        bannerTimer_ = 2.0f;
        phase_ = Phase::FloorIntro;
    }
}

//==============================================================================
// ボスの登場演出
//==============================================================================
void QuestScene::StartBossIntro()
{
    phase_ = Phase::BossIntro;
    bossIntroTimer_ = 0.0f;
    bossIntroRoared_ = false;
    bossIntroSkipped_ = false;
    bossIntroOutroStarted_ = false;
    bossIntroOutroFromX_ = 0.0f;
    bannerTimer_ = 0.0f;
    // ボスは演出が終わるまで登場状態で待つ（攻撃も被弾もしない）
    if (boss_) boss_->HoldIntro(true);
}

void QuestScene::EndBossIntro()
{
    if (boss_) boss_->HoldIntro(false);
    phase_ = Phase::Battle;
    phaseTimer_ = 0.0f;
}

float QuestScene::BossIntroCameraX() const
{
    // 普段の追従と同じ狙い（向いている側を少し広く見せる）
    const float playerX = player_.pos.x + static_cast<float>(player_.facing) * 140.0f;
    if (!boss_) return playerX;
    const float bossX = boss_->pos.x;

    const float t = bossIntroTimer_;
    if (bossIntroOutroStarted_) {
        const float back = EaseInOut((t - kBossIntroOutro) / (kBossIntroEnd - kBossIntroOutro));
        return math::Lerp(bossIntroOutroFromX_, playerX, back);
    }
    const float pan = EaseInOut((t - kBossIntroPanStart) / (kBossIntroPanEnd - kBossIntroPanStart));
    return math::Lerp(playerX, bossX, pan);
}

void QuestScene::UpdateBossIntro(float dt, const Input& input)
{
    // デバッグの殲滅などで演出中に倒れたら、そのまま戦闘へ（撃破の処理に進む）
    if (!BossAlive()) {
        EndBossIntro();
        return;
    }

    bossIntroTimer_ += dt;

    // 飛ばす：名前の札までを省いて、戻りのパンから見せる
    const bool skip = input.Pressed(GameAction::Confirm) || input.Pressed(GameAction::Attack)
                   || input.MouseClicked(MouseButton::Left);
    if (skip && bossIntroTimer_ >= kBossIntroSkipAfter && bossIntroTimer_ < kBossIntroOutro) {
        bossIntroSkipped_ = true;
        bossIntroTimer_ = kBossIntroOutro;
        boss_->RevealHpBars();
    }

    // 戻りのパンは、今のカメラ位置から始める（飛ばしたときも滑らかに戻る）
    if (!bossIntroOutroStarted_ && bossIntroTimer_ >= kBossIntroOutro) {
        bossIntroOutroStarted_ = true;
        bossIntroOutroFromX_ = camera_.Center().x;
        boss_->SetIntroRoar(false);
    }

    // 咆哮
    if (!bossIntroRoared_ && !bossIntroSkipped_ && bossIntroTimer_ >= kBossIntroRoar) {
        bossIntroRoared_ = true;
        const ColorRGB color = boss_->art.trim;
        boss_->SetIntroRoar(true);
        camera_.Shake(16.0f, 0.8f);
        combat_.AddRing(Vec2(boss_->pos.x, boss_->pos.y), 620.0f, color, 0.9f);
        combat_.AddRing(Vec2(boss_->pos.x, boss_->pos.y), 380.0f, palette::kWhite, 0.6f);
        combat_.AddImpact(Vec2(boss_->pos.x, boss_->pos.y - boss_->height * 0.6f), color, 46, 780.0f);
        combat_.AddDust(Vec2(boss_->pos.x, boss_->pos.y), 1, 18);
        combat_.AddDust(Vec2(boss_->pos.x, boss_->pos.y), -1, 18);
    }
    if (bossIntroRoared_ && bossIntroTimer_ >= kBossIntroRoar + kBossIntroRoarTime) {
        boss_->SetIntroRoar(false);
    }
    // 咆哮のあと、カメラがボスを映している間に HP バーを出して溜める
    if (bossIntroTimer_ >= kBossIntroBars) boss_->RevealHpBars();

    if (bossIntroTimer_ >= kBossIntroEnd) EndBossIntro();
}

void QuestScene::DrawBossIntro() const
{
    if (!boss_ || !boss_->Def()) return;
    const BossDef& def = *boss_->Def();
    const float t = bossIntroTimer_;

    // --- 暗転明け ---------------------------------------------------------------
    if (t < kBossIntroFadeIn) {
        draw::FillRect(Rect(0.0f, 0.0f, kScreenW, kScreenH), palette::kBlack,
                       static_cast<int>((1.0f - t / kBossIntroFadeIn) * 255.0f));
    }

    // --- 咆哮の閃光 ---------------------------------------------------------------
    if (bossIntroRoared_ && t >= kBossIntroRoar && t < kBossIntroRoar + 0.4f) {
        const float flash = 1.0f - (t - kBossIntroRoar) / 0.4f;
        draw::FillRect(Rect(0.0f, 0.0f, kScreenW, kScreenH), def.trim.Scaled(1.4f),
                       static_cast<int>(flash * 110.0f));
    }

    // --- 名前の札 ---------------------------------------------------------------
    if (!bossIntroSkipped_ && t >= kBossIntroCardIn && t < kBossIntroCardOut + 0.3f) {
        const float in = EaseInOut((t - kBossIntroCardIn) / 0.35f);
        const float out = 1.0f - EaseInOut((t - kBossIntroCardOut) / 0.3f);
        const float appear = math::MinF(in, out);
        const int alpha = static_cast<int>(appear * 255.0f);
        const float cx = kScreenW * 0.5f;
        const float slide = (1.0f - in) * -80.0f;   // 左から滑り込む
        const float top = 196.0f;

        // 帯（半透明の 1 枚。分割して重ねると継ぎ目が濃く見えるため）
        const Rect band(0.0f, top, kScreenW, top + 186.0f);
        draw::FillRect(band, def.main.Scaled(0.18f), static_cast<int>(appear * 200.0f));
        // 上下の線は中央から左右へ伸びる
        const float lineHalf = kScreenW * 0.5f * in;
        draw::Line(cx - lineHalf, band.top, cx + lineHalf, band.top, def.trim, 2.0f, alpha);
        draw::Line(cx - lineHalf, band.bottom, cx + lineHalf, band.bottom, def.trim, 2.0f, alpha);

        draw::TextAlpha(FontSize::Medium, cx + slide, top + 18.0f, def.trim, def.title, alpha,
                        draw::TextAlign::Center);
        draw::TextAlpha(FontSize::Huge, cx + slide * 0.6f, top + 58.0f, palette::kWhite, def.name, alpha,
                        draw::TextAlign::Center);
        // 難易度（★）
        if (quest_ && quest_->difficulty > 0) {
            std::string stars;
            for (int i = 0; i < quest_->difficulty; ++i) stars += "★";
            draw::TextAlpha(FontSize::Small, cx + slide * 0.3f, top + 138.0f, def.trim, stars, alpha,
                            draw::TextAlign::Center);
        }
    }

    // --- 上下の黒帯 ---------------------------------------------------------------
    const float barIn = EaseInOut(t / kBossIntroBarTime);
    const float barOut = EaseInOut((kBossIntroEnd - t) / kBossIntroBarTime);
    const float barH = kLetterboxHeight * math::MinF(barIn, barOut);
    if (barH > 0.5f) {
        draw::FillRect(Rect(0.0f, 0.0f, kScreenW, barH), palette::kBlack, 255);
        draw::FillRect(Rect(0.0f, kScreenH - barH, kScreenW, kScreenH), palette::kBlack, 255);
        const int textAlpha = static_cast<int>(math::Clamp((barH - 60.0f) / 60.0f, 0.0f, 1.0f) * 255.0f);
        if (textAlpha > 0) {
            draw::TextAlpha(FontSize::Small, 60.0f, barH - 46.0f, palette::kDanger, "BOSS BATTLE", textAlpha);
            if (!bossIntroOutroStarted_) {
                draw::TextAlpha(FontSize::Small, kScreenW - 60.0f, kScreenH - barH + 30.0f, palette::kTextDim,
                                "ENTER / クリックでスキップ", textAlpha, draw::TextAlign::Right);
            }
        }
    }
}

void QuestScene::SpawnFloorEnemies(const GameContext& context)
{
    (void)context;
    if (!quest_) return;

    const FloorDef& floor = FloorAt(floorIndex_);
    const EnemyDatabase& database = EnemyDatabase::Instance();

    for (const EnemySpawn& spawn : floor.spawns) {
        const EnemyDef* def = database.Find(spawn.enemyId);
        if (!def) continue;

        // 奥行きは指定が無ければフィールド内でばらけさせる
        const float spawnZ = (spawn.z >= 0.0f) ? stage_.ClampZ(spawn.z)
                                               : math::RandFloat(0.0f, stage_.Depth());
        const float baseY = (spawn.y > 0.0f) ? spawn.y : stage_.GroundY();

        std::unique_ptr<Enemy> enemy(new Enemy());
        enemy->Setup(*def, Vec2(spawn.x, baseY - spawnZ), FloorPowerScale(floor));
        enemy->z = spawnZ;
        enemies_.push_back(std::move(enemy));
    }
}

int QuestScene::AliveEnemyCount() const
{
    int count = 0;
    for (const std::unique_ptr<Enemy>& enemy : enemies_) {
        if (enemy->alive) ++count;
    }
    if (BossAlive()) ++count;
    // ボスラッシュの最後のフロアで、中ボスを倒して最後のボスを待っている間も「残 1 体」
    if (awaitingFinalBoss_) ++count;
    return count;
}

bool QuestScene::BossAlive() const
{
    return boss_ && boss_->alive;
}

void QuestScene::Update(float dt, GameContext& context, SceneManager& manager)
{
    const Input& input = Input::Instance();

    // --- メニュー -----------------------------------------------------------
    if (menu_.IsOpen()) {
        menu_.Update(dt, input, context);
        camera_.SetShakeEnabled(context.settings.screenShake);

        // メニューで装備を変えたら、その場で反映する
        if (menu_.EquipmentChanged()) {
            player_.RefreshEquipment(context.player);
        }
        if (menu_.RetireConfirmed()) {
            FinishQuest(false, true, context);
            manager.RequestChange(SceneId::Result);
            phase_ = Phase::Finished;
        }
        return;
    }

    // ローグライク：スキル選択・ショップ・鍛冶屋を開いている間はゲームを止める
    if (roguelike_ && RoguelikePanelOpen()) {
        UpdateRoguelikePanels(dt, input, context);
        return;
    }

    hud_.Update(dt, player_, boss_.get(), input);
    if (input.Pressed(GameAction::Menu) && phase_ != Phase::Finished) {
        menu_.Open();
        return;
    }

    // スキルアイコンのクリックで発動
    const int clickedSkill = hud_.ClickedSkillIndex();
    if (clickedSkill >= 0 && phase_ == Phase::Battle) {
        player_.UseSkill(clickedSkill, combat_);
    }

    // アイテムカード（E か手前のカードのクリックで使う。フロア移動の暗転中は使えない）
    if (hud_.ItemUseRequested() && !transitioning_
        && (phase_ == Phase::Battle || phase_ == Phase::FloorClear || phase_ == Phase::FloorIntro)) {
        UseQuickItem(context);
    }

    // --- フロア遷移中 --------------------------------------------------------
    if (transitioning_) {
        UpdateFloorTransition(dt, context);
        camera_.Update(dt);
        return;
    }

    UpdateDebug(input, context);
    if (context.settings.debugMode) UpdateDebugFloorButtons(input, dt);
    debugMessageTimer_ = math::MaxF(0.0f, debugMessageTimer_ - dt);

    phaseTimer_ += dt;
    bannerTimer_ = math::MaxF(0.0f, bannerTimer_ - dt);
    breakNoticeTimer_ = math::MaxF(0.0f, breakNoticeTimer_ - dt);
    runNoticeTimer_ = math::MaxF(0.0f, runNoticeTimer_ - dt);
    worldTime_ += dt;

    // ローグライク：フロアのショップ・鍛冶屋に入る
    if (roguelike_) {
        UpdateFacilities(input, context);
        if (RoguelikePanelOpen()) return;
    }

    if (phase_ == Phase::Battle || phase_ == Phase::FloorClear || phase_ == Phase::FloorIntro) {
        questTime_ += dt;
    }

    // --- ヒットストップ -------------------------------------------------------
    float actorDt = dt;
    if (hitStop_ > 0.0f) {
        hitStop_ = math::MaxF(0.0f, hitStop_ - dt);
        actorDt = 0.0f;
    }

    // --- コンボ -------------------------------------------------------------
    if (combo_ > 0) {
        comboTimer_ = math::MaxF(0.0f, comboTimer_ - dt);
        if (comboTimer_ <= 0.0f) combo_ = 0;
    }

    // --- フェーズ処理 --------------------------------------------------------
    switch (phase_) {
    case Phase::FloorIntro:
        if (phaseTimer_ >= 1.2f) {
            phase_ = Phase::Battle;
            phaseTimer_ = 0.0f;
        }
        break;

    case Phase::BossIntro:
        UpdateBossIntro(dt, input);
        break;

    case Phase::Battle:
        if (!player_.alive) {
            phase_ = Phase::Defeat;
            phaseTimer_ = 0.0f;
            camera_.Shake(18.0f, 0.5f);
        } else if (awaitingFinalBoss_) {
            // 中ボスの撃破演出のあと、最後のボスが現れる
            finalBossTimer_ += dt;
            if (finalBossTimer_ >= kFinalBossDelay) SpawnFinalBoss();
        } else if (boss_ && !boss_->alive) {
            combat_.AddPopup(Vec2(player_.pos.x, player_.pos.y - 240.0f), "BOSS DEFEATED",
                             palette::kAccentWarm, true);
            if (FinalBossPending()) {
                // ボスラッシュの最後のフロア：中ボスを倒すと、最後のボスが現れる
                awaitingFinalBoss_ = true;
                finalBossTimer_ = 0.0f;
                bannerMain_ = "???";
                bannerSub_ = "玉座の主が目を覚ます……";
                bannerTimer_ = kFinalBossDelay;
                camera_.Shake(10.0f, 1.2f);
            } else if (quest_ && quest_->bossRush && !IsHiddenFloor(floorIndex_)
                       && floorIndex_ < quest_->FloorCount() - 1) {
                // ボスラッシュの途中のフロア：中ボスを倒すとゲートが開く
                OpenBossRushGate();
            } else if (IsHiddenFloor(floorIndex_)) {
                hiddenBossDefeated_ = true;
                phase_ = Phase::Victory;
                phaseTimer_ = 0.0f;
            } else {
                mainBossDefeated_ = true;
                if (hiddenRoute_ && quest_ && quest_->hasHiddenFloor) {
                    OpenHiddenGate();   // 抽選に当たっていれば、勝利の代わりに隠しフロアへ
                } else {
                    phase_ = Phase::Victory;
                    phaseTimer_ = 0.0f;
                }
            }
        } else if (!boss_ && AliveEnemyCount() == 0) {
            phase_ = Phase::FloorClear;
            phaseTimer_ = 0.0f;
            stage_.SetGateOpen(true);
            bannerMain_ = "FLOOR CLEAR";
            bannerSub_ = "右のゲートへ進もう";
            bannerTimer_ = 2.4f;
        }
        break;

    case Phase::FloorClear:
        if (!player_.alive) {
            phase_ = Phase::Defeat;
            phaseTimer_ = 0.0f;
        } else if (stage_.ReachedGate(player_.pos.x)) {
            transitioning_ = true;
            transitionAlpha_ = 0.0f;
            pendingFloor_ = floorIndex_ + 1;
            transitionText_ = IsHiddenFloor(pendingFloor_) ? "隠しフロアへ..." : "次のフロアへ...";
        }
        break;

    case Phase::Victory:
        if (phaseTimer_ >= 3.0f) {
            FinishQuest(true, false, context);
            manager.RequestChange(SceneId::Result);
            phase_ = Phase::Finished;
        }
        break;

    case Phase::Defeat:
        if (phaseTimer_ >= 3.2f) {
            FinishQuest(false, false, context);
            manager.RequestChange(SceneId::Result);
            phase_ = Phase::Finished;
        }
        break;

    case Phase::Finished:
    default:
        break;
    }

    // --- 更新 ---------------------------------------------------------------
    stage_.Update(dt);
    UpdateActors(actorDt, context);

    if (actorDt > 0.0f) {
        ResolveHitBoxes(context);
        ResolveProjectiles(context);
        CleanupDead(context);
        combat_.Update(actorDt);
        if (roguelike_) UpdatePickups(actorDt, context);
    }

    // --- カメラ -------------------------------------------------------------
    if (phase_ == Phase::BossIntro) {
        // 登場演出：決めた動きのとおりに動かす（追従の遅れも、端の手前の減速も入れない）
        camera_.SetTarget(Vec2(BossIntroCameraX(), kScreenH * 0.5f));
        camera_.SnapToTarget(false);
        camera_.Update(dt, false);
        return;
    }
    float targetX = player_.pos.x + static_cast<float>(player_.facing) * 140.0f;
    if (BossAlive() && math::Abs(boss_->pos.x - player_.pos.x) < 900.0f) {
        // ボス戦では両者が収まるように中間を狙う
        targetX = (player_.pos.x * 0.65f + boss_->pos.x * 0.35f);
    }
    camera_.SetTarget(Vec2(targetX, kScreenH * 0.5f));
    camera_.Update(dt);
}

void QuestScene::UpdateDebug(const Input& input, GameContext& context)
{
    debugClickConsumed_ = false;
    if (!context.settings.debugMode) {
        debugInvincible_ = false;
        debugShowHitBoxes_ = false;
        return;
    }

    auto notify = [this](const std::string& text) {
        debugMessage_ = text;
        debugMessageTimer_ = 2.0f;
    };

    // F1 : 無敵の切り替え
    if (input.KeyPressed(KEY_INPUT_F1)) {
        debugInvincible_ = !debugInvincible_;
        notify(debugInvincible_ ? "無敵 ON" : "無敵 OFF");
    }
    // F2 : フロアの敵を殲滅（ボス含む）
    if (input.KeyPressed(KEY_INPUT_F2)) {
        int killed = 0;
        for (std::unique_ptr<Enemy>& enemy : enemies_) {
            if (!enemy->alive) continue;
            enemy->ApplyDirectDamage(static_cast<int>(enemy->hp) + 1, 0.0f, combat_);
            ++killed;
        }
        if (boss_ && boss_->alive) {
            // ボスは 1 回の攻撃でバー 1 本までしか削れないので、まとめて倒す
            boss_->ForceKill(combat_);
            ++killed;
        }
        notify(str::Format("敵を殲滅しました（%d 体）", killed));
    }
    // F3 : HP / MP 全回復
    if (input.KeyPressed(KEY_INPUT_F3)) {
        player_.FullHeal();
        notify("HP / MP を全回復しました");
    }
    // F4 : 当たり判定の表示
    if (input.KeyPressed(KEY_INPUT_F4)) {
        debugShowHitBoxes_ = !debugShowHitBoxes_;
        notify(debugShowHitBoxes_ ? "判定表示 ON" : "判定表示 OFF");
    }
    // F6 : ユニークスキルを全て解放可能にして習得する
    if (input.KeyPressed(KEY_INPUT_F6)) {
        context.player.DebugUnlockAllUniqueSkills();
        player_.RefreshEquipment(context.player);
        notify(str::Format("ユニークスキルを解放（%s）",
                           UniqueSkillName(context.player.UniqueSkill())));
    }
    // F5 : col と素材、スキルポイントを追加
    if (input.KeyPressed(KEY_INPUT_F5)) {
        context.player.GetInventory().AddCol(10000);
        context.player.GetInventory().AddMaterial(50);
        context.player.AddSkillPoints(5);
        notify("col +10000 / 強化結晶 +50 / SP +5");
    }

    if (debugInvincible_ && player_.alive) {
        player_.invincibleTimer = math::MaxF(player_.invincibleTimer, 0.5f);
    }
}

void QuestScene::LayoutDebugFloorButtons()
{
    debugFloorButtons_.clear();
    const int count = quest_ ? quest_->FloorCount() : 1;
    const Rect panel = DebugPanelRect();
    const float left = panel.left + 12.0f;
    const float width = panel.Width() - 24.0f;
    const float y = panel.top + kDebugFloorRowY;
    // ◀ と ▶ の間に、フロアの数だけ同じ幅で並べる（ローグライクのように多いときは ◀ ▶ だけ）
    const bool compact = count > 8;
    const float floorW = compact ? 0.0f
                       : (width - kDebugArrowW * 2.0f - kDebugButtonGap * static_cast<float>(count + 1))
                             / static_cast<float>(math::MaxI(1, count));

    debugPrevFloorButton_ = ui::Button(Rect::FromXYWH(left, y, kDebugArrowW, kDebugFloorButtonH), "◀",
                                       FontSize::Small);
    float x = left + kDebugArrowW + kDebugButtonGap;
    for (int i = 0; i < count && !compact; ++i) {
        // ボスラッシュはどのフロアにも中ボスがいるので、「ボス」は最後のフロアだけ
        const bool bossFloor = quest_ && quest_->floors[static_cast<size_t>(i)].bossId >= 0
                            && !(quest_->bossRush && i < count - 1);
        debugFloorButtons_.push_back(ui::Button(Rect::FromXYWH(x, y, floorW, kDebugFloorButtonH),
                                                bossFloor ? std::string("ボス") : str::Format("%dF", i + 1),
                                                FontSize::Tiny));
        x += floorW + kDebugButtonGap;
    }
    debugNextFloorButton_ = ui::Button(Rect::FromXYWH(x, y, kDebugArrowW, kDebugFloorButtonH), "▶",
                                       FontSize::Small);

    debugPrevFloorButton_.SetAccent(palette::kAccentWarm);
    debugNextFloorButton_.SetAccent(palette::kAccentWarm);
    for (ui::Button& button : debugFloorButtons_) button.SetAccent(palette::kAccentWarm);

    // 「隠しフロア」（抽選に関係なく、すぐ隠しフロアへ移る）
    debugHiddenFloorButton_ = ui::Button(Rect::FromXYWH(left, panel.top + kDebugHiddenRowY, kDebugHiddenButtonW,
                                                        kDebugFloorButtonH),
                                         "隠しフロア", FontSize::Tiny);
    debugHiddenFloorButton_.SetAccent(palette::kAccentWarm);
}

void QuestScene::UpdateDebugFloorButtons(const Input& input, float dt)
{
    if (!quest_) return;
    const int count = quest_->FloorCount();
    const bool allowed = DebugFloorMoveAllowed();

    int target = -1;
    debugPrevFloorButton_.SetEnabled(allowed && floorIndex_ > 0);
    debugNextFloorButton_.SetEnabled(allowed && floorIndex_ < count - 1);
    if (debugPrevFloorButton_.Update(input, dt)) target = floorIndex_ - 1;
    if (debugNextFloorButton_.Update(input, dt)) target = floorIndex_ + 1;
    for (int i = 0; i < static_cast<int>(debugFloorButtons_.size()); ++i) {
        ui::Button& button = debugFloorButtons_[static_cast<size_t>(i)];
        // いまいるフロアは印を付けて、押せないようにする
        button.SetSelected(i == floorIndex_);
        button.SetEnabled(allowed && i != floorIndex_);
        if (button.Update(input, dt)) target = i;
    }
    // 隠しフロア（隠しフロアのあるクエストだけ押せる）
    const bool hasHidden = quest_->hasHiddenFloor;
    debugHiddenFloorButton_.SetSelected(IsHiddenFloor(floorIndex_));
    debugHiddenFloorButton_.SetEnabled(allowed && hasHidden && !IsHiddenFloor(floorIndex_));
    if (debugHiddenFloorButton_.Update(input, dt)) target = count;

    // ボタンの段をクリックしたフレームは、同じクリックで攻撃しない（押せないボタンの上でも）
    if (input.MouseClicked(MouseButton::Left)) {
        const Rect row(debugPrevFloorButton_.GetRect().left, debugPrevFloorButton_.GetRect().top,
                       debugNextFloorButton_.GetRect().right, debugNextFloorButton_.GetRect().bottom);
        const float mx = static_cast<float>(input.MouseX());
        const float my = static_cast<float>(input.MouseY());
        if (row.Expanded(4.0f).Contains(mx, my) || debugHiddenFloorButton_.GetRect().Expanded(4.0f).Contains(mx, my)) {
            debugClickConsumed_ = true;
        }
    }

    if (target >= 0) DebugMoveToFloor(target);
}

bool QuestScene::DebugFloorMoveAllowed() const
{
    if (!quest_ || transitioning_) return false;
    // ボスを倒したあと（リザルトへ向かう間）と戦闘不能のあとは動かさない
    return phase_ == Phase::FloorIntro || phase_ == Phase::BossIntro || phase_ == Phase::Battle
        || phase_ == Phase::FloorClear;
}

void QuestScene::DebugMoveToFloor(int index)
{
    if (!DebugFloorMoveAllowed()) return;
    if (index < 0 || index > LastFloorIndex() || index == floorIndex_) return;

    // ゲートを通ったときと同じく、暗転してからフロアを読み込む（敵は出し直し、HP / MP はそのまま）
    transitioning_ = true;
    transitionAlpha_ = 0.0f;
    pendingFloor_ = index;
    transitionText_ = DebugFloorName(index) + " へ移動...";
    bannerTimer_ = 0.0f;
    debugMessage_ = str::Format("%s「%s」へ移動しました", DebugFloorName(index).c_str(), FloorAt(index).name.c_str());
    debugMessageTimer_ = 2.5f;
}

std::string QuestScene::DebugFloorName(int index) const
{
    if (IsHiddenFloor(index)) return "隠しフロア";
    if (quest_ && index >= 0 && index < quest_->FloorCount()
        && quest_->floors[static_cast<size_t>(index)].bossId >= 0) {
        return "ボスフロア";
    }
    return str::Format("%dF", index + 1);
}

void QuestScene::DrawDebugOverlay(const GameContext& context) const
{
    if (!context.settings.debugMode) return;

    if (debugShowHitBoxes_) {
        combat_.DrawDebugHitBoxes(camera_);

        // アクターの当たり判定も表示
        auto drawBounds = [this](const Actor& actor, const ColorRGB& color) {
            const Rect screen = camera_.WorldToScreen(actor.Bounds());
            draw::StrokeRect(screen, color, 1.0f, 150);
            const Vec2 feet = camera_.WorldToScreen(Vec2(actor.pos.x, actor.pos.y));
            draw::Line(feet.x - 14.0f, feet.y, feet.x + 14.0f, feet.y, color, 2.0f, 200);
        };
        drawBounds(player_, ColorRGB(120, 255, 180));
        for (const std::unique_ptr<Enemy>& enemy : enemies_) drawBounds(*enemy, ColorRGB(255, 140, 140));
        if (boss_) drawBounds(*boss_, ColorRGB(255, 100, 100));
    }

    // --- 情報パネル ---------------------------------------------------------
    const Rect panel = DebugPanelRect();
    draw::FillRect(panel, palette::kBlack, 170);
    draw::StrokeRect(panel, palette::kAccentWarm, 1.0f, 200);
    draw::Text(FontSize::Small, panel.left + 12.0f, panel.top + 8.0f, palette::kAccentWarm,
               "DEBUG MODE");

    float y = panel.top + 38.0f;
    const char* stateNames[] = { "Normal", "Attack", "Skill", "Dash", "Hurt", "Dead" };
    draw::Text(FontSize::Tiny, panel.left + 12.0f, y, palette::kText,
               str::Format("pos %.0f, %.0f   z %.0f   state %s", player_.pos.x, player_.pos.y,
                           player_.z, stateNames[static_cast<int>(player_.State())]));
    y += 24.0f;
    draw::Text(FontSize::Tiny, panel.left + 12.0f, y, palette::kText,
               str::Format("敵 %d 体   判定 %d 個   ヒットストップ %.2f",
                           AliveEnemyCount(), static_cast<int>(combat_.HitBoxCount()), hitStop_));
    y += 24.0f;
    draw::Text(FontSize::Tiny, panel.left + 12.0f, y, palette::kText,
               str::Format("無敵 %s   判定表示 %s", debugInvincible_ ? "ON" : "OFF",
                           debugShowHitBoxes_ ? "ON" : "OFF"));
    y += 24.0f;
    draw::Text(FontSize::Tiny, panel.left + 12.0f, y, palette::kTextDim,
               "F1 無敵 / F2 殲滅 / F3 全回復");
    y += 20.0f;
    draw::Text(FontSize::Tiny, panel.left + 12.0f, y, palette::kTextDim,
               "F4 判定表示 / F5 col・SP / F6 ユニーク解放");

    // --- フロア移動 ---------------------------------------------------------
    draw::Line(panel.left + 12.0f, panel.top + kDebugFloorTextY - 6.0f, panel.right - 12.0f,
               panel.top + kDebugFloorTextY - 6.0f, palette::kAccentWarm, 1.0f, 90);
    const int floorCount = quest_ ? quest_->FloorCount() : 1;
    draw::Text(FontSize::Tiny, panel.left + 12.0f, panel.top + kDebugFloorTextY, palette::kText,
               str::Format("フロア移動（いま %s / 全 %d フロア）", DebugFloorName(floorIndex_).c_str(),
                           floorCount));
    debugPrevFloorButton_.Draw();
    for (const ui::Button& button : debugFloorButtons_) button.Draw();
    debugNextFloorButton_.Draw();

    // --- 隠しフロア（ボタンと、抽選の様子）--------------------------------------------
    debugHiddenFloorButton_.Draw();
    {
        const Rect button = debugHiddenFloorButton_.GetRect();
        const float x = button.right + 10.0f;
        if (!quest_ || !quest_->hasHiddenFloor) {
            draw::Text(FontSize::Tiny, x, button.top + 10.0f, palette::kTextDim, "このクエストに隠しフロアはありません");
        } else {
            const char* result = !hiddenLotteryHeld_ ? "今回の抽選：なし" : (hiddenRoute_ ? "今回の抽選：当たり" : "今回の抽選：外れ");
            draw::Text(FontSize::Tiny, x, button.top + 1.0f, hiddenRoute_ ? palette::kAccentWarm : palette::kText, result);
            const HiddenFloorProgress* progress = context.player.HiddenFloor(quest_->id);
            const std::string next = (progress && progress->threshold > 0)
                ? str::Format("挑戦 %d 回目（%d 回目で抽選）", progress->attempts, progress->threshold)
                : std::string("まだ数えていません");
            draw::Text(FontSize::Tiny, x, button.top + 18.0f, palette::kTextDim, next);
        }
    }

    if (debugMessageTimer_ > 0.0f) {
        draw::Text(FontSize::Small, panel.left + 12.0f, panel.bottom + 8.0f, palette::kAccentWarm,
                   debugMessage_);
    }
}

void QuestScene::UpdateActors(float dt, GameContext& context)
{
    const Input& input = Input::Instance();
    const bool controlEnabled = (phase_ == Phase::Battle || phase_ == Phase::FloorClear
                                 || phase_ == Phase::FloorIntro);

    // アイテムカード・デバッグのボタンをクリックしたフレームは、同じクリックで攻撃を出さない
    player_.Update(dt, stage_, combat_, input,
                   controlEnabled && !menu_.IsOpen() && !hud_.ItemClickConsumed() && !debugClickConsumed_);

    for (std::unique_ptr<Enemy>& enemy : enemies_) {
        enemy->Update(dt, stage_, combat_, player_.pos, player_.alive, player_.z);
    }
    if (boss_) {
        boss_->Update(dt, stage_, combat_, player_.pos, player_.alive, player_.z);
    }
    (void)context;
}

Actor* QuestScene::FindActorById(int actorId)
{
    for (std::unique_ptr<Enemy>& enemy : enemies_) {
        if (enemy->id == actorId) return enemy.get();
    }
    if (boss_ && boss_->id == actorId) return boss_.get();
    return nullptr;
}

void QuestScene::UseQuickItem(GameContext& context)
{
    Inventory& inventory = context.player.GetInventory();
    const QuickSlot slot = hud_.ItemSlot();
    const ConsumableDef* item = ConsumableDatabase::Instance().Find(inventory.QuickItem(slot));
    if (!item) {
        hud_.ShowItemMessage(str::Format("%sアイテムが装備されていません", QuickSlotName(slot)));
        return;
    }
    if (inventory.ItemCount(item->id) <= 0) {
        hud_.ShowItemMessage(str::Format("%s がありません", item->name.c_str()));
        return;
    }

    switch (player_.UseItem(*item, combat_)) {
    case ItemUseResult::Used:
        inventory.ConsumeItem(item->id);
        break;
    case ItemUseResult::NoEffect:
        hud_.ShowItemMessage(item->mpRate > 0.0f && item->hpRate > 0.0f ? "HP と MP は満タンです"
                             : item->mpRate > 0.0f                      ? "MP は満タンです"
                                                                        : "HP は満タンです");
        break;
    case ItemUseResult::Cooldown:
    case ItemUseResult::Unusable:
    default:
        break;   // 連打や戦闘不能中は黙って無視する
    }
}

void QuestScene::HandleParrySuccess(GameContext& context)
{
    int sourceId = -1;
    if (!player_.ConsumeParrySignal(&sourceId)) return;

    ++parryCount_;
    // 神聖剣の解放条件はパリィの累計成功回数（UniqueSkill.h 参照）
    context.player.AddParrySuccess();
    hitStop_ = math::MaxF(hitStop_, kParryHitStop);
    camera_.Shake(24.0f, 0.32f);

    // 攻撃してきた相手をよろけさせて反撃の隙を作る
    if (Actor* attacker = FindActorById(sourceId)) {
        const bool isBoss = (boss_ && attacker == boss_.get());
        attacker->Stagger(isBoss ? kParryStaggerBoss : kParryStaggerEnemy);
        combat_.AddPopup(Vec2(attacker->pos.x, attacker->pos.y - attacker->height - 30.0f),
                         "体勢を崩した！", palette::kCritical, false);
    }
}

void QuestScene::ResolveHitBoxes(GameContext& context)
{
    for (HitBox& hitBox : combat_.HitBoxes()) {
        if (hitBox.team == Team::Player) {
            // プレイヤーの攻撃 → 敵
            auto hitActor = [&](Actor& target) {
                if (!target.alive || hitBox.AlreadyHit(target.id)) return;
                if (!hitBox.area.Intersects(target.Bounds())) return;
                // 奥行きが離れていると当たらない
                if (!hitBox.ignoreDepth && !target.WithinDepth(hitBox.z, hitBox.zRange)) return;

                const int damage = target.ApplyHit(hitBox, combat_);
                hitBox.MarkHit(target.id);
                if (damage <= 0) return;

                totalDamage_ += damage;
                ++combo_;
                comboTimer_ = kComboHold;
                maxCombo_ = math::MaxI(maxCombo_, combo_);
                // 二刀流なら左右どちらの武器も消耗する
                WearWeapons(context, kWeaponWearPerHit);

                hitStop_ = math::MaxF(hitStop_, hitBox.hitStop);
                camera_.Shake(hitBox.hitStop * 90.0f, 0.18f);
            };

            for (std::unique_ptr<Enemy>& enemy : enemies_) hitActor(*enemy);
            if (boss_) hitActor(*boss_);
        } else {
            // 敵の攻撃 → プレイヤー
            if (!player_.alive || hitBox.AlreadyHit(player_.id)) continue;
            if (!hitBox.area.Intersects(player_.Bounds())) continue;
            if (!hitBox.ignoreDepth && !player_.WithinDepth(hitBox.z, hitBox.zRange)) continue;

            const int damage = player_.ApplyHit(hitBox, combat_);
            hitBox.MarkHit(player_.id);

            // パリィが成立していれば被弾扱いにしない
            HandleParrySuccess(context);

            if (damage > 0) {
                damageTaken_ += damage;
                combo_ = 0;
                comboTimer_ = 0.0f;
                WearArmor(context, kArmorWearPerHit);
                // ガードで受け止めた場合は盾が余分に消耗する
                if (player_.IsGuarding()) WearEquipment(context, EquipSlot::Shield, kShieldWearPerGuard);
                hitStop_ = math::MaxF(hitStop_, 0.04f);
                camera_.Shake(14.0f, 0.22f);
            }
        }
    }
    (void)context;
}

void QuestScene::ResolveProjectiles(GameContext& context)
{
    (void)context;

    for (Projectile& projectile : combat_.Projectiles()) {
        if (!projectile.active) continue;

        // 地形との衝突
        if (projectile.pos.y >= stage_.GroundYAt(projectile.z) || projectile.pos.x < 0.0f
            || projectile.pos.x > stage_.Width()) {
            projectile.active = false;
            combat_.AddImpact(projectile.pos, projectile.color, 8, 260.0f);
            continue;
        }

        // 飛び道具から一時的な攻撃判定を作って解決する
        HitBox hitBox;
        hitBox.area = Rect::FromCenter(projectile.pos.x, projectile.pos.y,
                                       projectile.radius * 2.0f, projectile.radius * 2.0f);
        hitBox.team = projectile.team;
        hitBox.z = projectile.z;
        hitBox.zRange = config::kHitDepthRange;
        hitBox.sourceId = projectile.sourceId;
        hitBox.attack = projectile.attack;
        hitBox.damageMultiplier = projectile.damageMultiplier;
        hitBox.critRate = projectile.critRate;
        hitBox.critDamage = projectile.critDamage;
        hitBox.knockback = projectile.knockback;
        hitBox.color = projectile.color;

        if (projectile.team == Team::Enemy) {
            if (!player_.alive) continue;
            if (!hitBox.area.Intersects(player_.Bounds())) continue;

            if (!player_.WithinDepth(projectile.z, config::kHitDepthRange)) continue;

            const bool parryActive = player_.IsParryActive();
            const int damage = player_.ApplyHit(hitBox, combat_);

            if (parryActive && player_.ConsumeParrySignal()) {
                // 受け流した弾はプレイヤーの攻撃として跳ね返す
                ++parryCount_;
                hitStop_ = math::MaxF(hitStop_, kParryHitStop * 0.6f);
                camera_.Shake(16.0f, 0.24f);

                projectile.velocity = Vec2(-projectile.velocity.x * 1.4f,
                                           -projectile.velocity.y * 0.5f);
                projectile.team = Team::Player;
                projectile.sourceId = player_.id;
                projectile.attack = player_.AttackPower();
                projectile.damageMultiplier = 1.8f;
                projectile.critRate = player_.CritRate();
                projectile.critDamage = player_.stats.critDamage;
                projectile.color = palette::kCritical;
                projectile.life = math::MaxF(projectile.life, 2.0f);
                combat_.AddImpact(projectile.pos, palette::kCritical, 18, 460.0f);
                continue;
            }

            projectile.active = false;
            combat_.AddImpact(projectile.pos, projectile.color, 12, 340.0f);
            if (damage > 0) {
                damageTaken_ += damage;
                combo_ = 0;
                WearArmor(context, kArmorWearPerHit);
                camera_.Shake(10.0f, 0.18f);
            }
        } else {
            for (std::unique_ptr<Enemy>& enemy : enemies_) {
                if (!enemy->alive) continue;
                if (!hitBox.area.Intersects(enemy->Bounds())) continue;
                if (!enemy->WithinDepth(projectile.z, config::kHitDepthRange)) continue;

                const int damage = enemy->ApplyHit(hitBox, combat_);
                projectile.active = false;
                if (damage > 0) {
                    totalDamage_ += damage;
                    ++combo_;
                    comboTimer_ = kComboHold;
                    maxCombo_ = math::MaxI(maxCombo_, combo_);
                }
                break;
            }
        }
    }
}

void QuestScene::WearEquipment(GameContext& context, EquipSlot slot, float amount)
{
    Inventory& inventory = context.player.GetInventory();
    inventory.ApplyWear(slot, amount);
    HandleBrokenEquipment(context);
}

void QuestScene::WearWeapons(GameContext& context, float amount)
{
    Inventory& inventory = context.player.GetInventory();
    inventory.ApplyWeaponWear(amount);
    HandleBrokenEquipment(context);
}

void QuestScene::WearArmor(GameContext& context, float amount)
{
    Inventory& inventory = context.player.GetInventory();
    inventory.ApplyArmorWear(amount);
    HandleBrokenEquipment(context);
}

void QuestScene::HandleBrokenEquipment(GameContext& context)
{
    Inventory& inventory = context.player.GetInventory();
    const std::vector<std::string> broken = inventory.DestroyBrokenItems();
    if (broken.empty()) return;

    for (const std::string& name : broken) {
        brokenItems_.push_back(name);
        combat_.AddPopup(Vec2(player_.pos.x, player_.pos.y - player_.height - 40.0f),
                         str::Format("%s が壊れた！", name.c_str()), palette::kDanger, true);
    }

    breakNotice_ = (broken.size() == 1)
        ? str::Format("%s が壊れた！", broken[0].c_str())
        : str::Format("%d 点の装備が壊れた！", static_cast<int>(broken.size()));
    breakNoticeTimer_ = 3.0f;

    // 装備が外れた状態をプレイヤーへ即座に反映する
    context.player.RefreshSkillLoadoutForEquipment();
    player_.RefreshEquipment(context.player);

    camera_.Shake(20.0f, 0.4f);
    combat_.AddImpact(Vec2(player_.pos.x, player_.pos.y - player_.height * 0.5f),
                      palette::kDanger, 24, 480.0f);
}

void QuestScene::CleanupDead(GameContext& context)
{
    (void)context;

    for (size_t i = 0; i < enemies_.size();) {
        Enemy& enemy = *enemies_[i];

        // 撃破報酬は 1 度だけ加算する
        if (!enemy.alive && !enemy.rewardGranted) {
            enemy.rewardGranted = true;
            ++enemiesDefeated_;
            if (roguelike_) {
                // ローグライク：経験値はその場でランのキャラに入り、col などはその場に落ちる
                ++context.roguelike.kills;
                const int exp = GrantRunExp(rogue::kRunExpPerKill, context);
                combat_.AddPopup(Vec2(enemy.pos.x, enemy.pos.y - enemy.height - 20.0f),
                                 str::Format("+%d EXP", exp), palette::kExp, false);
                DropMobLoot(enemy, context);
            } else {
                expGained_ += enemy.ExpReward();
                colGained_ += enemy.ColReward();
                combat_.AddPopup(Vec2(enemy.pos.x, enemy.pos.y - enemy.height - 20.0f),
                                 str::Format("+%d EXP", enemy.ExpReward()), palette::kExp, false);
            }
        }

        if (enemy.IsRemovable()) {
            enemies_.erase(enemies_.begin() + static_cast<long>(i));
            continue;
        }
        ++i;
    }

    // ボス撃破報酬
    if (boss_ && !boss_->alive && !bossRewardGranted_) {
        bossRewardGranted_ = true;
        ++enemiesDefeated_;
        if (roguelike_) {
            ++context.roguelike.kills;
            GrantRunExp(rogue::kRunExpPerBoss, context);
            DropBossLoot(context);
        } else if (const BossDef* def = boss_->Def()) {
            expGained_ += static_cast<int>(static_cast<float>(def->expReward)
                                           * (quest_ ? quest_->enemyPowerScale : 1.0f));
            colGained_ += static_cast<int>(static_cast<float>(def->colReward)
                                           * (quest_ ? quest_->enemyPowerScale : 1.0f));
        }
    }
}

void QuestScene::UpdateFloorTransition(float dt, GameContext& context)
{
    transitionAlpha_ = math::Approach(transitionAlpha_, 1.0f, dt * 2.4f);
    if (transitionAlpha_ < 1.0f) return;

    if (pendingFloor_ <= LastFloorIndex()) {
        LoadFloor(pendingFloor_, context);
    }
    transitioning_ = false;
    transitionAlpha_ = 0.0f;
}

void QuestScene::FinishQuest(bool cleared, bool retired, GameContext& context)
{
    // ローグライクは別の締め方（ランを終えて本編のキャラに戻す）
    if (roguelike_) {
        FinishRoguelike(cleared, retired, context);
        return;
    }
    QuestResult& result = context.lastResult;
    result.Reset();

    // 最後のボスを倒していれば、隠しフロアで倒れても・リタイアしてもクリア扱い
    if (mainBossDefeated_) {
        cleared = true;
        retired = false;
    }

    result.cleared = cleared;
    result.retired = retired;
    result.questId = quest_ ? quest_->id : 0;
    result.questName = quest_ ? quest_->name : "";
    result.clearTime = questTime_;
    result.floorsCleared = cleared ? (quest_ ? quest_->FloorCount() : 0) : floorIndex_;
    result.floorCount = quest_ ? quest_->FloorCount() : 0;
    result.enemiesDefeated = enemiesDefeated_;
    result.maxCombo = maxCombo_;
    result.parryCount = parryCount_;
    result.totalDamage = totalDamage_;
    result.damageTaken = damageTaken_;

    int exp = expGained_;
    int col = colGained_;

    if (quest_) {
        if (cleared) {
            exp += quest_->expReward;
            col += quest_->colReward;
        } else {
            // 未クリア時は報酬半減
            exp = exp / 2;
            col = col / 2;
        }
    }

    // 初回クリアボーナス
    if (cleared && quest_ && !context.player.IsQuestCleared(quest_->id)) {
        result.firstClear = true;
        col += quest_->firstClearCol;
    }

    // ドロップ抽選
    if (quest_) {
        // LUK でドロップ率が上がる。初回クリアはボスドロップから必ず 1 つ落ちる。
        //   ボスドロップは最後のボスを倒したときだけ（開発者モードで隠しフロアへ直行したときは無し）
        result.drops = QuestDatabase::Instance().RollDrops(
            *quest_, mainBossDefeated_, enemiesDefeated_, context.player.DropRateMultiplier(),
            result.firstClear);
        // 素材（道中は倒した数だけ、ボスは撃破時）
        result.materials = QuestDatabase::Instance().RollMaterials(
            *quest_, mainBossDefeated_, enemiesDefeated_, context.player.DropRateMultiplier());

        // 隠しボスのドロップ（黒い剣は 1%。持ち帰れる数の上限で消えないよう先頭に置く）
        if (hiddenBossDefeated_) {
            const float rate = context.player.DropRateMultiplier();
            const std::vector<EquipmentItem> hidden = QuestDatabase::Instance().RollHiddenBossDrops(*quest_, rate);
            result.drops.insert(result.drops.begin(), hidden.begin(), hidden.end());
            if (result.drops.size() > 12) result.drops.resize(12);
            for (const ItemStack& stack : QuestDatabase::Instance().RollHiddenBossMaterials(*quest_, rate)) {
                bool merged = false;
                for (ItemStack& have : result.materials) {
                    if (have.itemId == stack.itemId) {
                        have.count += stack.count;
                        merged = true;
                    }
                }
                if (!merged) result.materials.push_back(stack);
            }
        }
    }
    result.hiddenFloorReached = hiddenFloorReached_;
    result.hiddenBossDefeated = hiddenBossDefeated_;

    // 隠しフロアの抽選をした回なら、ここで数え直す（当たっても外れても）
    if (quest_ && quest_->hasHiddenFloor) context.player.EndHiddenFloorAttempt(quest_->id);

    // LUK で獲得経験値が増える（撃破ぶん＋クリア報酬の合計に掛ける。未クリアの半減のあと）
    result.expRate = context.player.ExpRateMultiplier();
    exp = static_cast<int>(std::lround(static_cast<double>(exp) * static_cast<double>(result.expRate)));

    result.expGained = exp;
    result.colGained = col;
    result.materialGained = enemiesDefeated_ / 2 + (cleared ? 6 : 1);

    // クエスト中に壊れた装備（消耗と破棄は戦闘中に処理済み）
    result.brokenItems = brokenItems_;

    // --- プレイヤーへ反映 -----------------------------------------------------
    Inventory& inventory = context.player.GetInventory();
    inventory.AddItems(result.drops);
    for (const ItemStack& stack : result.materials) {
        inventory.AddItemCount(stack.itemId, stack.count);
    }
    inventory.AddCol(col);
    inventory.AddMaterial(result.materialGained);
    const int spBefore = context.player.SkillPoints();
    const int apBefore = context.player.AbilityPoints();
    result.levelsGained = context.player.AddExp(exp);

    // 初回クリアでスキルポイントを追加で付与
    if (cleared && quest_ && result.firstClear) {
        context.player.AddSkillPoints(2);
    }
    result.skillPointsGained = context.player.SkillPoints() - spBefore;
    result.abilityPointsGained = context.player.AbilityPoints() - apBefore;

    if (cleared && quest_) context.player.MarkQuestCleared(quest_->id);

    // --- ユニークスキルの解放条件判定 -----------------------------------------
    //   条件の値は UniqueSkill.h にまとめてあります。
    //   既に別のユニークスキルの条件を満たしている場合は解放されない。
    //   その時はリザルトにも何も出さない（解放できたものだけを知らせる）。
    if (cleared && quest_ && quest_->id == kDualWieldQuestId
        && questTime_ <= kDualWieldClearTimeLimit
        && context.player.MakeUniqueSkillAvailable(UniqueSkillType::DualWield)) {
        result.unlockedUniqueSkill = true;
        result.unlockedUniqueSkillName = UniqueSkillName(UniqueSkillType::DualWield);
        result.unlockedUniqueSkillType = UniqueSkillType::DualWield;
    }
    // 神聖剣 : パリィの累計成功回数（クリアしなくても判定する）
    else if (context.player.TryUnlockByParry()) {
        result.unlockedUniqueSkill = true;
        result.unlockedUniqueSkillName = UniqueSkillName(UniqueSkillType::HolySword);
        result.unlockedUniqueSkillType = UniqueSkillType::HolySword;
    }
}

//==============================================================================
// 描画
//==============================================================================
void QuestScene::Draw(GameContext& context)
{
    DrawWorld(context);

    // --- HUD ---------------------------------------------------------------
    ui::HudInfo info;
    info.floorName = quest_ ? FloorAt(floorIndex_).name : "";
    info.floorIndex = floorIndex_ + 1;
    info.floorCount = quest_ ? quest_->FloorCount() : 1;
    info.hiddenFloor = IsHiddenFloor(floorIndex_);
    info.enemiesRemaining = AliveEnemyCount();
    info.questTime = questTime_;
    info.combo = combo_;
    info.comboTimer = comboTimer_;
    info.showFps = context.settings.showFps;

    // ボスの登場演出の間は HUD を隠して映画のように見せる
    if (phase_ == Phase::BossIntro) {
        DrawBossIntro();
    } else {
        hud_.Draw(player_, context.player, boss_.get(), info);
    }

    if (roguelike_ && phase_ != Phase::BossIntro) DrawRunInfo(context);

    DrawDebugOverlay(context);

    if (breakNoticeTimer_ > 0.0f) DrawBreakNotice();
    if (bannerTimer_ > 0.0f) DrawBanner();
    if (phase_ == Phase::Defeat) DrawDefeatOverlay();
    if (phase_ == Phase::Victory) DrawVictoryOverlay();

    // ローグライク：施設とスキル選択の画面
    if (roguelike_) {
        draft_.Draw(context.player);
        shop_.Draw(context.roguelike, context.player);
        smith_.Draw(context);
    }

    menu_.Draw(context);

    // フロア間の暗転
    if (transitioning_) {
        draw::FillRect(Rect(0.0f, 0.0f, kScreenW, kScreenH), palette::kBlack,
                       static_cast<int>(transitionAlpha_ * 255.0f));
        draw::TextAlpha(FontSize::Medium, kScreenW * 0.5f, kScreenH * 0.5f, palette::kText,
                        transitionText_.empty() ? std::string("次のフロアへ...") : transitionText_,
                        static_cast<int>(transitionAlpha_ * 255.0f), draw::TextAlign::Center);
    }
}

void QuestScene::DrawWorld(const GameContext& context)
{
    stage_.DrawBackground(camera_);
    if (roguelike_) DrawFacilities();
    combat_.DrawBehindActors(camera_);

    // 奥にいるものから順に描く（手前が上に重なる）
    struct Drawable
    {
        float z;
        const Enemy* enemy;
        const Boss* boss;
        const Player* player;
    };
    std::vector<Drawable> drawables;
    drawables.reserve(enemies_.size() + 2);
    for (const std::unique_ptr<Enemy>& enemy : enemies_) {
        drawables.push_back({ enemy->z, enemy.get(), nullptr, nullptr });
    }
    if (boss_) drawables.push_back({ boss_->z, nullptr, boss_.get(), nullptr });
    drawables.push_back({ player_.z, nullptr, nullptr, &player_ });

    std::sort(drawables.begin(), drawables.end(),
              [](const Drawable& a, const Drawable& b) { return a.z > b.z; });

    for (const Drawable& item : drawables) {
        if (item.enemy) item.enemy->Draw(camera_);
        else if (item.boss) item.boss->Draw(camera_);
        else if (item.player) item.player->Draw(camera_);
    }
    if (roguelike_) pickups_.Draw(camera_, worldTime_);

    stage_.DrawForeground(camera_);
    combat_.DrawFrontOfActors(camera_, context.settings.showDamageNumbers);

    // 画面外の敵を示すマーカー
    for (const std::unique_ptr<Enemy>& enemy : enemies_) {
        if (!enemy->alive) continue;
        const float screenX = enemy->pos.x - camera_.ViewLeft();
        if (screenX >= -40.0f && screenX <= kScreenW + 40.0f) continue;

        const float x = (screenX < 0.0f) ? 40.0f : kScreenW - 40.0f;
        const float y = stage_.GroundY() - camera_.ViewTop() - 120.0f;
        const float dir = (screenX < 0.0f) ? -1.0f : 1.0f;
        draw::Triangle(Vec2(x + dir * 18.0f, y), Vec2(x - dir * 12.0f, y - 16.0f),
                       Vec2(x - dir * 12.0f, y + 16.0f), palette::kDanger, true, 180);
    }
}

void QuestScene::DrawBreakNotice() const
{
    const float t = math::Clamp(breakNoticeTimer_ / 0.6f, 0.0f, 1.0f);
    const int alpha = static_cast<int>(t * 255.0f);
    const float y = 460.0f;

    const Rect band(kScreenW * 0.5f - 420.0f, y - 12.0f, kScreenW * 0.5f + 420.0f, y + 78.0f);
    draw::FillRect(band, palette::kDanger.Scaled(0.35f), static_cast<int>(t * 200.0f));
    draw::StrokeRect(band, palette::kDanger, 2.0f, alpha);

    draw::TextAlpha(FontSize::Large, kScreenW * 0.5f, y, palette::kText, breakNotice_, alpha,
                    draw::TextAlign::Center);
    draw::TextAlpha(FontSize::Small, kScreenW * 0.5f, y + 48.0f, palette::kTextDim,
                    "ESC → 装備 から替えの装備に変更できます", alpha, draw::TextAlign::Center);
}

void QuestScene::DrawBanner() const
{
    const float t = math::Clamp(bannerTimer_, 0.0f, 1.0f);
    const int alpha = static_cast<int>(t * 255.0f);
    const float y = 300.0f;

    const Rect band(0.0f, y - 20.0f, kScreenW, y + 130.0f);
    draw::FillRect(band, palette::kBlack, static_cast<int>(t * 150.0f));
    draw::Line(band.left, band.top, band.right, band.top, palette::kAccent, 2.0f, alpha);
    draw::Line(band.left, band.bottom, band.right, band.bottom, palette::kAccent, 2.0f, alpha);

    draw::TextAlpha(FontSize::Huge, kScreenW * 0.5f, y + 6.0f, palette::kText, bannerMain_, alpha,
                    draw::TextAlign::Center);
    draw::TextAlpha(FontSize::Normal, kScreenW * 0.5f, y + 84.0f, palette::kAccent, bannerSub_, alpha,
                    draw::TextAlign::Center);
}

void QuestScene::DrawDefeatOverlay() const
{
    const float t = math::Clamp(phaseTimer_ / 1.2f, 0.0f, 1.0f);
    ui::DrawDimOverlay(static_cast<int>(t * 190.0f));
    draw::TextAlpha(FontSize::Title, kScreenW * 0.5f, kScreenH * 0.5f - 80.0f, palette::kDanger,
                    roguelike_ ? "RUN OVER" : "DEFEAT", static_cast<int>(t * 255.0f), draw::TextAlign::Center);
    draw::TextAlpha(FontSize::Normal, kScreenW * 0.5f, kScreenH * 0.5f + 30.0f, palette::kTextDim,
                    roguelike_ ? "戦闘不能になった… ランの成果はすべて失われる" : "戦闘不能になった…",
                    static_cast<int>(t * 255.0f), draw::TextAlign::Center);
}

void QuestScene::DrawVictoryOverlay() const
{
    const float t = math::Clamp(phaseTimer_ / 1.0f, 0.0f, 1.0f);
    ui::DrawDimOverlay(static_cast<int>(t * 120.0f));
    draw::TextAlpha(FontSize::Title, kScreenW * 0.5f, kScreenH * 0.5f - 80.0f, palette::kAccentWarm,
                    roguelike_ ? "RUN CLEAR" : "QUEST CLEAR", static_cast<int>(t * 255.0f), draw::TextAlign::Center);
}

//==============================================================================
// ローグライクモード
//==============================================================================
float QuestScene::FloorPowerScale(const FloorDef& floor) const
{
    if (floor.enemyPowerScale > 0.0f) return floor.enemyPowerScale;
    return quest_ ? quest_->enemyPowerScale : 1.0f;
}

bool QuestScene::RoguelikePanelOpen() const
{
    return draft_.IsOpen() || shop_.IsOpen() || smith_.IsOpen();
}

void QuestScene::UpdateRoguelikePanels(float dt, const Input& input, GameContext& context)
{
    if (draft_.IsOpen()) {
        draft_.Update(dt, input, context.player);
        if (!draft_.IsOpen()) {
            if (draft_.Changed()) player_.RefreshEquipment(context.player);
            if (!draft_.Message().empty()) ShowRunNotice(draft_.Message(), draft_.Changed());
            // 続けて拾っていたら次の選択へ
            if (pendingDrafts_ > 0) OpenNextDraft(context);
        }
        return;
    }
    if (shop_.IsOpen()) {
        shop_.Update(dt, input, context.roguelike, context.player);
        return;
    }
    if (smith_.IsOpen()) {
        smith_.Update(dt, input, context);
        // 強化・修理した装備をその場で能力に反映する
        player_.RefreshEquipment(context.player);
    }
}

int QuestScene::FacilityNear() const
{
    if (!quest_) return -1;
    const FloorDef& floor = FloorAt(floorIndex_);
    for (size_t i = 0; i < floor.facilities.size(); ++i) {
        if (math::Abs(player_.pos.x - floor.facilities[i].x) <= rogue::kFacilityRange) return static_cast<int>(i);
    }
    return -1;
}

bool QuestScene::EnemyNear(float x, float range) const
{
    for (const std::unique_ptr<Enemy>& enemy : enemies_) {
        if (enemy->alive && math::Abs(enemy->pos.x - x) < range) return true;
    }
    return BossAlive() && math::Abs(boss_->pos.x - x) < range;
}

void QuestScene::UpdateFacilities(const Input& input, GameContext& context)
{
    nearFacility_ = -1;
    if (transitioning_ || !quest_ || !player_.alive) return;
    if (!(phase_ == Phase::Battle || phase_ == Phase::FloorClear || phase_ == Phase::FloorIntro)) return;
    nearFacility_ = FacilityNear();
    if (nearFacility_ < 0 || !input.Pressed(GameAction::Confirm)) return;

    const FacilitySpot& spot = FloorAt(floorIndex_).facilities[static_cast<size_t>(nearFacility_)];
    // 敵がそばにいる間は入れない（買い物で戦闘を止めて逃げないように）
    if (EnemyNear(spot.x, rogue::kFacilityEnemyRange)) {
        ShowRunNotice("近くに敵がいる！ 先に倒そう", false);
        return;
    }
    if (spot.kind == FacilityKind::Shop) {
        rogue::ShopAt(context.roguelike, floorIndex_, nearFacility_);   // 初めて開くときに品ぞろえを決める
        shop_.Open(floorIndex_, nearFacility_);
    } else {
        smith_.Open();
    }
}

void QuestScene::UpdatePickups(float dt, GameContext& context)
{
    if (!player_.alive) return;
    std::vector<Pickup> collected;
    // 敵を全滅させたら（ボスを倒したら）、残っている col・アイテム・強化結晶はプレイヤーへ集まる
    const bool gather = (phase_ == Phase::FloorClear || phase_ == Phase::Victory);
    pickups_.Update(dt, stage_, player_.pos, player_.z, gather, collected);
    if (!collected.empty()) HandlePickups(collected, context);
}

void QuestScene::HandlePickups(const std::vector<Pickup>& collected, GameContext& context)
{
    Inventory& inventory = context.player.GetInventory();
    rogue::RunState& run = context.roguelike;
    const Vec2 head(player_.pos.x, player_.pos.y - player_.height - 34.0f);
    int col = 0;
    int crystals = 0;
    for (const Pickup& p : collected) {
        switch (p.kind) {
        case PickupKind::Col:
            inventory.AddCol(p.amount);
            run.colEarned += p.amount;
            col += p.amount;
            break;
        case PickupKind::Item: {
            const ConsumableDef* def = ConsumableDatabase::Instance().Find(p.itemId);
            if (inventory.AddItemCount(p.itemId, p.amount) > 0) {
                rogue::AutoAssignQuickSlots(inventory);
                combat_.AddPopup(Vec2(head.x, head.y - 30.0f), def ? def->name : "アイテム", palette::kHp, false);
            }
            break;
        }
        case PickupKind::Crystal:
            inventory.AddMaterial(p.amount);
            crystals += p.amount;
            break;
        case PickupKind::Gear:
            inventory.AddItem(p.gear);
            combat_.AddPopup(Vec2(head.x, head.y - 60.0f), p.gear.DisplayName(), palette::kAccentWarm, true);
            ShowRunNotice(str::Format("装備を拾った：%s（ESC のメニュー →「装備」で付け替え）",
                                      p.gear.DisplayName().c_str()));
            break;
        case PickupKind::Skill:
            ++pendingDrafts_;
            break;
        }
    }
    if (col > 0) combat_.AddPopup(head, str::Format("+%d col", col), palette::kAccentWarm, false);
    if (crystals > 0) {
        combat_.AddPopup(Vec2(head.x, head.y + 26.0f), str::Format("強化結晶 +%d", crystals), ColorRGB(120, 210, 255),
                         false);
    }
    if (pendingDrafts_ > 0 && !draft_.IsOpen()) OpenNextDraft(context);
}

void QuestScene::OpenNextDraft(GameContext& context)
{
    if (pendingDrafts_ <= 0) return;
    --pendingDrafts_;
    const std::vector<int> choices = rogue::DraftSkills(context.player, context.roguelike.weapon,
                                                        rogue::kSkillChoices, context.roguelike.rng);
    if (choices.empty()) {
        // どのスキルも Lv 5 になったら、代わりに col を渡す
        context.player.GetInventory().AddCol(100);
        ShowRunNotice("覚えられるスキルがもう無い（代わりに 100 col）");
        return;
    }
    draft_.Open(choices);
}

int QuestScene::GrantRunExp(float levelShare, GameContext& context)
{
    // 1 レベルぶんを超えるときは、超えたぶんを次のレベルの必要経験値で数える
    int exp = 0;
    int levels = 0;
    float share = levelShare;
    while (share > 0.0f) {
        const float part = math::MinF(share, 1.0f);
        const int amount = math::MaxI(1, static_cast<int>(std::lround(context.player.ExpToNext() * part)));
        exp += amount;
        levels += context.player.AddExp(amount);
        share -= part;
    }
    context.roguelike.expEarned += exp;
    if (levels <= 0) return exp;
    // ステータスは自動で振り、上がった能力をその場で反映して少し回復する
    rogue::AutoAllocate(context.player);
    player_.RefreshEquipment(context.player);
    player_.Recover(0.3f, 0.3f);
    combat_.AddPopup(Vec2(player_.pos.x, player_.pos.y - player_.height - 90.0f),
                     str::Format("LEVEL UP!  Lv %d", context.player.Level()), palette::kExp, true);
    return exp;
}

void QuestScene::DropMobLoot(const Enemy& enemy, GameContext& context)
{
    const rogue::DropRoll drop = rogue::RollMobDrop(context.roguelike, floorIndex_);
    const Vec2 at(enemy.pos.x, enemy.pos.y);
    if (drop.col > 0) pickups_.Add(PickupKind::Col, at, enemy.z, drop.col);
    for (const ItemStack& item : drop.items) pickups_.Add(PickupKind::Item, at, enemy.z, item.count, item.itemId);
    if (drop.crystals > 0) pickups_.Add(PickupKind::Crystal, at, enemy.z, drop.crystals);
    if (drop.skill) pickups_.Add(PickupKind::Skill, at, enemy.z);
}

void QuestScene::DropBossLoot(GameContext& context)
{
    if (!boss_) return;
    rogue::RunState& run = context.roguelike;
    const rogue::DropRoll drop = rogue::RollBossDrop(run, floorIndex_, context.player.DropRateMultiplier());
    const Vec2 at(boss_->pos.x, stage_.GroundYAt(boss_->z));
    const float z = boss_->z;
    // col はいくつかに分けてばらまく
    const int coins = 5;
    for (int i = 0; i < coins; ++i) {
        const int amount = drop.col / coins + (i == 0 ? drop.col % coins : 0);
        if (amount > 0) pickups_.Add(PickupKind::Col, at, z, amount);
    }
    for (const ItemStack& item : drop.items) pickups_.Add(PickupKind::Item, at, z, item.count, item.itemId);
    if (drop.crystals > 0) pickups_.Add(PickupKind::Crystal, at, z, drop.crystals);
    const bool last = rogue::IsLastFloor(floorIndex_);
    if (drop.skill && !last) pickups_.Add(PickupKind::Skill, at, z);
    for (const EquipmentItem& item : drop.gear) pickups_.Add(PickupKind::Gear, at, z, 1, 0, item);
    // ラスボスの迷宮の装備は、拾わなくても手に入る（クリアしたら本編へ持ち帰る）
    for (const EquipmentItem& item : drop.carryHome) {
        run.carryHome.push_back(item);
        combat_.AddPopup(Vec2(at.x, at.y - boss_->height - 60.0f), item.DisplayName(), ColorRGB(206, 160, 255), true);
        ShowRunNotice(str::Format("ラスボスが「%s」を落とした！ 本編へ持ち帰れる", item.DisplayName().c_str()));
    }
}

void QuestScene::ShowRunNotice(const std::string& text, bool good)
{
    runNotice_ = text;
    runNoticeGood_ = good;
    runNoticeTimer_ = 3.2f;
}

void QuestScene::DrawFacilities() const
{
    if (!quest_) return;
    const FloorDef& floor = FloorAt(floorIndex_);
    const float baseY = stage_.GroundYAt(stage_.Depth()) + 4.0f;
    for (size_t i = 0; i < floor.facilities.size(); ++i) {
        town::DrawStall(camera_, floor.facilities[i].kind, floor.facilities[i].x, baseY, worldTime_,
                        static_cast<int>(i) == nearFacility_);
    }
}

void QuestScene::DrawRunInfo(const GameContext& context) const
{
    const PlayerData& data = context.player;
    const rogue::RunState& run = context.roguelike;
    const ColorRGB violet(186, 140, 255);

    // 右上：ランの様子（レベル・col・強化結晶・撃破数・スキル）
    const Rect panel = Rect::FromXYWH(kScreenW - 452.0f, 24.0f, 424.0f, 128.0f);
    draw::ChamferRect(panel, 10.0f, palette::kPanelDark, 210);
    draw::StrokeRect(panel, violet.Scaled(0.8f), 1.0f, 200);
    draw::Text(FontSize::Small, panel.left + 16.0f, panel.top + 10.0f, violet, "LABYRINTH");
    draw::Text(FontSize::Small, panel.right - 16.0f, panel.top + 10.0f, palette::kText,
               str::Format("Lv %d", data.Level()), draw::TextAlign::Right);
    const Rect expBar = Rect::FromXYWH(panel.left + 16.0f, panel.top + 42.0f, panel.Width() - 32.0f, 8.0f);
    const float expRatio = (data.ExpToNext() > 0) ? static_cast<float>(data.Exp()) / static_cast<float>(data.ExpToNext()) : 0.0f;
    draw::Bar(expBar, expRatio, palette::kExp, palette::kPanel);
    draw::Text(FontSize::Small, panel.left + 16.0f, panel.top + 60.0f, palette::kAccentWarm,
               str::Format("%s col", str::Comma(data.GetInventory().Col()).c_str()));
    draw::Text(FontSize::Small, panel.left + 190.0f, panel.top + 60.0f, ColorRGB(120, 210, 255),
               str::Format("強化結晶 %d", data.GetInventory().Material()));
    draw::Text(FontSize::Small, panel.right - 16.0f, panel.top + 60.0f, palette::kTextDim,
               str::Format("撃破 %d", run.kills), draw::TextAlign::Right);
    draw::Text(FontSize::Tiny, panel.left + 16.0f, panel.top + 96.0f, palette::kTextDim,
               str::Format("スキル %d / %d ／ 持ち帰り %d 個", rogue::OwnedSkillCount(data), data.SkillSlotLimit(),
                           static_cast<int>(run.carryHome.size())));

    // ショップ・鍛冶屋の入口の案内
    //   フロア名などのバナーが出ている間は重ならないように出さない
    if (nearFacility_ >= 0 && quest_ && !RoguelikePanelOpen() && !menu_.IsOpen() && bannerTimer_ <= 0.0f) {
        const FacilitySpot& spot = FloorAt(floorIndex_).facilities[static_cast<size_t>(nearFacility_)];
        const bool shop = spot.kind == FacilityKind::Shop;
        town::DrawPromptAt(camera_, spot.x, stage_.GroundYAt(stage_.Depth()) + 4.0f,
                           context.settings.keys.PrimaryName(GameAction::Confirm), shop ? "ショップ" : "鍛冶屋",
                           shop ? "行商人：アイテム・装備・強化結晶を売っている" : "旅の鍛冶屋：装備の強化・修理・売却",
                           worldTime_);
    }

    // ひとこと（スキルを覚えた・装備を拾った・敵が近い など）
    if (runNoticeTimer_ > 0.0f) {
        const int alpha = static_cast<int>(math::Clamp(runNoticeTimer_ / 0.5f, 0.0f, 1.0f) * 255.0f);
        const float w = static_cast<float>(draw::TextWidth(FontSize::Normal, runNotice_)) + 60.0f;
        const Rect box = Rect::FromCenter(kScreenW * 0.5f, kScreenH - 290.0f, w, 52.0f);
        draw::ChamferRect(box, 10.0f, palette::kPanelDark, alpha * 220 / 255);
        draw::StrokeRect(box, runNoticeGood_ ? violet : palette::kDanger, 1.0f, alpha);
        draw::TextAlpha(FontSize::Normal, box.CenterX(), box.top + 12.0f,
                        runNoticeGood_ ? palette::kText : palette::kDanger, runNotice_, alpha, draw::TextAlign::Center);
    }
}

void QuestScene::FinishRoguelike(bool cleared, bool retired, GameContext& context)
{
    QuestResult& result = context.lastResult;
    result.Reset();
    rogue::RunState& run = context.roguelike;

    result.roguelike = true;
    result.cleared = cleared;
    result.retired = retired && !cleared;
    result.questId = rogue::kQuestId;
    result.questName = "ローグライク ― 迷宮";
    result.clearTime = questTime_;
    result.floorCount = quest_ ? quest_->FloorCount() : rogue::kFloorCount;
    result.floorsCleared = cleared ? result.floorCount : floorIndex_;
    result.enemiesDefeated = enemiesDefeated_;
    result.maxCombo = maxCombo_;
    result.parryCount = parryCount_;
    result.totalDamage = totalDamage_;
    result.damageTaken = damageTaken_;
    result.runLevel = context.player.Level();
    result.runColEarned = run.colEarned;
    result.expGained = run.expEarned;
    for (int i = 0; i < context.player.SkillSlotLimit(); ++i) {
        if (const SwordSkill* skill = context.player.SkillAt(i)) {
            result.runSkills.push_back(str::Format("%s Lv %d", skill->name.c_str(), context.player.SkillLevel(skill->id)));
        }
    }
    // 本編のキャラに戻す（ランの持ち物は捨て、クリアしたらラスボスの戦利品だけ持ち帰る）
    result.drops = rogue::EndRun(context, cleared, cleared ? result.floorCount : floorIndex_ + 1);
}

} // namespace ecl
