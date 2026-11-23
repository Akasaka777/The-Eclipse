#include "Scenes/ResultScene.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/DxInclude.h"
#include "Core/GameConfig.h"
#include "Core/Input.h"
#include "Core/SceneManager.h"
#include "Game/Consumable.h"
#include "Game/GameContext.h"
#include "Game/QuestDatabase.h"
#include "Game/SwordSkill.h"
#include "Game/UniqueSkill.h"
#include "Graphics/DrawUtil.h"

#include <cmath>

namespace ecl {

// 報酬パネルの下半分：左に「ドロップ装備」、右に「素材」を並べる
struct ResultRewardLayout
{
    Rect  dropColumn;       // ドロップ装備の列（見出しから下）
    Rect  materialColumn;   // 素材の列（見出しから下）
    float headerY = 0.0f;   // 見出しの行
    float listTop = 0.0f;   // 一覧（または壊れた装備のお知らせ）の始まり
    float dropTop = 0.0f;   // ドロップ装備の 1 行目
    int   visibleDrops = 0; // 一度に見せるドロップ装備の行数
};

namespace {

constexpr float kScreenW = static_cast<float>(config::kScreenWidth);
constexpr float kScreenH = static_cast<float>(config::kScreenHeight);
constexpr float kDropRowHeight = 64.0f;
constexpr float kMaterialRowHeight = 44.0f;

// 左右のパネル（戦闘記録 / 報酬）。下の「ホームへ戻る」ボタンに重ならない高さにする
const Rect kStatsPanel = Rect::FromXYWH(120.0f, 372.0f, 600.0f, 556.0f);
const Rect kRewardPanel = Rect::FromXYWH(760.0f, 372.0f, 1040.0f, 556.0f);

constexpr float kBrokenLineHeight = 26.0f;

ResultRewardLayout MakeRewardLayout(const QuestResult& result)
{
    ResultRewardLayout l;
    const Rect& panel = kRewardPanel;
    l.headerY = panel.top + 160.0f;
    l.listTop = l.headerY + 40.0f;
    constexpr float kMaterialWidth = 280.0f;
    constexpr float kColumnGap = 20.0f;
    const float inner = panel.left + 28.0f;
    const float right = panel.right - 28.0f;
    l.materialColumn = Rect(right - kMaterialWidth, l.headerY, right, panel.bottom - 16.0f);
    l.dropColumn = Rect(inner, l.headerY, l.materialColumn.left - kColumnGap, panel.bottom - 16.0f);

    // 耐久力が尽きて壊れた装備があるときは、そのお知らせのぶん一覧を下げる
    l.dropTop = l.listTop;
    if (!result.brokenItems.empty()) {
        l.dropTop += 44.0f + kBrokenLineHeight * static_cast<float>(result.brokenItems.size());
    }
    l.visibleDrops = math::MaxI(1, static_cast<int>((l.dropColumn.bottom - l.dropTop + 8.0f) / kDropRowHeight));
    return l;
}

// 開放・習得のお知らせ：リザルトの表示がひととおり出そろってから出す（秒）
constexpr float kAnnounceDelay = 1.0f;
constexpr float kAnnounceFade = 0.35f;
// 出してすぐの誤操作で閉じないよう、少し待ってから受け付ける（秒）
constexpr float kAnnounceInputDelay = 0.5f;

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

