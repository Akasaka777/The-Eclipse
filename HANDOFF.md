# HANDOFF（引き継ぎメモ）

前のセッションの会話は読めない前提で、このファイルだけで作業を再開できるようにまとめたものです。
**【確認済み】** は実際に動かして・見て確かめたこと、**【推測】** は確かめていないことです。

- 作成日：2026-11-05
- リポジトリ：`Akasaka777/The-Eclipse`（https://github.com/Akasaka777/The-Eclipse）
- 作業ブランチ：`claude/cool-clarke-2e2wss`（`main` は「Initial commit」だけで、作業はすべてこのブランチ）
- 公開済みの最新版：v0.7.0（コミット `bdfe659`）。このファイルは v0.7.1 として push（下の「0.」を参照）
- 2026-11-05 追記：v0.7.1 の公開を確認。続けて「左上のプレイヤーステータスのグラフィック向上」を v0.8.0、
  「HP / MP バーの流れるハイライトをなくす」を v0.8.1、「公式サイトにつながらないときは DXライブラリのキャッシュを使う」を
  v0.8.2 として push（下の「2.」の表）
- 2026-11-20 追記：「端でカメラをゆっくり止める」「LUK で獲得経験値を上げる」を v0.9.0 として push（下の「2.」の表）
- 2026-11-22 追記：ゲーム名を **Ars Gladii** に変更し、竜王の火山の隠しフロア・隠しボス・黒い剣・開発者モードのボタンを
  v0.10.0 として push（下の「2.」の表）。リポジトリ名・プロジェクト名・フォルダ名（TheEclipse）は変えていない
- 2026-11-22 追記：タイトル画面の背景とサブタイトルを Ars Gladii に合わせて作り直し、v0.10.1 として push

---

## 0. 最初に守ること（作業の決まり）

- **やり取り・コードのコメント・UI の文字・コミットメッセージ・README は日本語。**
- **`CLAUDE.md` を必ず読む。** 「変更を GitHub に push するときは命名規定でバージョンを上げ、そのつど Releases に公開する」
  という決まりがある（ユーザーの指示。2026-10-27）。
  - 命名規定：ゲーム内は `Ver. X.Y.Z`、タグ・Releases は `vX.Y.Z`。
    X＝大規模改修・互換性のない変更（Y・Z を 0 に）、Y＝新ステージ・新機能・大幅なバランス調整（Z を 0 に）、
    Z＝細かなバグ修正・誤字・軽微な調整。
  - 1 回に複数の種類が入るときは大きい方に合わせる（新機能＋修正なら Y）。← これは規定に無かったので前任が決めた解釈。
  - このファイルを追加するだけの push も、この決まりに従って Z を上げた（v0.7.1。ゲームの中身は v0.7.0 と同じ）。
- push 先は指定されたブランチだけ（前回は `claude/cool-clarke-2e2wss`。次のセッションで別のブランチ名を
  指定されたらそちらに従う）。`git push -u origin <ブランチ名>`。
- **プルリクエストはユーザーに頼まれるまで作らない。**
- コミットメッセージの末尾にはセッションの指示にある帰属行（`Co-Authored-By: …` と `Claude-Session: …`）を付ける。
  モデル名・モデル ID はコミット・README・コードに書かない。
- `gh` コマンドは使えない。GitHub の Actions・Releases の確認は GitHub の MCP ツール
  （`mcp__github__actions_list` / `actions_get` / `get_job_logs` / `get_release_by_tag` / `list_releases`）を使う。

---

## 1. タスクの目的と完了条件

### 目的
DxLib / C++17 / Visual Studio 2022 / 1920x1080 の横スクロール 2D アクション RPG「The Eclipse」に、
ユーザーから来る「変更・修正・追加」の依頼を順に実装していく。

