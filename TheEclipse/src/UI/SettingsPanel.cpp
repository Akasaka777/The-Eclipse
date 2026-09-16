#include "UI/SettingsPanel.h"

#include "Common/StringUtil.h"
#include "Core/GameConfig.h"
#include "Graphics/DrawUtil.h"

namespace ecl {
namespace ui {

namespace {

// 操作説明の一覧
struct KeyGuide
{
    const char* action;
    const char* key;
};

const KeyGuide kGuides[] = {
    { "移動",             "A / D  もしくは  ← / →" },
    { "ジャンプ",         "SPACE / W / ↑" },
    { "通常攻撃（3連）",  "左クリック / J" },
    { "ガード",           "右クリック / K（長押し）" },
    { "回避ダッシュ",     "SHIFT" },
    { "ソードスキル",     "1 / 2 / 3 / 4（アイコンのクリックでも可）" },
    { "メニュー",         "ESC / TAB" },
};

} // namespace

SettingsPanel::SettingsPanel()
{
    Layout();
}

void SettingsPanel::Layout()
{
    window_ = Rect::FromXYWH(430.0f, 130.0f, 1060.0f, 820.0f);

    const float left = window_.left + 50.0f;
    const float right = window_.right - 50.0f;
    float y = window_.top + 110.0f;

    bgmSlider_ = Slider(Rect(left, y, right, y + 44.0f), "BGM 音量", 70);
    y += 66.0f;
    seSlider_ = Slider(Rect(left, y, right, y + 44.0f), "SE 音量", 80);
    y += 78.0f;

    damageToggle_ = Toggle(Rect(left, y, right, y + 44.0f), "ダメージ数値を表示", true);
    y += 60.0f;
    shakeToggle_ = Toggle(Rect(left, y, right, y + 44.0f), "画面振動", true);
    y += 60.0f;
    fpsToggle_ = Toggle(Rect(left, y, right, y + 44.0f), "FPS を表示", false);
    y += 60.0f;
    fullScreenToggle_ = Toggle(Rect(left, y, right, y + 44.0f), "フルスクリーン", false);

    closeButton_ = Button(Rect::FromXYWH(window_.CenterX() - 110.0f, window_.bottom - 86.0f,
                                         220.0f, 58.0f), "閉じる");
}

void SettingsPanel::Open(const GameSettings& settings)
{
    open_ = true;
    closeRequested_ = false;

    bgmSlider_.SetValue(settings.bgmVolume);
    seSlider_.SetValue(settings.seVolume);
    damageToggle_.SetValue(settings.showDamageNumbers);
    shakeToggle_.SetValue(settings.screenShake);
    fpsToggle_.SetValue(settings.showFps);
    fullScreenToggle_.SetValue(settings.fullScreen);
}

void SettingsPanel::Update(float dt, const Input& input, GameContext& context)
{
    if (!open_) return;
    closeRequested_ = false;

    bgmSlider_.Update(input);
    seSlider_.Update(input);
    damageToggle_.Update(input);
    shakeToggle_.Update(input);
    fpsToggle_.Update(input);
    fullScreenToggle_.Update(input);

    context.settings.bgmVolume = bgmSlider_.Value();
    context.settings.seVolume = seSlider_.Value();
    context.settings.showDamageNumbers = damageToggle_.Value();
    context.settings.screenShake = shakeToggle_.Value();
    context.settings.showFps = fpsToggle_.Value();
    context.settings.fullScreen = fullScreenToggle_.Value();

    if (closeButton_.Update(input, dt) || input.Pressed(GameAction::Cancel)) {
        closeRequested_ = true;
        open_ = false;
    }
}

void SettingsPanel::Draw() const
{
    if (!open_) return;

    DrawWindow(window_, "設定");

    bgmSlider_.Draw();
    seSlider_.Draw();
    damageToggle_.Draw();
    shakeToggle_.Draw();
    fpsToggle_.Draw();
    fullScreenToggle_.Draw();

    // --- 操作説明 -----------------------------------------------------------
    const float guideTop = window_.top + 480.0f;
    draw::Line(window_.left + 50.0f, guideTop - 18.0f, window_.right - 50.0f, guideTop - 18.0f,
               palette::kBorder, 1.0f, 150);
    draw::Text(FontSize::Normal, window_.left + 50.0f, guideTop, palette::kAccent, "操作一覧");

    float y = guideTop + 42.0f;
    for (const KeyGuide& guide : kGuides) {
        draw::Text(FontSize::Small, window_.left + 60.0f, y, palette::kText, guide.action);
        draw::Text(FontSize::Small, window_.right - 60.0f, y, palette::kTextDim, guide.key,
                   draw::TextAlign::Right);
        y += 32.0f;
    }

    closeButton_.Draw();
}

} // namespace ui
} // namespace ecl
