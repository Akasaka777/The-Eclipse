#include "Scenes/QuestScene.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/DxInclude.h"
#include "Core/GameConfig.h"
#include "Core/Input.h"
#include "Core/SceneManager.h"
#include "Game/EnemyDatabase.h"
#include "Game/GameContext.h"
#include "Graphics/DrawUtil.h"
#include "UI/UIWidgets.h"

#include <cmath>

namespace ecl {

namespace {

constexpr float kScreenW = static_cast<float>(config::kScreenWidth);
constexpr float kScreenH = static_cast<float>(config::kScreenHeight);
constexpr float kComboHold = 2.4f;

} // namespace

QuestScene::QuestScene() = default;

void QuestScene::OnEnter(GameContext& context)
{
    quest_ = QuestDatabase::Instance().Find(context.selectedQuestId);
    if (!quest_) quest_ = QuestDatabase::Instance().Find(1);

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
    comboTimer_ = 0.0f;
    questTime_ = 0.0f;
    hitStop_ = 0.0f;
    transitioning_ = false;
    transitionAlpha_ = 0.0f;
    bossRewardGranted_ = false;

    camera_.SetShakeEnabled(context.settings.screenShake);
    LoadFloor(0, context);
}

void QuestScene::LoadFloor(int index, GameContext& context)
{
    if (!quest_) return;

    floorIndex_ = math::ClampInt(index, 0, quest_->FloorCount() - 1);
    const FloorDef& floor = quest_->floors[static_cast<size_t>(floorIndex_)];

    stage_.Load(floor);
    enemies_.clear();
    boss_.reset();
    combat_.Clear();

    player_.PlaceAt(Vec2(220.0f, stage_.GroundY()));

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
            boss_->Setup(*def, Vec2(stage_.Width() - 600.0f, stage_.GroundY()),
                         quest_->enemyPowerScale);
            bannerMain_ = def->name;
            bannerSub_ = def->title;
            bannerTimer_ = 2.6f;
        }
    } else {
        bannerMain_ = floor.name;
        bannerSub_ = quest_->name;
        bannerTimer_ = 2.0f;
    }

    phase_ = Phase::FloorIntro;
    phaseTimer_ = 0.0f;
}

void QuestScene::SpawnFloorEnemies(const GameContext& context)
{
    (void)context;
    if (!quest_) return;

    const FloorDef& floor = quest_->floors[static_cast<size_t>(floorIndex_)];
    const EnemyDatabase& database = EnemyDatabase::Instance();

    for (const EnemySpawn& spawn : floor.spawns) {
        const EnemyDef* def = database.Find(spawn.enemyId);
        if (!def) continue;

        const float y = (spawn.y > 0.0f) ? spawn.y : stage_.GroundY();
        std::unique_ptr<Enemy> enemy(new Enemy());
        enemy->Setup(*def, Vec2(spawn.x, y), quest_->enemyPowerScale);
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
        if (menu_.RetireConfirmed()) {
            FinishQuest(false, true, context);
            manager.RequestChange(SceneId::Result);
            phase_ = Phase::Finished;
        }
        return;
    }

    hud_.Update(dt, player_, boss_.get(), input);
    if (hud_.MenuClicked() || (input.Pressed(GameAction::Menu) && phase_ != Phase::Finished)) {
        menu_.Open();
        return;
    }

    // スキルアイコンのクリックで発動
    const int clickedSkill = hud_.ClickedSkillIndex();
    if (clickedSkill >= 0 && phase_ == Phase::Battle) {
        player_.UseSkill(clickedSkill, combat_);
    }

    // --- フロア遷移中 --------------------------------------------------------
    if (transitioning_) {
        UpdateFloorTransition(dt, context);
        camera_.Update(dt);
        return;
    }

    phaseTimer_ += dt;
    bannerTimer_ = math::MaxF(0.0f, bannerTimer_ - dt);

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

    case Phase::Battle:
        if (!player_.alive) {
            phase_ = Phase::Defeat;
            phaseTimer_ = 0.0f;
            camera_.Shake(18.0f, 0.5f);
        } else if (boss_ && !boss_->alive) {
            phase_ = Phase::Victory;
            phaseTimer_ = 0.0f;
            combat_.AddPopup(Vec2(player_.pos.x, player_.pos.y - 240.0f), "BOSS DEFEATED",
                             palette::kAccentWarm, true);
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
    }

    // --- カメラ -------------------------------------------------------------
    float targetX = player_.pos.x + static_cast<float>(player_.facing) * 140.0f;
    if (BossAlive() && math::Abs(boss_->pos.x - player_.pos.x) < 900.0f) {
        // ボス戦では両者が収まるように中間を狙う
        targetX = (player_.pos.x * 0.65f + boss_->pos.x * 0.35f);
    }
    camera_.SetTarget(Vec2(targetX, kScreenH * 0.5f));
    camera_.Update(dt);
}

