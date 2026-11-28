//==============================================================================
// Pickup.h : 地面に落ちる拾い物（ローグライクモード）
//   敵を倒すと col・アイテム・強化結晶・スキル・装備がその場に飛び出す。
//   col・アイテム・強化結晶は近づくと吸い寄せられ、スキルと装備は触れたときだけ拾う
//   （戦闘の途中でスキルの選択画面が勝手に出ないように）。
//   フロアの敵を全滅させると、残っている col・アイテム・強化結晶はプレイヤーへ集まる。
//==============================================================================
#pragma once

#include "Common/Types.h"
#include "Core/Camera.h"
#include "Game/Equipment.h"

#include <vector>

namespace ecl {

class Stage;

enum class PickupKind
{
    Col,       // お金
    Item,      // 回復・バフアイテム
    Crystal,   // 強化結晶
    Skill,     // スキル（拾うと 3 つの候補から選ぶ）
    Gear       // 武器・防具
};

struct Pickup
{
    PickupKind    kind = PickupKind::Col;
    Vec2          pos;          // 地面に接している点（ワールド座標）
    Vec2          velocity;
    float         height = 0.0f;   // 地面からの高さ（跳ねる）
    float         heightSpeed = 0.0f;
    float         z = 0.0f;
    int           amount = 1;   // col・結晶の数
    int           itemId = 0;   // アイテムの ID
    EquipmentItem gear;
    float         age = 0.0f;
    bool          collected = false;
};

constexpr float kPickupMagnetRange = 170.0f;   // これより近いと吸い寄せられる
constexpr float kPickupTouchRange = 54.0f;     // これより近いと拾う
constexpr float kPickupDelay = 0.45f;          // 落ちてから拾えるようになるまで

class PickupField
{
public:
    void Clear() { pickups_.clear(); }
    void Add(PickupKind kind, const Vec2& at, float z, int amount = 1, int itemId = 0,
             const EquipmentItem& gear = EquipmentItem());

    // 拾ったものを collected に入れて返す。
    //   gatherAll を立てると、col・アイテム・強化結晶は距離に関係なくプレイヤーへ集まる（全滅後）
    void Update(float dt, const Stage& stage, const Vec2& playerPos, float playerZ, bool gatherAll,
                std::vector<Pickup>& collected);
    void Draw(const Camera& camera, float time) const;

    const std::vector<Pickup>& Items() const { return pickups_; }
    int Count(PickupKind kind) const;

private:
    std::vector<Pickup> pickups_;
};

} // namespace ecl
