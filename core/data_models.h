#ifndef DATA_MODELS_H
#define DATA_MODELS_H

#include <QDateTime>
#include <QList>
#include <QString>

// Twitch APIから取得したフォロワー情報を保持する構造体
struct TwitchFollower {
  QString userId;
  QString userName;
  QString userLogin;
  QDateTime followedAt;      // Twitch APIから取得した最新のフォロー日時
  QList<QDateTime> followHistory;   // 過去のフォロー日時履歴
  QList<QDateTime> unfollowHistory; // 解除検知日時の履歴
  QList<int> groupIds; // 所属するグループIDのリスト。空の場合は「未所属」
  QString memo;       // ユーザーごとの自由記述メモ
};

// 操作履歴（Undo/Redo用）の1単位を示す構造体
struct ActionRecord {
  enum ActionType {
    AssignGroup,   // フォロワーをグループに追加
    UnassignGroup, // フォロワーをグループから削除
    CreateGroup,   // 新規グループ（フォルダ）作成
    DeleteGroup,   // グループ（フォルダ）削除
    RenameGroup    // グループ（フォルダ）名の変更
  };

  ActionType type;
  QString targetUserId;    // 対象のフォロワーID（グループ作成・削除・変更時は空）
  int targetGroupId;       // 対象のグループID
  QString targetGroupName; ///< 対象グループ名（作成・削除・変更後の名前）
  QString prevGroupName;   ///< 変更前のグループ名（名前変更 Undo 用）
  QList<int> prevGroupIds; ///< 操作前のグループ ID リスト（解除時 Undo 用）
};

#endif // DATA_MODELS_H