### 1 件の依頼の完了条件（これまでの進め方）
1. 実装する（Windows 専用の API 以外は Linux のテスト環境で動かして確かめる。下の「付録」）
2. テストを足して全部通す（壊して NG になることも確かめる）
3. README（と必要なら CHANGELOG）を更新する
4. `Version.h` と `CHANGELOG.md` を命名規定で上げ、同じコミットで push する
5. GitHub Actions の「リリース」が成功し、Releases に `vX.Y.Z` と zip が載ったことを MCP ツールで確かめる
6. 日本語で報告する（スクリーンショットはテスト環境の簡易描画であることを添える）

### いまの状態【確認済み】
- ユーザーからの依頼はすべて完了。**未着手の依頼は無い。**
- Releases に v0.1.0〜v0.7.0 の 7 版が zip 付きで載っている（2026-10-27 に MCP で確認）。
  https://github.com/Akasaka777/The-Eclipse/releases

---

## 2. 完了したこと

| 版 / コミット | 依頼（要約） | やったこと |
|---|---|---|
| `c25653b` | 二刀流の左手を奥に・ショップの購入数・ボスの形態変化・HP バー・ボス名・キー設定・リザルトの開放演出・HP バー本数 3〜10 | すべて実装 |
| `eefd90e` | ボスフロア到着時のボス演出 | 登場演出（パン・咆哮・名前の札・HP バーが溜まる） |
| `eb286dc` | リザルトの戦闘記録からユニークスキル通知を削除・HP バーを湾曲・名前を頭上・マウスのサイドボタン | 実装 |
| `744605a` → `93a17c5` → `99ccc75` → `532588d` | HP バーの形の試行錯誤（下の「4.」） | 最終的に「小さなまっすぐのバーを顔の右横」 |
| `abab403` | ボス名が左にずれる→小さく／バーの上で真ん中に | 名前を 14px にしてバーの上の中央へ |
| `0aec2fc`（v0.6.0） | ゲームのバージョンを追加・過去のバージョンを GitHub からダウンロード可能に | `Version.h`・`CHANGELOG.md`・リリース用ワークフロー。v0.1.0〜v0.6.0 を公開 |
| `770272a` | クエストのデバッグモードにフロア移動ボタン | ◀ / 1F〜 / ボス / ▶ |
| `bdfe659`（v0.7.0） | 今後は命名規定に従って GitHub に投稿 | 規定を README・Version.h・CLAUDE.md に記載。表記を `Ver. X.Y.Z` に。v0.7.0 を公開 |
| v0.8.0 | 画面左上のプレイヤーステータス（アイコン・HP・MP）のグラフィックを向上 | `UI/BattleHud.*` の `DrawPlayerStatus` / `DrawPortrait` を作り直し（丸いメダル・レベルの札・薄れる板・目盛りとつやのあるバー・被弾 / 回復の光・瀕死の赤い脈動）。配置は `BattleHud::StatusLayout()`、定数は `BattleHud.cpp` 冒頭 |
| v0.8.1 | プレイヤーの HP バーと MP バーの流れるハイライトをなくす | `DrawGauge` の流れる光と `kShimmer*` を削除（先端の光・被弾 / 回復の光・瀕死の脈動は残す）。**流れる光を戻さないこと** |
| v0.8.2 | リリースで DXライブラリの公式サイトにつながらないときはキャッシュを使う | v0.8.1 の 1 回目のリリースが公式サイトへの接続の時間切れ（WinError 10060）で失敗したため。`release.py` の `dxlib-url` は 3 回読み直し、だめなら `url=`（空）を出して止めない。ワークフローは url が空のとき `actions/cache/restore@v4`（`restore-keys: dxlib-http`、`fail-on-cache-miss`）で前回のキャッシュを使う |
| v0.9.0 | 端でカメラがいきなり止まる → 端に近づくとカメラを遅くする／LUK で経験値の獲得倍率を上げる | カメラ：`Core/Camera.*` の `EaseIntoRange` と `kCameraEdgeEase = 240`（追従の目標を端の内側 240px から寄せ、外側 240px でちょうど端。ボス登場演出は `SnapToTarget(false)` / `Update(dt, false)` で今までどおり）。LUK：`Game/Ability.*` の `kLukExpRatePerPoint = 0.01` と `AbilityExpRate`、`PlayerData::ExpRateMultiplier`、`QuestScene::FinishQuest` で合計に掛けて四捨五入（`QuestResult::expRate`）。ステータス画面に「獲得経験値」の行、リザルトに「LUK 補正 ×1.20 込み」 |
| v0.10.0 | ゲーム名を Ars Gladii に／竜王の火山に隠しフロア（10〜30 回に 1 回の抽選）／隠しボス ヴァジュラ・ラグナシュラ（上半身の仏像・腕 6 本・空から遠距離、たまに降りる）／黒い剣（1%・戦力 700〜710・クエスト詳細に出さない）／開発者モードに「隠しフロア」「隠しクエスト」 | 名前：`config::kGameTitle`。リリース：`release.py` の `product()` / `RENAMED_VERSION = (0,10,0)` で zip・exe・題名を版ごとに切り替え（ワークフローは `matrix.asset` / `matrix.title`）。抽選：`QuestDatabase.h` の `kHiddenFloor*`（確率 50% は**仕様に無く私が決めた**）、`PlayerData::Begin/EndHiddenFloorAttempt`、セーブの `hiddenfloor` 行。隠しフロア：`QuestDef::hasHiddenFloor / hiddenFloor / hiddenBoss*`（floors には含めず、番号は `FloorCount()`）、`QuestScene::FloorAt / IsHiddenFloor / OpenHiddenGate`、テーマ `StageTheme::Sanctum`。ボス：`kVajraBossId = 6`、`BossDef::flying / hoverHeight = 240`、攻撃 `SkyVolley / SkyThunder / SkyHalo / DiveSlam`、描画 `ArtStyle::Statue`（`CharacterArt.cpp` の `DrawStatue`）、雷 `CombatSystem::AddBolt`。剣：`kBlackSwordId = 170`、個体値 98〜100 で落とす。隠しクエストのボタンは**未実装の案内だけ** |
| v0.10.1 | タイトル画面を Ars Gladii に合う背景・サブタイトルへ | `Scenes/TitleScene.cpp` の `DrawTitleBackground`（夜明けの空・朝日と光の筋・山並み・丘・`PlantedSword`・`GreatSword`・火の粉・`Vignette`）。サブタイトル `kTitleSubtitle`「― 剣は技となり、技は道となる ―」、添え書き `kTitleCaption`、金色 `kTitleGold`（ボタンの縁も） |

