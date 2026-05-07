/**
 * @file app_controller.cpp
 * @brief AppController クラスの実装。
 */

#include "app_controller.h"
#include "../api/logger.h"
#include <QTimer>
#include <QFileDialog>
#include <QDateTime>
#include <QTextStream>
#include <QDir>
#include <QMessageBox>
#include <QRegularExpression>

/**
 * @brief コンストラクタ。メンバの初期化を行う。
 * @param pMainWindow メインウィンドウ。
 * @param pParent 親オブジェクト。
 */
AppController::AppController(MainWindow *pMainWindow, QObject *pParent)
    : QObject(pParent), pView(pMainWindow), bIsBsy(false), iHstryCrsr(-1), iNxtGrpId(1), iSlctdGrpId(FileManager::iGRP_ID_ALL)
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
    connect(pView, &MainWindow::groupSelected, this, &AppController::handleGroupSelected);
    connect(pView, &MainWindow::outputRequested, this, &AppController::handleOutputRequested);

    connect(pAuthntctr, &TwitchAuthenticator::authCompleted, this, &AppController::handleAuthCompleted);
    connect(pApiClient, &TwitchApiClient::currentUserFetched, this, &AppController::handleCurrentUserFetched);
    connect(pApiClient, &TwitchApiClient::followersFetched, this, &AppController::handleFollowersFetched);
    connect(pTmoutTmr, &QTimer::timeout, this, &AppController::handleTimeout);
    
    // ログテーブルの初期化
    m_infoTable[INF_APP_INIT]    = "アプリケーションを初期化しました。";
    m_infoTable[INF_LOGIN_START] = "Twitch ログイン処理を開始します。";
    m_infoTable[INF_AUTH_OK]     = "Twitch 認証に成功しました。";
    m_infoTable[INF_USR_FETCH_OK] = "ユーザー情報の取得に成功しました。";
    m_infoTable[INF_FLW_FETCH_OK] = "フォロワーリストの取得に成功しました。";
    m_infoTable[INF_EXPORT_OK]   = "CSV エクスポートが正常に完了しました。";

    m_warnTable[WRN_TIMEOUT]     = "通信がタイムアウトしました。";

    m_errorTable[ERR_USR_FETCH]  = "ユーザー情報の取得に失敗しました。";
    m_errorTable[ERR_FLW_FETCH]  = "フォロワーリストの取得に失敗しました。";

    log(INF_APP_INIT);
    
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
    pView->setFollowers(lstCrntFllwrs, mapCrntGrps);
    pView->setUndoRedoEnabled(false, false);
}

/**
 * @brief ログイン処理を開始し、タイムアウト監視を起動する。
 */
void AppController::handleLoginRequest() {
    log(INF_LOGIN_START);
    setBusy(true);
    pTmoutTmr->start(30000); // 30s timeout
    pAuthntctr->login();
}

/**
 * @brief 認証成功後のトークン設定とカレントユーザー取得開始。
 * @param szTkn アクセストークン。
 */
void AppController::handleAuthCompleted(const QString& szTkn) {
    log(INF_AUTH_OK);
    pView->setLoginStatus(true);
    pApiClient->setAccessToken(szTkn);
    pApiClient->fetchCurrentUser();
}

/**
 * @brief カレントユーザー ID 取得完了後の処理。暗号化キーを確定させる。
 * @param szUsrId Twitch ID。
 */
void AppController::handleCurrentUserFetched(const QString& szUsrId) {
    log(INF_USR_FETCH_OK);
    pFilMngr->setTwitchUserId(szUsrId);
    pApiClient->fetchFollowers();
}

/**
 * @brief フォロワーリスト取得後の差分更新ロジック。
 * @param lstFtchdFllwrs API から取得した最新リスト。
 */
void AppController::handleFollowersFetched(const QList<TwitchFollower>& lstFtchdFllwrs) {
    log(INF_FLW_FETCH_OK);
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
                if (oActn.targetGroupId == -1) {
                    oF.groupIds.clear();
                } else {
                    oF.groupIds.removeAll(oActn.targetGroupId);
                }
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
                oF.groupIds = oActn.prevGroupIds;
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
    QList<int> lstPrev;
    bool bFound = false;
    for (const auto& oF : lstCrntFllwrs) {
        if (oF.userId == szUsrId) {
            lstPrev = oF.groupIds;
            if (iGrpId == -1) {
                if (!oF.groupIds.isEmpty()) bFound = true;
            } else if (oF.groupIds.contains(iGrpId)) {
                bFound = true;
            }
            if (bFound) break;
        }
    }
    if (!bFound) return;

    ActionRecord oAct;
    oAct.type = ActionRecord::UnassignGroup;
    oAct.targetUserId = szUsrId;
    oAct.targetGroupId = iGrpId;
    oAct.prevGroupIds = lstPrev; // Undo 用に現在の所属を記録
    pushAction(oAct);
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
        log(WRN_TIMEOUT);
        setBusy(false);
        QMessageBox::warning(pView, "ネットワークエラー", 
            "通信がタイムアウトしました。ネットワーク接続を確認し、再度お試しください。");
    } else { /* skip */ }
}

