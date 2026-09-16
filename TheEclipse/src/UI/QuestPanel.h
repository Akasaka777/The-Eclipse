//==============================================================================
// QuestPanel.h : クエスト選択タブ
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/GameContext.h"
#include "Game/QuestDatabase.h"
#include "UI/UIWidgets.h"

#include <vector>

namespace ecl {
namespace ui {

class QuestPanel
{
public:
    QuestPanel();

    void Open(const GameContext& context);
    void Close() { open_ = false; }
    bool IsOpen() const { return open_; }

    void Update(float dt, const Input& input, GameContext& context);
    void Draw(const GameContext& context) const;

    bool CloseRequested() const { return closeRequested_; }
    // 出撃が選択されたら true
    bool StartRequested() const { return startRequested_; }
    int  SelectedQuestId() const { return selectedQuestId_; }

private:
    void Layout();

    Rect   window_;
    Button closeButton_;
    Button startButton_;
    std::vector<Button> questButtons_;

    int  selectedQuestId_ = 1;
    bool open_ = false;
    bool closeRequested_ = false;
    bool startRequested_ = false;
};

} // namespace ui
} // namespace ecl
