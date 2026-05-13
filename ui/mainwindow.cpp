/**
 * @file mainwindow.cpp
 * @brief MainWindow クラスの実装。
 */

#include "mainwindow.h"
#include "api/file_manager.h"
#include "ui_mainwindow.h"
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QSet>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QSettings>
#include <QUrl>

/**
 * @brief コンストラクタ。UI のセットアップを行う。
 * @param pParent 親オブジェクト。
 */
MainWindow::MainWindow(QWidget *pParent)
    : QMainWindow(pParent), pUi(new Ui::MainWindow) {
  pUi->setupUi(this);
  setupUiExtra();
}

MainWindow::~MainWindow() { delete pUi; }

/**
 * @brief ツリービューの項目がクリックされた際の処理。
 * @param index クリックされたインデックス。
 */
void MainWindow::onGroupTreeClicked(const QModelIndex &index) {
  int iGrpId = index.data(Qt::UserRole).toInt();
  emit groupSelected(iGrpId);
}

/**
 * @brief ドラッグ＆ドロップのドロップ処理をハンドルするイベントフィルター。
 */
bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
  if (obj == pUi->outDirTreeView || obj == pUi->outDirTreeView->viewport()) {
    if (event->type() == QEvent::DragEnter ||
        event->type() == QEvent::DragMove) {
      QDragMoveEvent *pDrgMv = static_cast<QDragMoveEvent *>(event);
      pDrgMv->acceptProposedAction();
      return true;
    }
    if (event->type() == QEvent::Drop) {
      QDropEvent *pDrpEv = static_cast<QDropEvent *>(event);
      QModelIndex idxTrgt =
          pUi->outDirTreeView->indexAt(pDrpEv->position().toPoint());

      if (idxTrgt.isValid()) {
        int iGid = idxTrgt.data(Qt::UserRole).toInt();
        // ユーザーグループ(>=0) または 未所属(-2) または すべて(-1)
        // へのドロップを許可
        if (iGid >= 0 || iGid == FileManager::iGRP_ID_UNASSIGNED ||
            iGid == FileManager::iGRP_ID_ALL || iGid == FileManager::iGRP_ID_DELETED) {
          QItemSelectionModel *pSlctn = pUi->followerListView->selectionModel();
          QModelIndexList lstIdxs = pSlctn->selectedRows(); // selectedIndexes ではなく行を選択
          QSet<int> setSourceRows;
          for (const QModelIndex &idxProxy : lstIdxs) {
            QModelIndex idxSrc = pProxyMdl->mapToSource(idxProxy);
            if (idxSrc.isValid()) {
              setSourceRows.insert(idxSrc.row());
            }
          }

          if (setSourceRows.isEmpty() &&
              pUi->followerListView->currentIndex().isValid()) {
            QModelIndex idxSrc = pProxyMdl->mapToSource(pUi->followerListView->currentIndex());
            if (idxSrc.isValid()) {
                setSourceRows.insert(idxSrc.row());
            }
          }

          QStringList lstTargetIds;
          for (int iRow : setSourceRows) {
            // インデックスに依存せず、0列目の UserRole から ID を取得
            QStandardItem *pItm = pMdlFllwr->item(iRow, 0);
            if (pItm) {
              QString szUsrId = pItm->data(ROLE_USER_ID).toString();
              if (!szUsrId.isEmpty()) {
                lstTargetIds << szUsrId;
              }
            }
          }

          if (!lstTargetIds.isEmpty()) {
            if (iGid >= 0) {
              // 特定グループへの割り当て
              emit followersAssignedToGroup(lstTargetIds, iGid);
            } else {
              // 未所属(-2) または すべて(-1) へのドロップは全解除とみなす
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
void MainWindow::onOutputButtonClicked() { emit outputRequested(); }

/**
 * @brief フォロワーリストのコンテキストメニュー表示。
 */
void MainWindow::onFollowerListContextMenu(const QPoint &pos) {
  QModelIndex index = pUi->followerListView->indexAt(pos);
  if (!index.isValid()) {
    return;
  }

  // 選択されている全てのユーザー ID を収集
  QItemSelectionModel *pSlctn = pUi->followerListView->selectionModel();
  QModelIndexList lstIdxs = pSlctn->selectedRows();
  QSet<int> setSrcRows;
  for (const QModelIndex &idxProxy : lstIdxs) {
    QModelIndex idxSrc = pProxyMdl->mapToSource(idxProxy);
    if (idxSrc.isValid()) {
        setSrcRows.insert(idxSrc.row());
    }
  }
  if (setSrcRows.isEmpty()) {
    QModelIndex idxSrc = pProxyMdl->mapToSource(index);
    if (idxSrc.isValid()) {
        setSrcRows.insert(idxSrc.row());
    }
  }

  QStringList lstTargetIds;
  for (int iRow : setSrcRows) {
    // インデックスに依存せず、0列目の UserRole から ID を取得
    lstTargetIds << pMdlFllwr->item(iRow, 0)->data(ROLE_USER_ID).toString();
  }

  QMenu oMenu(this);
  if (lstTargetIds.size() > 1) {
    oMenu.setTitle(QString("%1 名を選択中").arg(lstTargetIds.size()));
  } else {
    // 単一選択の場合は表示名を取得
    int iFirstRow = *setSrcRows.begin();
    oMenu.setTitle(pMdlFllwr->item(iFirstRow, COL_DISPLAY_NAME)->text());
  }

  // グループに追加サブメニュー
  QMenu *pSubAdd = oMenu.addMenu("グループに追加");

  // 現在のグループリストをツリーモデルから収集
  QStandardItem *pItmAll = nullptr;
  for (int i = 0; i < pMdlGrp->rowCount(); ++i) {
    if (pMdlGrp->item(i)->data(Qt::UserRole).toInt() ==
        FileManager::iGRP_ID_ALL) {
      pItmAll = pMdlGrp->item(i);
      break;
    }
  }

  if (pItmAll) {
    for (int i = 0; i < pItmAll->rowCount(); ++i) {
      QStandardItem *pChild = pItmAll->child(i);
      int iGid = pChild->data(Qt::UserRole).toInt();
      if (iGid > 0) { // ユーザー定義グループのみ (ID >= 1)
        QAction *pAct = pSubAdd->addAction(pChild->text());
        connect(pAct, &QAction::triggered, [this, lstTargetIds, iGid]() {
          emit followersAssignedToGroup(lstTargetIds, iGid);
        });
      }
    }
  }

  // グループから解除メニュー
  // 単一選択の場合は所属グループのみ表示、複数選択の場合は全解除
  if (lstTargetIds.size() == 1) {
    int iRow = *setSrcRows.begin();
    // インデックスに依存せず、0列目の UserRole + 1 から所属グループ ID を取得
    QString szGidsRaw = pMdlFllwr->item(iRow, 0)->data(ROLE_GROUP_IDS).toString();
    if (!szGidsRaw.isEmpty()) {
      QMenu *pSubRem = oMenu.addMenu("グループから解除");
      QStringList lstGids = szGidsRaw.split(",", Qt::SkipEmptyParts);
      for (const QString &szGid : lstGids) {
        int iGid = szGid.toInt();
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
        connect(pAct, &QAction::triggered, [this, lstTargetIds, iGid]() {
          emit followersUnassignedFromGroup(lstTargetIds, iGid);
        });
      }
    }
  } else {
    // 複数選択時は「すべてのグループから解除」を表示
    QAction *pActRemAll =
        oMenu.addAction("選択したユーザーを全グループから解除");
    connect(pActRemAll, &QAction::triggered, [this, lstTargetIds]() {
      emit followersUnassignedFromGroup(lstTargetIds, -1);
    });
  }

  oMenu.exec(pUi->followerListView->viewport()->mapToGlobal(pos));
}

/**
 * @brief UI 要素の初期設定とシグナル・スロットの接続。
 */
void MainWindow::setupUiExtra() {
  pMdlFllwr = new QStandardItemModel(this);
  pMdlFllwr->setHorizontalHeaderLabels(
      {"ニックネーム", "表示名", "ユーザー名", "ID", "チャンネルURL", "グループ", "フォロー開始日", "フォロー削除日", "メモ"});

  pProxyMdl = new QSortFilterProxyModel(this);
  pProxyMdl->setSourceModel(pMdlFllwr);
  pProxyMdl->setSortRole(ROLE_SORT_DATA); // ROLE_SORT_DATA に基づいてソート
  pProxyMdl->setFilterCaseSensitivity(Qt::CaseInsensitive);
  pProxyMdl->setFilterKeyColumn(-1); // 全列を検索対象にする

  pUi->followerListView->setModel(pProxyMdl);
  pUi->followerListView->setSortingEnabled(true); // ソートを有効化

  // 列幅を内容に合わせて自動調整し、入り切らない場合はスクロールバーを表示
  // 初期状態は手動調整可能にし、データセット時に内容に合わせる
  pUi->followerListView->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Interactive);
  pUi->followerListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  // ソート状態の復元 (v2.0)
  QSettings oSet;
  int iSortCol = oSet.value("ui/sortColumn", static_cast<int>(COL_DISPLAY_NAME)).toInt();
  Qt::SortOrder eOrder = static_cast<Qt::SortOrder>(oSet.value("ui/sortOrder", static_cast<int>(Qt::AscendingOrder)).toInt());
  pUi->followerListView->sortByColumn(iSortCol, eOrder);

  // ヘッダークリック時にソート状態を保存する
  connect(pUi->followerListView->horizontalHeader(), &QHeaderView::sectionClicked, [this]() {
      QSettings oSet;
      oSet.setValue("ui/sortColumn", pProxyMdl->sortColumn());
      oSet.setValue("ui/sortOrder", static_cast<int>(pProxyMdl->sortOrder()));
  });

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

  connect(pUi->twitchLoginButton, &QPushButton::clicked, this,
          &MainWindow::onLoginButtonClicked);
  connect(pUi->createFolderButton, &QPushButton::clicked, this,
          &MainWindow::onAddGroupButtonClicked);
  connect(pUi->deleteFolderButton, &QPushButton::clicked, this,
          &MainWindow::onDeleteGroupButtonClicked);
  connect(pUi->undoButton, &QPushButton::clicked, this,
          &MainWindow::onUndoButtonClicked);
  connect(pUi->redoButton, &QPushButton::clicked, this,
          &MainWindow::onRedoButtonClicked);
  connect(pUi->outDirTreeView, &QTreeView::clicked, this,
          &MainWindow::onGroupTreeClicked);

  // Output ボタンの追加
  pBtnOutput = new QPushButton("Output", this);
  pBtnOutput->setEnabled(false);
  pUi->horizontalLayout->insertWidget(4, pBtnOutput);
  connect(pBtnOutput, &QPushButton::clicked, this,
          &MainWindow::onOutputButtonClicked);

  // D&D 設定
  pUi->followerListView->setDragEnabled(true);
  pUi->followerListView->setDragDropMode(QAbstractItemView::DragOnly);
  pUi->outDirTreeView->setAcceptDrops(true);
  pUi->outDirTreeView->setDragDropMode(QAbstractItemView::DropOnly);

  // コンテキストメニュー設定
  pUi->followerListView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(pUi->followerListView, &QTreeView::customContextMenuRequested, this,
          &MainWindow::onFollowerListContextMenu);

  pUi->outDirTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(pUi->outDirTreeView, &QTreeView::customContextMenuRequested, this,
          &MainWindow::on_group_tree_context_menu);

  // リンククリック（ダブルクリック）でブラウザを開く
  connect(pUi->followerListView, &QTreeView::doubleClicked,
          [this](const QModelIndex &idxProxy) {
            QModelIndex idxSrc = pProxyMdl->mapToSource(idxProxy);
            if (idxSrc.isValid() && idxSrc.column() == COL_CHANNEL_URL) {
              QString szUrl = idxSrc.data().toString();
              if (szUrl.startsWith("http")) {
                QDesktopServices::openUrl(QUrl(szUrl));
              }
            }
          });

  // イベントフィルター登録
  pUi->outDirTreeView->installEventFilter(this);
  pUi->outDirTreeView->viewport()->installEventFilter(this);

  // 検索ボックスの接続
  connect(pUi->searchLineEdit, &QLineEdit::textChanged, this,
          &MainWindow::onSearchTextChanged);

  // 編集の検知 (v2.0, Nickname)
  connect(pMdlFllwr, &QStandardItemModel::dataChanged, [this](const QModelIndex &idx) {
      QStandardItem *pItm = pMdlFllwr->itemFromIndex(idx);
      if (!pItm) return;
      
      QString szUsrId = pMdlFllwr->item(idx.row(), 0)->data(ROLE_USER_ID).toString();
      
      if (idx.column() == COL_MEMO) {
          emit followerMemoChanged(szUsrId, pItm->text());
      } else if (idx.column() == COL_NICKNAME) {
          emit followerNicknameChanged(szUsrId, pItm->text());
      }
  });
}

void MainWindow::onSearchTextChanged(const QString &szTxt) {
  pProxyMdl->setFilterFixedString(szTxt);
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
void MainWindow::setFollowers(const QList<TwitchFollower> &lstFllwrs,
                              const QMap<int, QString> &mapGrps) {
  pMdlFllwr->setRowCount(0);
  for (const auto &oFllwr : lstFllwrs) {
    QList<QStandardItem *> lstItems;
    QStandardItem *pItmNickname = new QStandardItem(oFllwr.nickname);
    pItmNickname->setEditable(true);
    lstItems << pItmNickname;

    QStandardItem *pItmDisplayName = new QStandardItem(oFllwr.userName);
    pItmDisplayName->setEditable(false);
    lstItems << pItmDisplayName;

    QStandardItem *pItmUserLogin = new QStandardItem(oFllwr.userLogin);
    pItmUserLogin->setEditable(false);
    lstItems << pItmUserLogin;

    QStandardItem *pItmUserId = new QStandardItem(oFllwr.userId);
    pItmUserId->setEditable(false);
    lstItems << pItmUserId;

    // チャンネルURLを動的に生成し、リンクスタイルを適用
    QStandardItem *pItmUrl =
        new QStandardItem("https://twitch.tv/" + oFllwr.userLogin);
    pItmUrl->setForeground(Qt::blue);
    QFont fontUrl = pItmUrl->font();
    fontUrl.setUnderline(true);
    pItmUrl->setFont(fontUrl);
    pItmUrl->setEditable(false);
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
    pItmGrp->setEditable(false);
    lstItems << pItmGrp;

    // フォロー開始日 (最古)
    QString szFlwAt = oFllwr.followHistory.isEmpty() ? "" : oFllwr.followHistory.first().toString("yyyy-MM-dd HH:mm");
    QStandardItem *pItmFlw = new QStandardItem(szFlwAt);
    pItmFlw->setEditable(false);
    lstItems << pItmFlw;

    // 解除検知日 (最新)
    QString szUnflwAt = oFllwr.unfollowHistory.isEmpty() ? "" : oFllwr.unfollowHistory.last().toString("yyyy-MM-dd HH:mm");
    QStandardItem *pItmUnflw = new QStandardItem(szUnflwAt);
    pItmUnflw->setEditable(false);
    lstItems << pItmUnflw;

    // メモ (v2.0)
    QStandardItem *pItmMemo = new QStandardItem(oFllwr.memo);
    pItmMemo->setEditable(true);
    lstItems << pItmMemo;

    // --- メタデータとソート用データの設定 ---

    // 第0列にメタデータを格納（ID、グループリスト）
    pItmNickname->setData(oFllwr.userId, ROLE_USER_ID);
    pItmNickname->setData(lstGidsStr.join(","), ROLE_GROUP_IDS);

    // すべての列にソート用データを設定 (ROLE_SORT_DATA)
    pItmNickname->setData(oFllwr.nickname, ROLE_SORT_DATA);
    pItmDisplayName->setData(oFllwr.userName, ROLE_SORT_DATA);
    pItmUserLogin->setData(oFllwr.userLogin, ROLE_SORT_DATA);
    pItmUserId->setData(oFllwr.userId, ROLE_SORT_DATA);
    pItmUrl->setData(pItmUrl->text(), ROLE_SORT_DATA);
    pItmGrp->setData(pItmGrp->text(), ROLE_SORT_DATA);
    pItmFlw->setData(oFllwr.followHistory.isEmpty() ? QDateTime() : oFllwr.followHistory.first(), ROLE_SORT_DATA);
    pItmUnflw->setData(oFllwr.unfollowHistory.isEmpty() ? QDateTime() : oFllwr.unfollowHistory.last(), ROLE_SORT_DATA);
    pItmMemo->setData(oFllwr.memo, ROLE_SORT_DATA);

    // ツールチップに全履歴を表示 (HTML テーブル形式)
    QString szTooltip = "<html><head><style>th, td { padding-right: 15px; }</style></head><body>";
    szTooltip += "<b>履歴 (Follow History)</b><br>";
    szTooltip += "<table style='border-collapse: collapse;'>";
    szTooltip += "<tr><th align='left'>フォロー</th><th align='left'>アンフォロー</th></tr>";
    
    int iMax = qMax(oFllwr.followHistory.size(), oFllwr.unfollowHistory.size());
    for (int i = 0; i < iMax; ++i) {
        QString szF = (i < oFllwr.followHistory.size()) ? oFllwr.followHistory[i].toString("yyyy-MM-dd HH:mm") : "...";
        QString szU = (i < oFllwr.unfollowHistory.size()) ? oFllwr.unfollowHistory[i].toString("yyyy-MM-dd HH:mm") : "...";
        szTooltip += QString("<tr><td>%1</td><td>%2</td></tr>").arg(szF, szU);
    }
    szTooltip += "</table></body></html>";
    
    for (QStandardItem *pItm : lstItems) {
        pItm->setToolTip(szTooltip);
    }

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
void MainWindow::setGroups(const QMap<int, QString> &mapGrps,
                           const QMap<int, int> &mapCounts) {
  // 現在選択されているグループ ID を記憶
  int iSlctdId = INVALID_ID;
  QModelIndexList lstSlctd =
      pUi->outDirTreeView->selectionModel()->selectedIndexes();
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
    if (pItmAll->child(i)->data(Qt::UserRole).toInt() !=
        FileManager::iGRP_ID_UNASSIGNED) {
      pItmAll->removeRow(i);
    }
  }

  // ユーザー定義グループを追加する前に区切り線を入れる
  if (!mapGrps.isEmpty()) {
      QStandardItem *pSeparator = new QStandardItem("──────────");
      pSeparator->setData(-99, Qt::UserRole); // 明示的に無効な ID を設定
      pSeparator->setEnabled(false);
      pSeparator->setSelectable(false);
      pItmAll->appendRow(pSeparator);
  }

  // ユーザー定義グループを追加
  for (auto it = mapGrps.constBegin(); it != mapGrps.constEnd(); ++it) {
    if (it.value().trimmed().isEmpty()) {
      continue; // 空の名前は無視
    }

    int iGrpId = it.key();
    int iCount = mapCounts.value(iGrpId, 0);

    QStandardItem *pItm =
        new QStandardItem(QString("%1 (%2)").arg(it.value()).arg(iCount));
    pItm->setData(iGrpId, Qt::UserRole);
    pItmAll->appendRow(pItm);
  }

  // 「削除済みユーザー」の更新
  int iDltdCount = mapCounts.value(FileManager::iGRP_ID_DELETED, 0);
  QString szDltdTitle = QString("削除済みユーザー (%1)").arg(iDltdCount);
  bool bHasDeleted = false;
  for (int i = 0; i < pMdlGrp->rowCount(); ++i) {
    if (pMdlGrp->item(i)->data(Qt::UserRole).toInt() ==
        FileManager::iGRP_ID_DELETED) {
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
    QList<QStandardItem *> lstSearch;
    for (int i = 0; i < pMdlGrp->rowCount(); ++i)
      lstSearch << pMdlGrp->item(i);

    while (!lstSearch.isEmpty()) {
      QStandardItem *pItm = lstSearch.takeFirst();
      if (pItm->data(Qt::UserRole).toInt() == iSlctdId) {
        pUi->outDirTreeView->setCurrentIndex(pItm->index());
        break;
      }
      for (int i = 0; i < pItm->rowCount(); ++i)
        lstSearch << pItm->child(i);
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
void MainWindow::onLoginButtonClicked() { emit loginRequested(); }

/**
 * @brief グループ追加ボタンクリック時の内部処理。
 */
void MainWindow::onAddGroupButtonClicked() {
  bool bOk = false;
  QString szNm = QInputDialog::getText(
      this, "新規グループ", "グループ名を入力してください:", QLineEdit::Normal,
      "", &bOk);
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
    QMessageBox::information(this, "通知",
                             "削除するグループを選択してください。");
  }
}

/**
 * @brief Undo ボタンクリック時の内部処理。
 */
void MainWindow::onUndoButtonClicked() { emit undoRequested(); }

/**
 * @brief Redo ボタンクリック時の内部処理。
 */
void MainWindow::onRedoButtonClicked() { emit redoRequested(); }

/**
 * @brief グループツリーのコンテキストメニュー。
 */
void MainWindow::on_group_tree_context_menu(const QPoint &pos) {
  QModelIndex index = pUi->outDirTreeView->indexAt(pos);
  if (!index.isValid()) {
    return;
  }

  int iGrpId = index.data(Qt::UserRole).toInt();
  // ユーザー定義グループのみ名前変更可能
  if (iGrpId < 0) {
    return;
  }

  QMenu oMenu(this);
  QAction *pActRename = oMenu.addAction("名前を変更");
  QAction *pActDelete = oMenu.addAction("削除");

  QAction *pRes = oMenu.exec(pUi->outDirTreeView->viewport()->mapToGlobal(pos));
  if (pRes == pActRename) {
    bool bOk = false;
    QString szOldNm = index.data().toString();
    // 表示件数 "(n)" を取り除く
    int iIdx = szOldNm.lastIndexOf(" (");
    if (iIdx > 0) {
      szOldNm = szOldNm.left(iIdx);
    } else { /* skip */ }

    QString szNewNm = QInputDialog::getText(
        this, "グループ名変更", "新しい名前を入力してください:", QLineEdit::Normal,
        szOldNm, &bOk);
    if (bOk && !szNewNm.isEmpty() && szNewNm != szOldNm) {
      emit groupRenamed(iGrpId, szNewNm);
    } else { /* skip */ }
  } else if (pRes == pActDelete) {
    emit groupDeleted(iGrpId);
  } else { /* skip */ }
}
