#include "UI/SettingsPanel.h"

#include "Common/StringUtil.h"
#include "Core/GameConfig.h"
#include "Core/Version.h"
#include "Common/MathUtil.h"
#include "Game/SaveData.h"
#include "Graphics/DrawUtil.h"

#include <vector>

namespace ecl {
namespace ui {

namespace {

// 操作説明の 1 行
struct KeyGuide
{
    std::string action;
    std::string key;
};

// 対になる 2 つの操作（左右・奥手前）を「A / D   ← / →」のように枠ごとにまとめる
std::string PairKeys(const KeyConfig& keys, GameAction a, GameAction b)
{
    std::string text;
    for (int slot = 0; slot < kKeySlotsPerAction; ++slot) {
        std::string part;
        const int codeA = keys.Get(a, slot);
        const int codeB = keys.Get(b, slot);
        if (codeA != kNoInput) part = KeyConfig::InputName(codeA);
        if (codeB != kNoInput) part += (part.empty() ? "" : " / ") + KeyConfig::InputName(codeB);
        if (part.empty()) continue;
        if (!text.empty()) text += "   ";
        text += part;
    }
    return text.empty() ? std::string("未設定") : text;
}

// ソードスキル 1〜4 の 1 つ目のキーを並べる
std::string SkillKeys(const KeyConfig& keys)
{
    const GameAction skills[4] = { GameAction::Skill1, GameAction::Skill2, GameAction::Skill3,
                                   GameAction::Skill4 };
    std::string text;
    for (GameAction skill : skills) {
        if (!text.empty()) text += " / ";
        const std::string name = keys.PrimaryName(skill);
        text += name.empty() ? std::string("-") : name;
    }
    return text;
}

std::vector<KeyGuide> BuildGuides(const KeyConfig& keys)
{
    const std::string attack = keys.PrimaryName(GameAction::Attack);
    return {
        { "移動",            PairKeys(keys, GameAction::MoveLeft, GameAction::MoveRight) },
        { "奥・手前へ",      PairKeys(keys, GameAction::MoveUp, GameAction::MoveDown) },
        { "ジャンプ",        keys.Describe(GameAction::Jump) },
        { "通常攻撃（3連）", keys.Describe(GameAction::Attack) },
        { "ガード",          keys.Describe(GameAction::Guard) + "（長押し）" },
        { "パリィ",          "ガード中に " + (attack.empty() ? std::string("通常攻撃") : attack) },
        { "回避ダッシュ",    keys.Describe(GameAction::Dash) },
        { "ソードスキル",    SkillKeys(keys) },
        { "アイテム切替",    keys.Describe(GameAction::ItemSwitch) + "（回復 ⇔ バフの前後入替）" },
        { "アイテム使用",    keys.Describe(GameAction::ItemUse) + " / 手前のカードをクリック" },
        { "メニュー",        "ESC" },
    };
}

} // namespace

SettingsPanel::SettingsPanel()
{
    Layout();
}

void SettingsPanel::Layout()
{
    // 開発者向けの項目が増えたので、縦に少し広げている
    window_ = Rect::FromXYWH(360.0f, 80.0f, 1200.0f, 940.0f);

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

    // プレイヤー名の変更はデバッグモード中だけ押せる
    nameButton_ = Button(Rect::FromXYWH(left, window_.top + 640.0f, 260.0f, 48.0f),
                         "プレイヤー名を変更", FontSize::Small);
    nameButton_.SetAccent(palette::kExp);

    deleteSaveButton_ = Button(Rect::FromXYWH(left, window_.top + 750.0f, 260.0f, 52.0f),
                               "セーブデータ削除", FontSize::Small);
    deleteSaveButton_.SetAccent(palette::kDanger);

    logoutButton_ = Button(Rect::FromXYWH(left + 276.0f, window_.top + 750.0f, 260.0f, 52.0f),
                           "ログアウト", FontSize::Small);
    logoutButton_.SetAccent(palette::kAccentWarm);

    // キー設定（右カラムの操作一覧の下）
    keyConfigButton_ = Button(Rect::FromXYWH(right + 44.0f, window_.top + 640.0f, 300.0f, 52.0f),
                              "キー設定を変更", FontSize::Small);

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
    confirmingLogout_ = false;
    deleteSaveButton_.SetLabel("セーブデータ削除");
    logoutButton_.SetLabel("ログアウト");
    keyConfigPanel_.Close();
    message_.clear();
    messageTimer_ = 0.0f;
}

void SettingsPanel::Update(float dt, const Input& input, GameContext& context)
{
    if (!open_) return;
    closeRequested_ = false;

    // --- プレイヤー名の入力中はそちらを優先 --------------------------------------
    if (namePanel_.IsOpen()) {
        namePanel_.Update(dt, input);
        if (namePanel_.Confirmed()) {
            context.player.SetName(namePanel_.Result());
            message_ = str::Format("プレイヤー名を「%s」に変更しました",
                                   context.player.Name().c_str());
            messageTimer_ = 3.0f;
        }
        return;
    }

    // --- キー設定を開いている間はそちらを優先 --------------------------------------
    if (keyConfigPanel_.IsOpen()) {
        keyConfigPanel_.Update(dt, input, context);
        return;
    }
    if (keyConfigButton_.Update(input, dt)) {
        keyConfigPanel_.Open();
        return;
    }

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

    // --- プレイヤー名の変更（デバッグモード中のみ） --------------------------------
    nameButton_.SetEnabled(context.settings.debugMode);
    if (context.settings.debugMode && nameButton_.Update(input, dt)) {
        namePanel_.SetTitle("プレイヤー名の変更");
        namePanel_.SetCancelEnabled(true);
        namePanel_.Open(context.player.Name());
        return;
    }

    // --- セーブデータ削除（2 段階で確認する） ------------------------------------
    if (deleteSaveButton_.Update(input, dt)) {
        if (!confirmingDelete_) {
            confirmingDelete_ = true;
            deleteSaveButton_.SetLabel("本当に削除しますか？");
        } else {
            confirmingDelete_ = false;
            deleteSaveButton_.SetLabel("セーブデータ削除");
            if (SaveSystem::Remove()) {
                // 削除後に自動セーブで書き戻されると消えないため、保存を止める
                context.autoSaveEnabled = false;
                message_ = "セーブデータを削除しました（以降は自動セーブしません）";
            } else {
                message_ = SaveSystem::LastError();
            }
            messageTimer_ = 4.0f;
        }
    }

    // --- ログアウト（2 段階で確認する） ------------------------------------------
    if (logoutButton_.Update(input, dt)) {
        if (!confirmingLogout_) {
            confirmingLogout_ = true;
            logoutButton_.SetLabel("本当に終了しますか？");
            message_ = context.autoSaveEnabled ? "セーブしてからゲームを終了します"
                                               : "セーブせずにゲームを終了します";
            messageTimer_ = 4.0f;
        } else {
            // 終了要求。保存は Application 側で行う
            context.quitRequested = true;
        }
    }

    if (closeButton_.Update(input, dt) || input.Pressed(GameAction::Cancel)) {
        closeRequested_ = true;
        open_ = false;
        confirmingDelete_ = false;
        confirmingLogout_ = false;
        deleteSaveButton_.SetLabel("セーブデータ削除");
        logoutButton_.SetLabel("ログアウト");
        namePanel_.Close();
        keyConfigPanel_.Close();
    }
}

void SettingsPanel::Draw(const GameContext& context) const
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
    nameButton_.Draw();
    if (debugToggle_.Value()) {
        draw::Text(FontSize::Tiny, columnLeft_ + 12.0f, window_.top + 700.0f, palette::kAccentWarm,
                   "戦闘中 : F1 無敵 / F2 殲滅 / F3 全回復 / F4 判定表示");
        draw::Text(FontSize::Tiny, columnLeft_ + 12.0f, window_.top + 722.0f, palette::kAccentWarm,
                   "ホーム : 画面左下のボタンで col・結晶・SP・LV・装備・アイテム等を追加");
    } else {
        draw::Text(FontSize::Tiny, columnLeft_ + 288.0f, window_.top + 654.0f,
                   palette::kTextDisabled, "※ デバッグモード中のみ変更できます");
    }
    deleteSaveButton_.Draw();
    logoutButton_.Draw();

