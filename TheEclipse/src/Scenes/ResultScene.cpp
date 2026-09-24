#include "Scenes/ResultScene.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/DxInclude.h"
#include "Core/GameConfig.h"
#include "Core/Input.h"
#include "Core/SceneManager.h"
#include "Game/GameContext.h"
#include "Graphics/DrawUtil.h"

#include <cmath>

namespace ecl {

namespace {

constexpr float kScreenW = static_cast<float>(config::kScreenWidth);
constexpr float kScreenH = static_cast<float>(config::kScreenHeight);
constexpr int   kVisibleDrops = 7;
constexpr float kDropRowHeight = 68.0f;

ColorRGB RankColor(const char* rank)
{
    if (rank[0] == 'S') return ColorRGB(255, 214, 96);
    if (rank[0] == 'A') return ColorRGB(186, 130, 255);
    if (rank[0] == 'B') return ColorRGB(96, 186, 255);
    if (rank[0] == 'C') return ColorRGB(160, 170, 190);
    return ColorRGB(200, 110, 110);
}

} // namespace

ResultScene::ResultScene()
{
    homeButton_ = ui::Button(Rect::FromXYWH(kScreenW * 0.5f - 190.0f, kScreenH - 120.0f, 380.0f, 72.0f),
                             "ホームへ戻る", FontSize::Medium);
}

void ResultScene::OnEnter(GameContext& context)
{
    (void)context;
    time_ = 0.0f;
    scroll_ = 0;
}

void ResultScene::Update(float dt, GameContext& context, SceneManager& manager)
{
    time_ += dt;
    const Input& input = Input::Instance();

    const int dropCount = static_cast<int>(context.lastResult.drops.size());
    const int maxScroll = math::MaxI(0, dropCount - kVisibleDrops);
    const int wheel = input.WheelDelta();
    if (wheel != 0) scroll_ = math::ClampInt(scroll_ - wheel, 0, maxScroll);

    if (manager.IsTransitioning()) return;

    // 誤操作防止のため少し待ってから受け付ける
    if (time_ < 0.5f) return;

    if (homeButton_.Update(input, dt) || input.Pressed(GameAction::Confirm)
        || input.Pressed(GameAction::Cancel)) {
        manager.RequestChange(SceneId::Home);
    }
}

void ResultScene::Draw(GameContext& context)
{
    const QuestResult& result = context.lastResult;

    // --- 背景 ---------------------------------------------------------------
    const ColorRGB top = result.cleared ? ColorRGB(14, 20, 40) : ColorRGB(26, 12, 16);
    const ColorRGB bottom = result.cleared ? ColorRGB(30, 44, 74) : ColorRGB(42, 18, 24);
    draw::GradientRectV(Rect(0.0f, 0.0f, kScreenW, kScreenH), top, bottom, 255, 40);

    // 斜めのライン装飾
    for (int i = 0; i < 16; ++i) {
        const float x = std::fmod(static_cast<float>(i) * 180.0f + time_ * 30.0f, kScreenW + 400.0f) - 200.0f;
        draw::Line(x, 0.0f, x + 220.0f, kScreenH, palette::kWhite, 1.0f, 12);
    }

    DrawHeader(context);
    DrawStats(context);
    DrawRewards(context);

    if (time_ >= 0.5f) homeButton_.Draw();
}

void ResultScene::DrawHeader(const GameContext& context) const
{
    const QuestResult& result = context.lastResult;
    const float appear = math::Clamp(time_ / 0.4f, 0.0f, 1.0f);

    const std::string title = result.retired ? "RETIRE"
                            : (result.cleared ? "QUEST CLEAR" : "QUEST FAILED");
    const ColorRGB color = result.cleared ? palette::kAccentWarm : palette::kDanger;

    draw::TextAlpha(FontSize::Title, kScreenW * 0.5f, 60.0f, color, title,
                    static_cast<int>(appear * 255.0f), draw::TextAlign::Center);
    draw::Text(FontSize::Medium, kScreenW * 0.5f, 170.0f, palette::kText, result.questName,
               draw::TextAlign::Center);

    // 評価ランク
    if (result.cleared) {
        const float t = math::Clamp((time_ - 0.5f) / 0.4f, 0.0f, 1.0f);
        const float cx = kScreenW * 0.5f;
        const float cy = 286.0f;
        draw::Glow(cx, cy, 70.0f * t, RankColor(result.Rank()), 150, 5);
        draw::Circle(cx, cy, 58.0f, palette::kPanelDark, true, 1.0f, static_cast<int>(t * 230.0f));
        draw::Circle(cx, cy, 58.0f, RankColor(result.Rank()), false, 3.0f, static_cast<int>(t * 255.0f));
        draw::TextAlpha(FontSize::Huge, cx, cy - 38.0f, RankColor(result.Rank()), result.Rank(),
                        static_cast<int>(t * 255.0f), draw::TextAlign::Center);
        draw::TextAlpha(FontSize::Tiny, cx, cy + 62.0f, palette::kTextDim, "RANK",
                        static_cast<int>(t * 255.0f), draw::TextAlign::Center);
    }
}

void ResultScene::DrawStats(const GameContext& context) const
{
    const QuestResult& result = context.lastResult;
    const Rect panel = Rect::FromXYWH(180.0f, 380.0f, 660.0f, 500.0f);

    draw::Panel(panel, palette::kPanel, palette::kBorder, 230, 2.0f);
    draw::Text(FontSize::Medium, panel.left + 24.0f, panel.top + 14.0f, palette::kAccent, "戦闘記録");

    struct Row
    {
        const char* label;
        std::string value;
    };

    const Row rows[] = {
        { "クリアタイム",   str::TimeText(result.clearTime) },
        { "到達フロア",     str::Format("%d / %d", math::MaxI(result.floorsCleared, 0), result.floorCount) },
        { "撃破数",         str::Format("%d 体", result.enemiesDefeated) },
        { "最大コンボ",     str::Format("%d HIT", result.maxCombo) },
        { "パリィ成功",     str::Format("%d 回", result.parryCount) },
        { "与ダメージ",     str::Comma(result.totalDamage) },
        { "被ダメージ",     str::Comma(result.damageTaken) },
    };

    float y = panel.top + 80.0f;
    int index = 0;
    for (const Row& row : rows) {
        const float appear = math::Clamp((time_ - 0.3f - static_cast<float>(index) * 0.08f) / 0.3f,
                                         0.0f, 1.0f);
        const int alpha = static_cast<int>(appear * 255.0f);

        draw::FillRect(Rect(panel.left + 20.0f, y - 6.0f, panel.right - 20.0f, y + 44.0f),
                       palette::kPanelDark, static_cast<int>(appear * 140.0f));
        draw::TextAlpha(FontSize::Normal, panel.left + 36.0f, y, palette::kTextDim, row.label, alpha);
        draw::TextAlpha(FontSize::Normal, panel.right - 36.0f, y, palette::kText, row.value, alpha,
                        draw::TextAlign::Right);
        y += 56.0f;
        ++index;
    }

    // ユニークスキルの解放通知
    if (result.unlockedUniqueSkill) {
        const float pulse = 0.6f + 0.4f * std::sin(time_ * 6.0f);
        const Rect notice(panel.left + 20.0f, panel.bottom - 132.0f, panel.right - 20.0f,
                          panel.bottom - 76.0f);
        draw::FillRect(notice, palette::kExp.Scaled(0.35f), 220);
        draw::StrokeRect(notice, palette::kExp, 2.0f, static_cast<int>(230.0f * pulse));
        draw::Text(FontSize::Small, notice.CenterX(), notice.top + 6.0f, palette::kExp,
                   str::Format("ユニークスキル「%s」が解放可能に！",
                               result.unlockedUniqueSkillName.c_str()),
                   draw::TextAlign::Center);
        draw::Text(FontSize::Tiny, notice.CenterX(), notice.top + 32.0f, palette::kText,
                   str::Format("「スキル」タブ →「%s」の先頭スキルを解放すると習得",
                               result.unlockedUniqueSkillName.c_str()),
                   draw::TextAlign::Center);
    }

    // レベルアップ表示
    if (result.levelsGained > 0) {
        const float pulse = 0.7f + 0.3f * std::sin(time_ * 6.0f);
        draw::Text(FontSize::Medium, panel.CenterX(), panel.bottom - 66.0f, palette::kExp,
                   str::Format("LEVEL UP!  +%d  →  Lv %d", result.levelsGained,
                               context.player.Level()),
                   draw::TextAlign::Center);
        draw::Glow(panel.CenterX(), panel.bottom - 44.0f, 120.0f * pulse, palette::kExp, 70, 4);
    }
}

void ResultScene::DrawRewards(const GameContext& context) const
{
    const QuestResult& result = context.lastResult;
    const Rect panel = Rect::FromXYWH(900.0f, 380.0f, 840.0f, 500.0f);

    draw::Panel(panel, palette::kPanel, palette::kBorder, 230, 2.0f);
    draw::Text(FontSize::Medium, panel.left + 24.0f, panel.top + 14.0f, palette::kAccent, "報酬");

    // 通貨・経験値
    float y = panel.top + 76.0f;
    draw::Text(FontSize::Normal, panel.left + 32.0f, y, palette::kTextDim, "獲得 col");
    draw::Text(FontSize::Normal, panel.left + 360.0f, y, palette::kAccentWarm,
               str::Comma(result.colGained), draw::TextAlign::Right);

    draw::Text(FontSize::Normal, panel.left + 430.0f, y, palette::kTextDim, "獲得 EXP");
    draw::Text(FontSize::Normal, panel.right - 32.0f, y, palette::kExp,
               str::Comma(result.expGained), draw::TextAlign::Right);
    y += 42.0f;

    draw::Text(FontSize::Small, panel.left + 32.0f, y, palette::kTextDim,
               str::Format("強化結晶  +%d", result.materialGained));
    if (result.skillPointsGained > 0) {
        draw::Text(FontSize::Small, panel.left + 280.0f, y, palette::kExp,
                   (result.abilityPointsGained > 0)
                       ? str::Format("スキルP +%d ／ ステータスP +%d", result.skillPointsGained,
                                     result.abilityPointsGained)
                       : str::Format("スキルポイント  +%d", result.skillPointsGained));
    }
    if (result.firstClear) {
        draw::Text(FontSize::Small, panel.right - 32.0f, y, palette::kAccentWarm,
                   "初回クリアボーナス獲得！", draw::TextAlign::Right);
    }
    y += 44.0f;

    draw::Line(panel.left + 24.0f, y, panel.right - 24.0f, y, palette::kBorder, 1.0f, 140);
    y += 14.0f;

    // --- ドロップ -----------------------------------------------------------
    draw::Text(FontSize::Normal, panel.left + 32.0f, y, palette::kAccent,
               str::Format("ドロップ装備  %d 個", static_cast<int>(result.drops.size())));
    y += 42.0f;

    // --- 耐久力が尽きて壊れた装備 ---------------------------------------------
    if (!result.brokenItems.empty()) {
        const float pulse = 0.65f + 0.35f * std::sin(time_ * 5.0f);
        const Rect notice(panel.left + 28.0f, y - 6.0f, panel.right - 28.0f,
                          y + 30.0f + 26.0f * static_cast<float>(result.brokenItems.size()));
        draw::FillRect(notice, palette::kDanger.Scaled(0.30f), 210);
        draw::StrokeRect(notice, palette::kDanger, 2.0f, static_cast<int>(220.0f * pulse));
        draw::Text(FontSize::Small, notice.left + 12.0f, notice.top + 6.0f, palette::kDanger,
                   "耐久力が尽きて失われた装備");

        float noticeY = notice.top + 32.0f;
        for (const std::string& name : result.brokenItems) {
            draw::Text(FontSize::Small, notice.left + 26.0f, noticeY, palette::kText,
                       str::Format("・%s", name.c_str()));
            noticeY += 26.0f;
        }
        y = notice.bottom + 14.0f;
    }

    if (result.drops.empty()) {
        draw::Text(FontSize::Normal, panel.CenterX(), y + 40.0f, palette::kTextDisabled,
                   "ドロップはありませんでした", draw::TextAlign::Center);
        return;
    }

    for (int row = 0; row < kVisibleDrops; ++row) {
        const int index = scroll_ + row;
        if (index >= static_cast<int>(result.drops.size())) break;

        const EquipmentItem& item = result.drops[static_cast<size_t>(index)];
        const float rowY = y + kDropRowHeight * static_cast<float>(row);
        const Rect rect(panel.left + 28.0f, rowY, panel.right - 28.0f, rowY + kDropRowHeight - 8.0f);

        const float appear = math::Clamp((time_ - 0.6f - static_cast<float>(row) * 0.1f) / 0.25f,
                                         0.0f, 1.0f);
        if (appear <= 0.0f) continue;

        ui::DrawItemRow(rect, item, false, false, false);

        // 特に戦力の高い装備は光らせる（個体値そのものは表示しない）
        if (item.iv >= 70) {
            const float pulse = 0.6f + 0.4f * std::sin(time_ * 5.0f + static_cast<float>(row));
            draw::StrokeRect(rect.Expanded(3.0f), palette::kAccentWarm, 2.0f,
                             static_cast<int>(180.0f * pulse));
        }
    }

    if (static_cast<int>(result.drops.size()) > kVisibleDrops) {
        draw::Text(FontSize::Tiny, panel.right - 28.0f, panel.bottom - 34.0f, palette::kTextDim,
                   str::Format("ホイールでスクロール（%d / %d）",
                               scroll_ + kVisibleDrops > static_cast<int>(result.drops.size())
                                   ? static_cast<int>(result.drops.size())
                                   : scroll_ + kVisibleDrops,
                               static_cast<int>(result.drops.size())),
                   draw::TextAlign::Right);
    }
}

} // namespace ecl
