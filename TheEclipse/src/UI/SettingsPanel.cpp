#include "UI/SettingsPanel.h"

#include "Common/StringUtil.h"
#include "Core/GameConfig.h"
#include "Common/MathUtil.h"
#include "Game/SaveData.h"
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
    { "移動",           "A / D   ← / →" },
    { "奥・手前へ",     "W / S   ↑ / ↓" },
    { "ジャンプ",       "SPACE" },
    { "通常攻撃（3連）", "左クリック / J" },
    { "ガード",         "右クリック / K（長押し）" },
    { "パリィ",         "ガード中に左クリック" },
    { "回避ダッシュ",   "SHIFT" },
    { "ソードスキル",   "1 / 2 / 3 / 4" },
    { "メニュー",       "ESC" },
};

} // namespace

SettingsPanel::SettingsPanel()
{
    Layout();
}

void SettingsPanel::Layout()
{
    window_ = Rect::FromXYWH(360.0f, 110.0f, 1200.0f, 880.0f);

    // 左カラム: 設定項目 / 右カラム: 操作一覧
    columnLeft_ = window_.left + 36.0f;
    columnRight_ = columnLeft_ + 540.0f;

    const float left = columnLeft_;
    const float right = columnRight_;

    // --- 音量 ---------------------------------------------------------------
    float y = window_.top + 106.0f;
    bgmSlider_ = Slider(Rect(left, y, right, y + 40.0f), "BGM 音量", 70);
    y += 52.0f;
    seSlider_ = Slider(Rect(left, y, right, y + 40.0f), "SE 音量", 80);

    // --- 表示 ---------------------------------------------------------------
    y = window_.top + 268.0f;
    damageToggle_ = Toggle(Rect(left, y, right, y + 40.0f), "ダメージ数値", true);
    y += 48.0f;
    shakeToggle_ = Toggle(Rect(left, y, right, y + 40.0f), "画面振動", true);
    y += 48.0f;
    fpsToggle_ = Toggle(Rect(left, y, right, y + 40.0f), "FPS 表示", false);
    y += 48.0f;
    fullScreenToggle_ = Toggle(Rect(left, y, right, y + 40.0f), "フルスクリーン", false);
    y += 48.0f;
    softwareCursorToggle_ = Toggle(Rect(left, y, right, y + 40.0f), "カーソルを描画", false);

    // --- 開発者向け（一番下） --------------------------------------------------
    y = window_.top + 592.0f;
    debugToggle_ = Toggle(Rect(left, y, right, y + 40.0f), "デバッグモード", false);

    deleteSaveButton_ = Button(Rect::FromXYWH(left, window_.top + 690.0f, 260.0f, 52.0f),
                               "セーブデータ削除", FontSize::Small);
    deleteSaveButton_.SetAccent(palette::kDanger);

    closeButton_ = Button(Rect::FromXYWH(window_.CenterX() - 110.0f, window_.bottom - 80.0f,
                                         220.0f, 56.0f), "閉じる");

    // ラベルとつまみが重ならないよう、ラベル欄を広めに取る
    const FontSize labelSize = FontSize::Small;
    bgmSlider_.SetFontSize(labelSize);
    seSlider_.SetFontSize(labelSize);
    bgmSlider_.SetLabelWidth(200.0f);
    seSlider_.SetLabelWidth(200.0f);
    damageToggle_.SetFontSize(labelSize);
    shakeToggle_.SetFontSize(labelSize);
    fpsToggle_.SetFontSize(labelSize);
    fullScreenToggle_.SetFontSize(labelSize);
    softwareCursorToggle_.SetFontSize(labelSize);
    debugToggle_.SetFontSize(labelSize);
}

