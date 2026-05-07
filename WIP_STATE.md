# WIP_STATE (FollowerList)

## 現在の作業状況
2026-05-07: 共通暗号化ライブラリ（TransCipher）の完成を受け、FollowerList への統合フェーズを開始。
設計書（doc/）の更新と master への初期コミットまで完了。

## 現在のフェーズ
- **詳細設計完了・実装開始前**

## 完了済みの実施内容
- **ドキュメント整備**: `requirements.md`, `basic_design.md`, `detailed_design.md` に TransCipher 統合仕様を追記。
- **リポジトリ初期化**: `master` ブランチを作成し、`doc/` フォルダをプッシュ。

## 次回のタスク
- [ ] **実装フェーズ**: `CMakeLists.txt` の修正（TransCipher のリンク設定）。
- [ ] **ロジック移行**: `FileManager.cpp` の `encodeData/decodeData` を TransCipher 経由に差し替え。
- [ ] **動作検証**: 暗号化された `.dat` ファイルの生成と復元を確認。
- [ ] **GitHub へのプッシュ**: コード本体を含む安定版の初プッシュ。

## 留意事項
- `detailed_design.md` は一度要約しすぎてしまったため、元の詳細な記述（クラスメンバ等）を保持した状態で復元済み。
- 暗号鍵は一旦内部固定キーを使用する計画。
