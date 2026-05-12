# 詳細設計 (Detailed Design)

本ドキュメントは、要件定義および基本設計に基づき、各層の具体的なクラス設計、データ構造、および主要な処理フローを定義する。
全体のレイヤー構造については、[基本設計書](./basic_design.md) の図1（内部アーキテクチャ図）を参照のこと。

---

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
  - `void groupRenamed(int groupId, const QString& newName)`
- **スロット**:
  - `void on_twitchLoginButton_clicked()`
  - `void onLoginSuccess()`
  - `void updateFollowerList(const QList<TwitchFollower>& followers)`: メモ列を含む最新情報をリストに反映する。
  - `void updateGroupTree(const QMap<int, QString>& groups)`
  - `void on_outDirTreeView_clicked(const QModelIndex &index)` (ツリークリックでリストの絞り込みを実行)
  - `void on_group_tree_context_menu(const QPoint& pos)` (リネーム・削除メニュー)
- **ツリービューの構成**:
  - 以下の順序でカテゴリを表示し、視覚的な分離を行う。
  - 1. **システム予約グループ**: 「すべて (n)」「未所属 (n)」「削除済み (n)」
  - 2. **区切り線**: 選択・操作不可のダミーアイテム（テキスト: `──────────`）
  - 3. **ユーザー定義グループ**: ユーザーが作成したフォルダ群。
- **データアクセス規約**:
  - リストビューの各行アイテム（第0列等）に `Qt::UserRole` で `userId` を格納する。
  - プログラム内では `COL_USER_ID` 等の定数による列直接参照を極力避け、`UserRole` からのデータ取得を優先する。
- **ソート機能**:
  - `pProxyMdl->setSortRole(Qt::UserRole)` を設定し、IDや日付などの非表示データに基づく正確な並べ替えを行う。
  - `pUi->followerListView->setSortingEnabled(true)` により、ヘッダークリックでのソートを有効化する。
  - ソート状態（列インデックス・順序）は `QSettings` を用いて永続化する。

### 1.2 Core層 (Business Logic)
- **AppController クラス**:
    - アプリケーション全体のステートとフローを管理。
    - **状態管理**: 処理中フラグ（`isBusy`）を保持し、API 通信やファイル保存中は UI 層（MainWindow）の操作を無効化（`setEnabled(false)`）する。
    - **メンバ変数**:
      - `MainWindow *mainWindow`
      - `TwitchAuthenticator *authenticator`
      - `TwitchApiClient *apiClient`
      - `FileManager *fileManager`
      - `QList<TwitchFollower> currentFollowers`
      - `QMap<int, QString> currentGroups`
      - `QList<ActionRecord> actionHistory`
      - `int historyCursor`
    - **メソッド**:
      - `void initialize()`
      - `void applyAction(const ActionRecord& action)`
      - `void revertAction(const ActionRecord& action)`
      - `void pushAction(const ActionRecord& action)`

### 1.3 API層 (Data & Communication)
- **FileManager クラス**:
    - 設定およびデータの永続化を担当。
    - **パス解決**: Windows API (`GetModuleFileNameA`) を使用して実行ファイルのディレクトリを取得し、それを基準に `out/` および `Config/` への絶対パスを動的に決定する。
    - **暗号化連携**: `TransCipher` ライブラリを利用。保存時にログイン中の Twitch ユーザー ID (Numeric) を取得し、内部固定キーと組み合わせて暗号化キーを生成する。

---

## 2. データモデル (Data Models)

**`TwitchFollower` (struct)**
- `QString userId`: ユーザーID
- `QString userName`: 表示名 (Display Name)
- `QString userLogin`: ログインID
- `QDateTime followedAt`: Twitch APIから取得した最新のフォロー日時
- `QList<QDateTime> followHistory`: 過去のフォロー日時履歴（昇順）
- `QList<QDateTime> unfollowHistory`: 解除検知日時の履歴（昇順）
- `QList<int> groupIds`: 所属するグループIDのリスト
- `QString memo`: ユーザーごとの自由記述メモ
  - **内部保存用エンコード**: カンマ（`,`）➔ `&comma;`、引用符（`"`）➔ `&quot;`、改行（`\n`）➔ `&nl;` に置換して 1 行を維持する。
  - **外部出力用デコード**: エクスポート時は元の記号に戻し、RFC 4180 準拠の形式で出力する。

### 2.2 ActionRecord (操作履歴)
- 操作の種類、対象、および変更前後の値を保持。
- スタック構造 (`std::deque` または `QStack`) で保持し、最新の操作が先頭に来るように管理。
- 新規操作が行われた際、リドゥ用スタックがある場合は破棄する。
- `enum ActionType { AssignGroup, UnassignGroup, CreateGroup, DeleteGroup, RenameGroup }`
- `ActionType type`
- `QString targetUserId`
- `int targetGroupId`
- `QString targetGroupName` (新規作成、または変更後の名前)
- `QString prevGroupName` (変更前の名前、Undo用)

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

