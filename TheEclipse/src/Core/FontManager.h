//==============================================================================
// FontManager.h : 日本語フォントの生成と管理
//   assets/fonts に .ttf / .otf を置くと自動で読み込む（Windows のみ）。
//   assets/fonts/fontface.txt の1行目にフォント名を書くと優先的に使用する。
//==============================================================================
#pragma once

#include <string>
#include <vector>

namespace ecl {

enum class FontSize
{
    Tiny,    // 14px 補足
    Small,   // 18px 説明文
    Normal,  // 22px 標準
    Medium,  // 28px 見出し
    Large,   // 40px 強調
    Huge,    // 56px ダメージ / 大見出し
    Title,   // 78px タイトル
    Count
};

class FontManager
{
public:
    static FontManager& Instance();

    // DxLib_Init 後に呼ぶ
    bool Initialize();
    void Finalize();

    int Handle(FontSize size) const;
    const std::string& FaceName() const { return faceName_; }

private:
    FontManager() = default;

    // assets/fonts 内のフォントファイルを OS に一時登録
    void RegisterFontFiles();
    // fontface.txt もしくは既定の候補からフェイス名を決める
    void ResolveFaceName();

    int handles_[static_cast<int>(FontSize::Count)] = {};
    std::string faceName_;
    std::vector<std::string> registeredFiles_;
    bool initialized_ = false;
};

} // namespace ecl
