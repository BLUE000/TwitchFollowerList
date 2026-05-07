#ifndef DATA_MODELS_H
#define DATA_MODELS_H

#include <QString>
#include <QList>
#include <QDateTime>

// Twitch APIから取得したフォロワー情報を保持する構造体
struct TwitchFollower {
    QString userId;
    QString userName;
    QString userLogin;
    QDateTime followedAt;
    QList<int> groupIds; // 所属するグループIDのリスト。空の場合は「未所属」
};

// 操作履歴（Undo/Redo用）の1単位を示す構造体
struct ActionRecord {
    enum ActionType {
        AssignGroup,   // フォロワーをグループに追加
        UnassignGroup, // フォロワーをグループから削除
        CreateGroup,   // 新規グループ（フォルダ）作成
        DeleteGroup    // グループ（フォルダ）削除
    };

    ActionType type;
    QString targetUserId;  // 対象のフォロワーID（グループ作成・削除時は空）
    int targetGroupId;     // 対象のグループID
    QString targetGroupName;                ///< 対象グループ名（作成・削除時）
    QList<int> prevGroupIds;                ///< 操作前のグループ ID リスト（解除時 Undo 用）
};

#endif // DATA_MODELS_H
