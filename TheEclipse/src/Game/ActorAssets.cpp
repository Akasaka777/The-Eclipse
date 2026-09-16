#include "Game/ActorAssets.h"

#include "Common/StringUtil.h"
#include "Core/GameConfig.h"

namespace ecl {

ActorAssets& ActorAssets::Instance()
{
    static ActorAssets instance;
    return instance;
}

const AnimationSet* ActorAssets::Load(const std::string& key, const std::string& directory)
{
    auto found = sets_.find(key);
    if (found != sets_.end()) {
        return found->second.Empty() ? nullptr : &found->second;
    }

    AnimationSet set = LoadAnimationSet(key, directory, DefaultHumanoidClipSpecs());
    auto inserted = sets_.emplace(key, set);
    return inserted.first->second.Empty() ? nullptr : &inserted.first->second;
}

const AnimationSet* ActorAssets::PlayerSet()
{
    return Load("player", std::string(config::kAssetRoot) + "/characters/player");
}

const AnimationSet* ActorAssets::EnemySet(int enemyId, const std::string& folder)
{
    if (folder.empty()) return nullptr;
    return Load(str::Format("enemy_%d", enemyId),
                std::string(config::kAssetRoot) + "/enemies/" + folder);
}

const AnimationSet* ActorAssets::BossSet(int bossId, const std::string& folder)
{
    if (folder.empty()) return nullptr;
    return Load(str::Format("boss_%d", bossId),
                std::string(config::kAssetRoot) + "/bosses/" + folder);
}

} // namespace ecl
