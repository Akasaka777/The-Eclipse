//==============================================================================
// HomeTown.h : ホームの街（横スクロールで歩いて回れる拠点）
//   通りに沿って「自宅」「ショップ」「鍛冶屋」「迷宮の門」「ダンジョン入口」が並ぶ。
//   建物の入口の前に立って決定キー（初期設定は ENTER）を押すと、その施設の画面が開く。
//   ダンジョン入口は、奥のゲートに踏み込んだときもクエスト選択を開く。
//==============================================================================
#pragma once

#include "Common/Types.h"
#include "Core/Camera.h"
#include "Game/Stage.h"

#include <string>

namespace ecl {
namespace town {

// 施設（開く画面は HomeScene が決める）
enum class Facility
{
    Home,     // 自宅：ステータス・装備・アイテム・スキル（プレイヤー）
    Shop,     // ショップ
    Smith,    // 鍛冶屋
    Labyrinth,// 迷宮の門：ローグライクモード（全クエストクリアで開く）
    Dungeon   // ダンジョン入口：クエスト選択
};

struct Building
{
    Facility    facility;
    const char* name;     // 看板の名前
    const char* guide;    // 入口の前に立ったときに出す説明
    float       x;        // 入口の中心（ワールド座標）
    float       width;
    float       height;   // 壁の高さ（屋根やアーチは含まない）
};

constexpr float kTownWidth = 5200.0f;    // 街の幅
constexpr float kDoorRange = 130.0f;     // 入口の中心からこの距離までなら入れる
constexpr float kStartX = 960.0f;        // ホームに来たときに立つ場所（自宅とショップの間）
// ダンジョン入口のゲートに踏み込んだとみなす位置（入口の中心からの距離）
constexpr float kDungeonStepIn = 70.0f;

int BuildingCount();
const Building& BuildingAt(int index);
// 入口の前（kDoorRange 以内）にいる建物。いなければ -1
int  BuildingNear(float x);
// その施設の建物（必ずある）
const Building& BuildingFor(Facility facility);

// 建物（プレイヤーより奥）。baseY は建物の土台の高さ（通りの奥の端）。
//   highlighted は入口の前に立っている建物（-1 なら無し）
//   labyrinthOpen は迷宮の門が開いているか（ローグライクモードが解放されているか）
void DrawBuildings(const Camera& camera, float baseY, float time, int highlighted, bool labyrinthOpen = true);
// 通りの手前に立つもの（街灯と道しるべ。プレイヤーより手前に描く）
void DrawForeground(const Camera& camera, float groundY, float time);
// 入口の前に立ったときの案内（「ENTER  入る」と施設の説明）
//   guide を渡すと、建物の説明の代わりにそれを出す
void DrawEnterPrompt(const Camera& camera, int index, float baseY, const std::string& keyName, float time,
                     const char* guide = nullptr);
// 好きな場所に入口の案内を出す（ダンジョンの中のショップ・鍛冶屋でも使う）
void DrawPromptAt(const Camera& camera, float worldX, float baseY, const std::string& keyName, const char* name,
                  const std::string& guide, float time);
// ダンジョンの中の施設（ローグライクモードの行商人のテント・旅の鍛冶屋）
void DrawStall(const Camera& camera, FacilityKind kind, float worldX, float baseY, float time, bool lit);

} // namespace town
} // namespace ecl
