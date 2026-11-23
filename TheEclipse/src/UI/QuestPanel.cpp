#include "UI/QuestPanel.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/GameConfig.h"
#include "Game/ItemDatabase.h"
#include "Graphics/DrawUtil.h"

#include <utility>
#include <vector>

namespace ecl {
namespace ui {

namespace {
// 一覧に出すクエスト（特別クエストはスキルツリーから挑むので除く）
//   開発者モードではドロップ率を確かめられるよう、特別クエストも末尾に並べる。
std::vector<const QuestDef*> ListedQuests(bool includeSpecial)
{
    std::vector<const QuestDef*> list;
    for (const QuestDef& quest : QuestDatabase::Instance().Quests()) {
        if (quest.special) continue;
        list.push_back(&quest);
    }
    if (includeSpecial) {
        for (const QuestDef& quest : QuestDatabase::Instance().Quests()) {
            if (quest.special) list.push_back(&quest);
        }
    }
    return list;
}

// 確率の表示（小さい値ほど桁を多く出す）
std::string PercentText(float probability)
{
    const float percent = probability * 100.0f;
    if (percent >= 99.95f) return "100%";
    if (percent >= 10.0f) return str::Format("%.1f%%", percent);
    return str::Format("%.2f%%", percent);
}
} // namespace

QuestPanel::QuestPanel()
{
    Layout();
}

void QuestPanel::Layout()
{
    window_ = Rect::FromXYWH(200.0f, 110.0f, 1520.0f, 860.0f);

    questButtons_.clear();

    // 開発者モードで特別クエストも並べられるよう、全クエスト分のボタンを用意する。
    //   クエストが増えても枠（下端の 40 px 上まで）に収まるよう、入りきらないときは間隔を詰める
    const size_t count = ListedQuests(true).size();
    const float top = window_.top + 90.0f;
    const float room = window_.bottom - 40.0f - top;
    const float step = math::MinF(140.0f, room / static_cast<float>(math::MaxI(1, static_cast<int>(count))));
    float y = top;
    for (size_t i = 0; i < count; ++i) {
        questButtons_.push_back(Button(Rect::FromXYWH(window_.left + 40.0f, y, 520.0f, step - 16.0f),
                                       "", FontSize::Normal));
        y += step;
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
    // 特別クエストは開発者モードでしか一覧に出ないので、通常は先頭を選び直す
    const QuestDef* quest = QuestDatabase::Instance().Find(selectedQuestId_);
    if (!quest || (quest->special && !context.settings.debugMode)) {
        const std::vector<const QuestDef*> quests = ListedQuests(false);
        if (!quests.empty()) selectedQuestId_ = quests.front()->id;
    }
}

void QuestPanel::Update(float dt, const Input& input, GameContext& context)
{
    if (!open_) return;

    closeRequested_ = false;
    startRequested_ = false;

    const std::vector<const QuestDef*> quests = ListedQuests(context.settings.debugMode);
    for (size_t i = 0; i < questButtons_.size() && i < quests.size(); ++i) {
        questButtons_[i].SetSelected(quests[i]->id == selectedQuestId_);
        if (!questButtons_[i].Update(input, dt)) continue;
        selectedQuestId_ = quests[i]->id;
        // 特別クエストはスキルツリーから挑むので、出撃先には選ばない
        if (!quests[i]->special) context.selectedQuestId = selectedQuestId_;
    }

    // 特別クエスト（開発者モードで表示）はここからは出撃できない
    const QuestDef* selected = QuestDatabase::Instance().Find(selectedQuestId_);
    startButton_.SetEnabled(selected && !selected->special);

    const bool start = startButton_.Update(input, dt) || input.Pressed(GameAction::Confirm);
    if (start && startButton_.Enabled()) {
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

    const std::vector<const QuestDef*> quests = ListedQuests(context.settings.debugMode);
    const int playerPower = context.player.Power();

    // --- 一覧 ---------------------------------------------------------------
    for (size_t i = 0; i < questButtons_.size() && i < quests.size(); ++i) {
        const QuestDef& quest = *quests[i];
        const Rect rect = questButtons_[i].GetRect();
        const bool selected = (quest.id == selectedQuestId_);
        const bool cleared = context.player.IsQuestCleared(quest.id);

        const ColorRGB base = selected ? palette::kPanelLight : palette::kPanelDark;
        draw::GradientRectH(rect, base, base.Scaled(0.7f), 235, 14);
        draw::StrokeRect(rect, selected ? palette::kAccent : palette::kBorder.Scaled(0.7f),
                         selected ? 3.0f : 1.0f, 255);

        draw::Text(FontSize::Medium, rect.left + 20.0f, rect.top + 12.0f, palette::kText, quest.name);
        draw::Text(FontSize::Tiny, rect.left + 22.0f, rect.top + 50.0f, palette::kTextDim, quest.subtitle);
        // 開発者モード：ボスの HP バーの本数（推奨戦力から自動で決まる）
        if (context.settings.debugMode) {
            draw::Text(FontSize::Tiny, rect.right - 20.0f, rect.top + 16.0f, palette::kAccentWarm,
                       str::Format("ボス HP バー %d 本", QuestDatabase::BossHpBarCount(quest)),
                       draw::TextAlign::Right);
        }

        draw::Text(FontSize::Small, rect.left + 20.0f, rect.bottom - 34.0f, palette::kTextDim,
                   (quest.floorLevel > 0)
                       ? str::Format("第%d層 ／ 推奨戦力 %s ／ %dフロア", quest.floorLevel,
                                     str::Comma(quest.recommendedPower).c_str(), quest.FloorCount())
                       : str::Format("推奨戦力 %s ／ %dフロア",
                                     str::Comma(quest.recommendedPower).c_str(), quest.FloorCount()));

        if (quest.special) {
            draw::Text(FontSize::Small, rect.right - 20.0f, rect.bottom - 34.0f, palette::kExp,
                       "特別", draw::TextAlign::Right);
        } else if (cleared) {
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

    draw::Text(FontSize::Normal, detail.left + 24.0f, y, palette::kAccent,
               (quest->floorLevel > 0) ? str::Format("推奨戦力（第%d層）", quest->floorLevel) : std::string("推奨戦力"));
    {
        const bool enough = playerPower >= quest->recommendedPower;
        draw::Text(FontSize::Normal, detail.right - 24.0f, y, enough ? palette::kHp : palette::kDanger,
                   str::Format("%s  （現在 %s）", str::Comma(quest->recommendedPower).c_str(),
                               str::Comma(playerPower).c_str()),
                   draw::TextAlign::Right);
    }
    y += 40.0f;

    draw::Text(FontSize::Normal, detail.left + 24.0f, y, palette::kAccent, "報酬");
    draw::Text(FontSize::Normal, detail.right - 24.0f, y, palette::kText,
               str::Format("%s col ／ %s EXP", str::Comma(quest->colReward).c_str(),
                           str::Comma(quest->expReward).c_str()),
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
    // 開発者モード：伏せているクエストも含め、全候補の実際の確率を出す
    if (context.settings.debugMode) {
        DrawDebugDrops(context, *quest, detail, y);
        startButton_.Draw();
        closeButton_.Draw();
        return;
    }

    draw::Text(FontSize::Normal, detail.left + 24.0f, y, palette::kAccent, "主なボスドロップ");
    y += 40.0f;

    // 中身を伏せるクエストは「不明」とだけ出す
    if (quest->hideDrops) {
        draw::Text(FontSize::Small, detail.left + 32.0f, y + 5.0f, palette::kTextDisabled, "不明");
        startButton_.Draw();
        closeButton_.Draw();
        return;
    }

    // 6 件までは 1 列、それより多いとき（世界終焉の玉座は 11 件）は 2 列に並べる（最大 12 件）
    const ItemDatabase& items = ItemDatabase::Instance();
    std::vector<std::pair<const ItemTemplate*, float>> drops;
    for (const DropEntry& entry : quest->bossDrops) {
        if (const ItemTemplate* tmpl = items.Find(entry.templateId)) drops.emplace_back(tmpl, entry.chance);
    }
    constexpr int kRowsPerColumn = 6;
    const int columns = (static_cast<int>(drops.size()) > kRowsPerColumn) ? 2 : 1;
    const float columnGap = 32.0f;
    const float columnWidth = (detail.Width() - 48.0f - columnGap * static_cast<float>(columns - 1))
                            / static_cast<float>(columns);
    const int rows = (static_cast<int>(drops.size()) + columns - 1) / columns;
    for (size_t i = 0; i < drops.size() && static_cast<int>(i) < kRowsPerColumn * columns; ++i) {
        const int column = static_cast<int>(i) / math::MaxI(1, rows);
        const int rowIndex = static_cast<int>(i) % math::MaxI(1, rows);
        const float left = detail.left + 24.0f + (columnWidth + columnGap) * static_cast<float>(column);
        const Rect row(left, y + 38.0f * static_cast<float>(rowIndex), left + columnWidth,
                       y + 38.0f * static_cast<float>(rowIndex) + 34.0f);
        draw::Text(FontSize::Small, row.left + 8.0f, row.top + 5.0f, palette::kText, drops[i].first->name);
        draw::Text(FontSize::Small, row.right, row.top + 5.0f, palette::kTextDim,
                   str::Format("%.0f%%", drops[i].second * 100.0f), draw::TextAlign::Right);
    }

    startButton_.Draw();
    closeButton_.Draw();
}

void QuestPanel::DrawDebugDrops(const GameContext& context, const QuestDef& quest,
                                const Rect& detail, float y) const
{
    const float left = detail.left + 24.0f;
    const float right = detail.right - 24.0f;

    draw::Text(FontSize::Normal, left, y, palette::kAccentWarm, "ボスドロップ率（開発者）");
    if (quest.hideDrops) {
        draw::Text(FontSize::Tiny, right, y + 8.0f, palette::kTextDim, "通常の表示は「不明」",
                   draw::TextAlign::Right);
    }
    y += 36.0f;

    // 抽選のしかた（どちらの確率が次のクリアで使われるか）
    const float rate = context.player.DropRateMultiplier();
    const bool firstNext = !quest.special && !context.player.IsQuestCleared(quest.id);
    std::string rule;
    if (quest.special) {
        rule = "全候補を個別に抽選（全部外れたら先頭を確定）";
    } else {
        rule = "部位を 1 つ抽選 → その部位の候補を上から抽選";
    }
    draw::Text(FontSize::Tiny, left, y, palette::kTextDim,
               str::Format("LUK 補正 ×%.2f 込み ／ %s", rate, rule.c_str()));
    y += 24.0f;
    if (!quest.special) {
        draw::Text(FontSize::Tiny, left, y, firstNext ? palette::kAccentWarm : palette::kTextDim,
                   firstNext ? "次回は初回クリア：候補から 1 つが必ず落ちる（右の「初回」）"
                             : "初回クリア済み：次回からは左の「通常」の確率");
        y += 28.0f;
    }

    const std::vector<BossDropOdds> odds = QuestDatabase::Instance().CalcBossDropOdds(quest, rate);
    if (odds.empty()) {
        draw::Text(FontSize::Small, left + 8.0f, y + 4.0f, palette::kTextDisabled, "ボスドロップなし");
        return;
    }

    // 2 列で並べる（最大 9 件程度を想定）
    const float columnGap = 16.0f;
    const float columnWidth = (right - left - columnGap) * 0.5f;
    const float rowHeight = 30.0f;
    const int rowsPerColumn = static_cast<int>((odds.size() + 1) / 2);
    const ItemDatabase& items = ItemDatabase::Instance();
    float total = 0.0f;

    for (size_t i = 0; i < odds.size(); ++i) {
        const BossDropOdds& o = odds[i];
        const int column = static_cast<int>(i) / rowsPerColumn;
        const int row = static_cast<int>(i) % rowsPerColumn;
        const float x = left + (columnWidth + columnGap) * static_cast<float>(column);
        const float rowY = y + rowHeight * static_cast<float>(row);

        const ItemTemplate* tmpl = items.Find(o.templateId);
        draw::FillRect(Rect(x, rowY, x + columnWidth, rowY + rowHeight - 4.0f), palette::kPanel,
                       (row % 2 == 0) ? 150 : 90);
        draw::Text(FontSize::Tiny, x + 8.0f, rowY + 5.0f, palette::kText, tmpl ? tmpl->name : "???");

        const float normal = o.normal;
        if (quest.special) {
            draw::Text(FontSize::Tiny, x + columnWidth - 8.0f, rowY + 5.0f, palette::kAccentWarm,
                       PercentText(normal), draw::TextAlign::Right);
        } else {
            draw::Text(FontSize::Tiny, x + columnWidth - 8.0f, rowY + 5.0f,
                       firstNext ? palette::kAccentWarm : palette::kTextDim,
                       str::Format("初回 %s", PercentText(o.firstClear).c_str()),
                       draw::TextAlign::Right);
            draw::Text(FontSize::Tiny, x + columnWidth - 116.0f, rowY + 5.0f,
                       firstNext ? palette::kTextDim : palette::kAccentWarm,
                       str::Format("通常 %s", PercentText(normal).c_str()),
                       draw::TextAlign::Right);
        }
        total += firstNext ? o.firstClear : normal;
    }

    // 1 回の撃破で何か落ちる確率（特別クエストは複数落ちるので期待個数）
    y += rowHeight * static_cast<float>(rowsPerColumn) + 6.0f;
    draw::Text(FontSize::Tiny, right, y, palette::kTextDim,
               quest.special ? str::Format("1 回で落ちる数の期待値 %.2f 個", total)
                             : str::Format("次のクリアで何か 1 つ落ちる確率 %s",
                                           PercentText(total).c_str()),
               draw::TextAlign::Right);
}

} // namespace ui
} // namespace ecl
