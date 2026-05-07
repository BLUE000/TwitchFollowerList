/**
 * @file twitch_api_client.h
 * @brief Twitch API との通信を管理するクラスの定義。
 */

#ifndef TWITCH_API_CLIENT_H
#define TWITCH_API_CLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include "../core/data_models.h"

/**
 * @class TwitchApiClient
 * @brief Twitch ヘリックス API を呼び出し、ユーザー情報やフォロワー情報を取得するクラス。
 */
class TwitchApiClient : public QObject {
    Q_OBJECT
public:
    /**
     * @brief コンストラクタ。
     * @param pParent 親オブジェクト。
     */
    explicit TwitchApiClient(QObject *pParent = nullptr);

    /**
     * @brief アクセストークンを設定する。
     * @param szTkn OAuth2 アクセストークン。
     */
    void setAccessToken(const QString& szTkn);

    /**
     * @brief ログイン中のユーザー情報（特に ID）を取得する。
     */
    void fetchCurrentUser();

    /**
     * @brief 指定したユーザーのフォロワーリストを取得する。
     */
    void fetchFollowers();

signals:
    /**
     * @brief ユーザー ID の取得が完了した際のシグナル。
     * @param szUsrId Twitch User ID。
     */
    void currentUserFetched(const QString& szUsrId);

    /**
     * @brief フォロワーリストの取得が完了した際のシグナル。
     * @param lstFllwrs 取得されたリスト。
     */
    void followersFetched(const QList<TwitchFollower>& lstFllwrs);

private slots:
    void onCurrentUserReply(QNetworkReply *pRply);
    void onFollowersReply(QNetworkReply *pRply);

private:
    QNetworkAccessManager *pNtwrkMngr; ///< ネットワークアクセス管理
    QString szAccsTkn;                 ///< アクセストークン
    QString szClntId;                  ///< Twitch Client ID
    QString szCrntUsrId;               ///< 取得した自ユーザー ID
};

#endif // TWITCH_API_CLIENT_H
