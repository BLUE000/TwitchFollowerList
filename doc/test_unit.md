# 単体テスト仕様書 (Unit Test Specification)

詳細設計に基づき、GoogleTestフレームワークを用いた関数（メソッド）単位のテスト仕様を定義します。

## 1. テスト方針
- C++用テストフレームワーク **GoogleTest (gtest)** を使用する。
- 外部依存（ネットワーク通信や実際のファイルシステム）が絡むクラスは、Mock（gmock）を用いて隔離する。
- `core` および `api` 層のロジックを中心にテストする。

## 2. 対象: `api::FileManager`
ファイル入出力およびエンコード・デコード処理のテスト。

### 2.1 `encodeData` / `decodeData`
- **テスト内容**: CSV文字列を正しくエンコード/デコードできるか。
- **入力**: `"1,TestUser,test_user,12345,"`
- **期待結果**:
  - `encodeData`: URLエンコード → 先頭8バイト逆順 → Base64エンコード の結果と一致する。
  - `decodeData`: エンコードされた文字列を渡し、元のCSV文字列が返る。

### 2.2 `saveAllListDat`
- **テスト内容**: `TwitchFollower` のリストを渡し、指定フォーマットでファイル出力されるか。
- **入力**: フォロワー2名（1名は複数グループID `1|3` を持つ）。出力先はテスト用の一時ディレクトリ。
- **期待結果**: `out/AllList.dat` が生成され、`decodeData` で読み込むと期待通りのCSV形式（複数IDがパイプ区切り）になっている。

### 2.3 `memo` フィールドの保存と復元
- **テスト内容**: メモに含まれるカンマや特殊文字が正しく保存・復元されるか。
- **入力**: `memo = "テスト, メモ\n改行あり"` を持つフォロワー。
- **期待結果**: 保存されたファイルを `decodeData` して再度パースした際、`memo` の内容が完全に一致すること。

## 3. 対象: `core::AppController`
状態管理とUndo/Redo履歴管理のテスト。

### 3.1 `pushAction` と履歴のTruncate
- **テスト内容**: 新規アクション追加時、カーソル位置に基づく未来の履歴切り捨てが正常に機能するか。
- **事前状態**: 履歴が5件（cursor = 5）。
- **操作**: `historyCursor` を3に変更（Undoを2回実行した状態をエミュレート）し、`pushAction(newAction)` を実行。
- **期待結果**: 履歴のサイズが4になり、インデックス3に `newAction` が入る。インデックス4,5のアクションは破棄される。

### 3.2 `applyAction` / `revertAction` (Group Assign)
- **テスト内容**: グループへのアサイン/解除がデータモデルに正しく適用されるか。
- **事前状態**: フォロワーA（groupIds: 空）
- **操作**: `applyAction({AssignGroup, A, 1})` を実行。
- **期待結果**: フォロワーAの `groupIds` に `1` が追加される。
- **操作**: `revertAction({AssignGroup, A, 1})` を実行。
- **期待結果**: フォロワーAの `groupIds` から `1` が削除される。

## 4. 対象: `api::TwitchApiClient`
API通信のパース処理テスト。

### 4.1 JSONパース処理 (`fetchFollowers` 内部ロジック)
- **テスト内容**: Twitch APIのレスポンスJSONから `TwitchFollower` オブジェクト群が正しく構築されるか。
- **入力**: Twitch APIドキュメントに準拠したダミーのJSON文字列。
- **期待結果**: `followersFetched` シグナルが発火し、リストのサイズや各ユーザーのID、表示名がJSONの内容と一致する。
