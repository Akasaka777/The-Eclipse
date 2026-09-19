#include "UI/QuestPanel.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/GameConfig.h"
#include "Game/ItemDatabase.h"
#include "Graphics/DrawUtil.h"

namespace ecl {
namespace ui {

QuestPanel::QuestPanel()
{
    Layout();
}

void QuestPanel::Layout()
{
    window_ = Rect::FromXYWH(200.0f, 110.0f, 1520.0f, 860.0f);

    const QuestDatabase& database = QuestDatabase::Instance();
    questButtons_.clear();

    float y = window_.top + 90.0f;
    for (size_t i = 0; i < database.Quests().size(); ++i) {
        questButtons_.push_back(Button(Rect::FromXYWH(window_.left + 40.0f, y, 520.0f, 124.0f),
                                       "", FontSize::Normal));
        y += 140.0f;
    }

    closeButton_ = Button(Rect::FromXYWH(window_.right - 200.0f, window_.bottom - 86.0f, 160.0f, 58.0f),
                          "閉じる");
    startButton_ = Button(Rect::FromXYWH(window_.right - 420.0f, window_.bottom - 86.0f, 200.0f, 58.0f),
                          "出撃");
    startButton_.SetAccent(palette::kAccentWarm);
}

void QuestPanel::Open(const GameContext& context)
{
    open_ = true;
    closeRequested_ = false;
    startRequested_ = false;
    selectedQuestId_ = context.selectedQuestId;
}

void QuestPanel::Update(float dt, const Input& input, GameContext& context)
{
    if (!open_) return;

    closeRequested_ = false;
    startRequested_ = false;

    const std::vector<QuestDef>& quests = QuestDatabase::Instance().Quests();
    for (size_t i = 0; i < questButtons_.size() && i < quests.size(); ++i) {
        questButtons_[i].SetSelected(quests[i].id == selectedQuestId_);
        if (questButtons_[i].Update(input, dt)) {
            selectedQuestId_ = quests[i].id;
            context.selectedQuestId = selectedQuestId_;
        }
    }

    if (startButton_.Update(input, dt) || input.Pressed(GameAction::Confirm)) {
        context.selectedQuestId = selectedQuestId_;
        startRequested_ = true;
    }
    if (closeButton_.Update(input, dt) || input.Pressed(GameAction::Cancel)) {
        closeRequested_ = true;
        open_ = false;
    }
}

void QuestPanel::Draw(const GameContext& context) const
{
    if (!open_) return;

    DrawWindow(window_, "クエスト選択");

    const std::vector<QuestDef>& quests = QuestDatabase::Instance().Quests();
    const int playerPower = context.player.Power();

    // --- 一覧 ---------------------------------------------------------------
    for (size_t i = 0; i < questButtons_.size() && i < quests.size(); ++i) {
        const QuestDef& quest = quests[i];
        const Rect rect = questButtons_[i].GetRect();
        const bool selected = (quest.id == selectedQuestId_);
        const bool cleared = context.player.IsQuestCleared(quest.id);

        const ColorRGB base = selected ? palette::kPanelLight : palette::kPanelDark;
        draw::GradientRectH(rect, base, base.Scaled(0.7f), 235, 14);
        draw::StrokeRect(rect, selected ? palette::kAccent : palette::kBorder.Scaled(0.7f),
                         selected ? 3.0f : 1.0f, 255);

        draw::Text(FontSize::Medium, rect.left + 20.0f, rect.top + 12.0f, palette::kText, quest.name);
        draw::Text(FontSize::Tiny, rect.left + 22.0f, rect.top + 50.0f, palette::kTextDim, quest.subtitle);

        // 難易度（★）
        std::string stars;
        for (int s = 0; s < quest.difficulty; ++s) stars += "★";
        draw::Text(FontSize::Small, rect.right - 20.0f, rect.top + 14.0f, palette::kAccentWarm, stars,
                   draw::TextAlign::Right);

        draw::Text(FontSize::Small, rect.left + 20.0f, rect.bottom - 34.0f, palette::kTextDim,
                   str::Format("推奨戦力 %d ／ %dフロア", quest.recommendedPower, quest.FloorCount()));

        if (cleared) {
            draw::Text(FontSize::Small, rect.right - 20.0f, rect.bottom - 34.0f, palette::kHp,
                       "CLEAR", draw::TextAlign::Right);
        } else if (playerPower < quest.recommendedPower) {
            draw::Text(FontSize::Small, rect.right - 20.0f, rect.bottom - 34.0f, palette::kDanger,
                       "戦力不足", draw::TextAlign::Right);
        }
    }

    // --- 詳細 ---------------------------------------------------------------
    const QuestDef* quest = QuestDatabase::Instance().Find(selectedQuestId_);
    const Rect detail(window_.left + 600.0f, window_.top + 90.0f, window_.right - 40.0f,
                      window_.bottom - 110.0f);
    draw::FillRect(detail, palette::kPanelDark, 200);
    draw::StrokeRect(detail, palette::kBorder.Scaled(0.7f), 1.0f, 180);

    if (!quest) return;

    float y = detail.top + 20.0f;
    draw::Text(FontSize::Large, detail.left + 24.0f, y, palette::kText, quest->name);
    y += 58.0f;
    draw::Text(FontSize::Small, detail.left + 24.0f, y, palette::kTextDim, quest->description);
    y += 44.0f;

    draw::Line(detail.left + 24.0f, y, detail.right - 24.0f, y, palette::kBorder, 1.0f, 120);
    y += 16.0f;

    draw::Text(FontSize::Normal, detail.left + 24.0f, y, palette::kAccent, "ボス");
    draw::Text(FontSize::Normal, detail.right - 24.0f, y, palette::kText, quest->bossName,
               draw::TextAlign::Right);
    y += 40.0f;

    draw::Text(FontSize::Normal, detail.left + 24.0f, y, palette::kAccent, "推奨戦力");
    {
        const bool enough = playerPower >= quest->recommendedPower;
        draw::Text(FontSize::Normal, detail.right - 24.0f, y, enough ? palette::kHp : palette::kDanger,
                   str::Format("%d  （現在 %d）", quest->recommendedPower, playerPower),
                   draw::TextAlign::Right);
    }
    y += 40.0f;

    draw::Text(FontSize::Normal, detail.left + 24.0f, y, palette::kAccent, "報酬");
    draw::Text(FontSize::Normal, detail.right - 24.0f, y, palette::kText,
               str::Format("%s col ／ %d EXP", str::Comma(quest->colReward).c_str(), quest->expReward),
               draw::TextAlign::Right);
    y += 48.0f;

    if (!context.player.IsQuestCleared(quest->id)) {
        draw::Text(FontSize::Small, detail.left + 24.0f, y, palette::kAccentWarm,
                   str::Format("初回クリア報酬  %s col", str::Comma(quest->firstClearCol).c_str()));
        y += 36.0f;
    }

    // --- 主なドロップ --------------------------------------------------------
    draw::Line(detail.left + 24.0f, y, detail.right - 24.0f, y, palette::kBorder, 1.0f, 120);
    y += 16.0f;
    draw::Text(FontSize::Normal, detail.left + 24.0f, y, palette::kAccent, "主なボスドロップ");
    y += 40.0f;

    const ItemDatabase& items = ItemDatabase::Instance();
    int shown = 0;
    for (const DropEntry& entry : quest->bossDrops) {
        if (shown >= 6) break;
        const ItemTemplate* tmpl = items.Find(entry.templateId);
        if (!tmpl) continue;

        const Rect row(detail.left + 24.0f, y, detail.right - 24.0f, y + 34.0f);
        DrawIvBadge(Rect(row.left, row.top + 4.0f, row.left + 48.0f, row.bottom - 4.0f),
                    entry.maxIv);
        draw::Text(FontSize::Small, row.left + 62.0f, row.top + 5.0f, palette::kText, tmpl->name);
        draw::Text(FontSize::Tiny, row.left + 62.0f + 320.0f, row.top + 8.0f, palette::kTextDim,
                   str::Format("個体値 %d〜%d", entry.minIv, entry.maxIv));
        draw::Text(FontSize::Small, row.right, row.top + 5.0f, palette::kTextDim,
                   str::Format("%.0f%%", entry.chance * 100.0f), draw::TextAlign::Right);
        y += 38.0f;
        ++shown;
    }

    startButton_.Draw();
    closeButton_.Draw();
}

} // namespace ui
} // namespace ecl
