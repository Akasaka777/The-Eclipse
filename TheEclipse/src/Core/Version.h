//==============================================================================
// Version.h : ゲームのバージョン
//   タイトル画面・設定画面・ウィンドウのタイトルとセーブデータに出す。
//
//   命名規定 : X.Y.Z（ゲーム内の表記は「Ver. X.Y.Z」、GitHub のタグ・Releases は「vX.Y.Z」）
//     X（メジャー）: システムの大規模な改修、有料拡張レベルの刷新、互換性のない仕様変更
//                    → 大きな節目やリリース時に +1 し、Y と Z は 0 に戻す
//     Y（マイナー）: 新ステージの追加、新機能の実装、ゲームバランスの大幅調整
//                    → 機能追加のたびに +1 し、Z は 0 に戻す
//     Z（パッチ）  : 細かなバグ修正、誤字・テキスト修正、軽微な調整
//                    → 不具合修正ごとに +1 する
//
//   新しいバージョンを出すときは
//     1. ここの kVersion・kVersionDate・kVersionLabel を上げる
//     2. CHANGELOG.md の先頭に「## v0.8.0（2026-11-01）」の見出しと変更点を書く
//   この 2 つを同じコミットで push すると、GitHub の Releases に
//   その版のゲーム（zip）が自動で載る（.github/workflows/release.yml）。
//==============================================================================
#pragma once

namespace ecl {
namespace version {

constexpr const char* kVersion = "0.7.0";
constexpr const char* kVersionDate = "2026-10-27";
// 画面に出す表記（「Ver. X.Y.Z」）
constexpr const char* kVersionLabel = "Ver. 0.7.0";

} // namespace version
} // namespace ecl
