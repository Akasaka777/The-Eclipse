#include "Core/FontManager.h"

#include "Core/DxInclude.h"
#include "Core/GameConfig.h"

#include <fstream>

#if defined(_WIN32)
#pragma comment(lib, "gdi32.lib")
#endif

namespace ecl {

namespace {

struct FontSpec
{
    int size;
    int thickness;
    int edge;
};

// サイズ種別ごとの生成パラメータ
const FontSpec kSpecs[static_cast<int>(FontSize::Count)] = {
    { 14, 3, 1 }, // Tiny
    { 18, 4, 1 }, // Small
    { 22, 5, 1 }, // Normal
    { 28, 6, 2 }, // Medium
    { 40, 7, 2 }, // Large
    { 56, 8, 3 }, // Huge
    { 78, 9, 3 }, // Title
};

// 日本語が表示できる既定候補（先頭から順に採用）
const char* kFallbackFaces[] = {
    "Meiryo UI",
    "メイリオ",
    "Yu Gothic UI",
    "游ゴシック",
    "ＭＳ ゴシック",
    "MS Gothic",
};

} // namespace

FontManager& FontManager::Instance()
{
    static FontManager instance;
    return instance;
}

bool FontManager::Initialize()
{
    if (initialized_) return true;

    RegisterFontFiles();
    ResolveFaceName();

    for (int i = 0; i < static_cast<int>(FontSize::Count); ++i) {
        const FontSpec& spec = kSpecs[i];
        handles_[i] = CreateFontToHandle(faceName_.c_str(), spec.size, spec.thickness,
                                         DX_FONTTYPE_ANTIALIASING_EDGE, -1, spec.edge);
        if (handles_[i] < 0) {
            // フェイス名が無効な場合は既定フォントで生成し直す
            handles_[i] = CreateFontToHandle(NULL, spec.size, spec.thickness,
                                             DX_FONTTYPE_ANTIALIASING_EDGE, -1, spec.edge);
        }
    }

    initialized_ = true;
    return true;
}

void FontManager::Finalize()
{
    for (int i = 0; i < static_cast<int>(FontSize::Count); ++i) {
        if (handles_[i] >= 0) {
            DeleteFontToHandle(handles_[i]);
            handles_[i] = -1;
        }
    }

#if defined(_WIN32)
    for (const std::string& path : registeredFiles_) {
        RemoveFontResourceExA(path.c_str(), FR_PRIVATE, NULL);
    }
#endif
    registeredFiles_.clear();
    initialized_ = false;
}

int FontManager::Handle(FontSize size) const
{
    const int index = static_cast<int>(size);
    if (index < 0 || index >= static_cast<int>(FontSize::Count)) return -1;
    return handles_[index];
}

void FontManager::RegisterFontFiles()
{
#if defined(_WIN32)
    const std::string pattern = std::string(config::kAssetRoot) + "\\fonts\\*.*";

    WIN32_FIND_DATAA findData;
    HANDLE handle = FindFirstFileA(pattern.c_str(), &findData);
    if (handle == INVALID_HANDLE_VALUE) return;

    do {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) continue;

        std::string name = findData.cFileName;
        const size_t dot = name.find_last_of('.');
        if (dot == std::string::npos) continue;

        std::string ext = name.substr(dot);
        for (char& c : ext) c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
        if (ext != ".ttf" && ext != ".otf" && ext != ".ttc") continue;

        const std::string full = std::string(config::kAssetRoot) + "\\fonts\\" + name;
        if (AddFontResourceExA(full.c_str(), FR_PRIVATE, NULL) > 0) {
            registeredFiles_.push_back(full);
        }
    } while (FindNextFileA(handle, &findData) != 0);

    FindClose(handle);
#endif
}

void FontManager::ResolveFaceName()
{
    const std::string configPath = std::string(config::kAssetRoot) + "/fonts/fontface.txt";
    std::ifstream file(configPath);
    if (file) {
        std::string line;
        if (std::getline(file, line)) {
            // 末尾の改行・空白・BOM を除去
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
                line.pop_back();
            }
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }
            if (!line.empty()) {
                faceName_ = line;
                return;
            }
        }
    }

    // 既定候補から採用（DxLib は未インストール時に自動代替するため先頭を使う）
    faceName_ = kFallbackFaces[0];
    for (const char* face : kFallbackFaces) {
        const int probe = CreateFontToHandle(face, 20, 3, DX_FONTTYPE_NORMAL);
        if (probe >= 0) {
            DeleteFontToHandle(probe);
            faceName_ = face;
            break;
        }
    }
}

} // namespace ecl
