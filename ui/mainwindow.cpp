/**
 * @file mainwindow.cpp
 * @brief MainWindow クラスの実装。
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "api/file_manager.h"
#include <QStandardItemModel>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QMenu>
#include <QContextMenuEvent>
#include <QDropEvent>
#include <QDragMoveEvent>
#include <QMimeData>
#include <QDesktopServices>
#include <QUrl>
#include <QSet>

/**
 * @brief コンストラクタ。UI のセットアップを行う。
 * @param pParent 親オブジェクト。
 */
MainWindow::MainWindow(QWidget *pParent)
    : QMainWindow(pParent), pUi(new Ui::MainWindow)
{
    pUi->setupUi(this);
    setupUiExtra();
}

MainWindow::~MainWindow() {
    delete pUi;
}

/**
 * @brief ツリービューの項目がクリックされた際の処理。
 * @param index クリックされたインデックス。
 */
void MainWindow::onGroupTreeClicked(const QModelIndex& index) {
    int iGrpId = index.data(Qt::UserRole).toInt();
    emit groupSelected(iGrpId);
}

/**
 * @brief ドラッグ＆ドロップのドロップ処理をハンドルするイベントフィルター。
 */
bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == pUi->outDirTreeView || obj == pUi->outDirTreeView->viewport()) {
        if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove) {
            QDragMoveEvent *pDrgMv = static_cast<QDragMoveEvent*>(event);
            pDrgMv->acceptProposedAction();
            return true;
        }
        if (event->type() == QEvent::Drop) {
            QDropEvent *pDrpEv = static_cast<QDropEvent*>(event);
            QModelIndex idxTrgt = pUi->outDirTreeView->indexAt(pDrpEv->position().toPoint());
            
            if (idxTrgt.isValid()) {
                int iGid = idxTrgt.data(Qt::UserRole).toInt();
                if (iGid >= 0 || iGid == FileManager::iGRP_ID_UNASSIGNED) {
                    QItemSelectionModel *pSlctn = pUi->followerListView->selectionModel();
                    QModelIndexList lstIdxs = pSlctn->selectedIndexes();
                    QSet<int> setRows;
                    for (const QModelIndex& idx : lstIdxs) {
                        setRows.insert(idx.row());
                    }

                    if (setRows.isEmpty() && pUi->followerListView->currentIndex().isValid()) {
                        setRows.insert(pUi->followerListView->currentIndex().row());
                    }

                    QStringList lstTargetIds;
                    for (int iRow : setRows) {
                        QStandardItem *pItmId = pMdlFllwr->item(iRow, COL_USER_ID);
                        if (pItmId) {
                            QString szUsrId = pItmId->text().trimmed();
                            if (!szUsrId.isEmpty()) {
                                lstTargetIds << szUsrId;
                            }
                        }
                    }

                    if (!lstTargetIds.isEmpty()) {
                        if (iGid >= 0) {
                            emit followersAssignedToGroup(lstTargetIds, iGid);
                        } else {
                            emit followersUnassignedFromGroup(lstTargetIds, -1);
                        }
                    }
                }
            }
            pDrpEv->setDropAction(Qt::IgnoreAction);
            pDrpEv->accept();
            return true; // 標準のドロップ処理を完全に阻止
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

/**
 * @brief Output ボタンクリック時の処理。
 */
void MainWindow::onOutputButtonClicked() {
    emit outputRequested();
}

/**
 * @brief フォロワーリストのコンテキストメニュー表示。
 */
void MainWindow::onFollowerListContextMenu(const QPoint& pos) {
    QModelIndex index = pUi->followerListView->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    QString szUsrId = pMdlFllwr->item(index.row(), COL_USER_ID)->text(); // ID列
    QString szDisplayName = pMdlFllwr->item(index.row(), COL_DISPLAY_NAME)->text();

    QMenu oMenu(this);
    oMenu.setTitle(szDisplayName);

    // グループに追加サブメニュー
    QMenu *pSubAdd = oMenu.addMenu("グループに追加");
    
    // 現在のグループリストをツリーモデルから収集
    QStandardItem *pItmAll = nullptr;
    for (int i = 0; i < pMdlGrp->rowCount(); ++i) {
        if (pMdlGrp->item(i)->data(Qt::UserRole).toInt() == FileManager::iGRP_ID_ALL) {
            pItmAll = pMdlGrp->item(i);
            break;
        }
    }

    if (pItmAll) {
        for (int i = 0; i < pItmAll->rowCount(); ++i) {
            QStandardItem *pChild = pItmAll->child(i);
            int iGid = pChild->data(Qt::UserRole).toInt();
            if (iGid >= 0) { // ユーザー定義グループのみ
                QAction *pAct = pSubAdd->addAction(pChild->text());
                connect(pAct, &QAction::triggered, [this, szUsrId, iGid]() {
                    emit followerAssignedToGroup(szUsrId, iGid);
                });
            }
        }
    }

    // グループから解除メニュー
    QStandardItem *pItmGrp = pMdlFllwr->item(index.row(), COL_GROUPS);
    QString szGidsRaw = pItmGrp->data(Qt::UserRole).toString(); // UserRole から ID リストを取得
    if (!szGidsRaw.isEmpty()) {
        QMenu *pSubRem = oMenu.addMenu("グループから解除");
        QStringList lstGids = szGidsRaw.split(",", Qt::SkipEmptyParts);
        for (const QString& szGid : lstGids) {
            int iGid = szGid.toInt();
            // IDから名前を逆引き（ツリーから）
            QString szGNm = "Group " + szGid;
            if (pItmAll) {
                for (int j = 0; j < pItmAll->rowCount(); ++j) {
                    if (pItmAll->child(j)->data(Qt::UserRole).toInt() == iGid) {
                        szGNm = pItmAll->child(j)->text();
                        break;
                    }
                }
            }
            QAction *pAct = pSubRem->addAction(szGNm);
            connect(pAct, &QAction::triggered, [this, szUsrId, iGid]() {
                emit followerUnassignedFromGroup(szUsrId, iGid);
            });
        }
    }

    oMenu.exec(pUi->followerListView->viewport()->mapToGlobal(pos));
}

/**
 * @brief UI 要素の初期設定とシグナル・スロットの接続。
 */
void MainWindow::setupUiExtra() {
    pMdlFllwr = new QStandardItemModel(this);
    pMdlFllwr->setHorizontalHeaderLabels({"表示名", "ユーザー名", "ID", "チャンネルURL", "グループ"});
    pUi->followerListView->setModel(pMdlFllwr);
    
    // 列幅を内容に合わせて自動調整し、入り切らない場合はスクロールバーを表示
    // 初期状態は手動調整可能にし、データセット時に内容に合わせる
    pUi->followerListView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    pUi->followerListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    pMdlGrp = new QStandardItemModel(this);
    pMdlGrp->setHorizontalHeaderLabels({"グループ名"});
    pUi->outDirTreeView->setModel(pMdlGrp);

    // システム予約グループの初期追加
    QStandardItem *pItmAll = new QStandardItem("すべて");
    pItmAll->setData(FileManager::iGRP_ID_ALL, Qt::UserRole);
    pMdlGrp->appendRow(pItmAll);

    QStandardItem *pItmUnassigned = new QStandardItem("未所属");
    pItmUnassigned->setData(FileManager::iGRP_ID_UNASSIGNED, Qt::UserRole);
    pItmAll->appendRow(pItmUnassigned);

    pUi->outDirTreeView->expandAll();

    connect(pUi->twitchLoginButton, &QPushButton::clicked, this, &MainWindow::onLoginButtonClicked);
    connect(pUi->createFolderButton, &QPushButton::clicked, this, &MainWindow::onAddGroupButtonClicked);
    connect(pUi->deleteFolderButton, &QPushButton::clicked, this, &MainWindow::onDeleteGroupButtonClicked);
    connect(pUi->undoButton, &QPushButton::clicked, this, &MainWindow::onUndoButtonClicked);
    connect(pUi->redoButton, &QPushButton::clicked, this, &MainWindow::onRedoButtonClicked);
    connect(pUi->outDirTreeView, &QTreeView::clicked, this, &MainWindow::onGroupTreeClicked);

    // Output ボタンの追加
    pBtnOutput = new QPushButton("Output", this);
    pBtnOutput->setEnabled(false);
    pUi->horizontalLayout->insertWidget(4, pBtnOutput);
    connect(pBtnOutput, &QPushButton::clicked, this, &MainWindow::onOutputButtonClicked);

    // D&D 設定
    pUi->followerListView->setDragEnabled(true);
    pUi->followerListView->setDragDropMode(QAbstractItemView::DragOnly);
    pUi->outDirTreeView->setAcceptDrops(true);
    pUi->outDirTreeView->setDragDropMode(QAbstractItemView::DropOnly);

    // コンテキストメニュー設定
    pUi->followerListView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(pUi->followerListView, &QTreeView::customContextMenuRequested, this, &MainWindow::onFollowerListContextMenu);

    // リンククリック（ダブルクリック）でブラウザを開く
    connect(pUi->followerListView, &QTreeView::doubleClicked, [this](const QModelIndex& index) {
        if (index.column() == COL_CHANNEL_URL) {
            QString szUrl = index.data().toString();
            if (szUrl.startsWith("http")) {
                QDesktopServices::openUrl(QUrl(szUrl));
            }
        }
    });

    // イベントフィルター登録
    pUi->outDirTreeView->installEventFilter(this);
    pUi->outDirTreeView->viewport()->installEventFilter(this);
}

/**
 * @brief ログイン状態の表示を更新する。
 * @param bLgned ログイン済みなら true。
 */
void MainWindow::setLoginStatus(bool bLgned) {
    if (bLgned) {
        pUi->statusbar->showMessage("Twitch ログイン済み");
        pUi->twitchLoginButton->setText("Update (更新)");
    } else {
        pUi->statusbar->showMessage("未ログイン");
        pUi->twitchLoginButton->setText("Twitch Login");
    }
    pUi->twitchLoginButton->setEnabled(true);
}

/**
 * @brief フォロワーリストを UI モデルに反映する。
 * @param lstFllwrs 表示対象のリスト。
 */
void MainWindow::setFollowers(const QList<TwitchFollower>& lstFllwrs, const QMap<int, QString>& mapGrps) {
    pMdlFllwr->setRowCount(0);
    for (const auto& oFllwr : lstFllwrs) {
        QList<QStandardItem*> lstItems;
        lstItems << new QStandardItem(oFllwr.userName);
        lstItems << new QStandardItem(oFllwr.userLogin);
        lstItems << new QStandardItem(oFllwr.userId);
        
        // チャンネルURLを動的に生成し、リンクスタイルを適用
        QStandardItem *pItmUrl = new QStandardItem("https://twitch.tv/" + oFllwr.userLogin);
        pItmUrl->setForeground(Qt::blue);
        QFont fontUrl = pItmUrl->font();
        fontUrl.setUnderline(true);
        pItmUrl->setFont(fontUrl);
        lstItems << pItmUrl;
        
        // グループIDを名前に変換
        QStringList lstGNms;
        QStringList lstGidsStr;
        for (int iGid : oFllwr.groupIds) {
            if (mapGrps.contains(iGid)) {
                lstGNms << mapGrps.value(iGid);
            } else if (iGid == FileManager::iGRP_ID_UNASSIGNED) {
                lstGNms << "未所属";
            }
            lstGidsStr << QString::number(iGid);
        }
        QStandardItem *pItmGrp = new QStandardItem(lstGNms.join(", "));
        pItmGrp->setData(lstGidsStr.join(","), Qt::UserRole); // 内部的に ID をカンマ区切りで保持
        lstItems << pItmGrp;
        
        pMdlFllwr->appendRow(lstItems);
    }
    
    if (pBtnOutput) {
        pBtnOutput->setEnabled(!lstFllwrs.isEmpty());
    }

    // 内容に合わせて列幅を一度調整（見切れ防止）
    pUi->followerListView->resizeColumnsToContents();
}

/**
 * @brief グループマップを UI モデルに反映する。
 * @param mapGrps 表示対象のマップ。
 */
void MainWindow::setGroups(const QMap<int, QString>& mapGrps, const QMap<int, int>& mapCounts) {
    // 現在選択されているグループ ID を記憶
    int iSlctdId = INVALID_ID;
    QModelIndexList lstSlctd = pUi->outDirTreeView->selectionModel()->selectedIndexes();
    if (!lstSlctd.isEmpty()) {
        iSlctdId = lstSlctd.first().data(Qt::UserRole).toInt();
    }

    // 「すべて」ノードを探す
    QStandardItem *pItmAll = nullptr;
    QStandardItem *pItmUnassigned = nullptr;
    for (int i = 0; i < pMdlGrp->rowCount(); ++i) {
        QStandardItem *pItm = pMdlGrp->item(i);
        int iGid = pItm->data(Qt::UserRole).toInt();
        if (iGid == FileManager::iGRP_ID_ALL) {
            pItmAll = pItm;
            // 「すべて」の件数を更新
            int iCount = mapCounts.value(FileManager::iGRP_ID_ALL, 0);
            pItmAll->setText(QString("すべて (%1)").arg(iCount));
        }
    }

    if (!pItmAll) {
        return;
    }

    // 「未所属」ノードを探す（「すべて」の子として存在）
    for (int i = 0; i < pItmAll->rowCount(); ++i) {
        QStandardItem *pChild = pItmAll->child(i);
        if (pChild->data(Qt::UserRole).toInt() == FileManager::iGRP_ID_UNASSIGNED) {
            pItmUnassigned = pChild;
            break;
        }
    }

    if (pItmUnassigned) {
        int iCount = mapCounts.value(FileManager::iGRP_ID_UNASSIGNED, 0);
        pItmUnassigned->setText(QString("未所属 (%1)").arg(iCount));
    }

    // 「すべて」の子ノードを一旦クリア（「未所属」以外）
    for (int i = pItmAll->rowCount() - 1; i >= 0; --i) {
        if (pItmAll->child(i)->data(Qt::UserRole).toInt() != FileManager::iGRP_ID_UNASSIGNED) {
            pItmAll->removeRow(i);
        }
    }

    // ユーザー定義グループを追加
    for (auto it = mapGrps.constBegin(); it != mapGrps.constEnd(); ++it) {
        if (it.value().trimmed().isEmpty()) {
            continue; // 空の名前は無視
        }
        
        int iGrpId = it.key();
        int iCount = mapCounts.value(iGrpId, 0);

        QStandardItem *pItm = new QStandardItem(QString("%1 (%2)").arg(it.value()).arg(iCount));
        pItm->setData(iGrpId, Qt::UserRole);
        pItmAll->appendRow(pItm);
    }

    // 「削除済みユーザー」の更新
    int iDltdCount = mapCounts.value(FileManager::iGRP_ID_DELETED, 0);
    QString szDltdTitle = QString("削除済みユーザー (%1)").arg(iDltdCount);
    bool bHasDeleted = false;
    for (int i = 0; i < pMdlGrp->rowCount(); ++i) {
        if (pMdlGrp->item(i)->data(Qt::UserRole).toInt() == FileManager::iGRP_ID_DELETED) {
            pMdlGrp->item(i)->setText(szDltdTitle);
            bHasDeleted = true;
            break;
        }
    }
    if (!bHasDeleted) {
        QStandardItem *pItmDltd = new QStandardItem(szDltdTitle);
        pItmDltd->setData(FileManager::iGRP_ID_DELETED, Qt::UserRole);
        pMdlGrp->appendRow(pItmDltd);
    }

    pUi->outDirTreeView->expandAll();

    // 選択状態の復元
    if (iSlctdId != INVALID_ID) {
        // 全ノードを探索して一致する ID を探す
        QList<QStandardItem*> lstSearch;
        for(int i=0; i<pMdlGrp->rowCount(); ++i) lstSearch << pMdlGrp->item(i);
        
        while(!lstSearch.isEmpty()){
            QStandardItem* pItm = lstSearch.takeFirst();
            if(pItm->data(Qt::UserRole).toInt() == iSlctdId){
                pUi->outDirTreeView->setCurrentIndex(pItm->index());
                break;
            }
            for(int i=0; i<pItm->rowCount(); ++i) lstSearch << pItm->child(i);
        }
    }
}

/**
 * @brief Undo/Redo ボタンの有効化状態を制御する。
 * @param bCanWnd Undo 可能。
 * @param bCanRd Redo 可能。
 */
void MainWindow::setUndoRedoEnabled(bool bCanWnd, bool bCanRd) {
    pUi->undoButton->setEnabled(bCanWnd);
    pUi->redoButton->setEnabled(bCanRd);
}

/**
 * @brief ログインボタンクリック時の内部処理。
 */
void MainWindow::onLoginButtonClicked() {
    emit loginRequested();
}

/**
 * @brief グループ追加ボタンクリック時の内部処理。
 */
void MainWindow::onAddGroupButtonClicked() {
    bool bOk = false;
    QString szNm = QInputDialog::getText(this, "新規グループ", "グループ名を入力してください:", QLineEdit::Normal, "", &bOk);
    if (bOk && !szNm.isEmpty()) {
        emit groupCreated(szNm);
    } else {
        // キャンセルまたは空文字
    }
}

/**
 * @brief グループ削除ボタンクリック時の内部処理。
 */
void MainWindow::onDeleteGroupButtonClicked() {
    QModelIndex oIdx = pUi->outDirTreeView->currentIndex();
    if (oIdx.isValid()) {
        int iGrpId = oIdx.data(Qt::UserRole).toInt();
        emit groupDeleted(iGrpId);
    } else {
        QMessageBox::information(this, "通知", "削除するグループを選択してください。");
    }
}

/**
 * @brief Undo ボタンクリック時の内部処理。
 */
void MainWindow::onUndoButtonClicked() {
    emit undoRequested();
}

/**
 * @brief Redo ボタンクリック時の内部処理。
 */
void MainWindow::onRedoButtonClicked() {
    emit redoRequested();
}