void QuestScene::UpdateActors(float dt, GameContext& context)
{
    const Input& input = Input::Instance();
    const bool controlEnabled = (phase_ == Phase::Battle || phase_ == Phase::FloorClear
                                 || phase_ == Phase::FloorIntro);

    player_.Update(dt, stage_, combat_, input, controlEnabled && !menu_.IsOpen());

    for (std::unique_ptr<Enemy>& enemy : enemies_) {
        enemy->Update(dt, stage_, combat_, player_.pos, player_.alive);
    }
    if (boss_) {
        boss_->Update(dt, stage_, combat_, player_.pos, player_.alive);
    }
    (void)context;
}

void QuestScene::ResolveHitBoxes(GameContext& context)
{
    for (HitBox& hitBox : combat_.HitBoxes()) {
        if (hitBox.team == Team::Player) {
            // プレイヤーの攻撃 → 敵
            auto hitActor = [&](Actor& target) {
                if (!target.alive || hitBox.AlreadyHit(target.id)) return;
                if (!hitBox.area.Intersects(target.Bounds())) return;

                const int damage = target.ApplyHit(hitBox, combat_);
                hitBox.MarkHit(target.id);
                if (damage <= 0) return;

                totalDamage_ += damage;
                ++combo_;
                comboTimer_ = kComboHold;
                maxCombo_ = math::MaxI(maxCombo_, combo_);

                hitStop_ = math::MaxF(hitStop_, hitBox.hitStop);
                camera_.Shake(hitBox.hitStop * 90.0f, 0.18f);
            };

            for (std::unique_ptr<Enemy>& enemy : enemies_) hitActor(*enemy);
            if (boss_) hitActor(*boss_);
        } else {
            // 敵の攻撃 → プレイヤー
            if (!player_.alive || hitBox.AlreadyHit(player_.id)) continue;
            if (!hitBox.area.Intersects(player_.Bounds())) continue;

            const int damage = player_.ApplyHit(hitBox, combat_);
            hitBox.MarkHit(player_.id);
            if (damage > 0) {
                damageTaken_ += damage;
                combo_ = 0;
                comboTimer_ = 0.0f;
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
        if (projectile.pos.y >= stage_.GroundY() || projectile.pos.x < 0.0f
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

            const int damage = player_.ApplyHit(hitBox, combat_);
            projectile.active = false;
            combat_.AddImpact(projectile.pos, projectile.color, 12, 340.0f);
            if (damage > 0) {
                damageTaken_ += damage;
                combo_ = 0;
                camera_.Shake(10.0f, 0.18f);
            }
        } else {
            for (std::unique_ptr<Enemy>& enemy : enemies_) {
                if (!enemy->alive) continue;
                if (!hitBox.area.Intersects(enemy->Bounds())) continue;

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

void QuestScene::CleanupDead(GameContext& context)
{
    (void)context;

    for (size_t i = 0; i < enemies_.size();) {
        Enemy& enemy = *enemies_[i];

        // 撃破報酬は 1 度だけ加算する
        if (!enemy.alive && !enemy.rewardGranted) {
            enemy.rewardGranted = true;
            ++enemiesDefeated_;
            expGained_ += enemy.ExpReward();
            colGained_ += enemy.ColReward();
            combat_.AddPopup(Vec2(enemy.pos.x, enemy.pos.y - enemy.height - 20.0f),
                             str::Format("+%d EXP", enemy.ExpReward()), palette::kExp, false);
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
        if (const BossDef* def = boss_->Def()) {
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

    if (pendingFloor_ < quest_->FloorCount()) {
        LoadFloor(pendingFloor_, context);
    }
    transitioning_ = false;
    transitionAlpha_ = 0.0f;
}

void QuestScene::FinishQuest(bool cleared, bool retired, GameContext& context)
{
    QuestResult& result = context.lastResult;
    result.Reset();

    result.cleared = cleared;
    result.retired = retired;
    result.questId = quest_ ? quest_->id : 0;
    result.questName = quest_ ? quest_->name : "";
    result.clearTime = questTime_;
    result.floorsCleared = cleared ? (quest_ ? quest_->FloorCount() : 0) : floorIndex_;
    result.floorCount = quest_ ? quest_->FloorCount() : 0;
    result.enemiesDefeated = enemiesDefeated_;
    result.maxCombo = maxCombo_;
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
        result.drops = QuestDatabase::Instance().RollDrops(*quest_, cleared, enemiesDefeated_);
    }

    result.expGained = exp;
    result.colGained = col;
    result.materialGained = enemiesDefeated_ / 2 + (cleared ? 6 : 1);

    // --- プレイヤーへ反映 -----------------------------------------------------
    Inventory& inventory = context.player.GetInventory();
    inventory.AddItems(result.drops);
    inventory.AddCol(col);
    inventory.AddMaterial(result.materialGained);
    result.levelsGained = context.player.AddExp(exp);

    if (cleared && quest_) context.player.MarkQuestCleared(quest_->id);
}

//==============================================================================
// 描画
//==============================================================================
void QuestScene::Draw(GameContext& context)
{
    DrawWorld(context);

    // --- HUD ---------------------------------------------------------------
    ui::HudInfo info;
    info.floorName = quest_ ? quest_->floors[static_cast<size_t>(floorIndex_)].name : "";
    info.floorIndex = floorIndex_ + 1;
    info.floorCount = quest_ ? quest_->FloorCount() : 1;
    info.enemiesRemaining = AliveEnemyCount();
    info.questTime = questTime_;
    info.combo = combo_;
    info.comboTimer = comboTimer_;
    info.showFps = context.settings.showFps;

    hud_.Draw(player_, context.player, boss_.get(), info);

    if (bannerTimer_ > 0.0f) DrawBanner();
    if (phase_ == Phase::Defeat) DrawDefeatOverlay();
    if (phase_ == Phase::Victory) DrawVictoryOverlay();

    menu_.Draw(context);

    // フロア間の暗転
    if (transitioning_) {
        draw::FillRect(Rect(0.0f, 0.0f, kScreenW, kScreenH), palette::kBlack,
                       static_cast<int>(transitionAlpha_ * 255.0f));
        draw::TextAlpha(FontSize::Medium, kScreenW * 0.5f, kScreenH * 0.5f, palette::kText,
                        "次のフロアへ...", static_cast<int>(transitionAlpha_ * 255.0f),
                        draw::TextAlign::Center);
    }
}

void QuestScene::DrawWorld(const GameContext& context)
{
    stage_.DrawBackground(camera_);
    combat_.DrawBehindActors(camera_);

    for (const std::unique_ptr<Enemy>& enemy : enemies_) {
        enemy->Draw(camera_);
    }
    if (boss_) boss_->Draw(camera_);

    player_.Draw(camera_);

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
                    "DEFEAT", static_cast<int>(t * 255.0f), draw::TextAlign::Center);
    draw::TextAlpha(FontSize::Normal, kScreenW * 0.5f, kScreenH * 0.5f + 30.0f, palette::kTextDim,
                    "戦闘不能になった…", static_cast<int>(t * 255.0f), draw::TextAlign::Center);
}

void QuestScene::DrawVictoryOverlay() const
{
    const float t = math::Clamp(phaseTimer_ / 1.0f, 0.0f, 1.0f);
    ui::DrawDimOverlay(static_cast<int>(t * 120.0f));
    draw::TextAlpha(FontSize::Title, kScreenW * 0.5f, kScreenH * 0.5f - 80.0f, palette::kAccentWarm,
                    "QUEST CLEAR", static_cast<int>(t * 255.0f), draw::TextAlign::Center);
}

} // namespace ecl
