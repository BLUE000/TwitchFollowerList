/**
 * @file mainwindow.cpp
 * @brief MainWindow クラスの実装。
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QStandardItemModel>
#include <QInputDialog>
#include <QMessageBox>

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

/**
 * @brief デストラクタ。
 */
MainWindow::~MainWindow() {
    delete pUi;
}

/**
 * @brief UI 要素の初期設定とシグナル・スロットの接続。
 */
void MainWindow::setupUiExtra() {
    pMdlFllwr = new QStandardItemModel(this);
    pMdlFllwr->setHorizontalHeaderLabels({"表示名", "ユーザー名", "ID", "グループ"});
    pUi->followerListView->setModel(pMdlFllwr);
    
    pMdlGrp = new QStandardItemModel(this);
    pMdlGrp->setHorizontalHeaderLabels({"グループ名"});
    pUi->outDirTreeView->setModel(pMdlGrp);

    connect(pUi->twitchLoginButton, &QPushButton::clicked, this, &MainWindow::onLoginButtonClicked);
    connect(pUi->createFolderButton, &QPushButton::clicked, this, &MainWindow::onAddGroupButtonClicked);
    connect(pUi->deleteFolderButton, &QPushButton::clicked, this, &MainWindow::onDeleteGroupButtonClicked);
    connect(pUi->undoButton, &QPushButton::clicked, this, &MainWindow::onUndoButtonClicked);
    connect(pUi->redoButton, &QPushButton::clicked, this, &MainWindow::onRedoButtonClicked);
}

/**
 * @brief ログイン状態の表示を更新する。
 * @param bLgned ログイン済みなら true。
 */
void MainWindow::setLoginStatus(bool bLgned) {
    if (bLgned) {
        pUi->statusbar->showMessage("ログイン済み");
        pUi->twitchLoginButton->setEnabled(false);
    } else {
        pUi->statusbar->showMessage("未ログイン");
        pUi->twitchLoginButton->setEnabled(true);
    }
}

/**
 * @brief フォロワーリストを UI モデルに反映する。
 * @param lstFllwrs 表示対象のリスト。
 */
void MainWindow::setFollowers(const QList<TwitchFollower>& lstFllwrs) {
    pMdlFllwr->removeRows(0, pMdlFllwr->rowCount());
    for (const auto& oFllwr : lstFllwrs) {
        QList<QStandardItem*> lstItems;
        lstItems << new QStandardItem(oFllwr.userName);
        lstItems << new QStandardItem(oFllwr.userLogin);
        lstItems << new QStandardItem(oFllwr.userId);
        
        QStringList lstGids;
        for (int iGid : oFllwr.groupIds) {
            lstGids << QString::number(iGid);
        }
        lstItems << new QStandardItem(lstGids.join(", "));
        
        pMdlFllwr->appendRow(lstItems);
    }
}

/**
 * @brief グループマップを UI モデルに反映する。
 * @param mapGrps 表示対象のマップ。
 */
void MainWindow::setGroups(const QMap<int, QString>& mapGrps) {
    pMdlGrp->removeRows(0, pMdlGrp->rowCount());
    for (auto it = mapGrps.constBegin(); it != mapGrps.constEnd(); ++it) {
        QStandardItem *pItm = new QStandardItem(it.value());
        pItm->setData(it.key(), Qt::UserRole); // IDを隠しデータとして保持
        pMdlGrp->appendRow(pItm);
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
