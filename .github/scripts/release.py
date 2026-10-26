#!/usr/bin/env python3
"""
リリース用の補助スクリプト（.github/workflows/release.yml から呼ぶ）

  plan     CHANGELOG.md に書かれたバージョンのうち、まだ Releases に無いものを JSON で出す
  notes    CHANGELOG.md から 1 つのバージョンの変更点を取り出し、リリースノートにする
  dxlib    DXライブラリ（VC 用パッケージ）をダウンロードして展開する
  package  ビルドした exe と assets を配布用の zip にまとめる

バージョンの区切りは CHANGELOG.md の「## v0.6.0（2026-10-26）」の見出し。
どのコミットをその版とするかは
  - 見出しの下に「コミット: xxxxxxx」があれば、そのコミット
  - 無ければ、CHANGELOG.md の見出しと Version.h の番号の両方がそろった最初のコミット
Version.h があるコミットでは、番号・日付・表記が CHANGELOG.md と合っているかも確かめる。
"""

import argparse
import io
import json
import os
import re
import shutil
import subprocess
import sys
import urllib.parse
import urllib.request
import zipfile

CHANGELOG = "CHANGELOG.md"
VERSION_H = "TheEclipse/src/Core/Version.h"
REPO_URL = "https://github.com/Akasaka777/The-Eclipse"

HEADING = re.compile(r"^## v(\d+\.\d+\.\d+)（(\d{4}-\d{2}-\d{2})）\s*$")
COMMIT_LINE = re.compile(r"^コミット:\s*\[?`?([0-9a-fA-F]{7,40})`?\]?")
VERSION_DEF = re.compile(r'kVersion\s*=\s*"(\d+\.\d+\.\d+)"')
VERSION_DATE = re.compile(r'kVersionDate\s*=\s*"([^"]*)"')
VERSION_LABEL = re.compile(r'kVersionLabel\s*=\s*"([^"]*)"')


def fail(message):
    print(f"::error::{message}", file=sys.stderr)
    sys.exit(1)


def git(*args):
    return subprocess.run(["git", *args], check=True, capture_output=True, text=True,
                          encoding="utf-8").stdout.strip()


def version_key(version):
    return tuple(int(part) for part in version.split("."))


# --- CHANGELOG.md ---------------------------------------------------------------

def read_changelog(path=CHANGELOG):
    """[{version, date, commit, body}] を CHANGELOG.md の並び順（新しい順）で返す"""
    entries = []
    current = None
    with open(path, encoding="utf-8") as f:
        for line in f.read().splitlines():
            match = HEADING.match(line)
            if match:
                current = {"version": match.group(1), "date": match.group(2), "commit": None, "lines": []}
                entries.append(current)
                continue
            if line.startswith("## "):
                current = None   # バージョン以外の見出し
                continue
            if current is None:
                continue
            commit = COMMIT_LINE.match(line)
            if commit and current["commit"] is None:
                current["commit"] = commit.group(1)
            current["lines"].append(line)
    seen = set()
    for entry in entries:
        if entry["version"] in seen:
            fail(f"CHANGELOG.md に v{entry['version']} が 2 回あります")
        seen.add(entry["version"])
        entry["body"] = "\n".join(entry.pop("lines")).strip()
    return entries


# --- バージョンとコミットの対応 --------------------------------------------------

def first_commit_adding(text, path):
    """text を path に初めて書き足したコミット（HEAD までの履歴で）"""
    out = git("log", "--reverse", "--format=%H", "-S", text, "--", path)
    return out.splitlines()[0] if out else None


def version_in_commit(commit):
    """そのコミットの Version.h の (番号, 日付, 表記)。Version.h が無い古いコミットは None"""
    try:
        text = git("show", f"{commit}:{VERSION_H}")
    except subprocess.CalledProcessError:
        return None
    found = [pattern.search(text) for pattern in (VERSION_DEF, VERSION_DATE, VERSION_LABEL)]
    return tuple(match.group(1) if match else "" for match in found)


def resolve_commit(entry):
    version = entry["version"]
    if entry["commit"]:
        try:
            commit = git("rev-parse", "--verify", f"{entry['commit']}^{{commit}}")
        except subprocess.CalledProcessError:
            fail(f"v{version} のコミット {entry['commit']} が見つかりません")
    else:
        heading = first_commit_adding(f"## v{version}（", CHANGELOG)
        bump = first_commit_adding(f'kVersion = "{version}"', VERSION_H)
        if bump is None:
            fail(f"v{version} : Version.h の kVersion が \"{version}\" になっていません")
        # 見出しと番号の両方がそろった最初のコミット（後から入った方）
        if heading is None:
            commit = bump
        else:
            later_is_bump = subprocess.run(["git", "merge-base", "--is-ancestor", heading, bump]).returncode == 0
            commit = bump if later_is_bump else heading
    in_source = version_in_commit(commit)
    if in_source is not None:
        expected = (version, entry["date"], f"Ver {version}")
        if in_source != expected:
            fail(f"v{version} : コミット {commit[:7]} の Version.h（番号 {in_source[0]} / 日付 {in_source[1]} / "
                 f"表記 {in_source[2]}）が CHANGELOG.md（{expected[0]} / {expected[1]} / {expected[2]}）と合いません")
    return commit


