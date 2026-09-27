#include "UI/WeaponSelectPanel.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/DxInclude.h"
#include "Core/GameConfig.h"
#include "Game/ItemDatabase.h"
#include "Game/WeaponMotion.h"
#include "Graphics/DrawUtil.h"

#include <cmath>
#include <string>

namespace ecl {
namespace ui {

namespace {

// --- カードの寸法 -------------------------------------------------------------
constexpr float kCardWidth = 276.0f;
constexpr float kCardHeight = 410.0f;
constexpr float kCardGap = 18.0f;

// 武器種ごとの短い説明（カード内は 1 行 16 文字までに収める）
struct WeaponBlurb
{
    const char* line1;
    const char* line2;
    const char* detail;
};

const WeaponBlurb& BlurbFor(WeaponType type)
{
    static const WeaponBlurb kBlurbs[static_cast<int>(WeaponType::Count)] = {
        { "攻守のバランスが良く",   "一番扱いやすい",
          "振り下ろし・振り上げ・薙ぎ払いの素直な 3 段。迷ったらこれ。" },
        { "振りは遅いが一撃が重く", "吹き飛ばしが最大",
          "担ぎ上げてから叩き落とす 2 段。当てにくいぶん 1 発が大きい。" },
        { "間合いは最短だが",       "手数が最も多い",
          "刺突から回転二連まで一気に畳みかける 4 段。的が小さい相手向き。" },
        { "細い判定の刺突で",       "速く深く踏み込む",
          "上体を立てたまま突く 3 段。3 段目は大きく踏み込んで刺す。" },
        { "間合いが最長で",         "薙ぎ払いは範囲が広い",
          "腰を落として押し込む 2 段。2 段目は奥行き方向にも広く当たる。" },
    };
    return kBlurbs[math::ClampInt(static_cast<int>(type), 0, static_cast<int>(WeaponType::Count) - 1)];
}

// コンボの内訳（「刺突 → 斬り払い → …」）
std::string ComboText(WeaponType type)
{
    std::string text;
    for (int i = 0; i < WeaponComboLength(type); ++i) {
        if (i > 0) text += " → ";
        text += WeaponComboStep(type, i).name;
    }
    return text;
}

} // namespace

WeaponSelectPanel::WeaponSelectPanel()
{
    Layout();
}

void WeaponSelectPanel::Layout()
{
    window_ = Rect::FromXYWH(200.0f, 140.0f, 1520.0f, 800.0f);

    const float total = kCardWidth * static_cast<float>(kCount)
                      + kCardGap * static_cast<float>(kCount - 1);
    const float left = window_.left + (window_.Width() - total) * 0.5f;
    const float top = window_.top + 108.0f;
    for (int i = 0; i < kCount; ++i) {
        cards_[i] = Rect::FromXYWH(left + (kCardWidth + kCardGap) * static_cast<float>(i), top,
                                   kCardWidth, kCardHeight);
    }

    detailArea_ = Rect(window_.left + 24.0f, top + kCardHeight + 20.0f,
                       window_.right - 24.0f, window_.bottom - 96.0f);

    confirmButton_ = Button(Rect::FromXYWH(window_.CenterX() - 130.0f, window_.bottom - 78.0f,
                                           260.0f, 56.0f),
                            "この武器で始める", FontSize::Normal);
}

const Rect& WeaponSelectPanel::CardRect(int index) const
{
    return cards_[math::ClampInt(index, 0, kCount - 1)];
}

void WeaponSelectPanel::Open()
{
    open_ = true;
    confirmed_ = false;
    selected_ = -1;
    time_ = 0.0f;
}

WeaponType WeaponSelectPanel::Result() const
{
    if (selected_ < 0) return WeaponType::OneHandSword;
    return static_cast<WeaponType>(selected_);
}

void WeaponSelectPanel::Update(float dt, const Input& input)
{
    if (!open_) return;

    confirmed_ = false;
    time_ += dt;

    // --- カードの選択 ---------------------------------------------------------
    if (input.MouseClicked(MouseButton::Left)) {
        const float mouseX = static_cast<float>(input.MouseX());
        const float mouseY = static_cast<float>(input.MouseY());
        for (int i = 0; i < kCount; ++i) {
            if (cards_[i].Contains(mouseX, mouseY)) selected_ = i;
        }
    }

    // --- 決定 -----------------------------------------------------------------
    //   選ぶまでは押せない（うっかり初期値で始めてしまわないように）
    confirmButton_.SetEnabled(selected_ >= 0);
    if (confirmButton_.Update(input, dt) && confirmButton_.Enabled()) {
        confirmed_ = true;
        open_ = false;
    }
}

void WeaponSelectPanel::DrawCard(int index, float mouseX, float mouseY) const
{
    const Rect& card = cards_[index];
    const WeaponType type = static_cast<WeaponType>(index);
    const bool chosen = (selected_ == index);
    const bool hovered = card.Contains(mouseX, mouseY);

    // --- 枠 -------------------------------------------------------------------
    draw::FillRect(card, chosen ? palette::kPanelLight : palette::kPanelDark, 240);
    const ColorRGB edge = chosen ? palette::kAccent
                        : (hovered ? palette::kAccent.Scaled(0.7f) : palette::kBorder);
    draw::StrokeRect(card, edge, chosen ? 3.0f : 1.0f, 255);
    if (chosen) draw::Glow(card.CenterX(), card.top + 54.0f, 70.0f, palette::kAccent, 70, 3);

    // --- 武器アイコン ---------------------------------------------------------
    DrawWeaponIcon(Rect::FromXYWH(card.CenterX() - 42.0f, card.top + 12.0f, 84.0f, 84.0f),
                   type, chosen ? palette::kAccent : palette::kText);

    // --- 名前 -----------------------------------------------------------------
    draw::Text(FontSize::Medium, card.CenterX(), card.top + 104.0f,
               chosen ? palette::kAccent : palette::kText, WeaponTypeName(type),
               draw::TextAlign::Center);

    const ItemDatabase& db = ItemDatabase::Instance();
    const int templateId = StarterWeaponId(type);
    const ItemTemplate* tmpl = db.Find(templateId);
    draw::Text(FontSize::Small, card.CenterX(), card.top + 144.0f, palette::kTextDim,
               tmpl ? tmpl->name : "", draw::TextAlign::Center);

    draw::Line(card.left + 18.0f, card.top + 180.0f, card.right - 18.0f, card.top + 180.0f,
               palette::kBorder, 1.0f, 180);

    // --- 性能 -----------------------------------------------------------------
    const EquipmentItem sample = db.Create(templateId, kStarterIv);
    const Stats stats = sample.TotalStats();
    const struct { const char* label; std::string value; } rows[] = {
        { "攻撃力",       str::Format("%d", static_cast<int>(stats.attack)) },
        { "クリティカル", str::Format("%.0f%%", stats.critRate * 100.0f) },
        { "攻撃速度",     str::Format("x%.2f", WeaponSpeedScale(type) * (1.0f + stats.attackSpeed)) },
        { "間合い",       str::Format("x%.2f", WeaponReachScale(type)) },
        { "コンボ",       str::Format("%d 段", WeaponComboLength(type)) },
    };
    for (int i = 0; i < 5; ++i) {
        const float y = card.top + 194.0f + 30.0f * static_cast<float>(i);
        draw::Text(FontSize::Tiny, card.left + 18.0f, y, palette::kTextDim, rows[i].label);
        draw::Text(FontSize::Tiny, card.right - 18.0f, y, palette::kText, rows[i].value,
                   draw::TextAlign::Right);
    }

    // --- 一言 -----------------------------------------------------------------
    const WeaponBlurb& blurb = BlurbFor(type);
    draw::Text(FontSize::Tiny, card.left + 16.0f, card.top + 346.0f, palette::kTextDim, blurb.line1);
    draw::Text(FontSize::Tiny, card.left + 16.0f, card.top + 370.0f, palette::kTextDim, blurb.line2);
}

void WeaponSelectPanel::Draw() const
{
    if (!open_) return;

    DrawDimOverlay(200);
    DrawWindow(window_, "使う武器を選ぶ");

    draw::Text(FontSize::Small, window_.left + 24.0f, window_.top + 66.0f, palette::kText,
               "最初の武器を 1 つ選んでください。");

    const float mouseX = static_cast<float>(Input::Instance().MouseX());
    const float mouseY = static_cast<float>(Input::Instance().MouseY());
    for (int i = 0; i < kCount; ++i) DrawCard(i, mouseX, mouseY);

    // --- 選んだ武器の詳細 -----------------------------------------------------
    draw::FillRect(detailArea_, palette::kPanelDark, 200);
    draw::StrokeRect(detailArea_, palette::kBorder.Scaled(0.6f), 1.0f, 160);

    const float dx = detailArea_.left + 20.0f;
    if (selected_ < 0) {
        draw::Text(FontSize::Small, detailArea_.CenterX(), detailArea_.top + 22.0f,
                   palette::kTextDisabled, "武器をクリックすると、その武器のコンボが表示されます",
                   draw::TextAlign::Center);
    } else {
        const WeaponType type = Result();
        draw::Text(FontSize::Small, dx, detailArea_.top + 14.0f, palette::kAccent,
                   str::Format("%s : %d 段コンボ", WeaponTypeName(type), WeaponComboLength(type)));
        draw::Text(FontSize::Small, dx, detailArea_.top + 48.0f, palette::kText, ComboText(type));
        draw::Text(FontSize::Tiny, dx, detailArea_.top + 84.0f, palette::kTextDim,
                   BlurbFor(type).detail);
    }

    draw::Text(FontSize::Tiny, dx, detailArea_.bottom - 24.0f, palette::kTextDim,
               "※ 選ばなかった武器種は配られません。あとでクエストのドロップから手に入ります。");

    confirmButton_.Draw();
    if (selected_ < 0) {
        // 決定ボタンの左隣に出す（ボタンや詳細欄と重ならない位置）
        draw::Text(FontSize::Tiny, window_.CenterX() - 150.0f, window_.bottom - 64.0f,
                   palette::kTextDisabled, "武器を選ぶと決定できます", draw::TextAlign::Right);
    }
}

} // namespace ui
} // namespace ecl