過去の版とコミットの対応（`CHANGELOG.md` に書いてある）：
v0.1.0=`27072c7`、v0.2.0=`fcc879d`、v0.3.0=`9c6c725`、v0.4.0=`dd109be`、v0.5.0=`d81fcf4`、
v0.6.0=`0aec2fc`、v0.7.0=`bdfe659`。v0.5.0 以前はバージョンの仕組みを入れる前のコミットを後からビルドしたもの。

---

## 3. 変更したファイルと要点（`d81fcf4..HEAD`）

### ボスの HP バー・形態変化・登場演出
- `TheEclipse/src/Game/Boss.h / Boss.cpp`
  - HP バー本数 `kMinBossHpBars = 3`〜`kMaxBossHpBars = 10`。`Setup(def, pos, powerScale, hpBars)`。
  - 1 本削りきるごとに形態変化。1 回の攻撃でバーの境目までしか削れない（`ApplyDirectDamage` で上限）。
    デバッグの殲滅用に `ForceKill`。`HpBarFill(ratio, index, bars)` で各バーの残量。
  - 登場演出用：`HoldIntro` / `RevealHpBars` / `SetIntroRoar` / `barReveal_`。
  - 表示：`HeadCircle()` で顔の位置（竜・獣は体の前、振り向くと `headSide_` でなめらかに移る）、
    `HpBarLayout()` でバーと名前の位置、`DrawHpBars()` で描画。定数は `Boss.cpp` の HP バーの節：
    ```cpp
    constexpr float kHpBarWidth = 72.0f;   kHpBarHeight = 7.0f;  kHpBarGap = 7.0f;  kHpBarStagger = 2.0f;
    constexpr float kHpBarFaceGap = 12.0f; kHpBarNameGap = 6.0f; kHpBarNameMargin = 8.0f;
    constexpr FontSize kHpBarNameFont = FontSize::Tiny;   // 14px
    ```
  - 配置：いちばん下のバーが顔の高さ、上へ重ねる（上ほど 2px 右）。上のバーから右端→左へ減る。
    名前はいちばん上のバーの上でバー全体の中央（画面端では内側に寄せる）、その上に赤いひし形、さらに上に攻撃の予告。