def existing_tags():
    out = git("tag", "--list", "v*")
    return set(out.splitlines()) if out else set()


def existing_releases():
    out = subprocess.run(["gh", "release", "list", "--limit", "1000", "--json", "tagName"],
                         check=True, capture_output=True, text=True, encoding="utf-8").stdout
    return {item["tagName"] for item in json.loads(out or "[]")}


def cmd_plan(args):
    entries = read_changelog()
    if not entries:
        fail("CHANGELOG.md にバージョンの見出し（## v0.0.0（YYYY-MM-DD））がありません")
    released = set(args.existing.split(",")) if args.existing is not None else existing_releases()
    tags = existing_tags()
    newest = max((e["version"] for e in entries), key=version_key)

    todo = []
    for entry in sorted(entries, key=lambda e: version_key(e["version"])):   # 古い順に公開する
        tag = f"v{entry['version']}"
        if tag in released:
            print(f"{tag} : 公開済み", file=sys.stderr)
            continue
        if tag in tags:
            # タグだけ先にある（前回の途中で止まったなど）ときは、そのタグのコミットを使う
            commit = git("rev-parse", f"{tag}^{{commit}}")
        else:
            commit = resolve_commit(entry)
        print(f"{tag} : {commit[:7]} を公開します", file=sys.stderr)
        todo.append({
            "version": entry["version"],
            "tag": tag,
            "date": entry["date"],
            "commit": commit,
            "latest": "true" if entry["version"] == newest else "false",
        })
    print(f"matrix={json.dumps(todo, ensure_ascii=False)}")
    print(f"count={len(todo)}")


# --- リリースノート ----------------------------------------------------------------

def asset_name(version):
    return f"TheEclipse-v{version}-win64.zip"


def cmd_notes(args):
    for entry in read_changelog():
        if entry["version"] != args.version:
            continue
        body = entry["body"]
        if args.commit and not entry["commit"]:
            # 見出しの下にコミットが書かれていない版（最新の版など）は、公開したコミットを添える
            short = args.commit[:7]
            body = f"コミット: [`{short}`]({REPO_URL}/commit/{args.commit})\n\n{body}"
        print(body)
        print()
        print("### ダウンロード")
        print(f"- 下の **{asset_name(args.version)}** をダウンロードして展開し、`TheEclipse.exe` を起動してください"
              "（Windows 10 / 11、64 ビット）")
        print("- 遊び方とセーブデータの場所は、zip の中の `README.txt` に書いてあります")
        return
    fail(f"CHANGELOG.md に v{args.version} がありません")


# --- DXライブラリ ------------------------------------------------------------------

DXLIB_PAGE = "https://dxlib.xsrv.jp/dxdload.html"


def dxlib_url():
    url = os.environ.get("DXLIB_URL", "").strip()
    if url:
        return url
    with urllib.request.urlopen(DXLIB_PAGE, timeout=60) as response:
        raw = response.read()
    for encoding in ("utf-8", "cp932"):
        try:
            html = raw.decode(encoding)
            break
        except UnicodeDecodeError:
            continue
    else:
        html = raw.decode("utf-8", errors="replace")
    links = re.findall(r'href="([^"]*DxLib_VC[^"/]*\.zip)"', html, re.IGNORECASE)
    if not links:
        fail(f"{DXLIB_PAGE} に VC 用パッケージ（DxLib_VC*.zip）のリンクが見つかりません。"
             "リポジトリの変数 DXLIB_URL に zip の URL を設定してください")
    return urllib.parse.urljoin(DXLIB_PAGE, links[0])


def cmd_dxlib_url(args):
    print(f"url={dxlib_url()}")


def cmd_dxlib(args):
    """VC 用パッケージを展開し、DxLib.h のあるフォルダを dest にコピーする"""
    url = args.url or dxlib_url()
    os.makedirs(args.dest, exist_ok=True)
    print(f"DXライブラリ : {url}", file=sys.stderr)
    request = urllib.request.Request(url, headers={"User-Agent": "TheEclipse-release"})
    with urllib.request.urlopen(request, timeout=300) as response:
        data = response.read()
    # パッケージ内のフォルダ名は日本語（Shift_JIS）
    with zipfile.ZipFile(io.BytesIO(data), metadata_encoding="cp932") as archive:
        work = os.path.join(args.dest, "_package")
        shutil.rmtree(work, ignore_errors=True)
        archive.extractall(work)
    found = None
    for root, _, files in os.walk(work):
        if "DxLib.h" in files:
            found = root
            break
    if found is None:
        fail("DXライブラリのパッケージに DxLib.h が見つかりません")
    for name in os.listdir(found):
        source = os.path.join(found, name)
        if os.path.isfile(source):
            shutil.copy2(source, os.path.join(args.dest, name))
    # 著作権表示・使用条件らしい文書を licenses へ（配布 zip に同梱する）
    licenses = os.path.join(args.dest, "licenses")
    os.makedirs(licenses, exist_ok=True)
    for root, _, files in os.walk(work):
        for name in files:
            lower = name.lower()
            if any(key in lower for key in ("license", "licence", "copyright")) or any(
                    key in name for key in ("ライセンス", "著作権", "使用条件")):
                shutil.copy2(os.path.join(root, name), os.path.join(licenses, name))
    shutil.rmtree(work, ignore_errors=True)
    print(f"DxLib.h ほかを {args.dest} に置きました（同梱する文書 {len(os.listdir(licenses))} 件）", file=sys.stderr)


