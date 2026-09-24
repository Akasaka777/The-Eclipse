//==============================================================================
// StatusPanel.h : プレイヤータブ「ステータス」
//   STR / AGI / VIT / INT / LUK の確認と振り分けを行う。
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/Ability.h"
#include "Game/GameContext.h"
#include "UI/UIWidgets.h"

#include <string>
#include <vector>

namespace ecl {
namespace ui {

class StatusPanel
{
public:
    StatusPanel();

    void Open();
    void Close() { open_ = false; }
    bool IsOpen() const { return open_; }

    void Update(float dt, const Input& input, GameContext& context);
    void Draw(const GameContext& context) const;

    bool CloseRequested() const { return closeRequested_; }
    // このフレームでステータスが変わったか（戦闘中の反映に使う）
    bool StatusChanged() const { return statusChanged_; }

private:
    void Layout();
    void DrawProfile(const GameContext& context) const;
    void DrawAbilities(const GameContext& context) const;
    void DrawDerivedStats(const GameContext& context) const;

    Rect   window_;
    Rect   abilityArea_;
    Rect   detailArea_;
    Button closeButton_;
    // 各ステータスの「+」ボタン
    std::vector<Button> addButtons_;
    Rect   abilityRows_[kAbilityCount];

    int   selected_ = 0;
    bool  open_ = false;
    bool  closeRequested_ = false;
    bool  statusChanged_ = false;
    std::string message_;
    float messageTimer_ = 0.0f;
};

} // namespace ui
} // namespace ecl
