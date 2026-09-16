#include "Graphics/Animation.h"

#include "Common/MathUtil.h"

namespace ecl {

const char* PoseKindName(PoseKind pose)
{
    switch (pose) {
    case PoseKind::Idle:   return "idle";
    case PoseKind::Walk:   return "walk";
    case PoseKind::Run:    return "run";
    case PoseKind::Jump:   return "jump";
    case PoseKind::Fall:   return "fall";
    case PoseKind::Attack: return "attack";
    case PoseKind::Skill:  return "skill";
    case PoseKind::Guard:  return "guard";
    case PoseKind::Hurt:   return "hurt";
    case PoseKind::Dead:   return "dead";
    case PoseKind::Dash:   return "dash";
    default: return "idle";
    }
}

void AnimationSet::Set(PoseKind pose, const AnimationClip& clip)
{
    const int index = static_cast<int>(pose);
    if (index < 0 || index >= static_cast<int>(PoseKind::Count)) return;
    clips_[index] = clip;
    valid_[index] = clip.Valid();
    if (clip.Valid()) empty_ = false;
}

const AnimationClip* AnimationSet::Get(PoseKind pose) const
{
    const int index = static_cast<int>(pose);
    if (index < 0 || index >= static_cast<int>(PoseKind::Count)) return nullptr;
    if (!valid_[index]) return nullptr;
    return &clips_[index];
}

AnimationSet LoadAnimationSet(const std::string& keyPrefix, const std::string& dir,
                              const std::vector<ClipSpec>& specs)
{
    AnimationSet set;
    ResourceManager& resources = ResourceManager::Instance();

    for (const ClipSpec& spec : specs) {
        const std::string key = keyPrefix + "/" + spec.name;
        const TextureAsset* asset =
            resources.LoadAnimation(key, dir, spec.name, spec.cols, spec.rows, spec.frameCount);

        AnimationClip clip;
        clip.asset = (asset && asset->Valid()) ? asset : nullptr;
        clip.fps = spec.fps;
        clip.loop = spec.loop;
        set.Set(spec.pose, clip);
    }
    return set;
}

const std::vector<ClipSpec>& DefaultHumanoidClipSpecs()
{
    static const std::vector<ClipSpec> specs = {
        { PoseKind::Idle,   "idle",   4, 1, 4, 8.0f,  true  },
        { PoseKind::Walk,   "walk",   6, 1, 6, 12.0f, true  },
        { PoseKind::Run,    "run",    6, 1, 6, 14.0f, true  },
        { PoseKind::Jump,   "jump",   2, 1, 2, 8.0f,  false },
        { PoseKind::Fall,   "fall",   2, 1, 2, 8.0f,  false },
        { PoseKind::Attack, "attack", 5, 1, 5, 18.0f, false },
        { PoseKind::Skill,  "skill",  6, 1, 6, 18.0f, false },
        { PoseKind::Guard,  "guard",  2, 1, 2, 6.0f,  true  },
        { PoseKind::Hurt,   "hurt",   2, 1, 2, 10.0f, false },
        { PoseKind::Dead,   "dead",   4, 1, 4, 8.0f,  false },
        { PoseKind::Dash,   "dash",   2, 1, 2, 12.0f, false },
    };
    return specs;
}

void Animator::Play(PoseKind pose, bool restart)
{
    if (pose_ != pose) {
        pose_ = pose;
        timer_ = 0.0f;
    } else if (restart) {
        timer_ = 0.0f;
    }
}

void Animator::Update(float dt)
{
    timer_ += dt * speed_;
}

const AnimationClip* Animator::Clip() const
{
    if (!set_) return nullptr;
    return set_->Get(pose_);
}

bool Animator::HasArt() const
{
    const AnimationClip* clip = Clip();
    return clip != nullptr && clip->Valid();
}

int Animator::FrameIndex() const
{
    const AnimationClip* clip = Clip();
    if (!clip || clip->FrameCount() <= 0) return 0;

    const int count = clip->FrameCount();
    int index = static_cast<int>(timer_ * clip->fps);
    if (clip->loop) {
        index %= count;
        if (index < 0) index += count;
    } else {
        index = math::ClampInt(index, 0, count - 1);
    }
    return index;
}

bool Animator::Finished() const
{
    const AnimationClip* clip = Clip();
    if (!clip || clip->loop || clip->FrameCount() <= 0) return false;
    return timer_ * clip->fps >= static_cast<float>(clip->FrameCount());
}

float Animator::Phase() const
{
    const AnimationClip* clip = Clip();
    if (!clip || clip->FrameCount() <= 0) {
        // 素材が無い場合も時間から擬似的な位相を返す（プレースホルダー描画用）
        const float cycle = 1.0f;
        float phase = timer_ / cycle;
        phase -= static_cast<float>(static_cast<int>(phase));
        return phase;
    }
    const float total = static_cast<float>(clip->FrameCount()) / clip->fps;
    if (total <= 0.0f) return 0.0f;
    float phase = timer_ / total;
    if (clip->loop) {
        phase -= static_cast<float>(static_cast<int>(phase));
    } else {
        phase = math::Clamp(phase, 0.0f, 1.0f);
    }
    return phase;
}

} // namespace ecl
