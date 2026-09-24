#include "UI/StatusPanel.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/GameConfig.h"
#include "Graphics/DrawUtil.h"

namespace ecl {
namespace ui {

namespace {
constexpr float kRowHeight = 76.0f;
constexpr float kRowPitch = 88.0f;
} // namespace

StatusPanel::StatusPanel()
{
    Layout();
}

void StatusPanel::Layout()
{
    window_ = Rect::FromXYWH(120.0f, 100.0f, 1680.0f, 880.0f);

    abilityArea_ = Rect(window_.left + 40.0f, window_.top + 316.0f,
                        window_.left + 660.0f, window_.top + 316.0f + kRowPitch * 5.0f - 12.0f);
    detailArea_ = Rect(window_.left + 720.0f, window_.top + 80.0f,
                       window_.right - 40.0f, window_.bottom - 100.0f);

    addButtons_.clear();
    for (int i = 0; i < kAbilityCount; ++i) {
        const float top = abilityArea_.top + kRowPitch * static_cast<float>(i);
        abilityRows_[i] = Rect(abilityArea_.left, top, abilityArea_.right, top + kRowHeight);
        addButtons_.push_back(Button(Rect::FromXYWH(abilityRows_[i].right - 68.0f, top + 16.0f,
                                                    56.0f, 44.0f), "+"));
    }

    closeButton_ = Button(Rect::FromXYWH(window_.right - 200.0f, window_.bottom - 82.0f,
                                         160.0f, 54.0f), "閉じる");
}

void StatusPanel::Open()
{
    open_ = true;
    closeRequested_ = false;
    statusChanged_ = false;
    selected_ = 0;
    message_.clear();
    messageTimer_ = 0.0f;
}

void StatusPanel::Update(float dt, const Input& input, GameContext& context)
{
    if (!open_) return;
    closeRequested_ = false;
    statusChanged_ = false;
    messageTimer_ = math::MaxF(0.0f, messageTimer_ - dt);

    PlayerData& player = context.player;
    const float mouseX = static_cast<float>(input.MouseX());
    const float mouseY = static_cast<float>(input.MouseY());

    // --- 行の選択 ------------------------------------------------------------
    for (int i = 0; i < kAbilityCount; ++i) {
        if (!abilityRows_[i].Contains(mouseX, mouseY)) continue;
        if (input.MouseClicked(MouseButton::Left)) selected_ = i;
    }

    // --- 振り分け ------------------------------------------------------------
    const bool hasPoint = player.AbilityPoints() > 0;
    for (int i = 0; i < static_cast<int>(addButtons_.size()); ++i) {
        Button& button = addButtons_[static_cast<size_t>(i)];
        button.SetEnabled(hasPoint && player.Ability(static_cast<Ability>(i)) < kAbilityMaxValue);
        if (!button.Update(input, dt) || !button.Enabled()) continue;

        const Ability ability = static_cast<Ability>(i);
        if (player.SpendAbilityPoint(ability)) {
            selected_ = i;
            statusChanged_ = true;
            message_ = str::Format("%s を %d に上げました", AbilityShortName(ability),
                                   player.Ability(ability));
            messageTimer_ = 2.4f;
        }
    }

    if (closeButton_.Update(input, dt) || input.Pressed(GameAction::Cancel)) {
        closeRequested_ = true;
        open_ = false;
    }
}

void StatusPanel::Draw(const GameContext& context) const
{
    if (!open_) return;

    DrawWindow(window_, "ステータス");
    DrawProfile(context);
    DrawAbilities(context);
    DrawDerivedStats(context);

    closeButton_.Draw();

    if (messageTimer_ > 0.0f) {
        draw::Text(FontSize::Small, abilityArea_.left, window_.bottom - 78.0f, palette::kAccent,
                   message_);
    }
}

void StatusPanel::DrawProfile(const GameContext& context) const
{
    const PlayerData& player = context.player;
    const Rect card(window_.left + 40.0f, window_.top + 80.0f,
                    window_.left + 660.0f, window_.top + 260.0f);

    draw::GradientRectV(card, palette::kPanel, palette::kPanelDark, 225, 12);
    draw::StrokeRect(card, palette::kBorder.Scaled(0.6f), 1.0f, 170);

    draw::Text(FontSize::Large, card.left + 24.0f, card.top + 20.0f, palette::kText, player.Name());
    draw::Text(FontSize::Normal, card.right - 24.0f, card.top + 28.0f, palette::kAccent,
               str::Format("Lv %d", player.Level()), draw::TextAlign::Right);

    draw::Text(FontSize::Small, card.left + 24.0f, card.top + 82.0f, palette::kTextDim,
               str::Format("総合戦力 %d", player.Power()));

    // EXP バー
    const Rect bar(card.left + 24.0f, card.top + 124.0f, card.right - 24.0f, card.top + 140.0f);
    const int need = player.ExpToNext();
    const float ratio = (need > 0) ? math::Clamp(static_cast<float>(player.Exp())
                                                 / static_cast<float>(need), 0.0f, 1.0f)
                                   : 1.0f;
    draw::FillRect(bar, palette::kPanelDark, 255);
    draw::Bar(bar, ratio, palette::kExp, ColorRGB(38, 40, 50));
    draw::StrokeRect(bar, palette::kBorder.Scaled(0.7f), 1.0f, 160);
    draw::Text(FontSize::Tiny, bar.left, bar.bottom + 8.0f, palette::kTextDim,
               str::Format("EXP  %d / %d", player.Exp(), need));
}

void StatusPanel::DrawAbilities(const GameContext& context) const
{
    const PlayerData& player = context.player;

    draw::Text(FontSize::Normal, abilityArea_.left, window_.top + 278.0f, palette::kAccent,
               "ステータス");
    const int points = player.AbilityPoints();
    draw::Text(FontSize::Small, abilityArea_.right, window_.top + 284.0f,
               points > 0 ? palette::kExp : palette::kTextDim,
               str::Format("残りポイント %d", points), draw::TextAlign::Right);

    for (int i = 0; i < kAbilityCount; ++i) {
        const Ability ability = static_cast<Ability>(i);
        const Rect& row = abilityRows_[i];
        const bool selected = (i == selected_);

        draw::FillRect(row, selected ? palette::kPanelLight : palette::kPanelDark, 225);
        draw::StrokeRect(row, selected ? palette::kAccent : palette::kBorder.Scaled(0.5f),
                         selected ? 2.0f : 1.0f, 200);

        draw::Text(FontSize::Normal, row.left + 20.0f, row.top + 10.0f, palette::kAccent,
                   AbilityShortName(ability));
        draw::Text(FontSize::Tiny, row.left + 20.0f, row.top + 44.0f, palette::kTextDim,
                   AbilityName(ability));
        draw::Text(FontSize::Large, row.left + 300.0f, row.top + 12.0f, palette::kText,
                   str::Format("%d", player.Ability(ability)), draw::TextAlign::Right);
        draw::Text(FontSize::Tiny, row.left + 330.0f, row.top + 48.0f, palette::kTextDisabled,
                   AbilityShortEffect(ability));

        addButtons_[static_cast<size_t>(i)].Draw();
    }
}

void StatusPanel::DrawDerivedStats(const GameContext& context) const
{
    const PlayerData& player = context.player;

    draw::FillRect(detailArea_, palette::kPanelDark, 205);
    draw::StrokeRect(detailArea_, palette::kBorder.Scaled(0.6f), 1.0f, 170);

    const Stats current = player.TotalStats();

    // 選択中のステータスを 1 上げた場合
    AbilityScores preview = player.Abilities();
    const bool canPreview = player.AbilityPoints() > 0;
    if (canPreview) preview.Add(static_cast<Ability>(selected_), 1);
    const Stats next = canPreview ? ApplyAbilities(player.EquippedStats(), preview) : current;

    float y = detailArea_.top + 24.0f;
    draw::Text(FontSize::Normal, detailArea_.left + 24.0f, y, palette::kAccent, "能力値");
    draw::Text(FontSize::Tiny, detailArea_.right - 24.0f, y + 8.0f, palette::kTextDim,
               canPreview ? str::Format("%s を +1 した場合を表示",
                                        AbilityShortName(static_cast<Ability>(selected_)))
                          : "ポイントがあると変化量を表示します",
               draw::TextAlign::Right);
    y += 48.0f;

    DrawStatDiffLine(detailArea_.left + 24.0f, y, "攻撃力", current.attack, next.attack, false);
    y += 34.0f;
    DrawStatDiffLine(detailArea_.left + 24.0f, y, "防御力", current.defense, next.defense, false);
    y += 34.0f;
    DrawStatDiffLine(detailArea_.left + 24.0f, y, "最大HP", current.maxHp, next.maxHp, false);
    y += 34.0f;
    DrawStatDiffLine(detailArea_.left + 24.0f, y, "最大MP", current.maxMp, next.maxMp, false);
    y += 34.0f;
    DrawStatDiffLine(detailArea_.left + 24.0f, y, "クリティカル率", current.critRate,
                     next.critRate, true);
    y += 34.0f;
    DrawStatDiffLine(detailArea_.left + 24.0f, y, "クリティカル倍率", current.critDamage,
                     next.critDamage, true);
    y += 34.0f;
    DrawStatDiffLine(detailArea_.left + 24.0f, y, "移動速度", current.moveSpeed,
                     next.moveSpeed, false);
    y += 34.0f;
    DrawStatDiffLine(detailArea_.left + 24.0f, y, "攻撃速度", current.attackSpeed,
                     next.attackSpeed, true);
    y += 34.0f;

    // MP 回復とドロップ率は Stats の表示行に乗らないので個別に出す
    draw::Text(FontSize::Small, detailArea_.left + 24.0f, y, palette::kTextDim, "MP 回復");
    draw::Text(FontSize::Small, detailArea_.left + 360.0f, y, palette::kText,
               str::Format("%.0f 秒に 1", (current.mpRegen > 0.0f) ? 1.0f / current.mpRegen : 0.0f),
               draw::TextAlign::Right);
    y += 34.0f;

    {
        const float rate = player.DropRateMultiplier();
        AbilityScores lukPreview = player.Abilities();
        if (canPreview) lukPreview.Add(static_cast<Ability>(selected_), 1);
        const float nextRate = AbilityDropRate(lukPreview);

        draw::Text(FontSize::Small, detailArea_.left + 24.0f, y, palette::kTextDim, "ドロップ率");
        draw::Text(FontSize::Small, detailArea_.left + 360.0f, y, palette::kText,
                   str::Format("%.0f%%", rate * 100.0f), draw::TextAlign::Right);
        if (nextRate > rate + 0.0001f) {
            draw::Text(FontSize::Small, detailArea_.left + 392.0f, y, palette::kTextDim, "→");
            draw::Text(FontSize::Small, detailArea_.left + 540.0f, y, palette::kHp,
                       str::Format("%.0f%%", nextRate * 100.0f), draw::TextAlign::Right);
        }
        y += 44.0f;
    }

    // 選択中のステータスの説明
    draw::Line(detailArea_.left + 24.0f, y, detailArea_.right - 24.0f, y, palette::kBorder, 1.0f, 120);
    y += 16.0f;

    const Ability ability = static_cast<Ability>(selected_);
    draw::Text(FontSize::Normal, detailArea_.left + 24.0f, y, palette::kAccent,
               str::Format("%s（%s）", AbilityShortName(ability), AbilityName(ability)));
    y += 40.0f;
    draw::Text(FontSize::Small, detailArea_.left + 24.0f, y, palette::kText,
               str::Format("1 ポイントにつき %s", AbilityEffectText(ability).c_str()));
    y += 34.0f;
    draw::Text(FontSize::Tiny, detailArea_.left + 24.0f, y, palette::kTextDim,
               str::Format("ステータスポイントはレベルアップごとに %d 貰えます",
                           kAbilityPointsPerLevel));
}

} // namespace ui
} // namespace ecl