- `TheEclipse/src/Game/QuestDatabase.h / .cpp`：`QuestDef::bossHpBars`（0 なら推奨戦力から）、
  `BossHpBarCountForPower()`：推奨戦力 1500 で 4 本、以降 2 倍ごとに +1（`kBossHpBarBasePower` / `kBossHpBarPowerStep`）。
- `TheEclipse/src/Scenes/QuestScene.h / .cpp`：`Phase::BossIntro` と `StartBossIntro` / `UpdateBossIntro` /
  `EndBossIntro` / `BossIntroCameraX` / `DrawBossIntro`、時間は冒頭の `kBossIntro*`。演出中は HUD を隠す。

### デバッグモードのフロア移動（v0.7.0）
- `TheEclipse/src/Scenes/QuestScene.h / .cpp`
  - 情報パネル（左上、`DebugPanelRect()` = x28 y254 w420 h230）の下の段に ◀ / 1F〜 / ボス / ▶。
  - `LayoutDebugFloorButtons()`（`OnEnter` で）、`UpdateDebugFloorButtons()`、`DebugFloorMoveAllowed()`、
    `DebugMoveToFloor()`、`DebugFloorName()`。
  - ゲートと同じ暗転（`transitioning_` / `pendingFloor_`）で移動。暗転の文字は `transitionText_`。
  - 押せない場面：いるフロア・暗転中・Victory / Defeat / Finished。ボタンのクリックは `debugClickConsumed_` で攻撃に回さない。

### キー設定（マウスのサイドボタン含む）
- `TheEclipse/src/Core/KeyConfig.h / .cpp`（新規）：アクションごとに 2 枠。入力コードは 0〜255 がキー、
  `1000 + MouseButton`（Left, Right, Middle, SideBack=3, SideForward=4）がマウス、-1 が未設定。
  メニュー・決定・取り消しは固定。同じ入力は入れ替え。
- `TheEclipse/src/Core/Input.h / .cpp`：`ApplyKeyConfig`、`FirstPressedKey`。サイドボタンは DxLib の
  `MOUSE_INPUT_4/5` に加え、Windows では `GetAsyncKeyState(VK_XBUTTON1/2)` も見る（`Input.cpp` 56〜57 行）。
- `TheEclipse/src/UI/KeyConfigPanel.h / .cpp`（新規）、`UI/SettingsPanel.*`（「キー設定を変更」ボタン・操作一覧）、
  `UI/BattleHud.cpp`（キー表示）、`Game/SaveData.cpp`（`keybind` 行で保存）、`Core/GameConfig.h`（`settings.keys`）。

### そのほか
- `Graphics/CharacterArt.cpp`：二刀流の左手の武器を体の奥に描く。
- `UI/ShopPanel.*`・`UI/ItemPanel.cpp`：購入数の増減ボタン。
- `Scenes/ResultScene.*`・`Game/GameContext.h`：習得・開放したものを暗転＋中央に出す。戦闘記録の通知は削除。
- `Scenes/HomeScene.cpp`：設定の中の画面を ESC で閉じると設定が開き直す不具合を修正。

### バージョンと Releases（v0.6.0〜）
- `TheEclipse/src/Core/Version.h`（新規）：
  ```cpp
  constexpr const char* kVersion = "0.7.1";          // このファイルを足した時点
  constexpr const char* kVersionDate = "2026-11-05";
  constexpr const char* kVersionLabel = "Ver. 0.7.1";
  ```
  表示先：`Scenes/TitleScene.cpp`（右下）、`UI/SettingsPanel.cpp`（左下）、`Core/Application.cpp`（ウィンドウのタイトル）、
  `Game/SaveData.cpp`（1 行目のコメント。読み込みには影響しない）。
