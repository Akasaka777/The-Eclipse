//==============================================================================
// NamePanel.h : プレイヤー名の入力ウィンドウ
//   画面上の文字パネルをクリックして入力する（IME に依存しない方式）。
//   タイトル画面の初回起動時と、設定画面の開発者モードから開く。
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/GameContext.h"
#include "UI/UIWidgets.h"

#include <string>
#include <vector>

namespace ecl {
namespace ui {

//------------------------------------------------------------------------------
// 文字パネルの種類
//------------------------------------------------------------------------------
enum class CharSet
{
    Hiragana,
    Katakana,
    Alphabet,
    Count
};

class NamePanel
{
public:
    NamePanel();

    // 現在の名前を初期値にして開く
    void Open(const std::string& currentName);
    void Close() { open_ = false; }
    bool IsOpen() const { return open_; }

    // キャンセルを許可するか（初回入力では不可にする）
    void SetCancelEnabled(bool enabled) { cancelEnabled_ = enabled; }
    // 見出し（「プレイヤー名を入力」など）
    void SetTitle(const std::string& title) { title_ = title; }

    void Update(float dt, const Input& input);
    void Draw() const;

    // 決定されたフレームで true
    bool Confirmed() const { return confirmed_; }
    // キャンセルされたフレームで true
    bool Cancelled() const { return cancelled_; }
    // 決定された名前
    const std::string& Result() const { return input_; }

private:
    void Layout();
    const std::vector<const char*>& CurrentChars() const;
    void AppendChar(const char* text);
    // 末尾の文字に濁点・半濁点を付け外しする
    void CycleVoiced();
    // 末尾の文字を小書き文字と入れ替える
    void CycleSmall();

    Rect window_;
    Rect nameField_;
    Rect gridArea_;

    std::vector<Button> setButtons_;   // 文字種タブ
    std::vector<Rect>   cellRects_;    // 文字セル
    Button voicedButton_;
    Button smallButton_;
    Button backButton_;
    Button clearButton_;
    Button confirmButton_;
    Button cancelButton_;

    CharSet     charSet_ = CharSet::Hiragana;
    std::string title_ = "プレイヤー名を入力";
    std::string input_;
    std::string message_;
    float messageTimer_ = 0.0f;
    float time_ = 0.0f;
    bool  open_ = false;
    bool  confirmed_ = false;
    bool  cancelled_ = false;
    bool  cancelEnabled_ = true;
};

} // namespace ui
} // namespace ecl
