#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <QObject>
#include "../ui/mainwindow.h"
#include "../api/twitch_authenticator.h"
#include "../api/twitch_api_client.h"
#include "../api/file_manager.h"
#include "data_models.h"

class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(MainWindow *mainWindow, QObject *parent = nullptr);
    void initialize();

public slots:
    void handleLoginRequest();
    void handleAuthCompleted(const QString& token);
    void handleCurrentUserFetched(const QString& userId);
    void handleFollowersFetched(const QList<TwitchFollower>& followers);
    
    // UIからのアクション
    void handleFollowerAssigned(const QString& userId, int groupId);
    void handleFollowerUnassigned(const QString& userId, int groupId);
    void handleGroupCreated(const QString& groupName);
    void handleGroupDeleted(int groupId);
    void handleUndoRequested();
    void handleRedoRequested();
    void handleTimeout();

private:
    MainWindow *view;
    TwitchAuthenticator *authenticator;
    TwitchApiClient *apiClient;
    FileManager *fileManager;
    QTimer *timeoutTimer;

    bool m_isBusy;
    QList<TwitchFollower> currentFollowers;
    QList<TwitchFollower> currentDeletedUsers; // 追加: 削除済みユーザー
    QMap<int, QString> currentGroups;
    QList<ActionRecord> actionHistory;
    int historyCursor;
    int nextGroupId;

    void pushAction(const ActionRecord& action);
    void applyAction(const ActionRecord& action);
    void revertAction(const ActionRecord& action);
    
    void updateView();
    void saveAllState();
    void setBusy(bool busy);
};

#endif // APP_CONTROLLER_H
