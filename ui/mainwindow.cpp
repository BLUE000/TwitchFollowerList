#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QDropEvent>
#include <QMimeData>
#include <QMenu>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , treeModel(nullptr)
    , followerModel(nullptr)
    , proxyModel(nullptr)
    , currentSelectedGroupId(-1)
{
    ui->setupUi(this);
    setupModels();

    // Context menu for follower list
    ui->followerListView->setContextMenuPolicy(Qt::CustomContextMenu);

    // Event filter for tree view to handle drag & drop
    ui->outDirTreeView->viewport()->installEventFilter(this);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setupModels() {
    // Tree Model
    treeModel = new QStandardItemModel(this);
    ui->outDirTreeView->setModel(treeModel);
    ui->outDirTreeView->setHeaderHidden(true);

    // Follower Model
    followerModel = new QStandardItemModel(this);
    followerModel->setHorizontalHeaderLabels({"No", "表示名", "ユーザー名", "ユーザーID", "グループID"});

    // Proxy Model for filtering
    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(followerModel);
    proxyModel->setFilterKeyColumn(4); // Filter by Group ID column

    ui->followerListView->setModel(proxyModel);
    ui->followerListView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->followerListView->setSelectionMode(QAbstractItemView::SingleSelection);

    // Hide No and GroupID columns as per requirements
    ui->followerListView->setColumnHidden(0, true);
    ui->followerListView->setColumnHidden(4, true);

    // Drag and Drop settings
    ui->followerListView->setDragEnabled(true);
    ui->followerListView->setDragDropMode(QAbstractItemView::DragOnly);
}

void MainWindow::setLoginStatus(bool isLoggedIn) {
    ui->twitchLoginButton->setEnabled(!isLoggedIn);
    ui->twitchLoginButton->setText(isLoggedIn ? "ログイン済み" : "Twitchでログイン");
}

void MainWindow::setFollowers(const QList<TwitchFollower>& followers) {
    followerModel->removeRows(0, followerModel->rowCount());
    int no = 1;
    for (const auto& f : followers) {
        QList<QStandardItem*> items;
        items << new QStandardItem(QString::number(no++));
        items << new QStandardItem(f.userName);
        items << new QStandardItem(f.userLogin);
        items << new QStandardItem(f.userId);
        
        // Group IDs joined by pipe
        QStringList gids;
        for (int id : f.groupIds) gids << QString::number(id);
        items << new QStandardItem(gids.join("|"));
        
        // Store userId in first column for DnD
        items[0]->setData(f.userId, Qt::UserRole);
        
        followerModel->appendRow(items);
    }
}

void MainWindow::setGroups(const QMap<int, QString>& groups) {
    m_groups = groups;
    treeModel->clear();
    
    // Root item for "All"
    QStandardItem *rootItem = new QStandardItem("すべて");
    rootItem->setData(-1, Qt::UserRole);
    treeModel->appendRow(rootItem);

    // Add groups
    for (auto it = groups.begin(); it != groups.end(); ++it) {
        QStandardItem *item = new QStandardItem(it.value());
        item->setData(it.key(), Qt::UserRole);
        treeModel->appendRow(item);
    }
    ui->outDirTreeView->expandAll();
}

void MainWindow::setUndoRedoEnabled(bool canUndo, bool canRedo) {
    ui->undoButton->setEnabled(canUndo);
    ui->redoButton->setEnabled(canRedo);
}

void MainWindow::on_twitchLoginButton_clicked() {
    emit loginRequested();
}

void MainWindow::on_createFolderButton_clicked() {
    bool ok;
    QString name = QInputDialog::getText(this, "新規グループ", "グループ名:", QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        emit groupCreated(name);
    }
}

void MainWindow::on_deleteFolderButton_clicked() {
    if (currentSelectedGroupId > 0) {
        if (QMessageBox::question(this, "削除確認", "選択したグループを削除しますか？") == QMessageBox::Yes) {
            emit groupDeleted(currentSelectedGroupId);
        }
    }
}

void MainWindow::on_undoButton_clicked() {
    emit undoRequested();
}

void MainWindow::on_redoButton_clicked() {
    emit redoRequested();
}

void MainWindow::on_outDirTreeView_clicked(const QModelIndex &index) {
    QStandardItem *item = treeModel->itemFromIndex(index);
    if (item) {
        int groupId = item->data(Qt::UserRole).toInt();
        currentSelectedGroupId = groupId;
        
        if (groupId == -1) {
            proxyModel->setFilterFixedString("");
        } else {
            // Filter by exact group ID or handle pipe separation
            proxyModel->setFilterRegularExpression(QString("(^|\\|)%1(\\||$)").arg(groupId));
        }
    }
}

void MainWindow::on_followerListView_customContextMenuRequested(const QPoint &pos) {
    QModelIndex index = ui->followerListView->indexAt(pos);
    if (!index.isValid()) return;

    QModelIndex sourceIndex = proxyModel->mapToSource(index);
    QString userId = followerModel->item(sourceIndex.row(), 0)->data(Qt::UserRole).toString();
    QString currentGidsStr = followerModel->item(sourceIndex.row(), 4)->text();
    QStringList currentGids = currentGidsStr.split("|", Qt::SkipEmptyParts);

    QMenu menu(this);
    QMenu *addMenu = menu.addMenu("グループに追加");
    QMenu *removeMenu = menu.addMenu("グループから削除");

    for (auto it = m_groups.begin(); it != m_groups.end(); ++it) {
        int gid = it.key();
        QString name = it.value();
        
        if (!currentGids.contains(QString::number(gid))) {
            QAction *act = addMenu->addAction(name);
            connect(act, &QAction::triggered, [this, userId, gid]() {
                emit followerAssignedToGroup(userId, gid);
            });
        } else {
            QAction *act = removeMenu->addAction(name);
            connect(act, &QAction::triggered, [this, userId, gid]() {
                emit followerUnassignedFromGroup(userId, gid);
            });
        }
    }
    
    if (addMenu->isEmpty()) addMenu->setEnabled(false);
    if (removeMenu->isEmpty()) removeMenu->setEnabled(false);

    menu.exec(ui->followerListView->viewport()->mapToGlobal(pos));
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui->outDirTreeView->viewport()) {
        if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove) {
            event->accept();
            return true;
        } else if (event->type() == QEvent::Drop) {
            QDropEvent *dropEvent = static_cast<QDropEvent*>(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QModelIndex index = ui->outDirTreeView->indexAt(dropEvent->position().toPoint());
#else
            QModelIndex index = ui->outDirTreeView->indexAt(dropEvent->pos());
#endif
            if (index.isValid()) {
                QStandardItem *targetItem = treeModel->itemFromIndex(index);
                int groupId = targetItem->data(Qt::UserRole).toInt();
                
                if (groupId > 0) {
                    // Get selected userId from followerListView
                    QModelIndexList selected = ui->followerListView->selectionModel()->selectedRows();
                    if (!selected.isEmpty()) {
                        QModelIndex sourceIdx = proxyModel->mapToSource(selected.first());
                        QString userId = followerModel->item(sourceIdx.row(), 0)->data(Qt::UserRole).toString();
                        emit followerAssignedToGroup(userId, groupId);
                    }
                }
            }
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}
