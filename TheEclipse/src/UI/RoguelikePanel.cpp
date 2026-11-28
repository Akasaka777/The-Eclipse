#include "UI/RoguelikePanel.h"

#include "Common/StringUtil.h"
#include "Core/GameConfig.h"
#include "Core/Input.h"
#include "Game/GameContext.h"
#include "Game/QuestDatabase.h"
#include "Game/Roguelike.h"
#include "Graphics/DrawUtil.h"

namespace ecl {
namespace ui {

namespace {
constexpr float kScreenW = static_cast<float>(config::kScreenWidth);
const ColorRGB kViolet(186, 140, 255);

const char* const kRules[] = {
    "本編で手に入れた武器・装備・アイテム・col は持ち込めない（Lv 1・初期装備から始まる）",
    "3 階層 × 4 フロア。マップの形・敵・ショップと鍛冶屋の場所は毎回ランダム",
    "敵は col・アイテム・強化結晶を落とす。武器・防具は階層のボスから（雑魚敵は落とさない）",
    "敵やボスが落とすスキルを拾うと、3 つの候補から 1 つを選んでセットできる",
    "持っているスキルを選ぶと Lv が上がる（最大 Lv 5）。ショップで買い、鍛冶屋で強化して強くなろう",
    "パーマデス：倒れる・リタイアすると、ランの成果はすべて失われる",
    "ラスボスはまれに「ラビリンス」の装備を落とす。ラスボスを倒してクリアすれば本編へ持ち帰れる",
};
} // namespace

RoguelikePanel::RoguelikePanel()
{
    window_ = Rect::FromXYWH(200.0f, 110.0f, 1520.0f, 860.0f);
    const float bw = 236.0f;
    const float gap = 16.0f;
    const float left = window_.left + 60.0f;
    for (int i = 0; i < static_cast<int>(WeaponType::Count); ++i) {
        Button b(Rect::FromXYWH(left + (bw + gap) * static_cast<float>(i), window_.top + 560.0f, bw, 64.0f),
                 WeaponTypeName(static_cast<WeaponType>(i)), FontSize::Normal);
        b.SetAccent(kViolet);
        weaponButtons_.push_back(b);
    }
    startButton_ = Button(Rect::FromXYWH(window_.right - 480.0f, window_.bottom - 92.0f, 240.0f, 64.0f), "挑戦する",
                          FontSize::Medium);
    startButton_.SetAccent(kViolet);
    closeButton_ = Button(Rect::FromXYWH(window_.right - 220.0f, window_.bottom - 92.0f, 180.0f, 64.0f), "閉じる");
}

void RoguelikePanel::Open(const GameContext& context)
{
    open_ = true;
    closeRequested_ = false;
    startRequested_ = false;
    weapon_ = context.player.CurrentWeaponType();
}

void RoguelikePanel::Update(float dt, const Input& input, GameContext& context)
{
    if (!open_) return;
    closeRequested_ = false;
    startRequested_ = false;
    const bool unlocked = rogue::IsUnlocked(context.player);
    startButton_.SetEnabled(unlocked);

    for (size_t i = 0; i < weaponButtons_.size(); ++i) {
        weaponButtons_[i].SetSelected(static_cast<int>(i) == static_cast<int>(weapon_));
        weaponButtons_[i].SetEnabled(unlocked);
        if (weaponButtons_[i].Update(input, dt)) weapon_ = static_cast<WeaponType>(i);
    }
    if (startButton_.Update(input, dt) && unlocked) {
        startRequested_ = true;
        open_ = false;
        return;
    }
    if (closeButton_.Update(input, dt) || input.Pressed(GameAction::Cancel) || input.Pressed(GameAction::Menu)) {
        closeRequested_ = true;
        open_ = false;
    }
}

void RoguelikePanel::Draw(const GameContext& context) const
{
    if (!open_) return;
    DrawWindow(window_, "迷宮の門 ― ローグライクモード");
    const PlayerData& player = context.player;
    const bool unlocked = rogue::IsUnlocked(player);
    const float left = window_.left + 60.0f;
    float y = window_.top + 90.0f;

    if (!unlocked) {
        draw::Text(FontSize::Large, window_.CenterX(), y + 10.0f, palette::kDanger, "門は固く閉ざされている",
                   draw::TextAlign::Center);
        draw::Text(FontSize::Normal, window_.CenterX(), y + 80.0f, palette::kTextDim,
                   "クエスト一覧のクエストをすべてクリアすると開きます", draw::TextAlign::Center);
        y += 150.0f;
        for (const QuestDef& quest : QuestDatabase::Instance().Quests()) {
            if (quest.special) continue;
            const bool cleared = player.IsQuestCleared(quest.id);
            const Rect row = Rect::FromXYWH(window_.CenterX() - 360.0f, y, 720.0f, 52.0f);
            draw::FillRect(row, palette::kPanelDark, 200);
            draw::Text(FontSize::Normal, row.left + 24.0f, row.top + 10.0f, palette::kText, quest.name);
            draw::Text(FontSize::Normal, row.right - 24.0f, row.top + 10.0f, cleared ? palette::kHp : palette::kTextDisabled,
                       cleared ? "CLEAR" : "未クリア", draw::TextAlign::Right);
            y += 60.0f;
        }
        closeButton_.Draw();
        return;
    }

    draw::Text(FontSize::Normal, left, y, kViolet, "ルール");
    y += 46.0f;
    for (const char* rule : kRules) {
        draw::Text(FontSize::Small, left + 12.0f, y, palette::kText, str::Format("・%s", rule));
        y += 36.0f;
    }

    draw::Text(FontSize::Normal, left, window_.top + 510.0f, kViolet, "武器種を選ぶ（ラン中はこの武器種の武器とスキルだけ）");
    for (const Button& b : weaponButtons_) b.Draw();

    // これまでの記録
    const float ry = window_.top + 660.0f;
    draw::Text(FontSize::Normal, left, ry, kViolet, "これまでの記録");
    draw::Text(FontSize::Normal, left + 12.0f, ry + 46.0f, palette::kText,
               str::Format("挑戦 %d 回 ／ クリア %d 回 ／ いちばん奥 %d フロア目（全 %d フロア）",
                           player.RoguelikeRuns(), player.RoguelikeClears(), player.RoguelikeBestFloor(),
                           rogue::kFloorCount));

    startButton_.Draw();
    closeButton_.Draw();
    (void)kScreenW;
}

} // namespace ui
} // namespace ecl
