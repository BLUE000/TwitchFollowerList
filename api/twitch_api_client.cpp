#include "twitch_api_client.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

TwitchApiClient::TwitchApiClient(QObject *parent) : QObject(parent) {
    networkManager = new QNetworkAccessManager(this);
    clientId = "lfyil72vhg1s7baymtguh4g06h93qb"; // User's Client ID
}

void TwitchApiClient::setAccessToken(const QString& token) {
    accessToken = token;
}

void TwitchApiClient::fetchCurrentUser() {
    if (accessToken.isEmpty()) {
        emit errorOccurred("Access token is empty.");
        return;
    }

    QNetworkRequest request(QUrl("https://api.twitch.tv/helix/users"));
    request.setRawHeader("Authorization", ("Bearer " + accessToken).toUtf8());
    request.setRawHeader("Client-Id", clientId.toUtf8());

    QNetworkReply* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onUserReplyFinished(reply); });
}

void TwitchApiClient::onUserReplyFinished(QNetworkReply* reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred("Failed to fetch user: " + reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();
    QJsonArray dataArray = obj.value("data").toArray();

    if (!dataArray.isEmpty()) {
        QJsonObject userObj = dataArray.first().toObject();
        broadcasterId = userObj.value("id").toString();
        emit currentUserFetched(broadcasterId);
    } else {
        emit errorOccurred("User data not found in response.");
    }
}

void TwitchApiClient::fetchFollowers(const QString& cursor) {
    if (broadcasterId.isEmpty() || accessToken.isEmpty()) {
        emit errorOccurred("Broadcaster ID or access token is empty.");
        return;
    }

    // first=100 を指定して最大100件ずつ取得
    QString urlStr = "https://api.twitch.tv/helix/channels/followers?broadcaster_id=" + broadcasterId + "&first=100";
    if (!cursor.isEmpty()) {
        urlStr += "&after=" + cursor;
    }

    QNetworkRequest request((QUrl(urlStr)));
    request.setRawHeader("Authorization", ("Bearer " + accessToken).toUtf8());
    request.setRawHeader("Client-Id", clientId.toUtf8());

    QNetworkReply* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onFollowersReplyFinished(reply); });
}

void TwitchApiClient::onFollowersReplyFinished(QNetworkReply* reply) {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred("Failed to fetch followers: " + reply->errorString());
        // エラー時は蓄積データをクリアして終了
        accumulatedFollowers.clear();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();
    QJsonArray dataArray = obj.value("data").toArray();

    for (const QJsonValue& val : dataArray) {
        QJsonObject fObj = val.toObject();
        TwitchFollower f;
        f.userId = fObj.value("user_id").toString();
        f.userName = fObj.value("user_name").toString();
        f.userLogin = fObj.value("user_login").toString();
        f.followedAt = QDateTime::fromString(fObj.value("followed_at").toString(), Qt::ISODate);
        
        accumulatedFollowers.append(f);
    }

    // ページネーションのカーソルを取得
    QJsonObject pagination = obj.value("pagination").toObject();
    QString cursor = pagination.value("cursor").toString();

    // カーソルが存在し、かつデータが取得できている場合は次ページを取得
    if (!cursor.isEmpty() && dataArray.size() > 0) {
        fetchFollowers(cursor);
    } else {
        // 全ページの取得が完了したらシグナルを発火し、蓄積リストをクリア
        emit followersFetched(accumulatedFollowers);
        accumulatedFollowers.clear();
    }
}
