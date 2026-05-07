#ifndef TWITCH_API_CLIENT_H
#define TWITCH_API_CLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "../core/data_models.h"

class TwitchApiClient : public QObject {
    Q_OBJECT
public:
    explicit TwitchApiClient(QObject *parent = nullptr);
    void setAccessToken(const QString& token);
    void fetchCurrentUser();
    void fetchFollowers(const QString& cursor = "");

signals:
    void currentUserFetched(const QString& userId);
    void followersFetched(const QList<TwitchFollower>& followers);
    void errorOccurred(const QString& errorMessage);

private slots:
    void onUserReplyFinished(QNetworkReply* reply);
    void onFollowersReplyFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager *networkManager;
    QString accessToken;
    QString clientId;
    QString broadcasterId;
    QList<TwitchFollower> accumulatedFollowers;
};

#endif // TWITCH_API_CLIENT_H