    announceCard_ = Rect::FromCenter(kScreenW * 0.5f, kScreenH * 0.5f, 1000.0f, 560.0f);
    announceButton_ = ui::Button(Rect::FromXYWH(announceCard_.CenterX() - 130.0f, announceCard_.bottom - 92.0f,
                                                260.0f, 60.0f),
                                 "確認", FontSize::Medium);
    announceButton_.SetAccent(palette::kExp);
}

void ResultScene::OnEnter(GameContext& context)
{
    time_ = 0.0f;
    scroll_ = 0;
    BuildAnnouncements(context);
}

void ResultScene::BuildAnnouncements(const GameContext& context)
{
    announcements_.clear();
    announceIndex_ = 0;
    announceTimer_ = 0.0f;

    const QuestResult& result = context.lastResult;

    // --- ユニークスキルの解放 ----------------------------------------------------
    if (result.unlockedUniqueSkill) {
        const UniqueSkillDef* def = UniqueSkillDatabase::Instance().Find(result.unlockedUniqueSkillType);
        Announcement a;
        a.tag = "ユニークスキル開放";
        a.name = def ? def->name : result.unlockedUniqueSkillName;
        a.color = palette::kExp;
        if (def) {
            a.description = def->description;
            a.details = def->details;
            a.condition = str::Format("達成した条件 : %s", def->unlockCondition.c_str());
            if (def->HasDedicatedSkills()) {
                const SwordSkill* first = SkillDatabase::Instance().Find(def->skillIds.front());
                a.guide = str::Format("ホーム →「プレイヤー」→「スキル」→「%s」の「%s」を解放すると習得",
                                      def->name.c_str(), first ? first->name.c_str() : "先頭のスキル");
            } else {
                a.guide = str::Format("ホーム →「プレイヤー」→「スキル」→「%s」の習得ボタンで習得",
                                      def->name.c_str());
            }
        } else {
            a.guide = "ホーム →「プレイヤー」→「スキル」から習得できます";
        }
        announcements_.push_back(a);
    }
}

bool ResultScene::AnnouncementShown() const
{
    return time_ >= kAnnounceDelay && announceIndex_ < static_cast<int>(announcements_.size());
}

void ResultScene::Update(float dt, GameContext& context, SceneManager& manager)
{
    time_ += dt;
    const Input& input = Input::Instance();

    const int dropCount = static_cast<int>(context.lastResult.drops.size());
    const int maxScroll = math::MaxI(0, dropCount - MakeRewardLayout(context.lastResult).visibleDrops);
    const int wheel = input.WheelDelta();
    if (wheel != 0) scroll_ = math::ClampInt(scroll_ - wheel, 0, maxScroll);

    if (manager.IsTransitioning()) return;

    // 開放・習得のお知らせを確認するまでは、ほかの操作を受け付けない
    if (AnnouncementShown()) {
        announceTimer_ += dt;
        const bool ready = announceTimer_ >= kAnnounceInputDelay;
        if ((announceButton_.Update(input, dt) && ready)
            || (ready && (input.Pressed(GameAction::Confirm) || input.Pressed(GameAction::Cancel)))) {
            ++announceIndex_;
            announceTimer_ = 0.0f;
        }
        return;
    }

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

    // 開放・習得のお知らせは最前面（画面を薄暗くして中央に出す）
    if (AnnouncementShown()) DrawAnnouncement();
}

void ResultScene::DrawAnnouncement() const
{
    const Announcement& a = announcements_[static_cast<size_t>(announceIndex_)];
    // 1 件目は暗転から、2 件目以降は札だけを入れ替える
    const float since = (announceIndex_ == 0) ? time_ - kAnnounceDelay : announceTimer_;
    const float appear = math::Clamp(since / kAnnounceFade, 0.0f, 1.0f);
    const int alpha = static_cast<int>(appear * 255.0f);

    // 画面全体を薄暗くする
    draw::FillRect(Rect(0.0f, 0.0f, kScreenW, kScreenH), palette::kBlack, static_cast<int>(appear * 175.0f));

    // 中央の札（少し下から浮き上がる）
    const Rect card = announceCard_.Offset(0.0f, (1.0f - appear) * 30.0f);
    const float pulse = 0.6f + 0.4f * std::sin(time_ * 3.0f);
    draw::Glow(card.CenterX(), card.top + 128.0f, 260.0f * appear, a.color, static_cast<int>(70.0f * pulse), 5);
    draw::Panel(card, palette::kPanel, a.color, static_cast<int>(appear * 240.0f), 3.0f);

    const float cx = card.CenterX();
    float y = card.top + 34.0f;

    // 見出し（「― ユニークスキル開放 ―」）
    const float tagWidth = static_cast<float>(draw::TextWidth(FontSize::Normal, a.tag));
    draw::TextAlpha(FontSize::Normal, cx, y, a.color, a.tag, alpha, draw::TextAlign::Center);
    draw::Line(cx - tagWidth * 0.5f - 150.0f, y + 14.0f, cx - tagWidth * 0.5f - 20.0f, y + 14.0f, a.color, 2.0f, alpha);
    draw::Line(cx + tagWidth * 0.5f + 20.0f, y + 14.0f, cx + tagWidth * 0.5f + 150.0f, y + 14.0f, a.color, 2.0f, alpha);
    y += 48.0f;

    // 名前
    draw::TextAlpha(FontSize::Huge, cx, y, palette::kWhite, a.name, alpha, draw::TextAlign::Center);
    y += 84.0f;

    if (!a.description.empty()) {
        draw::TextAlpha(FontSize::Small, cx, y, palette::kText, a.description, alpha, draw::TextAlign::Center);
        y += 36.0f;
    }
    draw::Line(card.left + 80.0f, y, card.right - 80.0f, y, palette::kBorder, 1.0f, alpha / 2);
    y += 16.0f;

    // 効果（2 列に並べる）
    const size_t half = (a.details.size() + 1) / 2;
    for (size_t i = 0; i < a.details.size(); ++i) {
        const float x = (i < half) ? card.left + 110.0f : cx + 30.0f;
        const float rowY = y + 30.0f * static_cast<float>(i < half ? i : i - half);
        draw::TextAlpha(FontSize::Small, x, rowY, palette::kTextDim, a.details[i], alpha);
    }
    y += 30.0f * static_cast<float>(half) + 18.0f;

    if (!a.condition.empty()) {
        draw::TextAlpha(FontSize::Small, cx, y, palette::kAccentWarm, a.condition, alpha, draw::TextAlign::Center);
        y += 34.0f;
    }
    draw::TextAlpha(FontSize::Small, cx, y, palette::kAccent, a.guide, alpha, draw::TextAlign::Center);

    // 確認ボタン（何件目か）
    if (announceTimer_ >= kAnnounceInputDelay * 0.5f || since >= kAnnounceFade) announceButton_.Draw();
    if (announcements_.size() > 1) {
        draw::TextAlpha(FontSize::Tiny, card.right - 24.0f, card.bottom - 30.0f, palette::kTextDim,
                        str::Format("%d / %d", announceIndex_ + 1, static_cast<int>(announcements_.size())), alpha,
                        draw::TextAlign::Right);
    }
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
    const Rect& panel = kStatsPanel;

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
    const Rect& panel = kRewardPanel;
    const ResultRewardLayout layout = MakeRewardLayout(result);

    draw::Panel(panel, palette::kPanel, palette::kBorder, 230, 2.0f);
    draw::Text(FontSize::Medium, panel.left + 24.0f, panel.top + 14.0f, palette::kAccent, "報酬");

    // 通貨・経験値
    float y = panel.top + 70.0f;
    draw::Text(FontSize::Normal, panel.left + 32.0f, y, palette::kTextDim, "獲得 col");
    draw::Text(FontSize::Normal, panel.left + 440.0f, y, palette::kAccentWarm,
               str::Comma(result.colGained), draw::TextAlign::Right);

    draw::Text(FontSize::Normal, panel.left + 540.0f, y, palette::kTextDim, "獲得 EXP");
    draw::Text(FontSize::Normal, panel.right - 32.0f, y, palette::kExp,
               str::Comma(result.expGained), draw::TextAlign::Right);
    y += 40.0f;

    // LUK で増えたときは、獲得 EXP の下に倍率を出す
    if (result.expRate > 1.0001f) {
        draw::Text(FontSize::Tiny, panel.right - 32.0f, y + 2.0f, palette::kTextDim,
                   str::Format("LUK 補正 ×%.2f 込み", result.expRate), draw::TextAlign::Right);
    }

    draw::Text(FontSize::Small, panel.left + 32.0f, y, palette::kTextDim,
               str::Format("強化結晶  +%d", result.materialGained));
    if (result.skillPointsGained > 0) {
        draw::Text(FontSize::Small, panel.left + 280.0f, y, palette::kExp,
                   (result.abilityPointsGained > 0)
                       ? str::Format("スキルP +%d ／ ステータスP +%d", result.skillPointsGained,
                                     result.abilityPointsGained)
                       : str::Format("スキルポイント  +%d", result.skillPointsGained));
    }

    const float ruleY = layout.headerY - 12.0f;
    draw::Line(panel.left + 24.0f, ruleY, panel.right - 24.0f, ruleY, palette::kBorder, 1.0f, 140);
    // 2 つの列の仕切り
    const float splitX = (layout.dropColumn.right + layout.materialColumn.left) * 0.5f;
    draw::Line(splitX, ruleY + 12.0f, splitX, layout.dropColumn.bottom, palette::kBorder, 1.0f, 110);

    DrawDropColumn(result, layout);
    DrawMaterialColumn(result, layout);
}

void ResultScene::DrawDropColumn(const QuestResult& result, const ResultRewardLayout& layout) const
{
    const Rect& column = layout.dropColumn;
    const int dropCount = static_cast<int>(result.drops.size());
    draw::Text(FontSize::Normal, column.left + 4.0f, layout.headerY, palette::kAccent,
               str::Format("ドロップ装備  %d 個", dropCount));
    // 入りきらないときは、見出しの右に今の位置を出す
    if (dropCount > layout.visibleDrops) {
        draw::Text(FontSize::Tiny, column.right, layout.headerY + 8.0f, palette::kTextDim,
                   str::Format("ホイールでスクロール（%d / %d）",
                               math::MinI(scroll_ + layout.visibleDrops, dropCount), dropCount),
                   draw::TextAlign::Right);
    }

    // --- 耐久力が尽きて壊れた装備 ---------------------------------------------
    if (!result.brokenItems.empty()) {
        const float pulse = 0.65f + 0.35f * std::sin(time_ * 5.0f);
        const Rect notice(column.left, layout.listTop - 6.0f, column.right,
                          layout.dropTop - 14.0f);
        draw::FillRect(notice, palette::kDanger.Scaled(0.30f), 210);
        draw::StrokeRect(notice, palette::kDanger, 2.0f, static_cast<int>(220.0f * pulse));
        draw::Text(FontSize::Small, notice.left + 12.0f, notice.top + 6.0f, palette::kDanger,
                   "耐久力が尽きて失われた装備");

        float noticeY = notice.top + 32.0f;
        for (const std::string& name : result.brokenItems) {
            draw::Text(FontSize::Small, notice.left + 26.0f, noticeY, palette::kText,
                       str::Format("・%s", name.c_str()));
            noticeY += kBrokenLineHeight;
        }
    }

    if (result.drops.empty()) {
        draw::Text(FontSize::Normal, column.CenterX(), layout.dropTop + 40.0f, palette::kTextDisabled,
                   "ドロップはありませんでした", draw::TextAlign::Center);
        return;
    }

    // 初回クリアでは、先頭のドロップが「主なボスドロップから確定で 1 つ」の品
    const QuestDef* quest = QuestDatabase::Instance().Find(result.questId);
    const bool guaranteedFirst = result.firstClear && quest && !quest->special;

    for (int row = 0; row < layout.visibleDrops; ++row) {
        const int index = scroll_ + row;
        if (index >= dropCount) break;

        const EquipmentItem& item = result.drops[static_cast<size_t>(index)];
        const float rowY = layout.dropTop + kDropRowHeight * static_cast<float>(row);
        const Rect rect(column.left, rowY, column.right, rowY + kDropRowHeight - 8.0f);

        const float appear = math::Clamp((time_ - 0.6f - static_cast<float>(row) * 0.1f) / 0.25f,
                                         0.0f, 1.0f);
        if (appear <= 0.0f) continue;

        ui::DrawItemRow(rect, item, false, false, false);

        // 初回クリア報酬の確定ドロップ（ボスドロップは先頭に並ぶ）。戦力の数字の左に付ける
        if (index == 0 && guaranteedFirst) {
            const float powerWidth = static_cast<float>(
                draw::TextWidth(FontSize::Small, str::Format("戦力 %d", item.Power())));
            const float badgeRight = rect.right - 12.0f - powerWidth - 14.0f;
            const Rect badge(badgeRight - 112.0f, rect.top + 9.0f, badgeRight, rect.top + 33.0f);
            draw::FillRect(badge, palette::kAccentWarm.Scaled(0.35f), 230);
            draw::StrokeRect(badge, palette::kAccentWarm, 1.0f, 230);
            draw::Text(FontSize::Tiny, badge.CenterX(), badge.top + 4.0f, palette::kAccentWarm,
                       "初回クリア報酬", draw::TextAlign::Center);
        }

        // 特に戦力の高い装備は光らせる（個体値そのものは表示しない）
        if (item.iv >= 70) {
            const float pulse = 0.6f + 0.4f * std::sin(time_ * 5.0f + static_cast<float>(row));
            draw::StrokeRect(rect.Expanded(3.0f), palette::kAccentWarm, 2.0f,
                             static_cast<int>(180.0f * pulse));
        }
    }
}

void ResultScene::DrawMaterialColumn(const QuestResult& result, const ResultRewardLayout& layout) const
{
    const Rect& column = layout.materialColumn;

    // 名前が引けるものだけ数える
    std::vector<std::pair<const ConsumableDef*, int>> materials;
    for (const ItemStack& stack : result.materials) {
        if (stack.count <= 0) continue;
        if (const ConsumableDef* def = ConsumableDatabase::Instance().Find(stack.itemId)) {
            materials.emplace_back(def, stack.count);
        }
    }

    draw::Text(FontSize::Normal, column.left + 8.0f, layout.headerY, palette::kAccent,
               str::Format("素材  %d 種", static_cast<int>(materials.size())));

    if (materials.empty()) {
        draw::Text(FontSize::Small, column.CenterX(), layout.listTop + 48.0f, palette::kTextDisabled,
                   "素材はありませんでした", draw::TextAlign::Center);
        return;
    }

    // 入りきらないときは、最後の行を「ほか N 種」にする
    const int capacity = math::MaxI(1, static_cast<int>((column.bottom - layout.listTop) / kMaterialRowHeight));
    const int count = static_cast<int>(materials.size());
    const int shown = (count > capacity) ? capacity - 1 : count;

    for (int i = 0; i < shown; ++i) {
        const float rowY = layout.listTop + kMaterialRowHeight * static_cast<float>(i);
        const float appear = math::Clamp((time_ - 0.6f - static_cast<float>(i) * 0.08f) / 0.25f,
                                         0.0f, 1.0f);
        if (appear <= 0.0f) continue;

        const Rect rect(column.left, rowY, column.right, rowY + kMaterialRowHeight - 6.0f);
        draw::FillRect(rect, palette::kPanelDark, 200);
        draw::StrokeRect(rect, palette::kBorder.Scaled(0.6f), 1.0f, 200);
        ui::DrawConsumableIcon(Rect(rect.left + 6.0f, rect.top + 3.0f, rect.left + 38.0f, rect.bottom - 3.0f),
                               *materials[static_cast<size_t>(i)].first);
        draw::Text(FontSize::Small, rect.left + 46.0f, rect.top + 8.0f, palette::kText,
                   materials[static_cast<size_t>(i)].first->name);
        draw::Text(FontSize::Small, rect.right - 12.0f, rect.top + 8.0f, palette::kAccentWarm,
                   str::Format("×%d", materials[static_cast<size_t>(i)].second), draw::TextAlign::Right);
    }

    if (shown < count) {
        const float rowY = layout.listTop + kMaterialRowHeight * static_cast<float>(shown);
        draw::Text(FontSize::Small, column.right - 12.0f, rowY + 8.0f, palette::kTextDim,
                   str::Format("ほか %d 種", count - shown), draw::TextAlign::Right);
    }
}

} // namespace ecl
