/**
 * @file app_controller.cpp
 * @brief AppController クラスの実装。
 */

#include "app_controller.h"
#include <QTimer>
#include <QMessageBox>

/**
 * @brief コンストラクタ。メンバの初期化を行う。
 * @param pMainWindow メインウィンドウ。
 * @param pParent 親オブジェクト。
 */
AppController::AppController(MainWindow *pMainWindow, QObject *pParent)
    : QObject(pParent), pView(pMainWindow), bIsBsy(false), iHstryCrsr(-1), iNxtGrpId(1)
{
    pAuthntctr = new TwitchAuthenticator(this);
    pApiClient = new TwitchApiClient(this);
    pFilMngr = new FileManager(this);
    pTmoutTmr = new QTimer(this);
    pTmoutTmr->setSingleShot(true);
}

/**
 * @brief 各コンポーネント間のシグナル・スロット接続と初期データのロードを行う。
 */
void AppController::initialize() {
    connect(pView, &MainWindow::loginRequested, this, &AppController::handleLoginRequest);
    connect(pView, &MainWindow::followerAssignedToGroup, this, &AppController::handleFollowerAssigned);
    connect(pView, &MainWindow::followerUnassignedFromGroup, this, &AppController::handleFollowerUnassigned);
    connect(pView, &MainWindow::groupCreated, this, &AppController::handleGroupCreated);
    connect(pView, &MainWindow::groupDeleted, this, &AppController::handleGroupDeleted);
    connect(pView, &MainWindow::undoRequested, this, &AppController::handleUndoRequested);
    connect(pView, &MainWindow::redoRequested, this, &AppController::handleRedoRequested);

    connect(pAuthntctr, &TwitchAuthenticator::authCompleted, this, &AppController::handleAuthCompleted);
    connect(pApiClient, &TwitchApiClient::currentUserFetched, this, &AppController::handleCurrentUserFetched);
    connect(pApiClient, &TwitchApiClient::followersFetched, this, &AppController::handleFollowersFetched);
    connect(pTmoutTmr, &QTimer::timeout, this, &AppController::handleTimeout);
    
    // 既存ファイルのロード
    pFilMngr->loadAllListDat(lstCrntFllwrs);
    pFilMngr->loadGroupsListDat(mapCrntGrps);
    pFilMngr->loadDeletedUserDat(lstCrntDltdUsrs);
    
    if (!mapCrntGrps.isEmpty()) {
        iNxtGrpId = mapCrntGrps.lastKey() + 1;
    } else {
        iNxtGrpId = 1;
    }
    
    pView->setGroups(mapCrntGrps);
    pView->setFollowers(lstCrntFllwrs);
    pView->setUndoRedoEnabled(false, false);
}

/**
 * @brief ログイン処理を開始し、タイムアウト監視を起動する。
 */
void AppController::handleLoginRequest() {
    setBusy(true);
    pTmoutTmr->start(30000); // 30s timeout
    pAuthntctr->login();
}

/**
 * @brief 認証成功後のトークン設定とカレントユーザー取得開始。
 * @param szTkn アクセストークン。
 */
void AppController::handleAuthCompleted(const QString& szTkn) {
    pView->setLoginStatus(true);
    pApiClient->setAccessToken(szTkn);
    pApiClient->fetchCurrentUser();
}

/**
 * @brief カレントユーザー ID 取得完了後の処理。暗号化キーを確定させる。
 * @param szUsrId Twitch ID。
 */
void AppController::handleCurrentUserFetched(const QString& szUsrId) {
    pFilMngr->setTwitchUserId(szUsrId);
    pApiClient->fetchFollowers();
}

/**
 * @brief フォロワーリスト取得後の差分更新ロジック。
 * @param lstFtchdFllwrs API から取得した最新リスト。
 */
void AppController::handleFollowersFetched(const QList<TwitchFollower>& lstFtchdFllwrs) {
    QList<TwitchFollower> lstNwCrntFllwrs;
    
    QMap<QString, TwitchFollower> mapOldFllwrs;
    for (const auto& oF : lstCrntFllwrs) {
        mapOldFllwrs.insert(oF.userId, oF);
    }
    
    QMap<QString, TwitchFollower> mapDltdFllwrs;
    for (const auto& oF : lstCrntDltdUsrs) {
        mapDltdFllwrs.insert(oF.userId, oF);
    }

    QSet<QString> setFtchdIds;
    for (const auto& oF : lstFtchdFllwrs) {
        setFtchdIds.insert(oF.userId);
        TwitchFollower oNwFllwr = oF;
        
        if (mapOldFllwrs.contains(oF.userId)) {
            oNwFllwr.groupIds = mapOldFllwrs.value(oF.userId).groupIds;
        } else if (mapDltdFllwrs.contains(oF.userId)) {
            oNwFllwr.groupIds = mapDltdFllwrs.value(oF.userId).groupIds;
            for (int i = 0; i < lstCrntDltdUsrs.size(); ++i) {
                if (lstCrntDltdUsrs[i].userId == oF.userId) {
                    lstCrntDltdUsrs.removeAt(i);
                    break;
                } else { /* skip */ }
            }
        } else {
            // New follower
        }
        lstNwCrntFllwrs.append(oNwFllwr);
    }
    
    for (const auto& oF : lstCrntFllwrs) {
        if (!setFtchdIds.contains(oF.userId)) {
            if (!mapDltdFllwrs.contains(oF.userId)) {
                lstCrntDltdUsrs.append(oF);
            } else { /* skip */ }
        } else { /* skip */ }
    }

    lstCrntFllwrs = lstNwCrntFllwrs;
    updateView();
    saveAllState();
    pTmoutTmr->stop();
    setBusy(false);
}

