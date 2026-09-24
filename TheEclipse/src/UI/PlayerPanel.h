//==============================================================================
// PlayerPanel.h : ホームの「プレイヤー」タブ
//   ステータス / 装備 / アイテム / スキル を 1 つにまとめる。
//   開いた直後はステータスを表示する。
//==============================================================================
#pragma once

#include "Core/Input.h"
#include "Game/GameContext.h"
#include "UI/EquipPanel.h"
#include "UI/SkillPanel.h"
#include "UI/StatusPanel.h"
#include "UI/UIWidgets.h"

#include <vector>

namespace ecl {
namespace ui {

enum class PlayerTab
{
    Status,     // ステータス
    Equipment,  // 装備
    Item,       // アイテム（未実装）
    Skill,      // スキル
    Count
};

class PlayerPanel
{
public:
    PlayerPanel();

    void Open(const GameContext& context);
    void Close();
    bool IsOpen() const { return open_; }

    void Update(float dt, const Input& input, GameContext& context);
    void Draw(const GameContext& context) const;

    bool CloseRequested() const { return closeRequested_; }
    // 装備やステータスが変わったフレームで true（見た目や能力の反映に使う）
    bool StatsChanged() const { return statsChanged_; }
    // スキル構成が変わったフレームで true
    bool LoadoutChanged() const { return loadoutChanged_; }
    // ユニークスキルの特別クエストへ出撃したいか（0 なら要求なし）
    int  SpecialQuestRequested() const { return specialQuestId_; }

private:
    void Layout();
    void SelectTab(PlayerTab tab, const GameContext& context);
    void DrawTabs() const;
    void DrawItemTab() const;

    Rect   itemWindow_;
    Button itemCloseButton_;
    std::vector<Button> tabButtons_;

    StatusPanel status_;
    EquipPanel  equip_;
    SkillPanel  skill_;

    PlayerTab tab_ = PlayerTab::Status;
    bool open_ = false;
    bool closeRequested_ = false;
    bool statsChanged_ = false;
    bool loadoutChanged_ = false;
    int  specialQuestId_ = 0;
};

} // namespace ui
} // namespace ecl