---

## 4. データセキュリティと復旧仕様

### 4.1 暗号化キー生成アルゴリズム
`TransCipher` に渡す鍵文字列は以下のルールで生成する。
- `KeyString = [ProjectInternalSecret] + [TwitchNumericUserID]`
- **ProjectInternalSecret**: ソースコード内に定数として保持される 32 文字以上のランダムな文字列。
- **TwitchNumericUserID**: ログイン中のユーザーから API (`/helix/users`) 経由で取得した不変の数字 ID。

### 4.2 鍵のバージョニング
将来のセキュリティ強化に備え、保存ファイルのヘッダー領域（最初の数バイト）に `Key Version` (uint8) を記録する。
- **Version 1**: 2026-05-07 定義のアルゴリズム。
- アプリケーションは、読み込み時にヘッダーのバージョンを確認し、対応する内部秘密文字列を選択して復号を試みる。

### 4.3 データ復旧（リカバリ）
万が一、アプリケーション経由でデータが開けなくなった場合の救済措置。
1. **仕様ベースの復旧**: 本設計書に記載されたアルゴリズムと内部秘密文字列を用いて、独立した復号ツール（`TransCipher` CLI 等）により平文を抽出可能とする。
2. **開発者対応**: ユーザーから暗号化ファイルと Twitch ID の提供を受けた場合、開発者がソースコード上のアルゴリズムを用いて復号し、CSV 等で返却することを可能とする。

---

## 5. ログ出力仕様

### 5.1 基本ポリシー
- **ISO 8601 準拠**: タイムスタンプは `YYYY-MM-DDTHH:mm:ssZ` 形式（UTC）またはローカル時間＋オフセットで記録する。
- **ファイルローテーション**: `logs/` ディレクトリ配下に `YYYYMMDD.log` の形式で日別に保存する。
- **コンソールミラーリング**: 開発およびデバッグのため、`qDebug` 経由で標準出力にも同様の内容を流す。

### 5.2 メッセージ管理（分散管理方式）
- ログ出力の「仕組み」は共通の `Logger` クラスが提供するが、メッセージの「定義」は各処理クラスが個別に保持する。
- **ID 体系**: クラスごとに独立した数値 ID を使用する。クラス A とクラス B で ID が重複しても、各クラスの内部処理で文字列に変換されるため問題ない。
- **ログレベルの定義**:
  - **INFO**: アプリケーションの正常な動作、ユーザー操作の完了。
  - **WARN**: 内部的なエラーが発生したが、ユーザー操作は続行可能な状態（通信の一時失敗とリトライなど）。
  - **ERROR**: 致命的なエラーが発生し、現在の操作またはアプリケーション自体が続行不可能な状態。

### 5.3 ログ出力フロー
1. 処理クラス内でイベント（成功、失敗）が発生。
2. 処理クラスが自身の持つ「メッセージテーブル」から、イベントに対応する ID の文章を抽出。
3. `Logger::output(Level, Message)` を呼び出し、整形済みの文字列を渡す。
4. `Logger` がタイムスタンプを付与し、スレッドセーフにファイルとコンソールへ書き出す。

---

## 6. UI/UX 詳細
### 6.1 スマート・リフレッシュ・プロトコル (Smart Refresh Protocol)
- **概要**: ユーザーの「更新」要求に対し、トークンの有効期限（セッション状態）に応じて最適な処理を選択し、体感速度を向上させる。
- **処理分岐**:
    1. **初回 / 期限切れ時**: `TwitchAuthenticator` を介したフル・ログインフローを実行。この間、UI は原則としてロック（`setEnabled(false)`）し、ステータスバーに進捗を表示する。
    2. **期限内リフレッシュ**: ブラウザを介さず、メモリ上のデータを「再描画」することで即時の応答性を確保する。物理的な API 通信は行わないか、バックグラウンドでのサイレント更新に留める。
- **視覚的演出**: 「フリ」の際も `followerModel` を一度リセットして再構築することで、意図的に画面のチラつき（更新感）を発生させ、HSP 的な「手応え」をユーザーに与える。

### 6.2 処理中状態の制御
- **局所的ロック**: 全面的な `setEnabled(false)` は初回ログイン等の致命的な不整合が起きうる場面に限定し、通常のリフレッシュ時は操作（スクロールやグループ閲覧）を許可する。
- **フィードバック**: 画面中央をスピナーで塞ぐのではなく、ステータスバーやボタンのテキスト変更（「取得中...」）により、現在の状態をユーザーに伝える。
- **タイムアウトとリカバリ**:
  - 通信開始時にタイマーを起動し、30秒以内に応答がない場合は強制的にロックを解除する。
  - その際、「ネットワークエラー: 接続を確認してください」というメッセージボックスを表示し、ユーザーが再試行できるようにする。

