#include "Common/StringUtil.h"

#include <cstdarg>
#include <cstdio>
#include <vector>

namespace ecl {
namespace str {

std::string Format(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    va_list copy;
    va_copy(copy, args);
    const int needed = std::vsnprintf(nullptr, 0, fmt, copy);
    va_end(copy);

    std::string result;
    if (needed > 0) {
        std::vector<char> buffer(static_cast<size_t>(needed) + 1, '\0');
        std::vsnprintf(buffer.data(), buffer.size(), fmt, args);
        result.assign(buffer.data(), static_cast<size_t>(needed));
    }
    va_end(args);
    return result;
}

std::string Comma(int value)
{
    const bool negative = value < 0;
    long long v = negative ? -static_cast<long long>(value) : value;

    std::string digits = std::to_string(v);
    std::string out;
    int counter = 0;
    for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
        out.push_back(digits[static_cast<size_t>(i)]);
        if (++counter % 3 == 0 && i > 0) out.push_back(',');
    }
    if (negative) out.push_back('-');
    return std::string(out.rbegin(), out.rend());
}

std::string TimeText(float seconds)
{
    if (seconds < 0.0f) seconds = 0.0f;
    const int total = static_cast<int>(seconds);
    const int minutes = total / 60;
    const int secs = total % 60;
    const int hundredths = static_cast<int>((seconds - static_cast<float>(total)) * 100.0f);
    return Format("%02d:%02d.%02d", minutes, secs, hundredths);
}

std::string Signed(int value)
{
    return value >= 0 ? Format("+%d", value) : Format("%d", value);
}

//------------------------------------------------------------------------------
// UTF-8 ヘルパ
//------------------------------------------------------------------------------
namespace {
// 先頭バイトから、その文字のバイト数を求める
int Utf8CharBytes(unsigned char lead)
{
    if (lead < 0x80) return 1;
    if ((lead & 0xE0) == 0xC0) return 2;
    if ((lead & 0xF0) == 0xE0) return 3;
    if ((lead & 0xF8) == 0xF0) return 4;
    return 1; // 壊れたデータでも止まらないように 1 バイト進める
}

// 末尾の 1 文字が始まる位置
size_t LastCharBegin(const std::string& text)
{
    size_t begin = 0;
    for (size_t i = 0; i < text.size();) {
        begin = i;
        i += static_cast<size_t>(Utf8CharBytes(static_cast<unsigned char>(text[i])));
    }
    return begin;
}
} // namespace

int CharCount(const std::string& text)
{
    int count = 0;
    for (size_t i = 0; i < text.size(); ++count) {
        i += static_cast<size_t>(Utf8CharBytes(static_cast<unsigned char>(text[i])));
    }
    return count;
}

std::string BackChar(const std::string& text)
{
    if (text.empty()) return std::string();
    return text.substr(LastCharBegin(text));
}

void PopBackChar(std::string& text)
{
    if (text.empty()) return;
    text.erase(LastCharBegin(text));
}

std::string Truncate(const std::string& text, int maxChars)
{
    if (maxChars <= 0) return std::string();

    std::string result;
    int count = 0;
    for (size_t i = 0; i < text.size() && count < maxChars; ++count) {
        const int bytes = Utf8CharBytes(static_cast<unsigned char>(text[i]));
        result += text.substr(i, static_cast<size_t>(bytes));
        i += static_cast<size_t>(bytes);
    }
    return result;
}

} // namespace str
} // namespace ecl
