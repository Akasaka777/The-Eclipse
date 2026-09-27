//==============================================================================
// WeaponSelectPanel.h : 初回プレイ時の武器種選択ウィンドウ
//   タイトル画面で名前を決めた直後に一度だけ開く。
//   ここで選んだ武器種の武器だけが初期装備として配られる。
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/WeaponType.h"
#include "UI/UIWidgets.h"

namespace ecl {
namespace ui {

class WeaponSelectPanel
{
public:
    WeaponSelectPanel();

    void Open();
    void Close() { open_ = false; }
    bool IsOpen() const { return open_; }

    void Update(float dt, const Input& input);
    void Draw() const;

    // 決定されたフレームで true
    bool Confirmed() const { return confirmed_; }
    // 選ばれた武器種（未選択なら片手剣を返す）
    WeaponType Result() const;
    // まだ何も選んでいないか
    bool HasSelection() const { return selected_ >= 0; }

    // --- レイアウト参照（検証用）---------------------------------------------
    const Rect& WindowRect() const { return window_; }
    const Rect& CardRect(int index) const;
    const Rect& ConfirmRect() const { return confirmButton_.GetRect(); }

private:
    void Layout();
    void DrawCard(int index, float mouseX, float mouseY) const;

    static constexpr int kCount = static_cast<int>(WeaponType::Count);

    Rect   window_;
    Rect   detailArea_;
    Rect   cards_[kCount];
    Button confirmButton_;

    int   selected_ = -1;
    bool  open_ = false;
    bool  confirmed_ = false;
    float time_ = 0.0f;
};

} // namespace ui
} // namespace ecl