    // --- 操作一覧（右カラム） --------------------------------------------------
    DrawKeyGuide(context);
    keyConfigButton_.Draw();

    closeButton_.Draw();
    // バージョン（左下）
    draw::Text(FontSize::Tiny, columnLeft_, window_.bottom - 52.0f, palette::kTextDim,
               str::Format("The Eclipse %s（%s）", version::kVersionLabel, version::kVersionDate));

    if (messageTimer_ > 0.0f) {
        draw::Text(FontSize::Tiny, columnLeft_, window_.top + 812.0f, palette::kAccent, message_);
    }

    // 名前入力とキー設定は最前面に重ねる
    namePanel_.Draw();
    keyConfigPanel_.Draw(context);
}

void SettingsPanel::DrawKeyGuide(const GameContext& context) const
{
    const float left = columnRight_ + 44.0f;
    const float right = window_.right - 36.0f;

    DrawSectionHeader(left, window_.top + 70.0f, right - left, "操作一覧");

    float y = window_.top + 112.0f;
    for (const KeyGuide& guide : BuildGuides(context.settings.keys)) {
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
               str::Format("・ホームのタブはソードスキルのキー（%s）と ESC でも開けます",
                           SkillKeys(context.settings.keys).c_str()));
    y += 26.0f;
    draw::Text(FontSize::Tiny, left + 4.0f, y, palette::kTextDim,
               "・奥行きが合っていないと攻撃は当たりません");
}

} // namespace ui
} // namespace ecl