/**
 * @brief グループ選択変更時の処理。
 * @param iGrpId 選択されたグループ ID。
 */
void AppController::handleGroupSelected(int iGrpId) {
    iSlctdGrpId = iGrpId;
    updateView();
}

/**
 * @brief エクスポート（Output）要求をハンドルする。
 */
void AppController::handleOutputRequested() {
    QString szBaseDir = QFileDialog::getExistingDirectory(pView, "保存先フォルダを選択", QDir::homePath());
    if (szBaseDir.isEmpty()) return;

    QString szTs = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    QDir oBase(szBaseDir);
    if (!oBase.mkdir(szTs)) {
        QMessageBox::critical(pView, "エラー", "フォルダの作成に失敗しました。");
        return;
    }
    QDir oTarget(oBase.absoluteFilePath(szTs));

    // 全グループのリストを作成（システム予約＋ユーザー定義）
    QMap<int, QString> mapAllGrps = mapCrntGrps;
    mapAllGrps.insert(FileManager::iGRP_ID_ALL, "すべて");
    mapAllGrps.insert(FileManager::iGRP_ID_UNASSIGNED, "未所属");
    mapAllGrps.insert(FileManager::iGRP_ID_DELETED, "削除済みユーザー");

    for (auto it = mapAllGrps.constBegin(); it != mapAllGrps.constEnd(); ++it) {
        int iGid = it.key();
        QString szGNm = it.value();
        
        QList<TwitchFollower> lstSub;
        if (iGid == FileManager::iGRP_ID_ALL) {
            lstSub = lstCrntFllwrs;
        } else if (iGid == FileManager::iGRP_ID_UNASSIGNED) {
            for (const auto& oF : lstCrntFllwrs) if (oF.groupIds.isEmpty()) lstSub << oF;
        } else if (iGid == FileManager::iGRP_ID_DELETED) {
            lstSub = lstCrntDltdUsrs;
        } else {
            for (const auto& oF : lstCrntFllwrs) if (oF.groupIds.contains(iGid)) lstSub << oF;
        }

        // CSV 書き出し
        QString szFn = szGNm.trimmed().replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_") + ".csv";
        QFile oFile(oTarget.absoluteFilePath(szFn));
        if (oFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream oOut(&oFile);
            oOut.setEncoding(QStringConverter::Utf8);
            oOut.setGenerateByteOrderMark(true); // Excel 用に BOM を付与

            oOut << "表示名,ユーザー名,ユーザーID,グループ\n";
            for (const auto& oF : lstSub) {
                QStringList lstGNms;
                for (int iGid : oF.groupIds) {
                    if (mapCrntGrps.contains(iGid)) lstGNms << mapCrntGrps.value(iGid);
                    else if (iGid == FileManager::iGRP_ID_UNASSIGNED) lstGNms << "未所属";
                }
                QString szGNms = lstGNms.join(", ");
                oOut << QString("\"%1\",\"%2\",\"'%3\",\"%4\"\n")
                            .arg(oF.userName, oF.userLogin, oF.userId, szGNms);
            }
            oFile.close();
        }
    }

    log(INF_EXPORT_OK);
    QMessageBox::information(pView, "完了", "エクスポートが完了しました。\n" + oTarget.absolutePath());
}

/**
 * @brief UI の表示内容を最新データでリロードする。
 */
void AppController::updateView() {
    QList<TwitchFollower> lstFiltered;
    
    if (iSlctdGrpId == FileManager::iGRP_ID_ALL) {
        lstFiltered = lstCrntFllwrs;
    } else if (iSlctdGrpId == FileManager::iGRP_ID_UNASSIGNED) {
        for (const auto& oF : lstCrntFllwrs) {
            if (oF.groupIds.isEmpty()) {
                lstFiltered.append(oF);
            }
        }
    } else if (iSlctdGrpId == FileManager::iGRP_ID_DELETED) {
        lstFiltered = lstCrntDltdUsrs;
    } else {
        // ユーザー指定のグループ ID でフィルタリング
        for (const auto& oF : lstCrntFllwrs) {
            if (oF.groupIds.contains(iSlctdGrpId)) {
                lstFiltered.append(oF);
            }
        }
    }
    
    pView->setFollowers(lstFiltered, mapCrntGrps);
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

void AppController::log(int id) {
    if (m_errorTable.contains(id)) {
        Logger::output("ERROR", m_errorTable[id]);
    } else if (m_warnTable.contains(id)) {
        Logger::output("WARN", m_warnTable[id]);
    } else if (m_infoTable.contains(id)) {
        Logger::output("INFO", m_infoTable[id]);
    }
}
