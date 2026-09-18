//==============================================================================
// StringUtil.h : 文字列ユーティリティ
//==============================================================================
#pragma once

#include <string>

namespace ecl {
namespace str {

// printf 形式のフォーマット
std::string Format(const char* fmt, ...);
// 1234567 -> "1,234,567"
std::string Comma(int value);
// 秒数 -> "01:23.45"
std::string TimeText(float seconds);
// 符号付き表記 "+12" / "-3"
std::string Signed(int value);

//------------------------------------------------------------------------------
// UTF-8 の文字単位で扱うヘルパ（日本語の名前入力などで使う）
//------------------------------------------------------------------------------
// 文字数（バイト数ではない）
int CharCount(const std::string& text);
// 末尾の 1 文字（無ければ空文字）
std::string BackChar(const std::string& text);
// 末尾の 1 文字を取り除く
void PopBackChar(std::string& text);
// 先頭から maxChars 文字までに切り詰める
std::string Truncate(const std::string& text, int maxChars);

} // namespace str
} // namespace ecl
