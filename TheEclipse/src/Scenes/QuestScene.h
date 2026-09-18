//==============================================================================
// QuestScene.h : クエスト（戦闘）シーン
//   2〜4 フロアを進み、最終フロアのボスを倒すとクリア。
//==============================================================================
#pragma once

#include "Core/Camera.h"
#include "Core/Scene.h"
#include "Game/Boss.h"
#include "Game/Combat.h"
#include "Game/Enemy.h"
#include "Game/Player.h"
#include "Game/QuestDatabase.h"
#include "Game/Stage.h"
#include "UI/BattleHud.h"
#include "UI/BattleMenu.h"

#include <memory>
#include <vector>

namespace ecl {

class QuestScene : public Scene
{
public:
    QuestScene();

    void OnEnter(GameContext& context) override;
    void Update(float dt, GameContext& context, SceneManager& manager) override;
    void Draw(GameContext& context) override;

    // 戦闘中のプレイヤー（HUD やテストからの参照用）
    const Player& GetPlayer() const { return player_; }

private:
    enum class Phase
    {
        FloorIntro,  // フロア名の表示
        Battle,      // 戦闘中
        FloorClear,  // 全滅後、ゲートへ
        Victory,     // ボス撃破
        Defeat,      // 戦闘不能
        Finished     // リザルトへ遷移中
    };

    void LoadFloor(int index, GameContext& context);
    void SpawnFloorEnemies(const GameContext& context);
    void UpdateActors(float dt, GameContext& context);
    void ResolveHitBoxes(GameContext& context);
    void ResolveProjectiles(GameContext& context);
    void CleanupDead(GameContext& context);
    // 装備を消耗させ、壊れたらその場で反映する
    void WearEquipment(GameContext& context, EquipSlot slot, float amount);
    void WearArmor(GameContext& context, float amount);
    // 壊れた装備を取り除き、プレイヤーへ反映する
    void HandleBrokenEquipment(GameContext& context);
    void DrawBreakNotice() const;
    void UpdateFloorTransition(float dt, GameContext& context);
    // パリィ成功時の共通処理（攻撃者をよろけさせる）
    void HandleParrySuccess();
    // ID からアクターを探す
    Actor* FindActorById(int actorId);
    void FinishQuest(bool cleared, bool retired, GameContext& context);

    void DrawWorld(const GameContext& context);
    void DrawBanner() const;
    void DrawDefeatOverlay() const;
    void DrawVictoryOverlay() const;

    int  AliveEnemyCount() const;
    bool BossAlive() const;

    // --- デバッグモード -------------------------------------------------------
    void UpdateDebug(const Input& input, GameContext& context);
    void DrawDebugOverlay(const GameContext& context) const;

    const QuestDef* quest_ = nullptr;
    Stage  stage_;
    Player player_;
    Camera camera_;
    CombatSystem combat_;
    std::vector<std::unique_ptr<Enemy>> enemies_;
    std::unique_ptr<Boss> boss_;

    ui::BattleHud  hud_;
    ui::BattleMenu menu_;

    Phase phase_ = Phase::FloorIntro;
    int   floorIndex_ = 0;
    float phaseTimer_ = 0.0f;
    float questTime_ = 0.0f;
    float hitStop_ = 0.0f;

    // フロア間の暗転
    bool  transitioning_ = false;
    float transitionAlpha_ = 0.0f;
    int   pendingFloor_ = 0;

    // バナー表示
    std::string bannerMain_;
    std::string bannerSub_;
    float bannerTimer_ = 0.0f;

    // 集計
    int   enemiesDefeated_ = 0;
    int   totalDamage_ = 0;
    int   damageTaken_ = 0;
    int   expGained_ = 0;
    int   colGained_ = 0;
    bool  bossRewardGranted_ = false;
    int   combo_ = 0;
    int   maxCombo_ = 0;
    int   parryCount_ = 0;

    // クエスト中に壊れた装備（リザルトで報告する）
    std::vector<std::string> brokenItems_;
    // 破損通知の表示
    std::string breakNotice_;
    float breakNoticeTimer_ = 0.0f;
    float comboTimer_ = 0.0f;

    // デバッグ
    bool  debugInvincible_ = false;
    bool  debugShowHitBoxes_ = false;
    std::string debugMessage_;
    float debugMessageTimer_ = 0.0f;
};

} // namespace ecl