void SettingsPanel::DrawSectionHeader(float x, float y, float width, const char* title) const
{
    draw::Text(FontSize::Small, x, y, palette::kAccent, title);
    draw::Line(x, y + 26.0f, x + width, y + 26.0f, palette::kBorder, 1.0f, 130);
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
    softwareCursorToggle_.SetValue(settings.softwareCursor);
    debugToggle_.SetValue(settings.debugMode);
    confirmingDelete_ = false;
    message_.clear();
    messageTimer_ = 0.0f;
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
    softwareCursorToggle_.Update(input);
    debugToggle_.Update(input);
    messageTimer_ = math::MaxF(0.0f, messageTimer_ - dt);

    context.settings.bgmVolume = bgmSlider_.Value();
    context.settings.seVolume = seSlider_.Value();
    context.settings.showDamageNumbers = damageToggle_.Value();
    context.settings.screenShake = shakeToggle_.Value();
    context.settings.showFps = fpsToggle_.Value();
    context.settings.fullScreen = fullScreenToggle_.Value();
    context.settings.softwareCursor = softwareCursorToggle_.Value();
    context.settings.debugMode = debugToggle_.Value();

    // --- セーブデータ削除（2 段階で確認する） ------------------------------------
    if (deleteSaveButton_.Update(input, dt)) {
        if (!confirmingDelete_) {
            confirmingDelete_ = true;
            deleteSaveButton_.SetLabel("本当に削除しますか？");
        } else {
            confirmingDelete_ = false;
            deleteSaveButton_.SetLabel("セーブデータ削除");
            message_ = SaveSystem::Remove() ? "セーブデータを削除しました（次回起動時に反映）"
                                            : SaveSystem::LastError();
            messageTimer_ = 3.0f;
        }
    }

    if (closeButton_.Update(input, dt) || input.Pressed(GameAction::Cancel)) {
        closeRequested_ = true;
        open_ = false;
        confirmingDelete_ = false;
        deleteSaveButton_.SetLabel("セーブデータ削除");
    }
}

void SettingsPanel::Draw() const
{
    if (!open_) return;

    DrawWindow(window_, "設定");

    const float columnWidth = columnRight_ - columnLeft_;

    // --- 音量 ---------------------------------------------------------------
    DrawSectionHeader(columnLeft_, window_.top + 70.0f, columnWidth, "音量");
    bgmSlider_.Draw();
    seSlider_.Draw();

    // --- 表示 ---------------------------------------------------------------
    DrawSectionHeader(columnLeft_, window_.top + 232.0f, columnWidth, "表示");
    damageToggle_.Draw();
    shakeToggle_.Draw();
    fpsToggle_.Draw();
    fullScreenToggle_.Draw();
    softwareCursorToggle_.Draw();
    draw::Text(FontSize::Tiny, columnLeft_ + 12.0f, window_.top + 514.0f, palette::kTextDim,
               "※ フルスクリーンでカーソルが見えない時に ON");

    // --- 開発者向け ---------------------------------------------------------
    DrawSectionHeader(columnLeft_, window_.top + 556.0f, columnWidth, "開発者向け");
    debugToggle_.Draw();
    if (debugToggle_.Value()) {
        draw::Text(FontSize::Tiny, columnLeft_ + 12.0f, window_.top + 644.0f, palette::kAccentWarm,
                   "戦闘中 : F1 無敵 / F2 殲滅 / F3 全回復");
        draw::Text(FontSize::Tiny, columnLeft_ + 12.0f, window_.top + 666.0f, palette::kAccentWarm,
                   "         F4 判定表示 / F5 col・SP 追加");
    }
    deleteSaveButton_.Draw();

    // --- 操作一覧（右カラム） --------------------------------------------------
    DrawKeyGuide();

    closeButton_.Draw();

    if (messageTimer_ > 0.0f) {
        draw::Text(FontSize::Tiny, columnLeft_, window_.top + 752.0f, palette::kAccent, message_);
    }
}

void SettingsPanel::DrawKeyGuide() const
{
    const float left = columnRight_ + 44.0f;
    const float right = window_.right - 36.0f;

    DrawSectionHeader(left, window_.top + 70.0f, right - left, "操作一覧");

    float y = window_.top + 112.0f;
    for (const KeyGuide& guide : kGuides) {
        draw::Text(FontSize::Small, left + 4.0f, y, palette::kText, guide.action);
        draw::Text(FontSize::Tiny, right, y + 4.0f, palette::kTextDim, guide.key,
                   draw::TextAlign::Right);
        y += 34.0f;
    }

    // 補足
    y += 14.0f;
    draw::Line(left, y, right, y, palette::kBorder, 1.0f, 100);
    y += 16.0f;
    draw::Text(FontSize::Tiny, left + 4.0f, y, palette::kTextDim,
               "・UI はすべてマウスで操作できます");
    y += 26.0f;
    draw::Text(FontSize::Tiny, left + 4.0f, y, palette::kTextDim,
               "・ホームのタブは 1〜5 キーでも開けます");
    y += 26.0f;
    draw::Text(FontSize::Tiny, left + 4.0f, y, palette::kTextDim,
               "・奥行きが合っていないと攻撃は当たりません");
}

} // namespace ui
} // namespace ecl
