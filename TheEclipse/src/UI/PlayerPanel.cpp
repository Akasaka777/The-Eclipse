#include "UI/PlayerPanel.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/GameConfig.h"
#include "Graphics/DrawUtil.h"

namespace ecl {
namespace ui {

namespace {

struct TabDef
{
    PlayerTab   tab;
    const char* label;
};

const TabDef kTabs[] = {
    { PlayerTab::Status,    "ステータス" },
    { PlayerTab::Equipment, "装備" },
    { PlayerTab::Item,      "アイテム" },
    { PlayerTab::Skill,     "スキル" },
};

constexpr int   kTabCount = static_cast<int>(sizeof(kTabs) / sizeof(kTabs[0]));
constexpr float kTabWidth = 260.0f;
constexpr float kTabHeight = 56.0f;
constexpr float kTabGap = 12.0f;
constexpr float kTabTop = 28.0f;

} // namespace

PlayerPanel::PlayerPanel()
{
    Layout();
}

void PlayerPanel::Layout()
{
    // 各パネルのウィンドウ（上端 100）より上に並べる
    const float total = kTabWidth * static_cast<float>(kTabCount)
                      + kTabGap * static_cast<float>(kTabCount - 1);
    const float left = (static_cast<float>(config::kScreenWidth) - total) * 0.5f;

    tabButtons_.clear();
    for (int i = 0; i < kTabCount; ++i) {
        const Rect rect = Rect::FromXYWH(left + (kTabWidth + kTabGap) * static_cast<float>(i),
                                         kTabTop, kTabWidth, kTabHeight);
        tabButtons_.push_back(Button(rect, kTabs[i].label));
    }

}

void PlayerPanel::Open(const GameContext& context)
{
    open_ = true;
    closeRequested_ = false;
    statsChanged_ = false;
    loadoutChanged_ = false;
    specialQuestId_ = 0;
    // 開いた直後はステータス
    SelectTab(PlayerTab::Status, context);
}

void PlayerPanel::Close()
{
    open_ = false;
    status_.Close();
    equip_.Close();
    item_.Close();
    skill_.Close();
}

void PlayerPanel::SelectTab(PlayerTab tab, const GameContext& context)
{
    tab_ = tab;
    status_.Close();
    equip_.Close();
    item_.Close();
    skill_.Close();

    switch (tab) {
    case PlayerTab::Status:    status_.Open(); break;
    case PlayerTab::Equipment: equip_.Open(); break;
    case PlayerTab::Item:      item_.Open(); break;
    case PlayerTab::Skill:     skill_.Open(context); break;
    default: break;
    }
}

void PlayerPanel::Update(float dt, const Input& input, GameContext& context)
{
    if (!open_) return;
    closeRequested_ = false;
    statsChanged_ = false;
    loadoutChanged_ = false;
    specialQuestId_ = 0;

    // --- 上部のタブ ----------------------------------------------------------
    bool tabChanged = false;
    for (int i = 0; i < static_cast<int>(tabButtons_.size()); ++i) {
        Button& button = tabButtons_[static_cast<size_t>(i)];
        button.SetSelected(kTabs[i].tab == tab_);
        if (!button.Update(input, dt) || kTabs[i].tab == tab_) continue;
        SelectTab(kTabs[i].tab, context);
        tabChanged = true;
    }
    // タブを切り替えたフレームは中身を動かさない（同じクリックの二重処理を避ける）
    if (tabChanged) return;

    switch (tab_) {
    case PlayerTab::Status:
        status_.Update(dt, input, context);
        statsChanged_ = status_.StatusChanged();
        if (status_.CloseRequested()) closeRequested_ = true;
        break;
    case PlayerTab::Equipment:
        equip_.Update(dt, input, context);
        statsChanged_ = equip_.EquipmentChanged();
        if (equip_.CloseRequested()) closeRequested_ = true;
        break;
    case PlayerTab::Skill:
        skill_.Update(dt, input, context);
        loadoutChanged_ = skill_.LoadoutChanged();
        specialQuestId_ = skill_.SpecialQuestRequested();
        if (skill_.CloseRequested()) closeRequested_ = true;
        break;
    case PlayerTab::Item:
        item_.Update(dt, input, context);
        if (item_.CloseRequested()) closeRequested_ = true;
        break;
    default:
        break;
    }

    if (closeRequested_) Close();
}

void PlayerPanel::Draw(const GameContext& context) const
{
    if (!open_) return;

    switch (tab_) {
    case PlayerTab::Status:    status_.Draw(context); break;
    case PlayerTab::Equipment: equip_.Draw(context); break;
    case PlayerTab::Skill:     skill_.Draw(context); break;
    case PlayerTab::Item:      item_.Draw(context); break;
    default: break;
    }

    DrawTabs();
}

void PlayerPanel::DrawTabs() const
{
    for (const Button& button : tabButtons_) button.Draw();
}

} // namespace ui
} // namespace ecl
