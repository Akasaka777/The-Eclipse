//==============================================================================
// ResourceManager.h : 画像リソースの読み込み（スプライトシート / PNG 連番）
//   ファイルが無い場合も TextureAsset を返すが valid() が false になり、
//   描画側はプレースホルダー（矩形ベースの簡易描画）へフォールバックする。
//==============================================================================
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace ecl {

//------------------------------------------------------------------------------
// 読み込み済みテクスチャ（1つ以上のコマを持つ）
//------------------------------------------------------------------------------
class TextureAsset
{
public:
    std::vector<int> frames;  // DxLib グラフィックハンドル
    int frameWidth = 0;
    int frameHeight = 0;

    bool Valid() const { return !frames.empty(); }
    int  FrameCount() const { return static_cast<int>(frames.size()); }
    int  Frame(int index) const;
};

class ResourceManager
{
public:
    static ResourceManager& Instance();

    // 単一画像
    const TextureAsset* LoadSingle(const std::string& key, const std::string& path);
    // スプライトシート（横 cols × 縦 rows に等分割）
    const TextureAsset* LoadSheet(const std::string& key, const std::string& path,
                                  int cols, int rows, int frameCount = -1);
    // PNG 連番 "dir/name_000.png" 形式（digits 桁）
    const TextureAsset* LoadSequence(const std::string& key, const std::string& dir,
                                     const std::string& baseName, int maxCount = 64, int digits = 3);
    // スプライトシート→連番の順で探す統合ローダ
    const TextureAsset* LoadAnimation(const std::string& key, const std::string& dir,
                                      const std::string& baseName, int cols, int rows, int frameCount);

    const TextureAsset* Find(const std::string& key) const;
    void ReleaseAll();

    // 読み込みに失敗したキーの一覧（起動ログ用）
    const std::vector<std::string>& MissingKeys() const { return missingKeys_; }

private:
    ResourceManager() = default;

    TextureAsset& Entry(const std::string& key);

    std::unordered_map<std::string, TextureAsset> assets_;
    std::vector<std::string> missingKeys_;
    TextureAsset emptyAsset_;
};

} // namespace ecl
