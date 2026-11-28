//==============================================================================
// Stage.h : フロアの地形・背景・ゲート
//==============================================================================
#pragma once

#include "Common/Types.h"
#include "Core/Camera.h"

#include <string>
#include <vector>

namespace ecl {

//------------------------------------------------------------------------------
// 足場
//------------------------------------------------------------------------------
struct Platform
{
    Rect rect;
    bool oneWay = true; // 上からのみ乗れる

    Platform() = default;
    Platform(float left, float top, float width, float height, bool oneWay_ = true)
        : rect(Rect::FromXYWH(left, top, width, height)), oneWay(oneWay_) {}
};

//------------------------------------------------------------------------------
// 敵の配置
//------------------------------------------------------------------------------
struct EnemySpawn
{
    float x = 0.0f;
    float y = -1.0f;  // 負値なら地面に配置
    int   enemyId = 0;
    float z = -1.0f;  // 負値なら奥行きの中からランダム

    EnemySpawn() = default;
    EnemySpawn(float x_, int enemyId_, float y_ = -1.0f, float z_ = -1.0f)
        : x(x_), y(y_), enemyId(enemyId_), z(z_) {}
};

//------------------------------------------------------------------------------
// フロアに置く施設（ローグライクモードのショップ・鍛冶屋）
//------------------------------------------------------------------------------
enum class FacilityKind
{
    Shop,    // 行商人のショップ
    Smith    // 旅の鍛冶屋
};

struct FacilitySpot
{
    FacilityKind kind = FacilityKind::Shop;
    float        x = 0.0f;   // 入口（店先）の中心のワールド座標
};

//------------------------------------------------------------------------------
// 背景テーマ
//------------------------------------------------------------------------------
enum class StageTheme
{
    Forest, // 深緑の森
    Ruins,  // 石牢の回廊
    Altar,   // 蝕の祭壇
    Volcano, // 竜王の火山
    Home,    // 拠点
    Sanctum, // 金剛の伽藍（竜王の火山の隠しフロア）
    Throne   // 世界終焉の玉座（100 層）
};

//------------------------------------------------------------------------------
// フロア定義
//------------------------------------------------------------------------------
struct FloorDef
{
    std::string name = "FLOOR";
    float       width = 4200.0f;
    float       groundY = 880.0f;   // 最も手前（z = 0）の地面
    float       depth = 220.0f;     // 奥行きの幅
    StageTheme  theme = StageTheme::Forest;
    std::vector<Platform>   platforms;
    std::vector<EnemySpawn> spawns;
    int         bossId = -1; // ボスフロアなら 0 以上
    // ボスの HP バーの本数（3〜10）。0 ならクエストの設定（推奨戦力から自動）
    int         bossHpBars = 0;
    // このフロアのボスを倒したあとに現れる最後のボス（ボスラッシュの最後のフロア）。-1 なら無し
    int         finalBossId = -1;
    int         finalBossHpBars = 0;   // 0 ならクエストの設定
    // 敵とボスの強さの倍率。0 ならクエストの設定（QuestDef::enemyPowerScale）。
    //   ローグライクでは奥のフロアほど大きくする
    float       enemyPowerScale = 0.0f;
    // フロアに置く施設（ローグライクのショップ・鍛冶屋）
    std::vector<FacilitySpot> facilities;
};

//------------------------------------------------------------------------------
// ステージ本体
//------------------------------------------------------------------------------
class Stage
{
public:
    void Load(const FloorDef& def);
    void Update(float dt);

    const FloorDef& Def() const { return def_; }
    float Width() const { return def_.width; }
    float GroundY() const { return def_.groundY; }
    float Depth() const { return def_.depth; }
    // 指定の奥行きにおける地面の Y
    float GroundYAt(float z) const { return def_.groundY - z; }
    // 奥行きを移動可能範囲へ収める
    float ClampZ(float z) const;
    bool  IsBossFloor() const { return def_.bossId >= 0; }

    // 着地する Y 座標を返す（着地しない場合は非常に大きな値）
    float LandingY(float x, float halfWidth, float prevBottom, float newBottom, float z) const;
    // 移動可能な X 範囲にクランプ
    float ClampX(float x, float halfWidth) const;

    // --- ゲート -------------------------------------------------------------
    void  SetGateOpen(bool open);
    bool  GateOpen() const { return gateOpen_; }
    // 開いたゲートの上に出す案内（既定は「▶ 次のフロアへ」。フロアを読み込むと戻る）
    void  SetGateLabel(const std::string& label) { gateLabel_ = label; }
    const std::string& GateLabel() const { return gateLabel_; }
    float GateX() const { return def_.width - 180.0f; }
    // ゲートに到達したか
    bool  ReachedGate(float x) const;

    void DrawBackground(const Camera& camera) const;
    void DrawForeground(const Camera& camera) const;

private:
    void DrawSky(const Camera& camera) const;
    void DrawFarLayer(const Camera& camera) const;
    void DrawMidLayer(const Camera& camera) const;
    void DrawGround(const Camera& camera) const;
    // 奥行きを示す床の帯
    void DrawDepthField(const Camera& camera) const;
    void DrawPlatforms(const Camera& camera) const;
    void DrawGate(const Camera& camera) const;

    FloorDef def_;
    bool  gateOpen_ = false;
    float gateAnim_ = 0.0f;
    std::string gateLabel_ = "▶ 次のフロアへ";
    float time_ = 0.0f;
};

} // namespace ecl
