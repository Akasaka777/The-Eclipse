//==============================================================================
// SaveData.h : セーブ / ロード
//   save/save.txt に平文で保存する（内容を確認・編集しやすい形式）。
//==============================================================================
#pragma once

#include <string>

namespace ecl {

class GameContext;

class SaveSystem
{
public:
    // 既定のセーブファイルのパス
    static const char* FilePath();
    static const char* DirectoryPath();

    static bool Exists();
    // 保存（成功したら true）
    static bool Save(const GameContext& context);
    // 読み込み（ファイルが無い / 壊れている場合は false）
    static bool Load(GameContext& context);
    // セーブデータの削除
    static bool Remove();

    // 最後に発生したエラー内容
    static const std::string& LastError();
};

} // namespace ecl