/**
 * @brief アクションを履歴に積み、適用する。Undo後の操作なら未来を切り捨てる。
 * @param oActn 実行するアクション。
 */
void AppController::pushAction(const ActionRecord& oActn) {
    if (iHstryCrsr < lstActnHstry.size() - 1) {
        lstActnHstry.erase(lstActnHstry.begin() + iHstryCrsr + 1, lstActnHstry.end());
    } else { /* skip */ }
    
    lstActnHstry.append(oActn);
    iHstryCrsr = lstActnHstry.size() - 1;
    
    applyAction(oActn);
    updateView();
    saveAllState();
}

/**
 * @brief アクションを現在の状態（メモリ上）に適用する。
 * @param oActn 対象アクション。
 */
void AppController::applyAction(const ActionRecord& oActn) {
    if (oActn.type == ActionRecord::CreateGroup) {
        mapCrntGrps.insert(oActn.targetGroupId, oActn.targetGroupName);
        if (oActn.targetGroupId >= iNxtGrpId) {
            iNxtGrpId = oActn.targetGroupId + 1;
        } else { /* skip */ }
    } else if (oActn.type == ActionRecord::DeleteGroup) {
        mapCrntGrps.remove(oActn.targetGroupId);
        for (auto& oF : lstCrntFllwrs) {
            oF.groupIds.removeAll(oActn.targetGroupId);
        }
    } else if (oActn.type == ActionRecord::AssignGroup) {
        for (auto& oF : lstCrntFllwrs) {
            if (oF.userId == oActn.targetUserId) {
                if (!oF.groupIds.contains(oActn.targetGroupId)) {
                    oF.groupIds.append(oActn.targetGroupId);
                } else { /* skip */ }
                break;
            } else { /* skip */ }
        }
    } else if (oActn.type == ActionRecord::UnassignGroup) {
        for (auto& oF : lstCrntFllwrs) {
            if (oF.userId == oActn.targetUserId) {
                oF.groupIds.removeAll(oActn.targetGroupId);
                break;
            } else { /* skip */ }
        }
    } else { /* skip */ }
}

/**
 * @brief アクションの逆操作を行う。
 * @param oActn 対象アクション。
 */
void AppController::revertAction(const ActionRecord& oActn) {
    if (oActn.type == ActionRecord::CreateGroup) {
        mapCrntGrps.remove(oActn.targetGroupId);
    } else if (oActn.type == ActionRecord::DeleteGroup) {
        mapCrntGrps.insert(oActn.targetGroupId, oActn.targetGroupName);
    } else if (oActn.type == ActionRecord::AssignGroup) {
        for (auto& oF : lstCrntFllwrs) {
            if (oF.userId == oActn.targetUserId) {
                oF.groupIds.removeAll(oActn.targetGroupId);
                break;
            } else { /* skip */ }
        }
    } else if (oActn.type == ActionRecord::UnassignGroup) {
        for (auto& oF : lstCrntFllwrs) {
            if (oF.userId == oActn.targetUserId) {
                if (!oF.groupIds.contains(oActn.targetGroupId)) {
                    oF.groupIds.append(oActn.targetGroupId);
                } else { /* skip */ }
                break;
            } else { /* skip */ }
        }
    } else { /* skip */ }
}

/**
 * @brief UI からのグループ作成要求をハンドルする。
 * @param szGrpNm グループ名。
 */
void AppController::handleGroupCreated(const QString& szGrpNm) {
    ActionRecord oAct;
    oAct.type = ActionRecord::CreateGroup;
    oAct.targetGroupId = iNxtGrpId++;
    oAct.targetGroupName = szGrpNm;
    pushAction(oAct);
}

/**
 * @brief UI からのグループ削除要求をハンドルする。
 * @param iGrpId グループ ID。
 */
