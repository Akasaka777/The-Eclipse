#include "Core/ResourceManager.h"

#include "Common/StringUtil.h"
#include "Core/DxInclude.h"

namespace ecl {

int TextureAsset::Frame(int index) const
{
    if (frames.empty()) return -1;
    if (index < 0) index = 0;
    if (index >= static_cast<int>(frames.size())) index = static_cast<int>(frames.size()) - 1;
    return frames[static_cast<size_t>(index)];
}

ResourceManager& ResourceManager::Instance()
{
    static ResourceManager instance;
    return instance;
}

TextureAsset& ResourceManager::Entry(const std::string& key)
{
    return assets_[key];
}

const TextureAsset* ResourceManager::Find(const std::string& key) const
{
    auto it = assets_.find(key);
    if (it == assets_.end()) return nullptr;
    return &it->second;
}

const TextureAsset* ResourceManager::LoadSingle(const std::string& key, const std::string& path)
{
    if (const TextureAsset* found = Find(key)) return found;

    TextureAsset& asset = Entry(key);
    const int handle = LoadGraph(path.c_str());
    if (handle >= 0) {
        asset.frames.push_back(handle);
        GetGraphSize(handle, &asset.frameWidth, &asset.frameHeight);
    } else {
        missingKeys_.push_back(key);
    }
    return &asset;
}

const TextureAsset* ResourceManager::LoadSheet(const std::string& key, const std::string& path,
                                               int cols, int rows, int frameCount)
{
    if (const TextureAsset* found = Find(key)) return found;

    TextureAsset& asset = Entry(key);
    if (cols <= 0) cols = 1;
    if (rows <= 0) rows = 1;

    const int total = cols * rows;
    const int use = (frameCount > 0 && frameCount <= total) ? frameCount : total;

    // シート全体のサイズを取得してから分割する
    const int probe = LoadGraph(path.c_str());
    if (probe < 0) {
        missingKeys_.push_back(key);
        return &asset;
    }

    int sheetW = 0;
    int sheetH = 0;
    GetGraphSize(probe, &sheetW, &sheetH);
    DeleteGraph(probe);

    const int frameW = sheetW / cols;
    const int frameH = sheetH / rows;
    if (frameW <= 0 || frameH <= 0) {
        missingKeys_.push_back(key);
        return &asset;
    }

    std::vector<int> handles(static_cast<size_t>(total), -1);
    if (LoadDivGraph(path.c_str(), total, cols, rows, frameW, frameH, handles.data()) < 0) {
        missingKeys_.push_back(key);
        return &asset;
    }

    asset.frameWidth = frameW;
    asset.frameHeight = frameH;
    for (int i = 0; i < total; ++i) {
        if (i < use) {
            asset.frames.push_back(handles[static_cast<size_t>(i)]);
        } else {
            DeleteGraph(handles[static_cast<size_t>(i)]);
        }
    }
    return &asset;
}

const TextureAsset* ResourceManager::LoadSequence(const std::string& key, const std::string& dir,
                                                  const std::string& baseName, int maxCount, int digits)
{
    if (const TextureAsset* found = Find(key)) return found;

    TextureAsset& asset = Entry(key);
    const std::string format = str::Format("%%s/%%s_%%0%dd.png", digits);

    for (int i = 0; i < maxCount; ++i) {
        const std::string path = str::Format(format.c_str(), dir.c_str(), baseName.c_str(), i);
        const int handle = LoadGraph(path.c_str());
        if (handle < 0) break;
        if (asset.frames.empty()) {
            GetGraphSize(handle, &asset.frameWidth, &asset.frameHeight);
        }
        asset.frames.push_back(handle);
    }

    if (!asset.Valid()) missingKeys_.push_back(key);
    return &asset;
}

const TextureAsset* ResourceManager::LoadAnimation(const std::string& key, const std::string& dir,
                                                   const std::string& baseName, int cols, int rows,
                                                   int frameCount)
{
    if (const TextureAsset* found = Find(key)) return found;

    // 1) スプライトシート: dir/baseName.png
    {
        const std::string path = dir + "/" + baseName + ".png";
        const int probe = LoadGraph(path.c_str());
        if (probe >= 0) {
            DeleteGraph(probe);
            return LoadSheet(key, path, cols, rows, frameCount);
        }
    }

    // 2) PNG 連番: dir/baseName/baseName_000.png
    return LoadSequence(key, dir + "/" + baseName, baseName, frameCount > 0 ? frameCount : 64);
}

void ResourceManager::ReleaseAll()
{
    for (auto& pair : assets_) {
        for (int handle : pair.second.frames) {
            if (handle >= 0) DeleteGraph(handle);
        }
    }
    assets_.clear();
    missingKeys_.clear();
}

} // namespace ecl
