//==============================================================================
// Version.h : ゲームのバージョン
//   タイトル画面・設定画面・ウィンドウのタイトルとセーブデータに出す。
//
//   番号は「メジャー.マイナー.パッチ」
//     マイナー : 機能を追加・変更したとき
//     パッチ   : 不具合の修正だけのとき
//     メジャー : 正式版（1.0.0）になるまでは 0
//
//   新しいバージョンを出すときは
//     1. ここの kVersion と kVersionDate を上げる
//     2. CHANGELOG.md の先頭に「## v0.7.0（2026-11-01）」の見出しと変更点を書く
//   この 2 つを同じコミットで push すると、GitHub の Releases に
//   その版のゲーム（zip）が自動で載る（.github/workflows/release.yml）。
//==============================================================================
#pragma once

namespace ecl {
namespace version {

constexpr const char* kVersion = "0.6.0";
constexpr const char* kVersionDate = "2026-10-26";
// 画面に出す表記
constexpr const char* kVersionLabel = "Ver 0.6.0";

} // namespace version
} // namespace ecl
