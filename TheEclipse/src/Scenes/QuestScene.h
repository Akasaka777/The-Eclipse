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
#include "UI/UIWidgets.h"

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
        BossIntro,   // ボスフロアの登場演出（カメラがボスへ寄り、咆哮と名前の札）
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
    // 持っている武器すべて（二刀流なら左右とも）を摩耗させる
    void WearWeapons(GameContext& context, float amount);
    void WearArmor(GameContext& context, float amount);
    // 壊れた装備を取り除き、プレイヤーへ反映する
    void HandleBrokenEquipment(GameContext& context);
    void DrawBreakNotice() const;
    void UpdateFloorTransition(float dt, GameContext& context);
    // パリィ成功時の共通処理（攻撃者をよろけさせる）
    void HandleParrySuccess(GameContext& context);
    // アイテムスライダーで選んでいる枠のアイテムを使う
    void UseQuickItem(GameContext& context);
    // ID からアクターを探す
    Actor* FindActorById(int actorId);
    void FinishQuest(bool cleared, bool retired, GameContext& context);

    // --- ボスの登場演出 ---------------------------------------------------------
    void StartBossIntro();
    void UpdateBossIntro(float dt, const Input& input);
    void EndBossIntro();
    // 演出中のカメラの狙い（ワールド X）
    float BossIntroCameraX() const;
    void DrawBossIntro() const;

    void DrawWorld(const GameContext& context);
    void DrawBanner() const;
    void DrawDefeatOverlay() const;
    void DrawVictoryOverlay() const;

    int  AliveEnemyCount() const;
    bool BossAlive() const;

    // --- デバッグモード -------------------------------------------------------
    void UpdateDebug(const Input& input, GameContext& context);
    void DrawDebugOverlay(const GameContext& context) const;
    // フロア移動のボタン（◀ / 1F〜 / ボス / ▶）を情報パネルの中に並べる（フロア数はクエストで変わる）
    void LayoutDebugFloorButtons();
    void UpdateDebugFloorButtons(const Input& input, float dt);
    // いまフロアを移動できるか（暗転中・撃破後・戦闘不能のあとは移動しない）
    bool DebugFloorMoveAllowed() const;
    // 暗転してから index のフロアを読み込む
    void DebugMoveToFloor(int index);
    // 「2F」「ボスフロア」などの呼び名
    std::string DebugFloorName(int index) const;

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
    std::string transitionText_;   // 暗転中に出す文字

    // ボスの登場演出
    float bossIntroTimer_ = 0.0f;
    bool  bossIntroRoared_ = false;
    bool  bossIntroSkipped_ = false;
    float bossIntroOutroFromX_ = 0.0f;   // 戻りのパンを始めたときのカメラ位置
    bool  bossIntroOutroStarted_ = false;

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
    // フロア移動のボタン
    ui::Button debugPrevFloorButton_;
    ui::Button debugNextFloorButton_;
    std::vector<ui::Button> debugFloorButtons_;
    bool  debugClickConsumed_ = false;   // ボタンを押したフレームは、同じクリックで攻撃しない
};

} // namespace ecl
