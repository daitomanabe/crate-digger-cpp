# Crate Digger Java to C++ 移植プラン

## 概要

**オリジナル**: [Deep-Symmetry/crate-digger](https://github.com/Deep-Symmetry/crate-digger) (Java)
**移植先**: crate-digger-cpp (C++20)
**目的**: rekordbox export.pdb データベースファイルのパース

---

## 移植状況サマリー

| カテゴリ | 状況 | 完了率 |
|---------|------|--------|
| コア機能（データベースパース） | 完了 | 100% |
| インデックス構築 | 完了 | 100% |
| 型システム | 完了 | 100% |
| CLIツール | 完了 | 100% |
| Pythonバインディング | 完了 | 100% |
| APIスキーマ | 完了 | 100% |
| ロギング | 完了 | 100% |
| NumPy統合 | 完了 | 100% |
| 拡張機能（Tags等） | 完了 | 100% |
| 範囲検索 | 完了 | 100% |
| Safety Curtain | 完了 | 100% |
| Cue Points (ANLZ) | 完了 | 100% |
| **全体** | | **100%** |

---

## 完了済み機能

### 1. コアライブラリ (100%)

**ファイル**: `src/core/database.cpp`, `include/cratedigger/database.hpp`

- [x] `Database::open()` - export.pdbファイルを開く
- [x] `Database::open_ext()` - exportExt.pdbファイルを開く
- [x] プライマリインデックス (ID → Row)
- [x] セカンダリインデックス (Name → IDs)
- [x] 全データタイプのアクセサ

### 2. PDBファイルパーサー (100%)

**ファイル**: `src/core/rekordbox_pdb.cpp`, `include/cratedigger/rekordbox_pdb.hpp`

- [x] バイナリPDBファイルパース
- [x] ページベーステーブル走査
- [x] DeviceSQL文字列パース (ASCII/UTF-16LE)
- [x] リトルエンディアンデータ読み込み
- [x] メモリマップドファイルハンドリング

### 3. 型システム (100%)

**ファイル**: `include/cratedigger/types.hpp`

- [x] 強い型付けのID型 (9種類)
  - `TrackId`, `ArtistId`, `AlbumId`, `GenreId`
  - `LabelId`, `ColorId`, `KeyId`, `ArtworkId`, `PlaylistId`, `TagId`
- [x] Row構造体 (10種類)
  - `TrackRow`, `ArtistRow`, `AlbumRow`, `GenreRow`, `LabelRow`
  - `ColorRow`, `KeyRow`, `ArtworkRow`, `PlaylistFolderEntry`, `TagRow`
- [x] `Result<T>` エラーハンドリング型
- [x] エラーコード列挙型
- [x] `CuePoint` 構造体（将来拡張用スタブ）

### 4. インデックス構築 (100%)

**ファイル**: `src/core/database_util.cpp`

- [x] `build_indices()` - メインインデックス構築
- [x] `index_tracks()` - トラックインデックス（マルチアーティスト対応）
- [x] `index_artists()` - アーティスト名インデックス
- [x] `index_albums()` - アルバムインデックス
- [x] `index_genres()` - ジャンル名インデックス
- [x] `index_labels()` - レーベル名インデックス
- [x] `index_colors()` - カラー名インデックス
- [x] `index_keys()` - 調性インデックス
- [x] `index_artwork()` - アートワークパスインデックス
- [x] `index_playlists()` - プレイリストエントリインデックス
- [x] `index_playlist_folders()` - フォルダ階層
- [x] `index_history_playlists()` - 履歴プレイリスト
- [x] `index_history_entries()` - 履歴エントリ
- [x] `index_tags()` - タグインデックス (exportExt.pdb)
- [x] `index_tag_tracks()` - タグ-トラック関連インデックス

### 5. NumPyバルクデータ取得 (100%) ✓ NEW

**ファイル**: `src/core/database.cpp`, `include/cratedigger/database.hpp`

- [x] `get_all_bpms()` - 全トラックBPM一括取得
- [x] `get_all_durations()` - 全トラック長さ一括取得
- [x] `get_all_years()` - 全トラック年度一括取得
- [x] `get_all_ratings()` - 全トラックレーティング一括取得
- [x] `get_all_bitrates()` - 全トラックビットレート一括取得
- [x] `get_all_sample_rates()` - 全トラックサンプルレート一括取得

### 6. TrackRow追加フィールド (100%) ✓ NEW

**ファイル**: `include/cratedigger/types.hpp`, `src/core/database_util.cpp`

全21個の文字列オフセットフィールド実装済み:

| Index | フィールド | 状態 |
|-------|-----------|------|
| 0 | ISRC | 実装済み |
| 5 | Message | 実装済み |
| 10 | Date Added | 実装済み |
| 11 | Release Date | 実装済み |
| 12 | Mix Name | 実装済み |
| 14 | Analyze Path | 実装済み |
| 15 | Analyze Date | 実装済み |
| 16 | Comment | 実装済み |
| 17 | Title | 実装済み |
| 19 | Filename | 実装済み |
| 20 | File Path | 実装済み |

### 7. exportExt.pdb Tags機能 (100%) ✓ NEW

**ファイル**: `src/core/database_util.cpp`, `include/cratedigger/database.hpp`

- [x] `TagRow` 構造体定義
- [x] `TagId` 型定義
- [x] `RawTagRow`, `RawTagTrackRow` 構造体
- [x] `index_tags()` インデクサ
- [x] `index_tag_tracks()` インデクサ
- [x] `get_tag(TagId)` アクセサ
- [x] `find_tags_by_name(string)` 検索
- [x] `find_tracks_by_tag(TagId)` 検索
- [x] `find_tags_by_track(TrackId)` 逆引き
- [x] `all_tag_ids()` 一覧取得
- [x] `tag_count()` カウント

### 8. 範囲検索 (100%) ✓ NEW

**ファイル**: `src/core/database.cpp`, `include/cratedigger/database.hpp`

- [x] `find_tracks_by_bpm_range(min, max)` - BPM範囲検索
- [x] `find_tracks_by_duration_range(min, max)` - Duration範囲検索
- [x] `find_tracks_by_year_range(min, max)` - Year範囲検索
- [x] `find_tracks_by_rating_range(min, max)` - Rating範囲検索
- [x] `find_tracks_by_year(year)` - 年度完全一致検索
- [x] `find_tracks_by_rating(rating)` - レーティング完全一致検索

### 9. Safety Curtain (100%) ✓ NEW

**ファイル**: `include/cratedigger/types.hpp`

- [x] `SafetyLimits` 構造体（定数定義）
  - `MIN_BPM = 20.0f`
  - `MAX_BPM = 300.0f`
  - `MAX_DURATION_SECONDS = 86400`
  - `MIN_RATING = 0`, `MAX_RATING = 5`
- [x] `validate_bpm(float)` - BPMクランプ関数
- [x] `validate_duration(uint32_t)` - Durationクランプ関数
- [x] `validate_rating(uint16_t)` - Ratingクランプ関数
- [x] `is_valid_bpm(float)` - BPM検証関数
- [x] `is_valid_duration(uint32_t)` - Duration検証関数
- [x] `is_valid_rating(uint16_t)` - Rating検証関数

### 10. CLIツール (100%)

**ファイル**: `src/cli/main.cpp`

- [x] JSONL コマンド処理
- [x] `--schema` フラグ
- [x] `--help` / `--version` フラグ
- [x] インタラクティブモード

### 11. Pythonバインディング (100%)

**ファイル**: `src/python/bindings.cpp`

- [x] nanobind フレームワーク使用
- [x] 全ID型のエクスポート（9種類）
- [x] 全Row型のエクスポート（10種類）
- [x] Database クラスメソッド
- [x] バルクデータ取得メソッド
- [x] タグ関連メソッド
- [x] 範囲検索メソッド
- [x] Safety Curtain関数・定数
- [x] `__repr__` メソッド

### 12. APIスキーマ (100%)

**ファイル**: `src/core/api_schema.cpp`, `include/cratedigger/api_schema.hpp`

- [x] `describe_api()` - APIスキーマ自動生成
- [x] JSON シリアライゼーション
- [x] コマンドスキーマ定義（バルクデータ、範囲検索含む）
- [x] テンソル定義（NumPy用）

### 13. テスト (100%)

- [x] `tests/test_database.cpp` - 型・関数テスト（13テスト）
- [x] `tests/test_api_schema.cpp` - スキーマ検証（9テスト）
- [x] Safety Curtainテスト
- [x] Cue Pointテスト

### 14. Cue Points / Hot Cues (100%) ✓ NEW

**ファイル**: `src/core/rekordbox_anlz.cpp`, `include/cratedigger/rekordbox_anlz.hpp`

rekordboxのANLZファイル（解析ファイル）からCue Pointsを読み取り：
- `ANLZ0000.DAT` - 標準解析データ
- `ANLZ0000.EXT` - 拡張解析データ（カラー付きCue Points）

実装済み機能：
- [x] `RekordboxAnlz` クラス - ANLZファイルパーサー
- [x] `CuePointManager` クラス - Cue Pointインデックス管理
- [x] `CuePoint` 構造体 - Cue Point情報
- [x] `CuePointType` 列挙型 - Cue, FadeIn, FadeOut, Load, Loop
- [x] `load_cue_points(anlz_dir)` - ディレクトリスキャン
- [x] `load_anlz_file(path)` - 単一ファイル読み込み
- [x] `get_cue_points(track_path)` - パスでCue Point取得
- [x] `get_cue_points_for_track(TrackId)` - IDでCue Point取得
- [x] `find_cue_points_by_filename(filename)` - ファイル名検索
- [x] Pythonバインディング完全対応

---

## アーキテクチャ

| 項目 | Java オリジナル | C++20 移植版 |
|------|----------------|--------------|
| メモリ管理 | GC | RAII, スマートポインタ |
| エラーハンドリング | try/catch | `Result<T>` (std::variant) |
| 文字列 | String | std::string, std::string_view |
| コレクション | HashMap, ArrayList | std::map, std::vector |
| 型安全性 | ジェネリクス | テンプレート + 強い型 |
| GUI | あり | なし（ヘッドレスライブラリ） |

---

## ファイル構成

```
implementation/crate-digger-cpp/
├── include/cratedigger/
│   ├── cratedigger.hpp      # メインAPI
│   ├── database.hpp         # Database クラス
│   ├── types.hpp            # 型定義（Safety Curtain, CuePoint含む）
│   ├── rekordbox_pdb.hpp    # PDBフォーマット
│   ├── rekordbox_anlz.hpp   # ANLZフォーマット（Cue Points）
│   ├── api_schema.hpp       # APIスキーマ
│   └── logging.hpp          # ロギング
├── src/
│   ├── core/
│   │   ├── database.cpp
│   │   ├── database_util.cpp
│   │   ├── database_impl.hpp
│   │   ├── rekordbox_pdb.cpp
│   │   ├── rekordbox_anlz.cpp  # ANLZパーサー
│   │   ├── api_schema.cpp
│   │   └── logging.cpp
│   ├── cli/
│   │   └── main.cpp
│   └── python/
│       └── bindings.cpp
├── tests/
│   ├── test_database.cpp    # 13テスト
│   ├── test_api_schema.cpp  # 9テスト
│   └── golden_test.py
├── CMakeLists.txt
├── README.md
└── IR_SCHEMA.md
```

---

## 参考リンク

- [オリジナルJavaリポジトリ](https://github.com/Deep-Symmetry/crate-digger)
- [rekordboxフォーマット解析](https://github.com/flesniak/python-prodj-link)
- [Kaitai Struct定義](https://github.com/Deep-Symmetry/crate-digger/blob/main/src/main/kaitai/rekordbox_pdb.ksy)
