# 結合テスト仕様書 (Integration Test Specification)

基本設計の「データ連携フロー」に基づき、UI・Core・APIの各モジュール間連携を検証するテスト仕様を定義します。

## 1. テスト方針
- 各クラスを結合した状態で、一つの機能フローが通しで動作するかを確認する。
- 外部APIへの実際のリクエストは避け、Mockサーバーまたはダミーのレスポンスジェネレータを用いて通信をエミュレートする。

## 2. 結合テストケース

### IT-01: Twitchログイン連携フロー
- **目的**: UIのボタン押下から、ローカルサーバー経由でのトークン取得、APIでのユーザー取得までの一連の連携を確認。
- **手順**:
  1. `MainWindow` のログインボタン押下シグナルを発火させる。
  2. `TwitchAuthenticator` のローカルサーバー起動を確認。
  3. テストコードから `http://localhost:3000/#access_token=dummy_token` へHTTPリクエストを送信。
- **期待結果**:
  - `TwitchAuthenticator` が `authCompleted("dummy_token")` を発火。
  - `AppController` が受け取り、`TwitchApiClient` にトークンをセット。
  - UIのログインボタンが非活性状態（「ログイン済み」）に更新される。

### IT-02: フォロワー自動取得とファイル保存フロー
- **目的**: ログイン完了後に自動でフォロワーを取得し、エンコードされてファイルに保存される連携を確認。
- **手順**:
  1. `TwitchApiClient` がフォロワーJSONを受信したと仮定し、`followersFetched` シグナルを発火。
- **期待結果**:
  - `AppController` がデータを受信し、`MainWindow` にリスト表示を指示する。
  - 同時に `FileManager::saveAllListDat()` が呼び出され、`out/AllList.dat` が実際に作成される。

### IT-03: UI操作（DnD）からUndo/Redo履歴永続化フロー
- **目的**: UIでのドラッグ＆ドロップが履歴に追加され、ファイルとして永続化される流れを確認。
- **手順**:
  1. `MainWindow` でダミーのドラッグ＆ドロップイベント（フォロワーAをグループ1へ）を発行。
- **期待結果**:
  - `AppController` の `actionHistory` に要素が1つ追加される。
  - `FileManager` により `Config/ActionHistory.dat` が更新される。
  - `out/グループ1/Lists.dat` が再生成され、フォロワーAが含まれる。
