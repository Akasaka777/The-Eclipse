//==============================================================================
// Animation.h : アニメーションクリップと再生機
//   スプライトシート / PNG 連番のどちらでも同じクリップとして扱う。
//==============================================================================
#pragma once

#include "Core/ResourceManager.h"

#include <string>
#include <vector>

namespace ecl {

//------------------------------------------------------------------------------
// キャラクターのポーズ種別（素材が無い場合のプレースホルダー描画にも使う）
//------------------------------------------------------------------------------
enum class PoseKind
{
    Idle,
    Walk,
    Run,
    Jump,
    Fall,
    Attack,
    Skill,
    Guard,
    Hurt,
    Dead,
    Dash,
    Count
};

const char* PoseKindName(PoseKind pose);

//------------------------------------------------------------------------------
// 1つのアニメーション定義
//------------------------------------------------------------------------------
struct AnimationClip
{
    const TextureAsset* asset = nullptr;
    float fps = 12.0f;
    bool  loop = true;

    int  FrameCount() const { return asset ? asset->FrameCount() : 0; }
    bool Valid() const { return asset != nullptr && asset->Valid(); }
};

//------------------------------------------------------------------------------
// ポーズごとのクリップ表
//------------------------------------------------------------------------------
class AnimationSet
{
public:
    void Set(PoseKind pose, const AnimationClip& clip);
    const AnimationClip* Get(PoseKind pose) const;
    bool Empty() const { return empty_; }

private:
    AnimationClip clips_[static_cast<int>(PoseKind::Count)];
    bool valid_[static_cast<int>(PoseKind::Count)] = {};
    bool empty_ = true;
};

//------------------------------------------------------------------------------
// 読み込み定義（アセットが無ければ自動的に無効クリップになる）
//------------------------------------------------------------------------------
struct ClipSpec
{
    PoseKind pose;
    const char* name;  // ファイル名 / フォルダ名
    int cols;          // スプライトシートの列数
    int rows;          // スプライトシートの行数
    int frameCount;    // 使用コマ数
    float fps;
    bool loop;
};

// dir 以下から spec に従ってアニメーションを読み込む
// 例) dir="assets/characters/player", name="idle"
//     → assets/characters/player/idle.png (cols×rows 分割)
//     → assets/characters/player/idle/idle_000.png ...
AnimationSet LoadAnimationSet(const std::string& keyPrefix, const std::string& dir,
                              const std::vector<ClipSpec>& specs);

// 標準的な人型キャラ用のクリップ定義を返す
const std::vector<ClipSpec>& DefaultHumanoidClipSpecs();

//------------------------------------------------------------------------------
// 再生機
//------------------------------------------------------------------------------
class Animator
{
public:
    void SetSet(const AnimationSet* set) { set_ = set; }
    // ポーズ変更（同じポーズなら restart 指定時のみ巻き戻す）
    void Play(PoseKind pose, bool restart = false);
    void Update(float dt);
    void SetSpeed(float speed) { speed_ = speed; }

    PoseKind CurrentPose() const { return pose_; }
    int      FrameIndex() const;
    bool     Finished() const;
    // 0.0〜1.0 の再生位置
    float    Phase() const;
    float    Timer() const { return timer_; }
    const AnimationClip* Clip() const;
    bool     HasArt() const;

private:
    const AnimationSet* set_ = nullptr;
    PoseKind pose_ = PoseKind::Idle;
    float timer_ = 0.0f;
    float speed_ = 1.0f;
};

} // namespace ecl