void AppController::handleGroupDeleted(int iGrpId) {
    ActionRecord oAct;
    oAct.type = ActionRecord::DeleteGroup;
    oAct.targetGroupId = iGrpId;
    oAct.targetGroupName = mapCrntGrps.value(iGrpId);
    pushAction(oAct);
}

/**
 * @brief UI からのグループ割り当て要求をハンドルする。
 * @param szUsrId ユーザー ID。
 * @param iGrpId グループ ID。
 */
void AppController::handleFollowerAssigned(const QString& szUsrId, int iGrpId) {
    for (const auto& oF : lstCrntFllwrs) {
        if (oF.userId == szUsrId && oF.groupIds.contains(iGrpId)) {
            return; 
        } else { /* skip */ }
    }

    ActionRecord oAct;
    oAct.type = ActionRecord::AssignGroup;
    oAct.targetUserId = szUsrId;
    oAct.targetGroupId = iGrpId;
    pushAction(oAct);
}

/**
 * @brief UI からのグループ解除要求をハンドルする。
 * @param szUsrId ユーザー ID。
 * @param iGrpId グループ ID。
 */
void AppController::handleFollowerUnassigned(const QString& szUsrId, int iGrpId) {
    bool bFound = false;
    for (const auto& oF : lstCrntFllwrs) {
        if (oF.userId == szUsrId && oF.groupIds.contains(iGrpId)) {
            bFound = true;
            break;
        } else { /* skip */ }
    }
    if (!bFound) {
        return;
    } else {
        ActionRecord oAct;
        oAct.type = ActionRecord::UnassignGroup;
        oAct.targetUserId = szUsrId;
        oAct.targetGroupId = iGrpId;
        pushAction(oAct);
    }
}

/**
 * @brief Undo 要求をハンドルする。
 */
void AppController::handleUndoRequested() {
    if (iHstryCrsr >= 0) {
        revertAction(lstActnHstry[iHstryCrsr]);
        iHstryCrsr--;
        updateView();
        saveAllState();
    } else { /* skip */ }
}

/**
 * @brief Redo 要求をハンドルする。
 */
void AppController::handleRedoRequested() {
    if (iHstryCrsr < lstActnHstry.size() - 1) {
        iHstryCrsr++;
        applyAction(lstActnHstry[iHstryCrsr]);
        updateView();
        saveAllState();
    } else { /* skip */ }
}

/**
 * @brief 通信タイムアウト時の処理。ロックを解除し警告を表示する。
 */
void AppController::handleTimeout() {
    if (bIsBsy) {
        setBusy(false);
        QMessageBox::warning(pView, "ネットワークエラー", 
            "通信がタイムアウトしました。ネットワーク接続を確認し、再度お試しください。");
    } else { /* skip */ }
}

/**
 * @brief UI の表示内容を最新データでリロードする。
 */
void AppController::updateView() {
    pView->setFollowers(lstCrntFllwrs);
    pView->setGroups(mapCrntGrps);
    
    bool bCanWnd = (iHstryCrsr >= 0);
    bool bCanRd = (iHstryCrsr < lstActnHstry.size() - 1);
    
    pView->setUndoRedoEnabled(bCanWnd, bCanRd);
}

/**
 * @brief 現在の状態を各データファイルに保存する。
 */
void AppController::saveAllState() {
    bool bWasBsy = bIsBsy;
    if (!bWasBsy) {
        setBusy(true);
    } else { /* skip */ }
    
    pFilMngr->saveAllListDat(lstCrntFllwrs);
    pFilMngr->saveGroupsListDat(mapCrntGrps);
    pFilMngr->saveActionHistory(lstActnHstry);
    pFilMngr->saveDeletedUserDat(lstCrntDltdUsrs);
    
    for (auto it = mapCrntGrps.constBegin(); it != mapCrntGrps.constEnd(); ++it) {
        QList<TwitchFollower> lstGrpFllwrs;
        for (const auto& oF : lstCrntFllwrs) {
            if (oF.groupIds.contains(it.key())) {
                lstGrpFllwrs.append(oF);
            } else { /* skip */ }
        }
        pFilMngr->saveGroupListsDat(it.value(), lstGrpFllwrs);
    }
    
    QList<TwitchFollower> lstUnsnd;
    for (const auto& oF : lstCrntFllwrs) {
        if (oF.groupIds.isEmpty()) {
            lstUnsnd.append(oF);
        } else { /* skip */ }
    }
    pFilMngr->saveGroupListsDat("未所属", lstUnsnd);

    if (!bWasBsy) {
        setBusy(false);
    } else { /* skip */ }
}

/**
 * @brief UI の有効・無効を切り替える。
 * @param bBsy true ならロック。
 */
void AppController::setBusy(bool bBsy) {
    bIsBsy = bBsy;
    if (pView) {
        pView->setEnabled(!bBsy);
    } else { /* skip */ }
}
