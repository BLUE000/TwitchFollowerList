#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include "../core/data_models.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setLoginStatus(bool isLoggedIn);
    void setFollowers(const QList<TwitchFollower>& followers);
    void setGroups(const QMap<int, QString>& groups);
    void setUndoRedoEnabled(bool canUndo, bool canRedo);

signals:
    void loginRequested();
    void fetchFollowersRequested();
    void groupCreated(const QString& groupName);
    void groupDeleted(int groupId);
    void followerAssignedToGroup(const QString& userId, int groupId);
    void followerUnassignedFromGroup(const QString& userId, int groupId);
    void undoRequested();
    void redoRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void on_twitchLoginButton_clicked();
    void on_createFolderButton_clicked();
    void on_deleteFolderButton_clicked();
    void on_undoButton_clicked();
    void on_redoButton_clicked();
    void on_outDirTreeView_clicked(const QModelIndex &index);
    void on_followerListView_customContextMenuRequested(const QPoint &pos);

private:
    Ui::MainWindow *ui;
    QStandardItemModel *treeModel;
    QStandardItemModel *followerModel;
    QSortFilterProxyModel *proxyModel;
    int currentSelectedGroupId;
    QMap<int, QString> m_groups; // グループ情報を保持

    void setupModels();
};

#endif // MAINWINDOW_H