- `CHANGELOG.md`（新規）：`## vX.Y.Z（YYYY-MM-DD）` が版の区切り。過去版は見出しの下に `コミット: …`。
  `## 未公開` のような版でない見出しは公開対象にならない。
- `.github/workflows/release.yml`（新規）：`CHANGELOG.md` / `Version.h` / ワークフロー / スクリプトが変わった push で動く。
  - plan（ubuntu）→ build（`windows-2022`、DXライブラリを公式サイトから取得・キャッシュ、`msbuild TheEclipse.sln -p:Configuration=Release -p:Platform=x64`）
    → publish（`gh release create vX.Y.Z zip --target <commit>`、古い順に 1 つずつ）。
- `.github/scripts/release.py`（新規）：`plan` / `notes` / `dxlib-url` / `dxlib` / `package`。
  - 版のコミット＝見出しの下の `コミット:`、無ければ「CHANGELOG の見出し」と「Version.h の番号」の両方がそろった最初のコミット。
  - そのコミットの `Version.h` の番号・日付・表記が CHANGELOG と合わなければ止める（表記は `Ver. X.Y.Z`、v0.6.0 までの `Ver X.Y.Z` も通す）。
  - zip：`TheEclipse-vX.Y.Z/` に `TheEclipse.exe`・`README.txt`（BOM 付き UTF-8・CRLF）・`assets/`・`licenses/`（DXライブラリの文書）。
- `CLAUDE.md`（新規）：命名規定と公開の手順。`README.md` の「7. バージョン」に詳しい説明。
- `.gitignore`：`__pycache__/` を追加。

---

## 4. 決めたことと、その理由

- **ボスの HP バーの形**：ユーザーとのやり取りで次の順に変わった。**同じ案をもう一度出さないこと。**
  頭の周り → 頭を包む弧（湾曲）→ 頭の側面に沿う軽い湾曲 →「湾曲させる前（頭上に重ねる）に戻して」→
  SAO の参考画像のような反り返り＋ひし形（画像 2 枚）→「小さくして顔の横に、ぱっと見で見やすく」（3 枚目の画像）＋
  「HP バーはまっすぐに戻して」→ 名前が左にずれる →「小さくするかバーの上にして真ん中に」。
  いまは「小さなまっすぐのバーを顔の右横に重ね、名前（14px）と赤いひし形をバーの上の中央」。
  左端の斜めのカットは「まっすぐ」に合わせて外した（戻せると伝えたが返事は無い）。
- **形態変化はバー 1 本ごと**：依頼どおり。1 回の攻撃でバーの境目を越えて削れないようにしたので、F2（殲滅）は `ForceKill` を使う。
- **バー本数は推奨戦力から自動**：今後クエストを 100 個ほど作る予定（ユーザー談）なので、クエストごとに手で決めなくてよいようにした。個別指定は `bossHpBars`。
- **過去のバージョンの番号**：履歴を日付と内容で区切って v0.1.0〜v0.5.0 を後付けした（上の表）。v0.1.0 はビルドエラー修正後の `27072c7`（その前の `38e2cdb` は Debug 構成でビルドエラー）。
- **CHANGELOG.md を版の一覧の元にした**：人が読む履歴と公開の一覧を 1 か所にして書き忘れを防ぐため。README からは版の一覧表と「現在のバージョン」の数字を消した（毎回書き換えが要るため）。
- **DXライブラリは CI で毎回公式サイトから取得**：リポジトリに含めない方針（`.gitignore` の `external/`）を守るため。版によってライブラリの置き場所の想定が違っても通るよう、`external/DxLib` と `external/DxLib/x64` の両方に置く。
- **`windows-2022` ランナー**：プロジェクトが `PlatformToolset v143`（VS2022）のため。
- **zip 内のファイル名は英数字だけ**：Windows のエクスプローラーで日本語名が化けることがあるため（DXライブラリの文書も英数字名に変えて同梱）。
- **デバッグのフロア移動は「暗転して読み込む」**：ゲートを通ったときと同じ処理にして、状態の食い違いを避けるため。HP / MP・経過時間・撃破数は引き継ぐ。
- **このファイルの push も Z を上げた**：`CLAUDE.md` の「push のたびに上げて公開」に文字どおり従った。ゲームの中身は変わらない。

