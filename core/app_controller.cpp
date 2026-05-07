#include "app_controller.h"

AppController::AppController(MainWindow *mainWindow, QObject *parent)
    : QObject(parent), view(mainWindow), historyCursor(-1), nextGroupId(1)
{
    authenticator = new TwitchAuthenticator(this);
    apiClient = new TwitchApiClient(this);
    fileManager = new FileManager(this);
}

void AppController::initialize() {
    connect(view, &MainWindow::loginRequested, this, &AppController::handleLoginRequest);
    connect(view, &MainWindow::followerAssignedToGroup, this, &AppController::handleFollowerAssigned);
    connect(view, &MainWindow::followerUnassignedFromGroup, this, &AppController::handleFollowerUnassigned);
    connect(view, &MainWindow::groupCreated, this, &AppController::handleGroupCreated);
    connect(view, &MainWindow::groupDeleted, this, &AppController::handleGroupDeleted);
    connect(view, &MainWindow::undoRequested, this, &AppController::handleUndoRequested);
    connect(view, &MainWindow::redoRequested, this, &AppController::handleRedoRequested);

    connect(authenticator, &TwitchAuthenticator::authCompleted, this, &AppController::handleAuthCompleted);
    connect(apiClient, &TwitchApiClient::currentUserFetched, this, &AppController::handleCurrentUserFetched);
    connect(apiClient, &TwitchApiClient::followersFetched, this, &AppController::handleFollowersFetched);
    
    // 既存ファイルのロード
    fileManager->loadAllListDat(currentFollowers);
    fileManager->loadGroupsListDat(currentGroups);
    fileManager->loadDeletedUserDat(currentDeletedUsers);
    
    if (!currentGroups.isEmpty()) {
        nextGroupId = currentGroups.lastKey() + 1;
    }
    
    view->setGroups(currentGroups);
    view->setFollowers(currentFollowers);
    view->setUndoRedoEnabled(false, false);
}

void AppController::handleLoginRequest() {
    authenticator->login();
}

void AppController::handleAuthCompleted(const QString& token) {
    view->setLoginStatus(true);
    apiClient->setAccessToken(token);
    apiClient->fetchCurrentUser();
}

void AppController::handleCurrentUserFetched(const QString& userId) {
    apiClient->fetchFollowers();
}

void AppController::handleFollowersFetched(const QList<TwitchFollower>& fetchedFollowers) {
    QList<TwitchFollower> newCurrentFollowers;
    
    // 現在のフォロワーと削除済みフォロワーをMap化しておく
    QMap<QString, TwitchFollower> oldFollowersMap;
    for (const auto& f : currentFollowers) {
        oldFollowersMap.insert(f.userId, f);
    }
    
    QMap<QString, TwitchFollower> deletedFollowersMap;
    for (const auto& f : currentDeletedUsers) {
        deletedFollowersMap.insert(f.userId, f);
    }

    QSet<QString> fetchedIds;
    // APIから取得したユーザーの処理（新規追加・更新・復帰）
    for (const auto& f : fetchedFollowers) {
        fetchedIds.insert(f.userId);
        TwitchFollower newFollower = f;
        
        if (oldFollowersMap.contains(f.userId)) {
            // 既存ユーザー：所属グループ情報を引き継ぐ
            newFollower.groupIds = oldFollowersMap.value(f.userId).groupIds;
        } else if (deletedFollowersMap.contains(f.userId)) {
            // 一度削除されたユーザーが再フォローした場合：元のグループ情報を復元
            newFollower.groupIds = deletedFollowersMap.value(f.userId).groupIds;
            // 削除済みリストから除外する
            for (int i = 0; i < currentDeletedUsers.size(); ++i) {
                if (currentDeletedUsers[i].userId == f.userId) {
                    currentDeletedUsers.removeAt(i);
                    break;
                }
            }
        } else {
            // 完全新規：所属は空（未所属）のまま
        }
        
        newCurrentFollowers.append(newFollower);
    }
    
    // フォロー解除されたユーザーの処理
    for (const auto& f : currentFollowers) {
        if (!fetchedIds.contains(f.userId)) {
            // API結果に含まれない ＝ フォローを外した
            // 所属元のグループ情報は f の中に残したまま、DeletedUserリストへ移動する
            // 既に DeletedUser にいるかは一応チェック
            if (!deletedFollowersMap.contains(f.userId)) {
                currentDeletedUsers.append(f);
            }
        }
    }

    currentFollowers = newCurrentFollowers;
    updateView();
    saveAllState();
}

void AppController::pushAction(const ActionRecord& action) {
    // 現在位置が履歴の最後でない場合（Undo後に新規操作）、未来の履歴を切り捨てる(Truncate)
    if (historyCursor < actionHistory.size() - 1) {
        actionHistory.erase(actionHistory.begin() + historyCursor + 1, actionHistory.end());
    }
    
    actionHistory.append(action);
    historyCursor = actionHistory.size() - 1;
    
    applyAction(action);
    updateView();
    saveAllState();
}

