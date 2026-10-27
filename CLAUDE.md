# The Eclipse（Claude Code 向けの決まりごと）

DxLib / C++17 / Visual Studio 2022 の横スクロール 2D アクション RPG。
やり取り・コメント・UI の文字・コミットメッセージ・README は日本語で書く。

## バージョンと GitHub への公開（必ず守る）

変更を GitHub に push するときは、次の命名規定でバージョンを上げ、そのつど Releases に公開する。

- 表記：ゲーム内は `Ver. X.Y.Z`、GitHub のタグ・Releases は `vX.Y.Z`（例：`Ver. 1.2.3` / `v1.2.3`）
- **X（メジャー）**：システムの大規模な改修、有料拡張レベルの刷新、互換性のない仕様変更。
  大きな節目やリリース時に +1 し、Y・Z は `0` に戻す。
- **Y（マイナー）**：新ステージの追加、新機能の実装、ゲームバランスの大幅調整。
  機能追加のたびに +1 し、Z は `0` に戻す。
- **Z（パッチ）**：細かなバグ修正、誤字・テキスト修正、軽微な調整。不具合修正ごとに +1 する。
- 1 回の公開に何種類かの変更が入るときは、いちばん大きいものに合わせる（新機能＋バグ修正なら Y）。

### 公開の手順

1. `TheEclipse/src/Core/Version.h` の `kVersion`・`kVersionDate`・`kVersionLabel`（`"Ver. X.Y.Z"`）を上げる
2. `CHANGELOG.md` の先頭に `## vX.Y.Z（YYYY-MM-DD）` の見出しと変更点（追加 / 変更 / 修正）を書く
3. 1 と 2 を同じコミットにして push する
4. GitHub Actions の「リリース」（`.github/workflows/release.yml`）が成功し、
   https://github.com/Akasaka777/The-Eclipse/releases に `vX.Y.Z` が載ったことを確かめる

詳しくは README の「7. バージョン」を参照。
