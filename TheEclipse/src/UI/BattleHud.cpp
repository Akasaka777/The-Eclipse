#include "UI/BattleHud.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/DxInclude.h"
#include "Core/GameConfig.h"
#include "Graphics/CharacterArt.h"
#include "Graphics/DrawUtil.h"

#include <cmath>

namespace ecl {
namespace ui {

namespace {

constexpr float kScreenW = static_cast<float>(config::kScreenWidth);
constexpr float kSkillIconSize = 104.0f;
constexpr float kSkillBarBottom = 1030.0f;

// アイテムスロット（左下。スキルアイコンと同じ大きさの正方形を前後に重ねる）
constexpr float kItemCardLeft = 40.0f;
constexpr float kItemBackOffset = 26.0f;   // 奥のカードを右上へずらす量

// アイコンの左上に発動キーの札を出す（キー設定で長い名前になっても収まるよう幅を合わせる）
void DrawKeyTag(const Rect& icon, const std::string& name)
{
    if (name.empty()) return;
    const float width = math::MaxF(24.0f, static_cast<float>(draw::TextWidth(FontSize::Tiny, name)) + 10.0f);
    const Rect tag = Rect::FromXYWH(icon.left + 4.0f, icon.top + 4.0f, math::MinF(width, icon.Width() - 8.0f),
                                    22.0f);
    draw::FillRect(tag, palette::kBlack, 190);
    draw::Text(FontSize::Tiny, tag.CenterX(), tag.top + 2.0f, palette::kText, name, draw::TextAlign::Center);
}

// --- 左上のプレイヤーステータス -------------------------------------------------
//   顔の丸いメダル（下にレベルの札）を左に置き、その右へ薄れていく板に名前・HP・MP を並べる
constexpr float kPortraitCenterX = 96.0f;
constexpr float kPortraitCenterY = 84.0f;
constexpr float kPortraitRadius = 54.0f;
constexpr float kPlateTop = 18.0f;
constexpr float kPlateBottom = 126.0f;   // メダルとレベルの札は板から下へはみ出す
constexpr float kPlateRight = 692.0f;     // 上辺の右端
constexpr float kPlateSlant = 28.0f;      // 下辺の右端はこれだけ左（斜めに切る）
constexpr float kStatusContentLeft = 172.0f;
constexpr float kStatusNumbersRight = 652.0f;
constexpr float kHpBarTop = 54.0f;
constexpr float kHpBarHeight = 26.0f;
constexpr float kMpBarTop = 92.0f;
constexpr float kMpBarHeight = 16.0f;
constexpr float kMpBarWidth = 384.0f;
constexpr float kLevelBadgeWidth = 76.0f;
constexpr float kLevelBadgeHeight = 24.0f;
constexpr float kGuardBadgeMaxLeft = 414.0f;   // 名前が 12 文字（いちばん長い）のときの位置

constexpr float kHitFlashTime = 0.35f;    // 被弾の光が消えるまで
constexpr float kHealFlashTime = 0.6f;    // 回復の光が消えるまで
constexpr float kCriticalHpRatio = 0.25f; // これ以下で瀕死の赤い明滅
constexpr float kShimmerPeriod = 3.6f;    // バーの上を光が流れる間隔
constexpr float kShimmerSweep = 1.1f;     // 光が端から端まで流れる時間

const ColorRGB kPlateColor(10, 14, 24);

// 加算合成の矩形（光の表現）
void AddRect(const Rect& rect, const ColorRGB& color, int alpha)
{
    if (alpha <= 0 || rect.Width() <= 0.0f || rect.Height() <= 0.0f) return;
    SetDrawBlendMode(DX_BLENDMODE_ADD, math::ClampInt(alpha, 0, 255));
    DrawBoxAA(rect.left, rect.top, rect.right, rect.bottom, draw::ToDx(color), TRUE);
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 横方向に透明度が変わる帯（左 alphaLeft → 右 alphaRight）。
//   半透明を重ねると継ぎ目が濃くなるので、帯の境目は整数の座標にそろえて重ねない
void FadeRectH(const Rect& rect, const ColorRGB& color, int alphaLeft, int alphaRight, int bands)
{
    if (bands < 1) bands = 1;
    float prev = std::floor(rect.left);
    for (int i = 0; i < bands; ++i) {
        const float t = (bands > 1) ? static_cast<float>(i) / static_cast<float>(bands - 1) : 0.0f;
        const float next = (i == bands - 1)
                         ? rect.right
                         : std::floor(rect.left + rect.Width() * static_cast<float>(i + 1) / static_cast<float>(bands));
        const int alpha = static_cast<int>(math::Lerp(static_cast<float>(alphaLeft), static_cast<float>(alphaRight), t));
        if (next > prev && alpha > 0) draw::FillRect(Rect(prev, rect.top, next, rect.bottom), color, alpha);
        prev = next;
    }
}

// 上下のグラデーション（不透明）。draw::GradientRectV と違って下へはみ出さない
void GradientV(const Rect& rect, const ColorRGB& top, const ColorRGB& bottom, int bands)
{
    if (bands < 1) bands = 1;
    for (int i = 0; i < bands; ++i) {
        const float t = (bands > 1) ? static_cast<float>(i) / static_cast<float>(bands - 1) : 0.0f;
        const float y0 = std::floor(rect.top + rect.Height() * static_cast<float>(i) / static_cast<float>(bands));
        const float y1 = (i == bands - 1)
                       ? rect.bottom
                       : std::floor(rect.top + rect.Height() * static_cast<float>(i + 1) / static_cast<float>(bands));
        if (y1 > y0) draw::FillRect(Rect(rect.left, y0, rect.right, y1), ColorRGB::Lerp(top, bottom, t), 255);
    }
}

// 円弧（度。画面の下向きが +90 度）。重なりで濃くならないよう不透明で描く
void StrokeArc(float cx, float cy, float radius, float degFrom, float degTo, const ColorRGB& color, float thickness)
{
    constexpr float kDeg = 3.14159265f / 180.0f;
    const int steps = math::ClampInt(static_cast<int>(std::fabs(degTo - degFrom) / 5.0f), 2, 72);
    for (int i = 0; i < steps; ++i) {
        const float a0 = (degFrom + (degTo - degFrom) * static_cast<float>(i) / static_cast<float>(steps)) * kDeg;
        const float a1 = (degFrom + (degTo - degFrom) * static_cast<float>(i + 1) / static_cast<float>(steps)) * kDeg;
        draw::Line(cx + std::cos(a0) * radius, cy + std::sin(a0) * radius,
                   cx + std::cos(a1) * radius, cy + std::sin(a1) * radius, color, thickness, 255);
    }
}

// 左右の端をとがらせた六角形（不透明）
void FillHex(const Rect& rect, float point, const ColorRGB& color)
{
    draw::FillRect(Rect(rect.left + point, rect.top, rect.right - point, rect.bottom), color, 255);
    draw::Triangle(Vec2(rect.left, rect.CenterY()), Vec2(rect.left + point, rect.top),
                   Vec2(rect.left + point, rect.bottom), color, true, 255);
    draw::Triangle(Vec2(rect.right, rect.CenterY()), Vec2(rect.right - point, rect.top),
                   Vec2(rect.right - point, rect.bottom), color, true, 255);
}

// ひし形（不透明）
void FillDiamond(float cx, float cy, float half, const ColorRGB& color)
{
    draw::Triangle(Vec2(cx - half, cy), Vec2(cx, cy - half), Vec2(cx + half, cy), color, true, 255);
    draw::Triangle(Vec2(cx - half, cy), Vec2(cx, cy + half), Vec2(cx + half, cy), color, true, 255);
}

// HP / MP のゲージの見た目
struct GaugeLook
{
    ColorRGB    color;
    float       ratio = 0.0f;
    float       delayRatio = -1.0f;   // 減った分の残像（ratio より大きいときだけ）
    float       healFrom = -1.0f;     // 回復した分（healFrom〜ratio）を光らせる
    float       healFlash = 0.0f;     // 1 → 0
    float       hitFlash = 0.0f;      // 1 → 0（被弾で縁と中身が白く光る）
    float       shimmer = -1.0f;      // 流れる光の位置 0..1（負なら流さない）
    float       danger = 0.0f;        // 瀕死の赤い明滅 0..1
    int         ticks = 10;           // 目盛りで区切る数
    const char* label = "";
};

// 枠・溝・残像・本体・光・目盛り・ラベルの順に重ねてゲージを描く
void DrawGauge(const Rect& bar, const GaugeLook& look)
{
    const float ratio = math::Clamp(look.ratio, 0.0f, 1.0f);
    auto span = [&bar](float r0, float r1) {
        return Rect(bar.left + bar.Width() * math::Clamp(r0, 0.0f, 1.0f), bar.top,
                    bar.left + bar.Width() * math::Clamp(r1, 0.0f, 1.0f), bar.bottom);
    };

    // 外枠（暗い縁）と溝
    const Rect frame = bar.Expanded(2.0f);
    draw::FillRect(frame, ColorRGB(4, 6, 12), 225);
    GradientV(bar, ColorRGB(9, 12, 20), ColorRGB(24, 30, 46), 6);
    if (look.danger > 0.0f) {
        draw::FillRect(bar, palette::kHpCritical, static_cast<int>(look.danger * 55.0f));
    }

    // 減った分の残像（明るい赤から暗い赤へ）
    if (look.delayRatio > ratio) {
        GradientV(span(ratio, look.delayRatio), ColorRGB(255, 150, 120), ColorRGB(196, 60, 64), 6);
    }

    Rect fill = span(0.0f, ratio);
    if (ratio > 0.0f && fill.Width() < 2.0f) fill.right = fill.left + 2.0f;   // 残りわずかでも見えるように
    if (fill.Width() > 0.0f) {
        // 本体：上ほど明るいグラデーション＋上半分のつや＋下端の影
        GradientV(fill, look.color.Scaled(1.25f), look.color.Scaled(0.68f), 8);
        Rect gloss = fill;
        gloss.bottom = fill.top + std::floor(fill.Height() * 0.4f);
        draw::FillRect(gloss, palette::kWhite, 48);
        Rect shade = fill;
        shade.top = fill.bottom - 2.0f;
        draw::FillRect(shade, palette::kBlack, 60);

        // 回復した分を光らせる
        if (look.healFlash > 0.0f && look.healFrom >= 0.0f && look.healFrom < ratio) {
            AddRect(span(look.healFrom, ratio), ColorRGB(190, 255, 210), static_cast<int>(look.healFlash * 110.0f));
        }

        // 左から右へ流れる光（山なりに 6 段）
        if (look.shimmer >= 0.0f) {
            constexpr float kWidth = 64.0f;
            const float center = fill.left - kWidth + (fill.Width() + kWidth * 2.0f) * look.shimmer;
            for (int i = 0; i < 6; ++i) {
                const float x0 = center - kWidth * 0.5f + kWidth * static_cast<float>(i) / 6.0f;
                const float x1 = x0 + kWidth / 6.0f;
                const float peak = 1.0f - std::fabs((static_cast<float>(i) + 0.5f) / 3.0f - 1.0f);
                const Rect band(math::MaxF(x0, fill.left), fill.top, math::MinF(x1, fill.right), fill.bottom);
                AddRect(band, palette::kWhite, static_cast<int>(peak * 46.0f));
            }
        }

        // 先端の光（満タンでないとき）
        if (ratio < 1.0f) {
            for (int i = 0; i < 3; ++i) {
                const float x1 = fill.right - 3.0f * static_cast<float>(i);
                AddRect(Rect(math::MaxF(fill.left, x1 - 3.0f), fill.top, x1, fill.bottom), look.color,
                        60 - i * 18);
            }
            draw::FillRect(Rect(math::MaxF(fill.left, fill.right - 2.0f), fill.top, fill.right, fill.bottom),
                           palette::kWhite, 190);
        }

        // 被弾の瞬間は中身も白く光る
        if (look.hitFlash > 0.0f) draw::FillRect(fill, palette::kWhite, static_cast<int>(look.hitFlash * 70.0f));
    }

    // 目盛り（半分の位置は少し濃く）
    for (int i = 1; i < look.ticks; ++i) {
        const float x = std::floor(bar.left + bar.Width() * static_cast<float>(i) / static_cast<float>(look.ticks));
        draw::FillRect(Rect(x, bar.top + 1.0f, x + 1.0f, bar.bottom - 1.0f), palette::kBlack,
                       (i * 2 == look.ticks) ? 110 : 64);
    }

    // 縁の線（被弾で白く光る）
    const ColorRGB edge = ColorRGB::Lerp(palette::kBorder.Scaled(0.75f), palette::kWhite, look.hitFlash);
    draw::StrokeRect(frame, edge, 1.0f, 170 + static_cast<int>(85.0f * look.hitFlash));

    // ラベル（HP / MP）
    if (look.label && look.label[0] != '\0') {
        const float textY = bar.top + std::floor((bar.Height() - static_cast<float>(draw::TextHeight(FontSize::Tiny))) * 0.5f) - 1.0f;
        draw::Text(FontSize::Tiny, bar.left + 8.0f, textY, palette::kText, look.label);
    }
}

} // namespace

PlayerStatusLayout BattleHud::StatusLayout()
{
    PlayerStatusLayout layout;
    layout.portraitCenter = Vec2(kPortraitCenterX, kPortraitCenterY);
    layout.portraitRadius = kPortraitRadius;
    // 板はメダルの中心から右へ（メダルの丸い縁がそのまま左端になる）
    layout.plate = Rect(kPortraitCenterX, kPlateTop, kPlateRight, kPlateBottom);
    layout.plateSlant = kPlateSlant;
    layout.levelBadge = Rect::FromCenter(kPortraitCenterX, kPortraitCenterY + kPortraitRadius + 2.0f,
                                         kLevelBadgeWidth, kLevelBadgeHeight);
    layout.namePos = Vec2(kStatusContentLeft + 18.0f, kPlateTop + 8.0f);
    layout.hpBar = Rect(kStatusContentLeft, kHpBarTop, kStatusNumbersRight, kHpBarTop + kHpBarHeight);
    layout.mpBar = Rect::FromXYWH(kStatusContentLeft, kMpBarTop, kMpBarWidth, kMpBarHeight);
    layout.numbersRight = kStatusNumbersRight;
    // ガードの札は名前のすぐ右に出す。名前がいちばん長いときの位置をここに入れておき、
    // 描くときは名前の幅に合わせて左へ寄せる（HP の数値とは重ならない）
    layout.guardBadge = Rect::FromXYWH(kGuardBadgeMaxLeft, kPlateTop + 7.0f, 92.0f, 22.0f);
    // 飾りの弧（半径 + 9）まで含める
    const float outer = kPortraitRadius + 10.0f;
    layout.bounds = Rect(kPortraitCenterX - outer, kPlateTop, kPlateRight,
                         math::MaxF(layout.levelBadge.bottom + 2.0f, kPlateBottom));
    return layout;
}

BattleHud::BattleHud()
{
    // 右下にスキルアイコンを 4 つ並べる
    for (int i = 0; i < kSkillSlotCount; ++i) {
        const float x = kScreenW - 40.0f - kSkillIconSize * static_cast<float>(4 - i)
                      - 14.0f * static_cast<float>(3 - i);
        skillRects_[i] = Rect::FromXYWH(x, kSkillBarBottom - kSkillIconSize, kSkillIconSize, kSkillIconSize);
    }

    // アイテムスロット：手前のカードはスキルバーと底をそろえ、奥のカードを右上へずらす
    itemFront_ = Rect::FromXYWH(kItemCardLeft, kSkillBarBottom - kSkillIconSize, kSkillIconSize,
                                kSkillIconSize);
    itemBack_ = Rect::FromXYWH(itemFront_.left + kItemBackOffset, itemFront_.top - kItemBackOffset,
                               kSkillIconSize, kSkillIconSize);
}

Rect BattleHud::ItemAreaRect() const
{
    return Rect(itemFront_.left, itemBack_.top, itemBack_.right, itemFront_.bottom);
}

void BattleHud::ShowItemMessage(const std::string& message)
{
    itemMessage_ = message;
    itemMessageTimer_ = 1.6f;
}

void BattleHud::Reset()
{
    hpDelay_ = 1.0f;
    statusPrimed_ = false;
    hitFlash_ = 0.0f;
    healFlash_ = 0.0f;
    healFrom_ = -1.0f;
    clickedSkill_ = -1;
    time_ = 0.0f;
    itemUseRequested_ = false;
    itemClickConsumed_ = false;
    itemMessage_.clear();
    itemMessageTimer_ = 0.0f;
    // 選んでいた枠は持ち越す（毎回 回復 に戻ると使いにくいので）
    itemSlide_ = (itemSlot_ == QuickSlot::Buff) ? 1.0f : 0.0f;
}

void BattleHud::Update(float dt, const Player& player, const Boss* boss, const Input& input)
{
    time_ += dt;
    clickedSkill_ = -1;
    itemUseRequested_ = false;
    itemClickConsumed_ = false;
    itemMessageTimer_ = math::MaxF(0.0f, itemMessageTimer_ - dt);

    // HP バーの減少残像
    hpDelay_ = math::Approach(hpDelay_, player.HpRatio(), dt * 0.55f);
    if (hpDelay_ < player.HpRatio()) hpDelay_ = player.HpRatio();

    // 被弾 / 回復の光（最初のフレームは今の HP を覚えるだけ）
    hitFlash_ = math::MaxF(0.0f, hitFlash_ - dt / kHitFlashTime);
    healFlash_ = math::MaxF(0.0f, healFlash_ - dt / kHealFlashTime);
    if (healFlash_ <= 0.0f) healFrom_ = -1.0f;
    if (statusPrimed_) {
        if (player.hp < prevHp_ - 0.01f) {
            hitFlash_ = 1.0f;
            healFlash_ = 0.0f;
            healFrom_ = -1.0f;
        } else if (player.hp > prevHp_ + 0.01f) {
            // 続けて回復したときは、最初に光り始めた位置から光らせる
            healFrom_ = (healFrom_ >= 0.0f) ? math::MinF(healFrom_, prevHpRatio_) : prevHpRatio_;
            healFlash_ = 1.0f;
        }
    }
    statusPrimed_ = true;
    prevHp_ = player.hp;
    prevHpRatio_ = player.HpRatio();

    // ボスの HP バーは Boss::Draw が頭上に描く（減少の残像もボス側で持つ）
    (void)boss;

    // スキルアイコンのクリック
    const float mouseX = static_cast<float>(input.MouseX());
    const float mouseY = static_cast<float>(input.MouseY());
    if (input.MouseClicked(MouseButton::Left)) {
        for (int i = 0; i < kSkillSlotCount; ++i) {
            if (player.Skill(i) == nullptr) continue;
            if (skillRects_[i].Contains(mouseX, mouseY)) {
                clickedSkill_ = i;
                break;
            }
        }
    }

    // --- アイテムスロット ------------------------------------------------------
    auto swapSlot = [this]() {
        itemSlot_ = (itemSlot_ == QuickSlot::Recovery) ? QuickSlot::Buff : QuickSlot::Recovery;
    };
    if (input.Pressed(GameAction::ItemSwitch)) swapSlot();
    if (input.Pressed(GameAction::ItemUse)) itemUseRequested_ = true;

    if (input.MouseClicked(MouseButton::Left) && ItemAreaRect().Contains(mouseX, mouseY)) {
        // 手前のカードは使う、見えている奥のカードは前後を入れ替える
        if (itemFront_.Contains(mouseX, mouseY)) {
            itemUseRequested_ = true;
            itemClickConsumed_ = true;
        } else if (itemBack_.Contains(mouseX, mouseY)) {
            swapSlot();
            itemClickConsumed_ = true;
        }
    }

    const float target = (itemSlot_ == QuickSlot::Buff) ? 1.0f : 0.0f;
    itemSlide_ = math::Approach(itemSlide_, target, dt * 7.0f);
}

void BattleHud::Draw(const Player& player, const PlayerData& data, const Boss* boss,
                     const HudInfo& info) const
{
    (void)boss;   // ボスの HP バーは Boss::Draw で頭上に描く
    DrawPlayerStatus(player, data);
    DrawSkillBar(player, data);
    DrawItemSlots(player, data);
    DrawFloorInfo(info);
    DrawCombo(info);

    if (info.showFps) {
        draw::Text(FontSize::Small, kScreenW - 30.0f, 30.0f, palette::kTextDim,
                   str::Format("FPS %.1f", GetFPS()), draw::TextAlign::Right);
    }
}

void BattleHud::DrawPlayerStatus(const Player& player, const PlayerData& data) const
{
    const PlayerStatusLayout layout = StatusLayout();
    const Rect& plate = layout.plate;
    const float plateBodyRight = plate.right - layout.plateSlant;
    const bool dead = player.IsDead();
    const bool critical = !dead && player.HpRatio() <= kCriticalHpRatio;
    const float pulse = 0.5f + 0.5f * std::sin(time_ * 7.0f);

    // --- 背景の板：左ほど濃く、右へ薄れる。右端は斜めに切る ---------------------
    FadeRectH(Rect(plate.left, plate.top, plateBodyRight, plate.bottom), kPlateColor, 235, 150, 24);
    draw::Triangle(Vec2(plateBodyRight, plate.top), Vec2(plate.right, plate.top), Vec2(plateBodyRight, plate.bottom),
                   kPlateColor, true, 150);
    // 上の方をうっすら明るく（ガラスのつや）
    FadeRectH(Rect(plate.left, plate.top, plateBodyRight, plate.top + 40.0f), palette::kWhite, 12, 0, 12);
    // 上辺のアクセントの線（右へ消えていく）、下辺の細い線、斜めの端の線
    FadeRectH(Rect(plate.left, plate.top, plate.right, plate.top + 2.0f), palette::kAccent, 235, 30, 24);
    FadeRectH(Rect(plate.left, plate.bottom - 1.0f, plateBodyRight, plate.bottom), palette::kBorder, 190, 20, 24);
    draw::Line(plate.right, plate.top, plateBodyRight, plate.bottom, palette::kBorder, 1.0f, 120);

    // --- 顔のメダルとレベルの札 ------------------------------------------------
    DrawPortrait(layout, player, data);

    // --- 名前（頭に小さなひし形）------------------------------------------------
    FillDiamond(layout.namePos.x - 10.0f, layout.namePos.y + 10.0f, 5.0f,
                critical ? palette::kHpCritical : palette::kAccent);
    draw::Text(FontSize::Small, layout.namePos.x, layout.namePos.y, palette::kText, data.Name());
    const float nameRight = layout.namePos.x + static_cast<float>(draw::TextWidth(FontSize::Small, data.Name()));

    // --- HP の数値（今の値を大きく、最大値を小さく）-----------------------------
    const int hpNow = static_cast<int>(std::ceil(math::MaxF(0.0f, player.hp)));
    const std::string hpMaxText = str::Format(" / %d", static_cast<int>(player.maxHp));
    const float hpMaxWidth = static_cast<float>(draw::TextWidth(FontSize::Small, hpMaxText));
    draw::Text(FontSize::Small, layout.numbersRight, layout.hpBar.top - 26.0f, palette::kTextDim, hpMaxText,
               draw::TextAlign::Right);
    draw::Text(FontSize::Normal, layout.numbersRight - hpMaxWidth, layout.hpBar.top - 30.0f,
               critical ? palette::kHpCritical : palette::kText, str::Format("%d", hpNow), draw::TextAlign::Right);

    // --- HP ゲージ（残り 25% ごとに色が変わる：緑 → 黄緑 → 黄 → 赤）-------------
    GaugeLook hp;
    hp.color = palette::HpColor(player.HpRatio());
    hp.ratio = player.HpRatio();
    hp.delayRatio = hpDelay_;
    hp.healFrom = healFrom_;
    hp.healFlash = healFlash_;
    hp.hitFlash = hitFlash_;
    const float hpSweep = std::fmod(time_, kShimmerPeriod) / kShimmerSweep;
    hp.shimmer = (!dead && hpSweep <= 1.0f) ? hpSweep : -1.0f;
    hp.danger = critical ? 0.35f + 0.65f * pulse : 0.0f;
    hp.ticks = 10;
    hp.label = "HP";
    DrawGauge(layout.hpBar, hp);

    // --- MP ゲージと数値 -------------------------------------------------------
    GaugeLook mp;
    mp.color = palette::kMp;
    mp.ratio = player.MpRatio();
    const float mpSweep = std::fmod(time_ + kShimmerPeriod * 0.5f, kShimmerPeriod) / kShimmerSweep;
    mp.shimmer = (!dead && mpSweep <= 1.0f) ? mpSweep : -1.0f;
    mp.ticks = 4;
    mp.label = "MP";
    DrawGauge(layout.mpBar, mp);

    const std::string mpMaxText = str::Format(" / %d", static_cast<int>(player.MaxMp()));
    const float mpMaxWidth = static_cast<float>(draw::TextWidth(FontSize::Tiny, mpMaxText));
    const float mpTextY = layout.mpBar.top + 1.0f;
    draw::Text(FontSize::Tiny, layout.numbersRight, mpTextY, palette::kTextDim, mpMaxText, draw::TextAlign::Right);
    draw::Text(FontSize::Tiny, layout.numbersRight - mpMaxWidth, mpTextY, palette::kText,
               str::Format("%d", static_cast<int>(player.Mp())), draw::TextAlign::Right);

    // --- パリィ状態（ガード中のみ。名前のすぐ右に札を出す）-----------------------
    if (player.IsGuarding() || player.IsParryActive()) {
        const float badgeLeft = math::MinF(nameRight + 14.0f, layout.guardBadge.left);
        const Rect badge = layout.guardBadge.Offset(badgeLeft - layout.guardBadge.left, 0.0f);

        ColorRGB color = palette::kAccent;
        const char* label = "パリィ可";
        if (player.IsParryActive()) {
            color = palette::kCritical;
            label = "受付中";
        } else if (player.ParryCooldown() > 0.0f) {
            color = palette::kTextDisabled;
            label = "硬直";
        }

        draw::FillRect(badge, color.Scaled(0.28f), 225);
        draw::StrokeRect(badge, color, 1.0f, 210);
        draw::FillRect(Rect(badge.left, badge.top, badge.left + 4.0f, badge.bottom), color, 255);
        draw::Text(FontSize::Tiny, badge.CenterX() + 2.0f, badge.top + 3.0f, palette::kText, label,
                   draw::TextAlign::Center);
    }
}

void BattleHud::DrawPortrait(const PlayerStatusLayout& layout, const Player& player, const PlayerData& data) const
{
    const float cx = layout.portraitCenter.x;
    const float cy = layout.portraitCenter.y;
    const float r = layout.portraitRadius;
    const bool dead = player.IsDead();
    const bool critical = !dead && player.HpRatio() <= kCriticalHpRatio;
    const float pulse = 0.5f + 0.5f * std::sin(time_ * 7.0f);

    // 輪の色：ふだんはシアン、瀕死は赤く脈打つ、戦闘不能は灰色。被弾の瞬間は赤く光る
    ColorRGB ring = palette::kAccent;
    if (dead) ring = palette::kTextDisabled;
    else if (critical) ring = ColorRGB::Lerp(palette::kHpCritical.Scaled(0.55f), palette::kHpCritical, pulse);
    ring = ColorRGB::Lerp(ring, palette::kHpCritical, hitFlash_);

    // 瀕死の赤い後光（メダルの後ろ）
    if (critical) draw::Glow(cx, cy, r + 14.0f, palette::kHpCritical, static_cast<int>(30.0f + 70.0f * pulse), 4);

    // 影と金属の縁
    draw::Circle(cx, cy + 3.0f, r + 7.0f, palette::kBlack, true, 1.0f, 110);
    draw::Circle(cx, cy, r + 6.0f, ColorRGB(44, 58, 86), true);
    draw::Circle(cx, cy, r + 4.0f, ColorRGB(16, 21, 33), true);

    // 内側：中央の少し上ほど明るい
    for (int i = 0; i < 8; ++i) {
        const float t = static_cast<float>(i) / 7.0f;
        draw::Circle(cx, cy - 6.0f * t, r * (1.0f - 0.62f * t),
                     ColorRGB::Lerp(ColorRGB(14, 18, 30), ColorRGB(46, 62, 96), t), true);
    }
    if (!dead) draw::Glow(cx, cy - 10.0f, r * 0.8f, ring.Scaled(0.45f), 60, 4);

    // プレイヤーの姿
    DrawActor(Rect::FromXYWH(cx - 31.0f, cy - 46.0f, 62.0f, 84.0f), 1, dead ? PoseKind::Dead : PoseKind::Idle,
              std::fmod(time_ * 0.5f, 1.0f), player.art, nullptr, 255, 0.0f);

    // 被弾の赤み・戦闘不能の暗さ
    if (hitFlash_ > 0.0f) draw::Circle(cx, cy, r, palette::kHpCritical, true, 1.0f, static_cast<int>(hitFlash_ * 120.0f));
    if (dead) draw::Circle(cx, cy, r, palette::kBlack, true, 1.0f, 120);

    // 輪と、上の左右の飾りの弧
    draw::Circle(cx, cy, r, ring, false, 2.5f, 255);
    draw::Circle(cx, cy, r + 4.0f, palette::kBorder, false, 1.0f, 150);
    StrokeArc(cx, cy, r + 9.0f, 205.0f, 250.0f, ring, 3.0f);
    StrokeArc(cx, cy, r + 9.0f, 290.0f, 335.0f, ring, 3.0f);

    // --- レベルの札（メダルの下に重ねる六角形）------------------------------------
    const Rect& badge = layout.levelBadge;
    const float point = 10.0f;
    FillHex(badge.Expanded(2.0f), point + 1.0f, ColorRGB(6, 9, 16));
    FillHex(badge, point, dead ? palette::kTextDisabled : palette::kAccent);
    draw::FillRect(Rect(badge.left + point, badge.top + 1.0f, badge.right - point, badge.top + 3.0f),
                   palette::kWhite, 90);

    const std::string lvText = "Lv";
    const std::string numText = str::Format("%d", data.Level());
    const float lvWidth = static_cast<float>(draw::TextWidth(FontSize::Tiny, lvText));
    const float numWidth = static_cast<float>(draw::TextWidth(FontSize::Small, numText));
    const float textLeft = badge.CenterX() - (lvWidth + 4.0f + numWidth) * 0.5f;
    const ColorRGB ink(8, 18, 30);
    draw::Text(FontSize::Tiny, textLeft, badge.CenterY() - 8.0f, ink, lvText);
    draw::Text(FontSize::Small, textLeft + lvWidth + 4.0f, badge.CenterY() - 11.0f, ink, numText);
}

void BattleHud::DrawSkillBar(const Player& player, const PlayerData& data) const
{
    const int limit = data.SkillSlotLimit();
    for (int i = 0; i < limit; ++i) {
        const Rect rect = skillRects_[i];
        const SwordSkill* skill = player.Skill(i);

        draw::ChamferRect(rect, 10.0f, palette::kPanelDark, 225);

        if (!skill) {
            draw::StrokeRect(rect, palette::kTextDisabled, 2.0f, 180);
            draw::Text(FontSize::Small, rect.CenterX(), rect.CenterY() - 10.0f, palette::kTextDisabled,
                       "---", draw::TextAlign::Center);
            continue;
        }

        const bool ready = player.CanUseSkill(i);
        const float cooldownRatio = player.SkillCooldownRatio(i);
        const ColorRGB frame = ready ? skill->effectColor : palette::kTextDisabled;

        // アイコン本体
        draw::GradientRectV(rect.Expanded(-4.0f), skill->effectColor.Scaled(ready ? 0.45f : 0.18f),
                            palette::kPanelDark, 235, 10);
        DrawWeaponIcon(Rect::FromCenter(rect.CenterX(), rect.CenterY() - 6.0f, 56.0f, 56.0f),
                       skill->weapon, ready ? skill->effectColor : palette::kTextDisabled);

        // クールダウン（下から満ちる）
        if (cooldownRatio > 0.0f) {
            Rect cover = rect.Expanded(-4.0f);
            cover.top = cover.bottom - cover.Height() * cooldownRatio;
            draw::FillRect(cover, palette::kBlack, 165);
            draw::Text(FontSize::Medium, rect.CenterX(), rect.CenterY() - 18.0f, palette::kText,
                       str::Format("%.1f", player.SkillCooldown(i)), draw::TextAlign::Center);
        } else if (!player.HasWeapon()) {
            // 素手ではソードスキルを振れない
            draw::FillRect(rect.Expanded(-4.0f), palette::kBlack, 130);
            draw::Text(FontSize::Small, rect.CenterX(), rect.CenterY() - 10.0f, palette::kDanger,
                       "素手", draw::TextAlign::Center);
        } else if (player.Mp() < skill->mpCost) {
            // MP 不足
            draw::FillRect(rect.Expanded(-4.0f), palette::kBlack, 120);
            draw::Text(FontSize::Small, rect.CenterX(), rect.CenterY() - 10.0f, palette::kDanger,
                       "MP不足", draw::TextAlign::Center);
        }

        draw::StrokeRect(rect, frame, ready ? 3.0f : 2.0f, 255);
        if (ready) {
            const float pulse = 0.5f + 0.5f * std::sin(time_ * 4.0f + static_cast<float>(i));
            draw::StrokeRect(rect.Expanded(3.0f), skill->effectColor,
                             1.0f, static_cast<int>(120.0f * pulse));
        }

        // 発動キー（キー設定で変えたものを出す）
        const GameAction skillKeys[4] = { GameAction::Skill1, GameAction::Skill2,
                                          GameAction::Skill3, GameAction::Skill4 };
        DrawKeyTag(rect, Input::Instance().PrimaryKeyName(skillKeys[i % 4]));

        // スキル名と消費 MP
        draw::Text(FontSize::Tiny, rect.CenterX(), rect.bottom - 22.0f, palette::kText,
                   skill->name, draw::TextAlign::Center);
        draw::Text(FontSize::Tiny, rect.right - 6.0f, rect.top + 4.0f, palette::kMp,
                   str::Format("%d", static_cast<int>(skill->mpCost)), draw::TextAlign::Right);
    }

    // 操作ヒント
    draw::Text(FontSize::Tiny, skillRects_[0].left, skillRects_[0].top - 26.0f, palette::kTextDim,
               str::Format("%s / クリックでソードスキル     ESC : メニュー",
                           Input::Instance().GetKeyConfig().SkillKeysSummary(limit).c_str()));
}

void BattleHud::DrawItemSlots(const Player& player, const PlayerData& data) const
{
    // 入れ替えの途中は 2 枚が前後の位置を行き来する。
    //   回復カードは itemSlide_ = 0 で手前、1 で奥。バフカードはその逆。
    auto lerpRect = [](const Rect& a, const Rect& b, float t) {
        return Rect(math::Lerp(a.left, b.left, t), math::Lerp(a.top, b.top, t),
                    math::Lerp(a.right, b.right, t), math::Lerp(a.bottom, b.bottom, t));
    };
    // 奥へ回るカードは一度左上へ浮かせて、すれ違うように見せる
    const float lift = std::sin(itemSlide_ * 3.14159265f) * 22.0f;
    Rect recovery = lerpRect(itemFront_, itemBack_, itemSlide_);
    Rect buff = lerpRect(itemBack_, itemFront_, itemSlide_);
    if (itemSlot_ == QuickSlot::Buff) { recovery.top -= lift; recovery.bottom -= lift; }
    else                               { buff.top -= lift; buff.bottom -= lift; }

    // 手前に近い方を後から描く
    const bool recoveryFront = itemSlide_ < 0.5f;
    if (recoveryFront) {
        DrawItemCard(buff, QuickSlot::Buff, false, player, data);
        DrawItemCard(recovery, QuickSlot::Recovery, true, player, data);
    } else {
        DrawItemCard(recovery, QuickSlot::Recovery, false, player, data);
        DrawItemCard(buff, QuickSlot::Buff, true, player, data);
    }

    // 選んでいる枠と操作キー
    const Rect area = ItemAreaRect();
    draw::Text(FontSize::Tiny, area.left, area.top - 24.0f, palette::kTextDim,
               str::Format("%sアイテム   %s : 切替  %s : 使用", QuickSlotName(itemSlot_),
                           Input::Instance().ActionKeyName(GameAction::ItemSwitch),
                           Input::Instance().ActionKeyName(GameAction::ItemUse)));

    // 通知（スロットの上）
    if (itemMessageTimer_ > 0.0f) {
        const int alpha = static_cast<int>(math::Clamp(itemMessageTimer_ / 0.4f, 0.0f, 1.0f) * 255.0f);
        draw::TextAlpha(FontSize::Small, area.left, area.top - 54.0f, palette::kDanger,
                        itemMessage_, alpha);
    }
}

void BattleHud::DrawItemCard(const Rect& rect, QuickSlot slot, bool front, const Player& player,
                             const PlayerData& data) const
{
    const Inventory& inventory = data.GetInventory();
    const ConsumableDef* item = ConsumableDatabase::Instance().Find(inventory.QuickItem(slot));
    const int count = item ? inventory.ItemCount(item->id) : 0;
    const bool usable = item && count > 0 && player.ItemCooldown() <= 0.0f;
    const ColorRGB slotColor = (slot == QuickSlot::Buff) ? ColorRGB(255, 170, 60)
                                                         : ColorRGB(110, 230, 130);
    const ColorRGB itemColor = item ? item->color : slotColor;

    draw::ChamferRect(rect, 10.0f, palette::kPanelDark, front ? 235 : 215);
    draw::GradientRectV(rect.Expanded(-4.0f), itemColor.Scaled(front && usable ? 0.42f : 0.16f),
                        palette::kPanelDark, front ? 235 : 200, 10);

    if (!front) {
        // 奥のカード：見えている右上の帯に種類と切り替えキーだけ出す
        draw::StrokeRect(rect, slotColor.Scaled(0.6f), 2.0f, 200);
        const ConsumableDef* buff = (slot == QuickSlot::Buff) ? player.ItemBuff() : nullptr;
        draw::Text(FontSize::Tiny, rect.right - 6.0f, rect.top + 4.0f,
                   buff ? buff->color : palette::kTextDim,
                   buff ? str::Format("%s %.0fs", QuickSlotName(slot), player.ItemBuffRemain())
                        : std::string(QuickSlotName(slot)),
                   draw::TextAlign::Right);
        return;
    }

    if (!item) {
        draw::StrokeRect(rect, palette::kTextDisabled, 2.0f, 200);
        draw::Text(FontSize::Small, rect.CenterX(), rect.CenterY() - 20.0f, palette::kTextDisabled,
                   "---", draw::TextAlign::Center);
        draw::Text(FontSize::Tiny, rect.CenterX(), rect.bottom - 22.0f, palette::kTextDisabled,
                   "未装備", draw::TextAlign::Center);
    } else {
        // 左上の使用キーと右上の個数に掛からない大きさ
        DrawConsumableIcon(Rect::FromCenter(rect.CenterX(), rect.CenterY() + 2.0f, 48.0f, 48.0f),
                           *item, count <= 0);

        // 使った直後の待ち時間（スキルと同じく下から満ちる）
        if (player.ItemCooldown() > 0.0f) {
            Rect cover = rect.Expanded(-4.0f);
            cover.top = cover.bottom - cover.Height() * (player.ItemCooldown() / kItemUseCooldown);
            draw::FillRect(cover, palette::kBlack, 150);
        }

        draw::Text(FontSize::Tiny, rect.CenterX(), rect.bottom - 22.0f,
                   count > 0 ? palette::kText : palette::kTextDisabled, item->name,
                   draw::TextAlign::Center);
        draw::Text(FontSize::Tiny, rect.right - 6.0f, rect.top + 4.0f,
                   count > 0 ? palette::kAccentWarm : palette::kDanger,
                   str::Format("×%d", count), draw::TextAlign::Right);
        draw::StrokeRect(rect, usable ? item->color : palette::kTextDisabled, usable ? 3.0f : 2.0f, 255);
        if (usable) {
            const float pulse = 0.5f + 0.5f * std::sin(time_ * 4.0f);
            draw::StrokeRect(rect.Expanded(3.0f), item->color, 1.0f, static_cast<int>(120.0f * pulse));
        }
    }

    // 効いているバフの残り時間（バフカードの上端に細い帯）
    if (slot == QuickSlot::Buff) {
        if (const ConsumableDef* buff = player.ItemBuff()) {
            const Rect bar(rect.left + 6.0f, rect.top + 28.0f, rect.right - 6.0f, rect.top + 33.0f);
            draw::Bar(bar, player.ItemBuffRatio(), buff->color, palette::kPanel);
        }
    }

    // 使用キー（スキルの番号と同じ位置）
    DrawKeyTag(rect, Input::Instance().PrimaryKeyName(GameAction::ItemUse));
}

void BattleHud::DrawFloorInfo(const HudInfo& info) const
{
    const Rect panel = Rect::FromXYWH(28.0f, 166.0f, 420.0f, 74.0f);
    draw::ChamferRect(panel, 10.0f, palette::kPanelDark, 200);
    draw::StrokeRect(panel, palette::kBorder.Scaled(0.7f), 1.0f, 160);

    draw::Text(FontSize::Small, panel.left + 16.0f, panel.top + 8.0f, palette::kAccent,
               str::Format("FLOOR %d / %d", info.floorIndex, info.floorCount));
    draw::Text(FontSize::Small, panel.right - 16.0f, panel.top + 8.0f, palette::kTextDim,
               str::TimeText(info.questTime), draw::TextAlign::Right);
    draw::Text(FontSize::Small, panel.left + 16.0f, panel.top + 38.0f, palette::kText, info.floorName);

    if (info.enemiesRemaining > 0) {
        draw::Text(FontSize::Small, panel.right - 16.0f, panel.top + 38.0f, palette::kDanger,
                   str::Format("残 %d 体", info.enemiesRemaining), draw::TextAlign::Right);
    } else {
        draw::Text(FontSize::Small, panel.right - 16.0f, panel.top + 38.0f, palette::kHp,
                   "CLEAR", draw::TextAlign::Right);
    }
}

void BattleHud::DrawCombo(const HudInfo& info) const
{
    if (info.combo < 2) return;

    const float fade = math::Clamp(info.comboTimer / 0.6f, 0.0f, 1.0f);
    const int alpha = static_cast<int>(255.0f * fade);
    const float scale = 1.0f + math::Clamp(info.comboTimer - 1.2f, 0.0f, 0.4f);

    const float x = 120.0f;
    const float y = 300.0f;

    draw::TextAlpha(FontSize::Huge, x, y, palette::kAccentWarm, str::Format("%d", info.combo), alpha);
    draw::TextAlpha(FontSize::Medium, x + 96.0f * scale, y + 24.0f, palette::kText, "COMBO", alpha);
}

} // namespace ui
} // namespace ecl
