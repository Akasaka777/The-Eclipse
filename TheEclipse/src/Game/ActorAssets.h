//==============================================================================
// ActorAssets.h : キャラクター用アニメーション素材の読み込み窓口
//   assets/ 以下に素材を置くと自動的に使用され、無ければ図形による代替表示になる。
//
//   例) プレイヤー
//       assets/characters/player/idle.png          … 横 4 分割のスプライトシート
//       assets/characters/player/run/run_000.png   … PNG 連番でも可
//==============================================================================
#pragma once

#include "Graphics/Animation.h"

#include <string>
#include <unordered_map>

namespace ecl {

class ActorAssets
{
public:
    static ActorAssets& Instance();

    // 素材が存在しない場合は nullptr（＝代替表示）を返す
    const AnimationSet* PlayerSet();
    const AnimationSet* EnemySet(int enemyId, const std::string& folder);
    const AnimationSet* BossSet(int bossId, const std::string& folder);

private:
    ActorAssets() = default;

    const AnimationSet* Load(const std::string& key, const std::string& directory);

    std::unordered_map<std::string, AnimationSet> sets_;
};

} // namespace ecl
