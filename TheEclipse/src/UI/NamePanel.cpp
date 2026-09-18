#include "UI/NamePanel.h"

#include "Common/MathUtil.h"
#include "Common/StringUtil.h"
#include "Core/DxInclude.h"
#include "Core/GameConfig.h"
#include "Game/PlayerData.h"
#include "Graphics/DrawUtil.h"

#include <cmath>

namespace ecl {
namespace ui {

namespace {

// --- 文字パネルの寸法 ---------------------------------------------------------
constexpr int   kGridColumns = 10;
constexpr int   kGridRows = 7;
constexpr float kCellWidth = 52.0f;
constexpr float kCellHeight = 40.0f;
constexpr float kCellGap = 4.0f;

//------------------------------------------------------------------------------
// 文字表（左上から右下へ 10 列で並べる。空文字は空きマス）
//------------------------------------------------------------------------------
const std::vector<const char*>& HiraganaChars()
{
    static const std::vector<const char*> chars = {
        "あ", "か", "さ", "た", "な", "は", "ま", "や", "ら", "わ",
        "い", "き", "し", "ち", "に", "ひ", "み", "",   "り", "を",
        "う", "く", "す", "つ", "ぬ", "ふ", "む", "ゆ", "る", "ん",
        "え", "け", "せ", "て", "ね", "へ", "め", "",   "れ", "ー",
        "お", "こ", "そ", "と", "の", "ほ", "も", "よ", "ろ", "・",
        "",   "",   "",   "",   "",   "",   "",   "",   "",   "",
        "",   "",   "",   "",   "",   "",   "",   "",   "",   "",
    };
    return chars;
}

const std::vector<const char*>& KatakanaChars()
{
    static const std::vector<const char*> chars = {
        "ア", "カ", "サ", "タ", "ナ", "ハ", "マ", "ヤ", "ラ", "ワ",
        "イ", "キ", "シ", "チ", "ニ", "ヒ", "ミ", "",   "リ", "ヲ",
        "ウ", "ク", "ス", "ツ", "ヌ", "フ", "ム", "ユ", "ル", "ン",
        "エ", "ケ", "セ", "テ", "ネ", "ヘ", "メ", "",   "レ", "ー",
        "オ", "コ", "ソ", "ト", "ノ", "ホ", "モ", "ヨ", "ロ", "・",
        "",   "",   "",   "",   "",   "",   "",   "",   "",   "",
        "",   "",   "",   "",   "",   "",   "",   "",   "",   "",
    };
    return chars;
}

const std::vector<const char*>& AlphabetChars()
{
    static const std::vector<const char*> chars = {
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J",
        "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",
        "U", "V", "W", "X", "Y", "Z", "a", "b", "c", "d",
        "e", "f", "g", "h", "i", "j", "k", "l", "m", "n",
        "o", "p", "q", "r", "s", "t", "u", "v", "w", "x",
        "y", "z", "0", "1", "2", "3", "4", "5", "6", "7",
        "8", "9", "-", "_", ".", "!", "?", "&", "",  "",
    };
    return chars;
}

//------------------------------------------------------------------------------
// 濁点・半濁点・小書きの切り替え表
//   1 番目 → 2 番目 → 3 番目 → 1 番目 の順に巡回する（3 番目が空なら 2 つで往復）
//------------------------------------------------------------------------------
struct CharCycle
{
    const char* a;
    const char* b;
    const char* c;
};

const CharCycle kVoicedTable[] = {
    { "か", "が", "" }, { "き", "ぎ", "" }, { "く", "ぐ", "" }, { "け", "げ", "" }, { "こ", "ご", "" },
    { "さ", "ざ", "" }, { "し", "じ", "" }, { "す", "ず", "" }, { "せ", "ぜ", "" }, { "そ", "ぞ", "" },
    { "た", "だ", "" }, { "ち", "ぢ", "" }, { "つ", "づ", "" }, { "て", "で", "" }, { "と", "ど", "" },
    { "は", "ば", "ぱ" }, { "ひ", "び", "ぴ" }, { "ふ", "ぶ", "ぷ" },
    { "へ", "べ", "ぺ" }, { "ほ", "ぼ", "ぽ" },
    { "う", "ゔ", "" },
    { "カ", "ガ", "" }, { "キ", "ギ", "" }, { "ク", "グ", "" }, { "ケ", "ゲ", "" }, { "コ", "ゴ", "" },
    { "サ", "ザ", "" }, { "シ", "ジ", "" }, { "ス", "ズ", "" }, { "セ", "ゼ", "" }, { "ソ", "ゾ", "" },
    { "タ", "ダ", "" }, { "チ", "ヂ", "" }, { "ツ", "ヅ", "" }, { "テ", "デ", "" }, { "ト", "ド", "" },
    { "ハ", "バ", "パ" }, { "ヒ", "ビ", "ピ" }, { "フ", "ブ", "プ" },
    { "ヘ", "ベ", "ペ" }, { "ホ", "ボ", "ポ" },
    { "ウ", "ヴ", "" },
};

const CharCycle kSmallTable[] = {
    { "あ", "ぁ", "" }, { "い", "ぃ", "" }, { "う", "ぅ", "" }, { "え", "ぇ", "" }, { "お", "ぉ", "" },
    { "つ", "っ", "" }, { "や", "ゃ", "" }, { "ゆ", "ゅ", "" }, { "よ", "ょ", "" }, { "わ", "ゎ", "" },
    { "ア", "ァ", "" }, { "イ", "ィ", "" }, { "ウ", "ゥ", "" }, { "エ", "ェ", "" }, { "オ", "ォ", "" },
    { "ツ", "ッ", "" }, { "ヤ", "ャ", "" }, { "ユ", "ュ", "" }, { "ヨ", "ョ", "" }, { "ワ", "ヮ", "" },
};

// 表に従って次の文字を返す（該当が無ければ空文字）
std::string NextInCycle(const CharCycle* table, size_t count, const std::string& current)
{
    for (size_t i = 0; i < count; ++i) {
        const CharCycle& entry = table[i];
        const bool hasThird = (entry.c[0] != '\0');
        if (current == entry.a) return entry.b;
        if (current == entry.b) return hasThird ? entry.c : entry.a;
        if (hasThird && current == entry.c) return entry.a;
    }
    return std::string();
}

const char* CharSetName(CharSet set)
{
    switch (set) {
    case CharSet::Hiragana: return "ひらがな";
    case CharSet::Katakana: return "カタカナ";
    default:                return "英数字";
    }
}

} // namespace

NamePanel::NamePanel()
{
    Layout();
}

void NamePanel::Layout()
{
    window_ = Rect::FromXYWH(640.0f, 190.0f, 640.0f, 700.0f);

    nameField_ = Rect::FromXYWH(window_.left + 40.0f, window_.top + 76.0f, 560.0f, 64.0f);

    // --- 文字種タブ -----------------------------------------------------------
    setButtons_.clear();
    const float tabWidth = 180.0f;
    const float tabGap = 10.0f;
    for (int i = 0; i < static_cast<int>(CharSet::Count); ++i) {
        const float x = window_.left + 40.0f + (tabWidth + tabGap) * static_cast<float>(i);
        setButtons_.push_back(Button(Rect::FromXYWH(x, window_.top + 156.0f, tabWidth, 46.0f),
                                     CharSetName(static_cast<CharSet>(i)), FontSize::Small));
    }

    // --- 文字セル -------------------------------------------------------------
    const float gridLeft = window_.left + 42.0f;
    const float gridTop = window_.top + 210.0f;
    const float gridWidth = kCellWidth * kGridColumns + kCellGap * (kGridColumns - 1);
    const float gridHeight = kCellHeight * kGridRows + kCellGap * (kGridRows - 1);
    gridArea_ = Rect(gridLeft, gridTop, gridLeft + gridWidth, gridTop + gridHeight);

    cellRects_.clear();
    for (int row = 0; row < kGridRows; ++row) {
        for (int col = 0; col < kGridColumns; ++col) {
            cellRects_.push_back(
                Rect::FromXYWH(gridLeft + (kCellWidth + kCellGap) * static_cast<float>(col),
                               gridTop + (kCellHeight + kCellGap) * static_cast<float>(row),
                               kCellWidth, kCellHeight));
        }
    }

    // --- 補助ボタン -----------------------------------------------------------
    const float helperWidth = 130.0f;
    const float helperGap = 10.0f;
    const float helperLeft = window_.left + 45.0f;
    const float helperTop = window_.top + 528.0f;
    voicedButton_ = Button(Rect::FromXYWH(helperLeft, helperTop, helperWidth, 48.0f),
                           "゛ ゜", FontSize::Small);
    smallButton_ = Button(Rect::FromXYWH(helperLeft + (helperWidth + helperGap), helperTop,
                                         helperWidth, 48.0f), "小文字", FontSize::Small);
    backButton_ = Button(Rect::FromXYWH(helperLeft + (helperWidth + helperGap) * 2.0f, helperTop,
                                        helperWidth, 48.0f), "1 文字削除", FontSize::Small);
    clearButton_ = Button(Rect::FromXYWH(helperLeft + (helperWidth + helperGap) * 3.0f, helperTop,
                                         helperWidth, 48.0f), "全消去", FontSize::Small);
    clearButton_.SetAccent(palette::kAccentWarm);

    // --- 決定 / キャンセル -----------------------------------------------------
    confirmButton_ = Button(Rect::FromXYWH(window_.left + 50.0f, window_.top + 604.0f,
                                           260.0f, 60.0f), "決定");
    cancelButton_ = Button(Rect::FromXYWH(window_.left + 330.0f, window_.top + 604.0f,
                                          260.0f, 60.0f), "キャンセル");
    cancelButton_.SetAccent(palette::kTextDim);
}

void NamePanel::Open(const std::string& currentName)
{
    open_ = true;
    confirmed_ = false;
    cancelled_ = false;
    charSet_ = CharSet::Hiragana;
    input_ = currentName;
    message_.clear();
    messageTimer_ = 0.0f;
    time_ = 0.0f;
}

const std::vector<const char*>& NamePanel::CurrentChars() const
{
    switch (charSet_) {
    case CharSet::Hiragana: return HiraganaChars();
    case CharSet::Katakana: return KatakanaChars();
    default:                return AlphabetChars();
    }
}

void NamePanel::AppendChar(const char* text)
{
    if (!text || text[0] == '\0') return;
    if (str::CharCount(input_) >= PlayerData::MaxNameLength()) {
        message_ = str::Format("%d 文字までです", PlayerData::MaxNameLength());
        messageTimer_ = 2.0f;
        return;
    }
    input_ += text;
}

void NamePanel::CycleVoiced()
{
    const std::string last = str::BackChar(input_);
    const std::string next = NextInCycle(kVoicedTable,
                                         sizeof(kVoicedTable) / sizeof(kVoicedTable[0]), last);
    if (next.empty()) {
        message_ = "濁点を付けられない文字です";
        messageTimer_ = 2.0f;
        return;
    }
    str::PopBackChar(input_);
    input_ += next;
}

void NamePanel::CycleSmall()
{
    const std::string last = str::BackChar(input_);
    const std::string next = NextInCycle(kSmallTable,
                                         sizeof(kSmallTable) / sizeof(kSmallTable[0]), last);
    if (next.empty()) {
        message_ = "小文字にできない文字です";
        messageTimer_ = 2.0f;
        return;
    }
    str::PopBackChar(input_);
    input_ += next;
}

void NamePanel::Update(float dt, const Input& input)
{
    if (!open_) return;

    confirmed_ = false;
    cancelled_ = false;
    time_ += dt;
    messageTimer_ = math::MaxF(0.0f, messageTimer_ - dt);

    // --- 文字種タブ -----------------------------------------------------------
    for (int i = 0; i < static_cast<int>(setButtons_.size()); ++i) {
        setButtons_[static_cast<size_t>(i)].SetSelected(static_cast<CharSet>(i) == charSet_);
        if (setButtons_[static_cast<size_t>(i)].Update(input, dt)) {
            charSet_ = static_cast<CharSet>(i);
        }
    }

    // --- 文字セル -------------------------------------------------------------
    if (input.MouseClicked(MouseButton::Left)) {
        const float mouseX = static_cast<float>(input.MouseX());
        const float mouseY = static_cast<float>(input.MouseY());
        const std::vector<const char*>& chars = CurrentChars();
        for (size_t i = 0; i < cellRects_.size() && i < chars.size(); ++i) {
            if (chars[i][0] == '\0') continue;
            if (cellRects_[i].Contains(mouseX, mouseY)) AppendChar(chars[i]);
        }
    }

    // --- 補助ボタン -----------------------------------------------------------
    if (voicedButton_.Update(input, dt)) CycleVoiced();
    if (smallButton_.Update(input, dt)) CycleSmall();
    if (backButton_.Update(input, dt)) str::PopBackChar(input_);
    if (clearButton_.Update(input, dt)) input_.clear();

    // BACKSPACE でも 1 文字消せるようにする
    if (input.KeyPressed(KEY_INPUT_BACK)) str::PopBackChar(input_);

    // --- 決定 / キャンセル -----------------------------------------------------
    confirmButton_.SetEnabled(!input_.empty());
    if (confirmButton_.Update(input, dt) && confirmButton_.Enabled()) {
        confirmed_ = true;
        open_ = false;
        return;
    }
    if (!input_.empty() && input.Pressed(GameAction::Confirm)) {
        confirmed_ = true;
        open_ = false;
        return;
    }

    cancelButton_.SetEnabled(cancelEnabled_);
    const bool cancelPressed = cancelButton_.Update(input, dt) && cancelEnabled_;
    if (cancelPressed || (cancelEnabled_ && input.Pressed(GameAction::Cancel))) {
        cancelled_ = true;
        open_ = false;
    }
}

void NamePanel::Draw() const
{
    if (!open_) return;

    DrawDimOverlay(180);
    DrawWindow(window_, title_);

    // --- 入力欄 ---------------------------------------------------------------
    draw::FillRect(nameField_, palette::kPanelDark, 235);
    draw::StrokeRect(nameField_, palette::kAccent, 2.0f, 255);

    const std::string shown = input_.empty() ? std::string("（未入力）") : input_;
    const ColorRGB textColor = input_.empty() ? palette::kTextDisabled : palette::kText;
    draw::Text(FontSize::Medium, nameField_.left + 16.0f, nameField_.top + 16.0f, textColor, shown);

    // 入力位置のカーソル
    if (!input_.empty() && std::sin(time_ * 6.0f) > 0.0f) {
        const float cursorX = nameField_.left + 20.0f
                            + static_cast<float>(draw::TextWidth(FontSize::Medium, input_));
        draw::Line(cursorX, nameField_.top + 14.0f, cursorX, nameField_.bottom - 14.0f,
                   palette::kAccent, 2.0f, 220);
    }

    draw::Text(FontSize::Tiny, nameField_.right, nameField_.top - 22.0f, palette::kTextDim,
               str::Format("%d / %d 文字", str::CharCount(input_), PlayerData::MaxNameLength()),
               draw::TextAlign::Right);

    // --- 文字種タブ -----------------------------------------------------------
    for (const Button& button : setButtons_) button.Draw();

    // --- 文字セル -------------------------------------------------------------
    draw::FillRect(gridArea_.Expanded(8.0f), palette::kPanelDark, 190);
    draw::StrokeRect(gridArea_.Expanded(8.0f), palette::kBorder.Scaled(0.6f), 1.0f, 160);

    const std::vector<const char*>& chars = CurrentChars();
    const float mouseX = static_cast<float>(Input::Instance().MouseX());
    const float mouseY = static_cast<float>(Input::Instance().MouseY());
    for (size_t i = 0; i < cellRects_.size() && i < chars.size(); ++i) {
        if (chars[i][0] == '\0') continue;
        const Rect& cell = cellRects_[i];
        const bool hovered = cell.Contains(mouseX, mouseY);

        draw::FillRect(cell, hovered ? palette::kPanelLight : palette::kPanel, 235);
        draw::StrokeRect(cell, hovered ? palette::kAccent : palette::kBorder.Scaled(0.6f),
                         hovered ? 2.0f : 1.0f, 255);
        draw::Text(FontSize::Normal, cell.CenterX(), cell.top + 6.0f, palette::kText, chars[i],
                   draw::TextAlign::Center);
    }

    // --- ボタン ---------------------------------------------------------------
    voicedButton_.Draw();
    smallButton_.Draw();
    backButton_.Draw();
    clearButton_.Draw();
    confirmButton_.Draw();
    if (cancelEnabled_) cancelButton_.Draw();

    if (messageTimer_ > 0.0f) {
        draw::Text(FontSize::Tiny, window_.CenterX(), window_.top + 582.0f, palette::kDanger,
                   message_, draw::TextAlign::Center);
    }
}

} // namespace ui
} // namespace ecl