void AppController::applyAction(const ActionRecord& action) {
    if (action.type == ActionRecord::CreateGroup) {
        currentGroups.insert(action.targetGroupId, action.targetGroupName);
        if (action.targetGroupId >= nextGroupId) {
            nextGroupId = action.targetGroupId + 1;
        }
    } else if (action.type == ActionRecord::DeleteGroup) {
        currentGroups.remove(action.targetGroupId);
        for (auto& f : currentFollowers) {
            f.groupIds.removeAll(action.targetGroupId);
        }
    } else if (action.type == ActionRecord::AssignGroup) {
        for (auto& f : currentFollowers) {
            if (f.userId == action.targetUserId) {
                if (!f.groupIds.contains(action.targetGroupId)) {
                    f.groupIds.append(action.targetGroupId);
                }
                break;
            }
        }
    } else if (action.type == ActionRecord::UnassignGroup) {
        for (auto& f : currentFollowers) {
            if (f.userId == action.targetUserId) {
                f.groupIds.removeAll(action.targetGroupId);
                break;
            }
        }
    }
}

void AppController::revertAction(const ActionRecord& action) {
    if (action.type == ActionRecord::CreateGroup) {
        currentGroups.remove(action.targetGroupId);
    } else if (action.type == ActionRecord::DeleteGroup) {
        currentGroups.insert(action.targetGroupId, action.targetGroupName);
        // 注: 今回の簡易実装ではフォルダ削除時の所属解除はUndoで復元されません（要件外）。
        // 完全復元が必要な場合、ActionRecordに削除されたユーザーIDリストを持たせる必要があります。
    } else if (action.type == ActionRecord::AssignGroup) {
        for (auto& f : currentFollowers) {
            if (f.userId == action.targetUserId) {
                f.groupIds.removeAll(action.targetGroupId);
                break;
            }
        }
    } else if (action.type == ActionRecord::UnassignGroup) {
        for (auto& f : currentFollowers) {
            if (f.userId == action.targetUserId) {
                if (!f.groupIds.contains(action.targetGroupId)) {
                    f.groupIds.append(action.targetGroupId);
                }
                break;
            }
        }
    }
}

void AppController::handleGroupCreated(const QString& groupName) {
    ActionRecord act;
    act.type = ActionRecord::CreateGroup;
    act.targetGroupId = nextGroupId++;
    act.targetGroupName = groupName;
    pushAction(act);
}

void AppController::handleGroupDeleted(int groupId) {
    ActionRecord act;
    act.type = ActionRecord::DeleteGroup;
    act.targetGroupId = groupId;
    act.targetGroupName = currentGroups.value(groupId);
    pushAction(act);
}

void AppController::handleFollowerAssigned(const QString& userId, int groupId) {
    // 重複チェック
    for (const auto& f : currentFollowers) {
        if (f.userId == userId && f.groupIds.contains(groupId)) {
            return; // 既に所属しているため何もしない
        }
    }

    ActionRecord act;
    act.type = ActionRecord::AssignGroup;
    act.targetUserId = userId;
    act.targetGroupId = groupId;
    pushAction(act);
}

void AppController::handleFollowerUnassigned(const QString& userId, int groupId) {
    // 所属チェック
    bool found = false;
    for (const auto& f : currentFollowers) {
        if (f.userId == userId && f.groupIds.contains(groupId)) {
            found = true;
            break;
        }
    }
    if (!found) return; // 所属していないため何もしない

    ActionRecord act;
    act.type = ActionRecord::UnassignGroup;
    act.targetUserId = userId;
    act.targetGroupId = groupId;
    pushAction(act);
}

void AppController::handleUndoRequested() {
    if (historyCursor >= 0) {
        revertAction(actionHistory[historyCursor]);
        historyCursor--;
        updateView();
        saveAllState();
    }
}

void AppController::handleRedoRequested() {
    if (historyCursor < actionHistory.size() - 1) {
        historyCursor++;
        applyAction(actionHistory[historyCursor]);
        updateView();
        saveAllState();
    }
}

void AppController::updateView() {
    view->setFollowers(currentFollowers);
    view->setGroups(currentGroups);
    
    // UI上では直近5回までと表示要件があるが、内部的には全履歴を保持しているため
    // 単純にカーソル位置からUndo/Redoが可能かどうかでボタンを制御
    bool canUndo = (historyCursor >= 0);
    bool canRedo = (historyCursor < actionHistory.size() - 1);
    
    view->setUndoRedoEnabled(canUndo, canRedo);
}

void AppController::saveAllState() {
    fileManager->saveAllListDat(currentFollowers);
    fileManager->saveGroupsListDat(currentGroups);
    fileManager->saveActionHistory(actionHistory);
    fileManager->saveDeletedUserDat(currentDeletedUsers); // 削除済みユーザーを保存
    
    // 各グループの出力
    for (auto it = currentGroups.constBegin(); it != currentGroups.constEnd(); ++it) {
        QList<TwitchFollower> groupFollowers;
        for (const auto& f : currentFollowers) {
            if (f.groupIds.contains(it.key())) {
                groupFollowers.append(f);
            }
        }
        fileManager->saveGroupListsDat(it.value(), groupFollowers);
    }
    
    // 未所属の出力
    QList<TwitchFollower> unassigned;
    for (const auto& f : currentFollowers) {
        if (f.groupIds.isEmpty()) {
            unassigned.append(f);
        }
    }
    fileManager->saveGroupListsDat("未所属", unassigned);
}