---

## 5. 試してうまくいかなかったこと（と理由）

- **湾曲した HP バー**：`DrawArcRange` に逆向きの角度を渡すと側面の弧が描かれなかった（角度を入れ替えて直した）。その後ユーザーの指示で湾曲自体をやめた。
- **登場演出の名前の札のグラデーション**：帯の継ぎ目が見えたので 1 枚の塗りにした。
- **HP バーが溜まる演出が画面外で起きていた**：演出の中へ移した。
- **スクリーンショットに古い描画が出た**：わざと壊して試したあと、オブジェクトを作り直さずに撮影用プログラムをリンクしたため。撮影の前に必ず `build_lib.sh` を通す。
- **バージョンのテストで保存に失敗**：テスト用ハーネスが `autoSaveEnabled = false` にしているため `SaveSystem::Save` が false を返した。テスト側で true にした。
- **`__pycache__` をコミットしてしまった**：push 前に気づいて取り除き、`.gitignore` に追加した。
- **デバッグのフロア移動のテストが 1 件 NG**：「聖剣の試練」はボス戦だけの 1 フロアなので、最初のフロアが「1F」ではなく「ボスフロア」になる。テストの期待値の誤りで、コードは正しかった。
- **クラウドのコンテナから DXライブラリのサイトに接続できない**（ローカルでのパッケージ確認ができない）：
  ```
  curl: (56) CONNECT tunnel failed, response 403
  dxlib.xsrv.jp:443 — connect_rejected
  ```
  GitHub Actions からは取得できる（`https://dxlib.xsrv.jp/DxLib/DxLib_VC3_25a.zip` を使ってビルド成功）。
- **`get_job_logs` の出力が大きすぎて読めない**：DXライブラリの C4828 警告が数千行出るため。ファイルに保存されたログを `grep -v C4828` で絞って読む。

---

## 6. 未解決の問題

### 確認済み（実際に出ている）
- **DXライブラリの文字コードの警告（大量・ビルドは成功）**。GitHub Actions のビルドで 1 回のビルドに 1,600 件以上：
  ```
  D:\a\The-Eclipse\The-Eclipse\game\external\DxLib\DxLib.h(5576,1): warning C4828: The file contains a character starting at offset 0x9cbbf that is illegal in the current source character set (codepage 65001).
  ```
  原因は `/utf-8` と DxLib.h の Shift_JIS のコメント。害は無いが、消すなら vcxproj に `/wd4828` を足す（未対応・ユーザーに未提案）。
- **GitHub Actions の Node.js 20 の非推奨警告**（動作はする）。保存したログの 1 行（末尾は途中で切れている）：
  ```
  ##[warning]Node.js 20 is deprecated. The following actions target Node.js 20 but are being forced to run on Node.js 24: actions/cache@v4, actions/setup-python@v5, actions/upload-artifact@v4, microsoft/setup-msbuild@v2. For more inf
  ```
  新しい版のアクションに上げれば消える【推測：どの版が Node 24 対応かは未確認】。
- **Actions の「Run workflow」（手動実行）が使えない**：ワークフローが既定のブランチ（`main`）に無いため。`main` は Initial commit のまま。マージやプルリクエストはユーザーの指示待ち。