### 6.3 件数表示 (n) の更新ロジック
- **算出方法**:
    - メモリ効率を考慮し、**`AppController` がメモリ上のフォロワーリストを走査して各カテゴリの件数（集計結果）を算出する。**
    - `MainWindow` はリスト本体を保持せず、`AppController` から渡された集計数値のみを受け取って表示を更新する。
    - **ユーザーグループ**: `follower.groupIds.contains(groupId)` の数を集計。
    - **未所属**: `follower.groupIds.isEmpty()` の数を集計。
    - **すべて**: 全フォロワーの実数を集計。
- **描画タイミング**: メモリ上のデータモデル（`TwitchFollower` のリスト）に変更があった際、即座にツリービューの再計算と描画を実行する。
    1. 起動時：ファイルからメモリへデータがロードされた直後
    2. 操作時：DnD や Undo/Redo によりメモリ上の所属情報が変更された直後
    3. 同期時：「Update」ボタンにより最新のフォロワー情報がメモリに反映された直後
### 6.4 チャンネルURLの生成
- **生成ルール**: `https://twitch.tv/` + `userLogin`
- **設計方針**: メモリ節約のため、`TwitchFollower` 構造体には URL 文字列を保持せず、以下のタイミングで動的に生成する。
    1. **リストビュー表示時**: `MainWindow::setFollowers` 内で各行にセット。
    2. **CSVエクスポート時**: `AppController` での書き出し時に生成。

### 6.5 履歴詳細（ツールチップ）の表示仕様
- **表示内容**: `followHistory` と `unfollowHistory` を対応させた履歴テーブル。
- **フォーマット**: HTML を使用し、罫線なしの整列された表形式で表示。
- **生成ルール**:
    - フォロー日とアンフォロー日をペアにして 1 行ずつ表示。
    - 現在フォロー中の場合、最後の「アンフォロー」セルは空文字または「...」とする。
    - メインリスト表示用には `followHistory.first()` (最古) と `unfollowHistory.last()` (最新) を抽出して使用する。

---

## 7. 自動テスト設計 (Automated Testing)

### 7.1 UI イベントシミュレーション (`Qt Test`)
- **MainWindow テスト**:
    - `QTest::mouseClick()`: ボタンクリックやリスト選択をエミュレート。
    - `QTest::keyClicks()`: 文字入力（グループ名など）をエミュレート。
    - `QContextMenuEvent` シミュレーション: 右クリックメニューの表示と選択をテスト。
- **検証（Assertion）対象**:
    - コントローラー内の `lstActnHstry` のサイズと内容。
    - `FileManager` が出力する物理ファイルの内容（Base64デコード後の整合性）。
    - UI モデル（`followerModel`）の行数や特定セルの状態。

### 7.2 テスト環境の分離
- **Mock 通信**: Twitch API は実際には叩かず、`TwitchApiClient` の Mock を作成して JSON レスポンスを注入する。
- **一時ディレクトリ**: ファイル出力テストは Windows の `%TEMP%` 以下の隔離された場所で行い、既存のユーザーデータを汚染しないようにする。

### 7.3 CI/CD 連携 (Jenkins)
- **サーバー**: `192.168.0.45:8080` (DESKTOP-P0F9PN0)
- **実行方式**: 
  - `ctest` コマンドによりテストをバッチ実行。
  - 結果は `-T Test` オプションにより XML 形式（JUnit 互換）で出力。
- **パイプライン**:
  - Git プッシュを検知して自動ビルドを開始。
  - テスト失敗時はビルドを不安定（Unstable）または失敗（Failure）とし、開発者に通知。

---

## 8. 履歴蓄積ロジック (History Synchronization)

### 8.1 差分チェックアルゴリズム
API から取得した `NewList` とローカルの `OldList` を `userId` をキーに比較する。

1.  **新規出現 (New in API)**:
    - ローカルに存在しない場合：`followHistory` に `followedAt` を追加。
    - ローカルに存在（かつ前回消失状態）していた場合：`followHistory` に最新の `followedAt` を追加して「復帰」とみなす。
2.  **継続 (Exists in both)**:
    - API の `followedAt` がローカルの最新履歴より新しい場合：`followHistory` に追加（Twitch 側で一度解除して即再フォローされた可能性などへの対応）。
3.  **消失 (Missing in API)**:
    - ローカルには存在するが API にない場合：そのユーザーを `unfollowHistory` に「現在時刻」を追加して保存。
