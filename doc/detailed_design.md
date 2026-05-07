# 詳細設計 (Detailed Design)

本ドキュメントは、要件定義および基本設計に基づき、各層の具体的なクラス設計、データ構造、および主要な処理フローを定義する。

## 1. クラス設計 (Class Design)

### 1.1 UI層 (`ui/`)
**`MainWindow`**
- **役割**: メイン画面のUI制御、ドラッグ＆ドロップ操作のハンドリング、絞り込み表示。
- **メンバ変数**:
  - `Ui::MainWindow *ui`
  - `QStandardItemModel *treeModel` (ツリービューのグループ一覧用)
  - `QStandardItemModel *followerModel` (リストビューのフォロワー表示用)
  - `QSortFilterProxyModel *proxyModel` (リストビューのフォルダ絞り込み用)
- **シグナル**:
  - `void loginRequested()`
  - `void followerAssignedToGroup(const QString& userId, int groupId)`
  - `void undoRequested()`
  - `void redoRequested()`
  - `void groupCreated(const QString& groupName)`
  - `void groupDeleted(int groupId)`
- **スロット**:
  - `void on_twitchLoginButton_clicked()`
  - `void onLoginSuccess()`
  - `void updateFollowerList(const QList<TwitchFollower>& followers)`
  - `void updateGroupTree(const QMap<int, QString>& groups)`
  - `void on_outDirTreeView_clicked(const QModelIndex &index)` (ツリークリックでリストの絞り込みを実行)
- **イベント**:
  - `void dropEvent(QDropEvent *event)` (リストからツリーへのDnD処理)

### 1.2 Core層 (`core/`)
**`AppController`**
- **役割**: アプリケーション全体の制御、UIとAPIモジュールの橋渡し、操作履歴（Undo/Redo）の管理、データの状態保持。
- **メンバ変数**:
  - `MainWindow *mainWindow`
  - `TwitchAuthenticator *authenticator`
  - `TwitchApiClient *apiClient`
  - `FileManager *fileManager`
  - `QList<TwitchFollower> currentFollowers` (現在の全フォロワー状態)
  - `QMap<int, QString> currentGroups` (ID -> Group Name のマッピング)
  - `QList<ActionRecord> actionHistory` (起動時からの全操作履歴リスト)
  - `int historyCursor` (現在のUndo/Redo位置を示すインデックス)
- **メソッド**:
  - `void initialize()`
  - `void applyAction(const ActionRecord& action)`: 操作をデータモデルに適用する。
  - `void revertAction(const ActionRecord& action)`: 操作を打ち消す（逆の操作を適用する）。
  - `void pushAction(const ActionRecord& action)`: 新規操作を履歴に追加する。カーソル位置より未来の履歴（Redo用の履歴）がある場合はそれを切り捨て（Truncate）てから追記し、ファイルへ永続化する。
- **スロット**:
  - `void handleLoginRequest()`
  - `void handleAuthCompleted(const QString& token)`
  - `void handleCurrentUserFetched(const QString& userId)`
  - `void handleFollowerAssigned(const QString& userId, int groupId)`
  - `void handleUndoRequested()`
  - `void handleRedoRequested()`

### 1.3 API層 (`api/`)
**`TwitchAuthenticator` & `TwitchApiClient`**
- (役割: Twitch OAuth認証フローとHelix APIによる情報取得。変更なし)

**`FileManager`**
- **役割**: ローカルファイル (`out/` および `Config/`) の読み書き、専用エンコード処理。
- **メソッド**:
  - `void saveAllListDat(const QList<TwitchFollower>& followers)`: `out/AllList.dat` へ書き込み（パイプ区切りで複数ID対応）。
  - `void saveGroupsListDat(const QMap<int, QString>& groups)`: `out/GroupsList.dat` へ書き込み。
  - `void saveGroupListsDat(int groupId, const QString& groupName, const QList<TwitchFollower>& groupFollowers)`: `out/グループ名/Lists.dat` へ書き込み。未所属の場合は `out/未所属/Lists.dat`。
  - `void saveActionHistory(const QList<ActionRecord>& history)`: `Config/ActionHistory.dat` へ書き込み。
  - `QString encodeData(const QString& csvData)`: **共通暗号化 DLL (TransCipher)** を使用して暗号化し、結果を Base64 文字列として返す。
  - `QString decodeData(const QString& encodedData)`: Base64 デコード後、**TransCipher** を使用して元の文字列へ復号する。

---

## 2. データモデル (Data Models)

**`TwitchFollower` (struct)**
- `QString userId`: ユーザーID
- `QString userName`: 表示名 (Display Name)
- `QString userLogin`: ログインID
- `QList<int> groupIds`: 所属するグループIDのリスト（複数可。要素数0の場合は「未所属」として扱う）

**`ActionRecord` (struct)**
操作履歴（Undo/Redo用）の1単位。
- `enum ActionType { AssignGroup, UnassignGroup, CreateGroup, DeleteGroup }`
- `ActionType type`
- `QString targetUserId`
- `int targetGroupId`
- `QString targetGroupName`

---

## 3. 主要処理フロー (Sequence)

### 3.1 認証・フォロワー取得・初期ファイル生成フロー
1. `TwitchAuthenticator` を経由してトークンを取得。
2. `TwitchApiClient` で自分自身のIDを取得後、フォロワー一覧を取得。
3. `AppController` がデータを受け取り、全フォロワーの `groupIds` を空（未所属）に初期化。
4. `AppController` が `FileManager` を呼び出し、`out/AllList.dat` およびデフォルトの `out/未所属/Lists.dat` を生成。
5. UIのリストビューとツリービューに初期状態を反映。

### 3.2 フォロワーのグループ振り分け（DnD）フロー
1. ユーザーが `followerListView` から `outDirTreeView` の特定フォルダへアイテムをドラッグ＆ドロップ。
2. `MainWindow` が `followerAssignedToGroup(userId, groupId)` シグナルを発火。
3. `AppController` が受け取り、該当フォロワーの `groupIds` に対象IDを追加。
4. `AppController` はこの操作を `ActionRecord` として履歴に追加し（`pushAction`）、`Config/ActionHistory.dat` を更新。
5. `AppController` が `FileManager` を呼び出し、更新された `AllList.dat` および該当フォルダの `Lists.dat` を再出力する。
6. UIのリストビュー・ツリービュー表示を最新状態に更新する。

### 3.3 アンドゥ（Undo）フロー
1. ユーザーが「Undo」ボタンを押下。UI上で直近5回以内の操作であれば許可される。
2. `AppController` は `historyCursor` を1つ戻し、直前の `ActionRecord` に対して逆の操作（`revertAction`）をメモリ上のデータモデルに適用する。
3. 更新された状態（未所属に戻る、フォルダが消える等）をもとに、`FileManager` 経由で関連する `.dat` ファイルをすべて上書き出力する。
4. UIを更新する。（※全履歴データ自体は消去されずカーソルが移動しただけなので、直後にRedoが可能となる）