### 推測・未確認
- **実機（Windows）でゲームを動かしていない**。見た目の確認はすべて Linux の簡易描画（IPA フォント）。DxLib の実際の見た目（フォント・アンチエイリアス）とは違う可能性がある。
- **マウスのサイドボタン**：Windows 専用の `GetAsyncKeyState(VK_XBUTTON1/2)` の経路は動かしていない（コンパイルは CI で通っている）。
- **DXライブラリの使用条件**：zip に `licenses/dxlicense.html`（パッケージ同梱の著作権関係の文書）を入れているが、条件を満たしているかは確認していない。一般公開の前にユーザーに確認してもらう必要がある（ユーザーには伝え済み）。
- **古い版（v0.1.0〜v0.5.0）の zip が実際に遊べるか**：ビルドは成功したが、起動は確かめていない。

---

## 7. 次にやること（優先順）

1. **v0.7.1（このファイルを足した版）の公開を確かめる**：GitHub Actions の「リリース」が成功し、Releases に `v0.7.1` と
   `TheEclipse-v0.7.1-win64.zip` が載っているか（`mcp__github__get_release_by_tag` で `v0.7.1`）。失敗していたらログを見て直す。
2. **ユーザーの次の依頼を待つ／受けたら「1. 完了条件」の手順で進める**（push のたびに命名規定で版を上げて公開）。
3. （ユーザーが望めば）Windows の実機で、ボスの HP バー・名前・登場演出・キー設定のサイドボタン・デバッグのフロア移動の見た目と動きを確かめてもらう。
4. （提案どまり）C4828 警告の抑止（`/wd4828`）、Actions のアクションを Node 24 対応の版へ、`main` へのマージ。どれもユーザーの了承を得てから。

---

## 付録：前のセッションのテスト環境（消えている）

前のセッションでは、Windows が無い Linux のコンテナでゲームのロジックと描画を確かめるため、
**セッション専用の作業フォルダ（scratchpad）に** 次のものを作っていた。**リポジトリには入っていないので、
新しいセッションには残っていない。** 必要なら作り直す（下は作り方の要点）。

- `dxstub/DxLib.h`・`dxstub.cpp`：DxLib の代わり。使っている関数だけを宣言し、キー（DIK コード）・マウス
  （`MOUSE_INPUT_1〜5`）の状態をテストから入れられるようにし、文字・矩形・線・三角形・円の描画を記録
  （`g_stubTexts` / `g_stubBoxes` / … 、ブレンドモード付き）。4x4 のスーパーサンプリングで PPM に描く簡易キャンバス、
  `SetMainWindowText` の記録（`g_stubWindowTitle`）。
- `build_lib.sh`：`TheEclipse/src` の `main.cpp` 以外を `g++ -std=c++17 -Wall -Wextra -Wshadow -fsanitize=address,undefined` で
  オブジェクトにする（警告は warn.txt。最終的に 0 件）。
- `tests/harness.h`：`#define private public` で中を覗く。`Harness{ctx, sm}`、`Step(n)`（1/60 秒）、`Click` / `Press` / `Hold` /
  `MoveMouse`、`HasText` / `FindText`、`CHECK` マクロ、`Finish`。
- テスト（最終結果はすべて fail=0）：boss_intro 205、boss_phase 2499、debug_floor 169、dual_wield 108、keyconfig 700、
  result_announce 50、shop_qty 68、smoke 61、version 20。
- 2026-11-05 のセッションで dxstub・build_lib.sh・harness.h を作り直し、status_hud（88 件、fail=0）と
  shots_status（左上の撮影）を足した。これらもリポジトリには入っていない。
- 2026-11-20 のセッションでは camera_edge（50 件）・luk_exp（36 件）・shots_luk を足した（すべて fail=0）。
- 2026-11-22 のセッションでは hidden_floor（150 件）・test_release_names.py（20 件）・shots_vajra・shots_debug を足した。
  続けて title_screen（16 件）・shots_title を足した。
- `tests/shots.cpp` ＋ FreeType（IPAGothic）：スクリーンショット。
- `regen_vcxproj.py`：`src` 以下の .cpp / .h から vcxproj と .filters の一覧を作り直す（ファイルを足したら必要）。

ファイルを足したときは **`TheEclipse/TheEclipse.vcxproj` と `.vcxproj.filters` に登録する** のを忘れないこと
（Visual Studio でビルドされない）。