# --- 配布用 zip -------------------------------------------------------------------

README_TEXT = """The Eclipse v{version}（{date}）
横スクロール型 2D アクション RPG

■ 遊び方
  1. この zip を好きな場所に展開します（zip の中から直接起動しないでください）。
  2. TheEclipse.exe をダブルクリックして起動します。
     「Windows によって PC が保護されました」と出たときは「詳細情報」→「実行」を押してください。
  操作は、ゲームの「設定」タブの「操作一覧」で確認できます（キー設定もそこから変えられます）。

■ セーブデータ
  TheEclipse.exe と同じフォルダの save\\save.txt に保存されます。
  バージョンごとに別のフォルダへ展開すれば、セーブデータも別々になります。
  新しいバージョンへ引き継ぐときは、save フォルダをコピーしてください。
  （古いバージョンでは、新しいバージョンのセーブデータを読めないことがあります）

■ 動作環境
  Windows 10 / 11（64 ビット）

■ ほかのバージョン・更新履歴
  {repo}/releases
  このバージョンのソースコード : {repo}/tree/{commit}

■ 使用しているライブラリ
  DXライブラリ  https://dxlib.xsrv.jp/
  DXライブラリと、その内部で使われているライブラリの著作権表示・使用条件は
  licenses フォルダと、DXライブラリのサイトの「使用条件」を参照してください。
"""


def cmd_package(args):
    exe = os.path.join(args.bin, "TheEclipse.exe")
    if not os.path.isfile(exe):
        fail(f"{exe} がありません（ビルドに失敗しています）")
    top = f"TheEclipse-v{args.version}"
    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with zipfile.ZipFile(args.out, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        archive.write(exe, f"{top}/TheEclipse.exe")
        readme = README_TEXT.format(version=args.version, date=args.date, repo=REPO_URL, commit=args.commit)
        # メモ帳で文字化けしないよう BOM つき UTF-8・CRLF にする
        archive.writestr(f"{top}/README.txt", "﻿" + readme.replace("\n", "\r\n"))
        assets = os.path.join(args.game, "assets")
        for root, dirs, files in os.walk(assets):
            dirs.sort()
            rel = os.path.relpath(root, args.game).replace(os.sep, "/")
            archive.writestr(f"{top}/{rel}/", "")   # 空のフォルダも残す
            for name in sorted(files):
                if name == ".gitkeep":
                    continue
                archive.write(os.path.join(root, name), f"{top}/{rel}/{name}")
        if args.licenses and os.path.isdir(args.licenses):
            # Windows のエクスプローラーで文字化けしないよう、同梱する文書の名前は英数字にする
            for index, name in enumerate(sorted(os.listdir(args.licenses)), 1):
                path = os.path.join(args.licenses, name)
                if not os.path.isfile(path):
                    continue
                ext = os.path.splitext(name)[1] or ".txt"
                ascii_name = name if name.isascii() else f"DxLib_{index}{ext}"
                archive.write(path, f"{top}/licenses/{ascii_name}")
    print(f"{args.out} を作りました", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)

    plan = sub.add_parser("plan")
    plan.add_argument("--existing", help="公開済みのタグ（カンマ区切り。省略時は gh で調べる）")
    plan.set_defaults(func=cmd_plan)

    notes = sub.add_parser("notes")
    notes.add_argument("version")
    notes.add_argument("--commit", default="")
    notes.set_defaults(func=cmd_notes)

    url = sub.add_parser("dxlib-url")
    url.set_defaults(func=cmd_dxlib_url)

    dx = sub.add_parser("dxlib")
    dx.add_argument("dest")
    dx.add_argument("--url", default="")
    dx.set_defaults(func=cmd_dxlib)

    pack = sub.add_parser("package")
    pack.add_argument("--version", required=True)
    pack.add_argument("--date", required=True)
    pack.add_argument("--commit", required=True)
    pack.add_argument("--game", required=True, help="ビルドしたソース（assets を含む）")
    pack.add_argument("--bin", required=True, help="TheEclipse.exe のあるフォルダ")
    pack.add_argument("--licenses", default="")
    pack.add_argument("--out", required=True)
    pack.set_defaults(func=cmd_package)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
